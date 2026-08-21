#ifndef VIDEO_TAKEOVER_FACADE_H
#define VIDEO_TAKEOVER_FACADE_H

#include <QByteArray>
#include <QObject>
#include <QSize>

class HomeWebBridge;
class QWebEngineView;
class VideoFramePipeline;
class VideoTakeoverPanelController;
class VideoTakeoverWidget;

class VideoTakeoverFacade : public QObject
{
    Q_OBJECT

  public:
    explicit VideoTakeoverFacade(QWidget *hostWidget, QWebEngineView *mapWebView, QObject *parent = nullptr);

    void setWebBridge(HomeWebBridge *bridge);
    void setMapPageLoaded(bool loaded);
    void updateMapGeometry();

  public slots:
    void onUserRequest(bool enabled, quint32 frequencyKhz, quint32 targetId);
    void on290(quint32 targetId, bool enabled, bool success, const QString &msg);
    void on291(const QByteArray &jpegPayload, const QSize &frameSize);
    void onConnectionLost();

  signals:
    void takeoverRequested(bool enabled, quint32 frequencyKhz, quint32 targetId);
    void toastRequested(const QString &text);

  private:
    void wireInternal();

    QWebEngineView *mapWebView_ = nullptr;
    HomeWebBridge *webBridge_ = nullptr;
    bool mapPageLoaded_ = false;
    VideoTakeoverPanelController *controller_ = nullptr;
    VideoFramePipeline *pipeline_ = nullptr;
    VideoTakeoverWidget *widget_ = nullptr;
};

#endif // VIDEO_TAKEOVER_FACADE_H
