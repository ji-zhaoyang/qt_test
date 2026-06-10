#include "app_controller.h"
#include "coordinators/home_coordinator.h"
#include "coordinators/settings_coordinator.h"
#include "network/core/tcp_manager.h"
#include "services/local_time_service_client.h"
#include "views/home/home_page.h"
#include "views/settings/settings_page.h"
#include <QDebug>
#include <QJsonObject>

AppController::AppController(HomePage *homePage, SettingsPage *settingsPage, QObject *parent)
    : AppController(homePage, settingsPage, AppConfig::defaultConnectionConfig(), parent)
{
}

AppController::AppController(HomePage *homePage, SettingsPage *settingsPage, const ConnectionConfig &connectionConfig,
                             QObject *parent)
    : QObject(parent), homeCoordinator(nullptr), settingsCoordinator(nullptr),
      localTimeServiceClient(new LocalTimeServiceClient(this)), tcpManager(new TcpManager(this)),
      connectionConfigValue(connectionConfig)
{
    tcpManager->setReconnectIntervalMs(connectionConfigValue.reconnectIntervalMs);
    homeCoordinator = new HomeCoordinator(homePage, settingsPage, tcpManager, this);
    settingsCoordinator = new SettingsCoordinator(settingsPage, tcpManager, localTimeServiceClient, this);
    setupConnections();
}

void AppController::connectToDevice()
{
    qDebug() << "正在尝试连接到服务端:" << connectionConfigValue.host << "端口:" << connectionConfigValue.port;
    updateDeviceStatus(DeviceConnectionState::Connecting);
    tcpManager->connectToServer(connectionConfigValue.host, connectionConfigValue.port);
}

void AppController::setConnectionConfig(const ConnectionConfig &connectionConfig)
{
    connectionConfigValue = connectionConfig;
    tcpManager->setReconnectIntervalMs(connectionConfigValue.reconnectIntervalMs);
}

const ConnectionConfig &AppController::connectionConfig() const
{
    return connectionConfigValue;
}

const DeviceStatus &AppController::deviceStatus() const
{
    return deviceStatusValue;
}

void AppController::setupConnections()
{
    connect(tcpManager, &TcpManager::connected, this, &AppController::onTcpConnected);
    connect(tcpManager, &TcpManager::disconnected, this, &AppController::onTcpDisconnected);
    connect(tcpManager, &TcpManager::errorOccurred, this, &AppController::onTcpError);
    connect(tcpManager, &TcpManager::deviceInfoParsed, this, &AppController::onDeviceInfoReceived);
    if (homeCoordinator)
    {
        homeCoordinator->setupConnections();
        connect(tcpManager, &TcpManager::deviceInfoParsed,
                homeCoordinator, &HomeCoordinator::handleDeviceInfo);
        connect(tcpManager, &TcpManager::droneTargetReported,
                homeCoordinator, &HomeCoordinator::handleDroneTargetReported);
        homeCoordinator->initializeState();
    }
    if (settingsCoordinator)
    {
        settingsCoordinator->setupConnections();
        connect(tcpManager, &TcpManager::deviceInfoParsed,
                settingsCoordinator, &SettingsCoordinator::handleDeviceInfo);
    }
}

void AppController::onTcpConnected()
{
    qDebug() << "TCP 客户端连接成功！";
    updateDeviceStatus(DeviceConnectionState::Connected);
}

void AppController::onTcpDisconnected()
{
    qDebug() << "TCP 连接已断开！";
    updateDeviceStatus(DeviceConnectionState::Disconnected);
}

void AppController::onTcpError(const QString &errorStr)
{
    qDebug() << "TCP 连接错误:" << errorStr;
    updateDeviceStatus(DeviceConnectionState::Error, errorStr);
}

void AppController::onDeviceInfoReceived(const QJsonObject &deviceInfo)
{
    if (deviceInfo.value("protocolDataType").toInt() == 2)
    {
        emit deviceStatusInfoUpdated(deviceInfo);
    }
}

void AppController::updateDeviceStatus(DeviceConnectionState state, const QString &errorMessage)
{
    deviceStatusValue.connectionState = state;
    deviceStatusValue.lastError = errorMessage;
    emit deviceStatusChanged();
}
