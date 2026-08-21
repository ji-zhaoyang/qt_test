#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include <QDateTime>
#include <QHash>
#include <QJsonObject>
#include <QTimer>
#include <QWidget>

class BottomConsole;
class HomeWebBridge;
class QLabel;
class QResizeEvent;
class QWebEngineView;
class VideoTakeoverFacade;
class WhitelistRepository;

class HomePage : public QWidget
{
    Q_OBJECT

  public:
    explicit HomePage(QWidget *parent = nullptr);
    ~HomePage() override;

    VideoTakeoverFacade *videoTakeoverFacade() const;

    void updateDeviceInfo(double lng, double lat, double alt, double yaw, double pitch);
    void updateDroneTargetInfo(const QJsonObject &targetInfo);
    void updateDroneDirectionFindingResponse(quint32 targetId, bool enabled, bool success, const QString &msg);
    void updateDroneDirectionPowerReport(const QJsonObject &reportData);
    void updateDronePrecisionStrikeResponse(quint32 targetId, bool enabled, bool success, const QString &msg);
    void updateDroneWideBandJammingResponse(quint32 targetId, bool enabled, bool success, const QString &msg);
    void updateDeviceJammingSetResponse(int mode, int switchStatus, bool success, const QString &msg);
    void updateDeviceJammingReported(int mode, int switchStatus);
    void setWarningRemoveTimeSeconds(int seconds);
    void setWhitelistRepository(WhitelistRepository *repository);
    void setScreenFlashEnabled(bool enabled);
    void setHomePageVisible(bool visible);
    void addTargetToWhitelist(const QString &serialNumber, const QString &recordKey);

  signals:
    void fullscreenChanged(bool isFullscreen);
    void commJammingToggled(bool checked);
    void navJammingToggled(bool checked);
    void requestDroneDirectionFinding(bool enabled, quint32 targetId);
    void requestDronePrecisionStrike(bool enabled, quint32 timestamp, const QString &sn, int type, quint32 targetId);
    void requestDroneWideBandJamming(bool enabled, quint32 frequencyKhz, const QString &sn, quint32 targetId);
    void requestDroneVideoTakeover(bool enabled, quint32 frequencyKhz, quint32 targetId);

  private:
    void setupUi();
    void setupConnections();
    void resizeEvent(QResizeEvent *event) override;
    void showHomeToast(const QString &text);
    void updateHomeToastPosition();
    void cleanupExpiredDroneTargets();
    void dispatchAllDroneTargetsToMap();
    void evaluateMapAlarmFlash(bool forcePush = false);

    QWebEngineView *mapWebView;
    HomeWebBridge *homeWebBridge;
    VideoTakeoverFacade *videoTakeoverFacade_;
    WhitelistRepository *whitelistRepository;
    QWidget *bottomBar;
    BottomConsole *bottomConsole;
    bool mapPageLoaded;
    bool hasPendingDeviceInfo;
    bool screenFlashEnabled;
    bool homePageVisible;
    bool mapAlarmFlashActive;
    double pendingLng;
    double pendingLat;
    double pendingAlt;
    double pendingYaw;
    double pendingPitch;
    int pendingWarningRemoveTimeSeconds;
    bool commJammingEnabled;
    bool navJammingEnabled;
    QWidget *toastWidget;
    QLabel *toastLabel;
    QTimer *toastTimer;
    QTimer *droneTargetCleanupTimer;
    QHash<QString, QJsonObject> pendingDroneTargets;
    QHash<QString, QDateTime> pendingDroneTargetLastSeenAt;
};

#endif // HOME_PAGE_H
