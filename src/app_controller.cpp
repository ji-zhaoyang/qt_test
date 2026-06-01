#include "app_controller.h"
#include "network/core/tcp_manager.h"
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
    : QObject(parent), homePage(homePage), settingsPage(settingsPage), tcpManager(new TcpManager(this)),
      connectionConfigValue(connectionConfig)
{
    tcpManager->setReconnectIntervalMs(connectionConfigValue.reconnectIntervalMs);
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
    connect(tcpManager, &TcpManager::connected, settingsPage, &SettingsPage::onDeviceConnectionRestored);
    connect(tcpManager, &TcpManager::disconnected, settingsPage, &SettingsPage::onDeviceConnectionLost);

    connect(tcpManager, &TcpManager::firmwareVersionsQueried, settingsPage, &SettingsPage::updateFirmwareVersions);
    connect(tcpManager, &TcpManager::deviceGpsQueried, settingsPage, &SettingsPage::updateGpsInfo);
    connect(tcpManager, &TcpManager::deviceGpsSetResponse, settingsPage, &SettingsPage::updateGpsSaveResult);
    connect(tcpManager, &TcpManager::detectBandsSetResponse, settingsPage, &SettingsPage::updateDetectBandSaveResult);
    connect(tcpManager, &TcpManager::detectBandsQueried, settingsPage, &SettingsPage::updateDetectBands);
    connect(tcpManager, &TcpManager::droneReportModeSetResponse, settingsPage,
            &SettingsPage::updateDroneReportModeSaveResult);
    connect(tcpManager, &TcpManager::droneReportModeQueried, settingsPage, &SettingsPage::updateDroneReportMode);
    connect(tcpManager, &TcpManager::suppressionModeSetResponse, settingsPage, &SettingsPage::updateJamModeSaveResult);
    connect(tcpManager, &TcpManager::suppressionModeQueried, settingsPage, &SettingsPage::updateJamMode);
    connect(tcpManager, &TcpManager::o4ServerModeSetResponse, settingsPage, &SettingsPage::updateNetworkModeSaveResult);
    connect(tcpManager, &TcpManager::o4ServerModeQueried, settingsPage, &SettingsPage::updateNetworkMode);
    connect(tcpManager, &TcpManager::uavCategoryDisplayModeSetResponse, settingsPage,
            &SettingsPage::updateUavCategoryDisplayModeSaveResult);
    connect(tcpManager, &TcpManager::uavCategoryDisplayModeQueried, settingsPage,
            &SettingsPage::updateUavCategoryDisplayMode);
    connect(tcpManager, &TcpManager::dataEnableSetResponse, settingsPage, &SettingsPage::updateDataEnableSaveResult);
    connect(tcpManager, &TcpManager::dataEnableQueried, settingsPage, &SettingsPage::updateDataEnable);
    connect(tcpManager, &TcpManager::featureModesSetResponse, settingsPage, &SettingsPage::updateFeatureModeSaveResult);
    connect(tcpManager, &TcpManager::featureModesQueried, settingsPage, &SettingsPage::updateFeatureModes);
    connect(tcpManager, &TcpManager::compassCalibrationResponse, settingsPage, &SettingsPage::updateCompassCalibrationResult);
    connect(tcpManager, &TcpManager::spectrogramSwitchResponse, settingsPage, &SettingsPage::updateSpectrumSwitchResult);
    connect(tcpManager, &TcpManager::spectrumDataReported, settingsPage, &SettingsPage::updateSpectrumReport);
    connect(tcpManager, &TcpManager::fullSpectrumSwitchResponse, settingsPage, &SettingsPage::updateFullSpectrumSwitchResult);
    connect(tcpManager, &TcpManager::fullSpectrumReported, settingsPage, &SettingsPage::updateFullSpectrumReport);
    connect(tcpManager, &TcpManager::alarmHistoryQueried, settingsPage, &SettingsPage::updateAlarmHistory);
    connect(tcpManager, &TcpManager::deviceUsageInfoQueried, settingsPage, &SettingsPage::updateDeviceUsageInfo);
    connect(tcpManager, &TcpManager::buzzerEnabledQueried, settingsPage, &SettingsPage::updateBuzzerEnabled);
    connect(tcpManager, &TcpManager::buzzerEnabledSetResponse, settingsPage, &SettingsPage::updateBuzzerEnabledSaveResult);
    connect(tcpManager, &TcpManager::systemTimeSetResponse, settingsPage, &SettingsPage::updateSystemTimeSaveResult);
    connect(tcpManager, &TcpManager::deviceRebootResponse, settingsPage, &SettingsPage::updateRebootResult);
    connect(tcpManager, &TcpManager::modelLibraryModeQueried, settingsPage, &SettingsPage::updateModelLibraryMode);
    connect(tcpManager, &TcpManager::modelLibraryModeSetResponse, settingsPage,
            &SettingsPage::updateModelLibraryModeSaveResult);
    connect(tcpManager, &TcpManager::modelLibraryRecordsQueried, settingsPage, &SettingsPage::updateModelLibraryRecords);
    connect(tcpManager, &TcpManager::modelLibraryRecordSetResponse, settingsPage,
            &SettingsPage::updateModelLibraryRecordSaveResult);
    connect(tcpManager, &TcpManager::strikeFrequencyBandsQueried, settingsPage, &SettingsPage::updateStrikeFrequencyBands);
    connect(tcpManager, &TcpManager::strikeFrequencyBandsSetResponse, settingsPage,
            &SettingsPage::updateStrikeFrequencySaveResult);
    connect(tcpManager, &TcpManager::powerAmplifierParamsQueried, settingsPage, &SettingsPage::updatePowerAmplifierParams);
    connect(tcpManager, &TcpManager::powerAmplifierParamsSetResponse, settingsPage,
            &SettingsPage::updatePowerAmplifierSaveResult);
    connect(tcpManager, &TcpManager::directionCalibrationValuesQueried, settingsPage,
            &SettingsPage::updateDirectionCalibrationValues);
    connect(tcpManager, &TcpManager::directionCalibrationValuesSetResponse, settingsPage,
            &SettingsPage::updateDirectionCalibrationSaveResult);
    connect(tcpManager, &TcpManager::deviceJammingStatusQueried, settingsPage, &SettingsPage::updateStrikeStatus);
    connect(tcpManager, &TcpManager::signalSourceParamsQueried, settingsPage, &SettingsPage::updateSignalSourceParams);
    connect(tcpManager, &TcpManager::signalSourceParamsSetResponse, settingsPage,
            &SettingsPage::updateSignalSourceParamsSaveResult);
    connect(tcpManager, &TcpManager::fullScanParamsQueried, settingsPage, &SettingsPage::updateFullScanSettings);
    connect(tcpManager, &TcpManager::fullScanParamsSetResponse, settingsPage, &SettingsPage::updateFullScanSaveResult);
    connect(tcpManager, &TcpManager::deviceIpQueried, settingsPage, &SettingsPage::updateDeviceIpSettings);
    connect(tcpManager, &TcpManager::deviceIpSetResponse, settingsPage, &SettingsPage::updateDeviceIpSaveResult);
    connect(tcpManager, &TcpManager::tcpServerIpQueried, settingsPage, &SettingsPage::updateTcpServerIpSettings);
    connect(tcpManager, &TcpManager::tcpServerIpSetResponse, settingsPage, &SettingsPage::updateTcpServerIpSaveResult);
    connect(settingsPage, &SettingsPage::requestSaveGps, tcpManager, &TcpManager::setDeviceGps);
    connect(settingsPage, &SettingsPage::requestSaveDetectBands, tcpManager, &TcpManager::setDetectBands);
    connect(settingsPage, &SettingsPage::requestSaveDroneReportMode, tcpManager, &TcpManager::setDroneReportMode);
    connect(settingsPage, &SettingsPage::requestSaveJamMode, tcpManager, &TcpManager::setSuppressionMode);
    connect(settingsPage, &SettingsPage::requestSaveNetworkMode, tcpManager, &TcpManager::setO4ServerMode);
    connect(settingsPage, &SettingsPage::requestSaveUavCategoryDisplayMode, tcpManager,
            &TcpManager::setUavCategoryDisplayMode);
    connect(settingsPage, &SettingsPage::requestSaveDataEnable, tcpManager, &TcpManager::setDataEnable);
    connect(settingsPage, &SettingsPage::requestSaveFeatureModes, tcpManager, &TcpManager::setFeatureModes);
    connect(settingsPage, &SettingsPage::requestStartCompassCalibration, tcpManager, &TcpManager::startCompassCalibration);
    connect(settingsPage, &SettingsPage::requestFinishCompassCalibration, tcpManager, &TcpManager::finishCompassCalibration);
    connect(settingsPage, &SettingsPage::requestConfirmCompassCalibration, tcpManager,
            &TcpManager::confirmCompassCalibration);
    connect(settingsPage, &SettingsPage::requestCancelCompassCalibration, tcpManager, &TcpManager::cancelCompassCalibration);
    connect(settingsPage, &SettingsPage::requestOpenSpectrogram, tcpManager, &TcpManager::openSpectrogram);
    connect(settingsPage, &SettingsPage::requestCloseSpectrogram, tcpManager, &TcpManager::closeSpectrogram);
    connect(settingsPage, &SettingsPage::requestOpenSpectrum, tcpManager, &TcpManager::openFullSpectrum);
    connect(settingsPage, &SettingsPage::requestCloseSpectrum, tcpManager, &TcpManager::closeFullSpectrum);
    connect(settingsPage, &SettingsPage::requestQueryAlarmHistory, tcpManager, &TcpManager::queryDeviceAlarmHistory);
    connect(settingsPage, &SettingsPage::requestQueryDeviceUsageInfo, tcpManager, &TcpManager::queryDeviceUsageInfo);
    connect(settingsPage, &SettingsPage::requestSaveBuzzerEnabled, tcpManager, &TcpManager::setBuzzerEnabled);
    connect(settingsPage, &SettingsPage::requestSaveSystemTime, tcpManager,
            static_cast<void (TcpManager::*)(const QDateTime &)>(&TcpManager::setSystemTime));
    connect(settingsPage, &SettingsPage::requestRebootDevice, tcpManager, &TcpManager::rebootDevice);
    connect(settingsPage, &SettingsPage::requestSaveModelLibraryMode, tcpManager, &TcpManager::setModelLibraryMode);
    connect(settingsPage, &SettingsPage::requestUpdateModelLibraryRecord, tcpManager, &TcpManager::setModelLibraryRecord);
    connect(settingsPage, &SettingsPage::requestSaveStrikeFrequencyBands, tcpManager, &TcpManager::setStrikeFrequencyBands);
    connect(settingsPage, &SettingsPage::requestSavePowerAmplifierParams, tcpManager, &TcpManager::setPowerAmplifierParams);
    connect(settingsPage, &SettingsPage::requestSaveDirectionCalibrationValues, tcpManager,
            &TcpManager::setDirectionCalibrationValues);
    connect(settingsPage, &SettingsPage::requestSaveSignalSourceParams, tcpManager, &TcpManager::setSignalSourceParams);
    connect(settingsPage, &SettingsPage::requestSaveFullScan, tcpManager, &TcpManager::setFullScanParams);
    connect(settingsPage, &SettingsPage::requestSaveDeviceIp, tcpManager, &TcpManager::setDeviceIp);
    connect(settingsPage, &SettingsPage::requestSaveTcpServerIp, tcpManager, &TcpManager::setTcpServerIp);
    connect(settingsPage, &SettingsPage::requestQueryGps, tcpManager, &TcpManager::queryDeviceGps);
    connect(settingsPage, &SettingsPage::requestQueryDetectBands, tcpManager, &TcpManager::queryDetectBands);
    connect(settingsPage, &SettingsPage::requestQueryDroneReportMode, tcpManager, &TcpManager::queryDroneReportMode);
    connect(settingsPage, &SettingsPage::requestQueryJamMode, tcpManager, &TcpManager::querySuppressionMode);
    connect(settingsPage, &SettingsPage::requestQueryNetworkMode, tcpManager, &TcpManager::queryO4ServerMode);
    connect(settingsPage, &SettingsPage::requestQueryUavCategoryDisplayMode, tcpManager,
            &TcpManager::queryUavCategoryDisplayMode);
    connect(settingsPage, &SettingsPage::requestQueryDataEnable, tcpManager, &TcpManager::queryDataEnable);
    connect(settingsPage, &SettingsPage::requestQueryFeatureModes, tcpManager, &TcpManager::queryFeatureModes);
    connect(settingsPage, &SettingsPage::requestQueryFirmwareVersions, tcpManager, &TcpManager::queryFirmwareVersions);
    connect(settingsPage, &SettingsPage::requestQueryStrikeFrequencyBands, tcpManager, &TcpManager::queryStrikeFrequencyBands);
    connect(settingsPage, &SettingsPage::requestQueryStrikeStatus, tcpManager, &TcpManager::queryDeviceJammingMode);
    connect(settingsPage, &SettingsPage::requestQueryPowerAmplifierParams, tcpManager, &TcpManager::queryPowerAmplifierParams);
    connect(settingsPage, &SettingsPage::requestQueryDirectionCalibrationValues, tcpManager,
            &TcpManager::queryDirectionCalibrationValues);
    connect(settingsPage, &SettingsPage::requestQuerySignalSourceParams, tcpManager, &TcpManager::querySignalSourceParams);
    connect(settingsPage, &SettingsPage::requestQueryBuzzerEnabled, tcpManager, &TcpManager::queryBuzzerEnabled);
    connect(settingsPage, &SettingsPage::requestQueryModelLibraryMode, tcpManager, &TcpManager::queryModelLibraryMode);
    connect(settingsPage, &SettingsPage::requestQueryModelLibraryRecords, this,
            [this](int current, int size)
            {
                ModelLibraryPageQuery query;
                query.current = current;
                query.size = size;
                tcpManager->queryModelLibraryRecords(query);
            });
    connect(settingsPage, &SettingsPage::requestQueryFullScan, tcpManager, &TcpManager::queryFullScanParams);
    connect(settingsPage, &SettingsPage::requestQueryDeviceIp, tcpManager, &TcpManager::queryDeviceIp);
    connect(settingsPage, &SettingsPage::requestQueryTcpServerIp, tcpManager, &TcpManager::queryTcpServerIp);

    connect(homePage, &HomePage::commJammingToggled, this,
            [this](bool checked)
            {
                tcpManager->setDeviceJammingMode(0, checked ? 1 : 0);
            });
    connect(homePage, &HomePage::navJammingToggled, this,
            [this](bool checked)
            {
                tcpManager->setDeviceJammingMode(1, checked ? 1 : 0);
            });
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
    const QString deviceTimestamp = deviceInfo.value("timestamp").toString().trimmed();
    if (!deviceTimestamp.isEmpty())
    {
        settingsPage->updateDeviceReportedTime(deviceTimestamp);
    }

    if (deviceInfo.value("protocolDataType").toInt() == 2)
    {
        const QString deviceSerial = deviceInfo.value("deviceName").toString().trimmed();
        settingsPage->updateFirmwareDeviceSerial(deviceSerial);
        emit deviceStatusInfoUpdated(deviceInfo);
    }

    if (!deviceInfo.contains("longitude") || !deviceInfo.contains("latitude"))
    {
        return;
    }

    const double lng = deviceInfo["longitude"].toDouble();
    const double lat = deviceInfo["latitude"].toDouble();
    const double alt = deviceInfo["altitude"].toDouble();
    const double yaw = deviceInfo["azimuth"].toDouble();
    const double pitch = deviceInfo["pitch"].toDouble();

    homePage->updateDeviceInfo(lng, lat, alt, yaw, pitch);
}

void AppController::updateDeviceStatus(DeviceConnectionState state, const QString &errorMessage)
{
    deviceStatusValue.connectionState = state;
    deviceStatusValue.lastError = errorMessage;
    emit deviceStatusChanged();
}
