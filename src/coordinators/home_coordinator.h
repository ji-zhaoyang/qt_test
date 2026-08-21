#ifndef HOME_COORDINATOR_H
#define HOME_COORDINATOR_H

#include <QObject>

class HomePage;
class SettingsPage;
class StatsRepository;
class TcpManager;
class QJsonObject;

class HomeCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit HomeCoordinator(HomePage *homePage, SettingsPage *settingsPage, TcpManager *tcpManager,
                             StatsRepository *statsRepository = nullptr, QObject *parent = nullptr);

    void setupConnections();
    void initializeState();

public slots:
    void handleDeviceInfo(const QJsonObject &deviceInfo);
    void handleDroneTargetReported(const QJsonObject &targetInfo);

private:
    HomePage *homePage_;
    SettingsPage *settingsPage_;
    TcpManager *tcpManager_;
    StatsRepository *statsRepository_;
};

#endif // HOME_COORDINATOR_H
