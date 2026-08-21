#ifndef VIDEO_TAKEOVER_PANEL_CONTROLLER_H
#define VIDEO_TAKEOVER_PANEL_CONTROLLER_H

#include <QByteArray>
#include <QObject>
#include <QSize>

class QTimer;

class VideoTakeoverPanelController : public QObject
{
    Q_OBJECT

  public:
    explicit VideoTakeoverPanelController(QObject *parent = nullptr);

    void onUserRequest(bool enabled, quint32 frequencyKhz, quint32 targetId);
    void onDeviceResponse(quint32 targetId, bool enabled, bool success, const QString &msg);
    void onVideoFrame(const QByteArray &jpegPayload, const QSize &frameSize);
    void onConnectionLost();

  signals:
    void takeoverRequested(bool enabled, quint32 frequencyKhz, quint32 targetId);
    void toastRequested(const QString &text);
    void panelShowRequested(quint32 targetId, quint32 frequencyKhz, const QString &metaText);
    void panelHideRequested();
    void panelClearRequested();
    void panelMetaChanged(const QString &metaText);
    void panelFrameReady(const QByteArray &jpegPayload, const QSize &frameSize);

  private:
    void syncVideoTakeoverPanelMeta(const QSize &frameSize = QSize());
    QString buildMetaText(const QSize &frameSize = QSize()) const;
    void onFirstFrameTimeout();
    void flushPendingFrame();

    QTimer *firstFrameTimer_;
    QTimer *displayTimer_;
    bool active_;
    bool pending_;
    bool firstFrameReceived_;
    quint32 targetId_;
    quint32 frequencyKhz_;
    QByteArray pendingJpeg_;
    QSize pendingFrameSize_;
    qint64 lastMetaUpdateMs_ = 0;
};

#endif // VIDEO_TAKEOVER_PANEL_CONTROLLER_H
