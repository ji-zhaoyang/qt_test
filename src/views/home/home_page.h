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

class HomePage : public QWidget
{
    Q_OBJECT

  public:
    explicit HomePage(QWidget *parent = nullptr);

    void updateDeviceInfo(double lng, double lat, double alt, double yaw, double pitch);
    void updateDroneTargetInfo(const QJsonObject &targetInfo);
    void updateDroneDirectionFindingResponse(quint32 targetId, bool enabled, bool success, const QString &msg);
    void updateDroneDirectionPowerReport(const QJsonObject &reportData);
    void updateDronePrecisionStrikeResponse(quint32 targetId, bool enabled, bool success, const QString &msg);
    void updateDroneWideBandJammingResponse(quint32 targetId, bool enabled, bool success, const QString &msg);
    void updateDeviceJammingSetResponse(int mode, int switchStatus, bool success, const QString &msg);
    void updateDeviceJammingReported(int mode, int switchStatus);
    void setWarningRemoveTimeSeconds(int seconds);

  signals:
    void fullscreenChanged(bool isFullscreen);
    void commJammingToggled(bool checked);
    void navJammingToggled(bool checked);
    void requestDroneDirectionFinding(bool enabled, quint32 targetId);
    void requestDronePrecisionStrike(bool enabled, quint32 timestamp, const QString &sn, int type, quint32 targetId);
    void requestDroneWideBandJamming(bool enabled, quint32 frequencyKhz, const QString &sn, quint32 targetId);

  private:
    void setupUi();
    void setupConnections();
    void resizeEvent(QResizeEvent *event) override;
    void showHomeToast(const QString &text);
    void updateHomeToastPosition();
    void updateRightPanelVisibility();
    void refreshRightPanelContent(const QJsonObject &targetInfo);
    void refreshRightPanelContentFromPendingTargets();
    void cleanupExpiredDroneTargets();
    QString formatPanelFrequency(double frequencyKhz) const;
    QString resolvePanelSerialNumber(const QJsonObject &targetInfo) const;
    QString resolvePanelModelName(const QJsonObject &targetInfo) const;
    void dispatchAllDroneTargetsToMap();

    QWebEngineView *mapWebView;
    HomeWebBridge *homeWebBridge;
    QWidget *bottomBar;
    QWidget *rightPanel;
    QLabel *rightPanelTitleLabel;
    QLabel *rightPanelCountValueLabel;
    QLabel *rightPanelModelValueLabel;
    QLabel *rightPanelSerialValueLabel;
    QLabel *rightPanelFrequencyValueLabel;
    QLabel *rightPanelDistanceValueLabel;
    BottomConsole *bottomConsole;
    bool mapPageLoaded;
    bool hasPendingDeviceInfo;
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
