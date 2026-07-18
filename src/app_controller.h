#ifndef APP_CONTROLLER_H
#define APP_CONTROLLER_H

#include "app_config.h"
#include "device_status.h"
#include <QObject>

class HomePage;
class HomeCoordinator;
class HistoryCoordinator;
class HistoryPage;
class HistoryRepository;
class LocalTimeServiceClient;
class SettingsPage;
class SettingsCoordinator;
class TcpManager;
class DatabaseManager;
class QJsonObject;

class AppController : public QObject
{
    Q_OBJECT

  public:
    explicit AppController(HomePage *homePage, HistoryPage *historyPage, SettingsPage *settingsPage, QObject *parent = nullptr);
    AppController(HomePage *homePage, HistoryPage *historyPage, SettingsPage *settingsPage, const ConnectionConfig &connectionConfig,
                  QObject *parent = nullptr);

    void connectToDevice();
    void setConnectionConfig(const ConnectionConfig &connectionConfig);
    const ConnectionConfig &connectionConfig() const;
    const DeviceStatus &deviceStatus() const;

  signals:
    void deviceStatusChanged();
    void deviceStatusInfoUpdated(const QJsonObject &deviceInfo);

  private slots:
    void onTcpConnected();
    void onTcpDisconnected();
    void onTcpError(const QString &errorStr);
    void onDeviceInfoReceived(const QJsonObject &deviceInfo);

  private:
    void setupConnections();
    void updateDeviceStatus(DeviceConnectionState state, const QString &errorMessage = QString());

    HomeCoordinator *homeCoordinator;
    HistoryCoordinator *historyCoordinator;
    SettingsCoordinator *settingsCoordinator;
    DatabaseManager *databaseManager;
    HistoryRepository *historyRepository;
    LocalTimeServiceClient *localTimeServiceClient;
    TcpManager *tcpManager;
    ConnectionConfig connectionConfigValue;
    DeviceStatus deviceStatusValue;
};

#endif // APP_CONTROLLER_H
