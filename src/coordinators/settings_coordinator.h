#ifndef SETTINGS_COORDINATOR_H
#define SETTINGS_COORDINATOR_H

#include <QObject>

class LocalTimeServiceClient;
class SettingsPage;
class TcpManager;
class QJsonObject;

class SettingsCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit SettingsCoordinator(SettingsPage *settingsPage, TcpManager *tcpManager,
                                 LocalTimeServiceClient *localTimeServiceClient, QObject *parent = nullptr);

    void setupConnections();

public slots:
    void handleDeviceInfo(const QJsonObject &deviceInfo);

private:
    SettingsPage *settingsPage_;
    TcpManager *tcpManager_;
    LocalTimeServiceClient *localTimeServiceClient_;
};

#endif // SETTINGS_COORDINATOR_H
