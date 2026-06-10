#ifndef HOME_WEB_BRIDGE_H
#define HOME_WEB_BRIDGE_H

#include <QJsonObject>
#include <QObject>
#include <QPointer>
#include <QString>

class QWebEngineView;

class HomeWebBridge : public QObject
{
    Q_OBJECT

public:
    explicit HomeWebBridge(QWebEngineView *webView = nullptr, QObject *parent = nullptr);

    void setWebView(QWebEngineView *webView);
    QWebEngineView *webView() const;

    // Phase 1 entry point: HomePage forwards title changes here instead of parsing them inline.
    bool handleTitleCommand(const QString &title);

    // Phase 1 exit point: HomePage/AppController sends named events back to the web page here.
    void sendEventToWeb(const QString &eventName, const QJsonObject &payload);

    // Typed wrappers that can gradually replace HomePage::dispatch...ToMap().
    void sendDeviceInfo(double lng, double lat, double alt, double yaw, double pitch);
    void sendDroneTarget(const QJsonObject &targetInfo);
    void sendWarningRemoveTimeSeconds(int seconds);
    void sendDirectionFindingResponse(quint32 targetId, bool enabled, bool success, const QString &message);
    void sendDirectionPowerReport(const QJsonObject &reportData);
    void sendPrecisionStrikeResponse(quint32 targetId, bool enabled, bool success, const QString &message);
    void sendWideBandJammingResponse(quint32 targetId, bool enabled, bool success, const QString &message);

signals:
    void fullscreenRequested(bool enabled);
    void directionFindingRequested(bool enabled, quint32 targetId);
    void precisionStrikeRequested(bool enabled, quint32 timestamp, const QString &sn, int type, quint32 targetId);
    void wideBandJammingRequested(bool enabled, quint32 frequencyKhz, const QString &sn, quint32 targetId);
    void unknownCommandReceived(const QString &title);

private:
    bool handleDirectionFindingCommand(const QStringList &parts);
    bool handlePrecisionStrikeCommand(const QStringList &parts);
    bool handleWideBandJammingCommand(const QStringList &parts);
    void resetPageTitle() const;

    QPointer<QWebEngineView> webView_;
};

#endif // HOME_WEB_BRIDGE_H
