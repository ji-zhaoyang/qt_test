#include "video_takeover_panel_controller.h"

#include "video_takeover_constants.h"

#include <QDateTime>
#include <QTimer>

VideoTakeoverPanelController::VideoTakeoverPanelController(QObject *parent)
    : QObject(parent), firstFrameTimer_(new QTimer(this)), displayTimer_(new QTimer(this)), active_(false),
      pending_(false), firstFrameReceived_(false), targetId_(0), frequencyKhz_(0)
{
    firstFrameTimer_->setSingleShot(true);
    firstFrameTimer_->setInterval(VideoTakeover::kFirstFrameTimeoutMs);
    connect(firstFrameTimer_, &QTimer::timeout, this, &VideoTakeoverPanelController::onFirstFrameTimeout);

    displayTimer_->setInterval(VideoTakeover::kDisplayIntervalMs);
    displayTimer_->setTimerType(Qt::PreciseTimer);
    connect(displayTimer_, &QTimer::timeout, this, &VideoTakeoverPanelController::flushPendingFrame);
}

void VideoTakeoverPanelController::onUserRequest(bool enabled, quint32 frequencyKhz, quint32 targetId)
{
    targetId_ = targetId;
    frequencyKhz_ = frequencyKhz;

    if (enabled)
    {
        active_ = false;
        pending_ = true;
        firstFrameReceived_ = false;
        firstFrameTimer_->stop();
        displayTimer_->stop();
        pendingJpeg_.clear();
        pendingFrameSize_ = QSize();
        lastMetaUpdateMs_ = 0;
        emit panelClearRequested();
        emit panelShowRequested(targetId_, frequencyKhz_, buildMetaText());
    }
    else
    {
        active_ = false;
        pending_ = false;
        firstFrameTimer_->stop();
        displayTimer_->stop();
        pendingJpeg_.clear();
        emit panelClearRequested();
        emit panelMetaChanged(QStringLiteral("正在关闭图传接管..."));
    }

    emit takeoverRequested(enabled, frequencyKhz_, targetId_);
}

void VideoTakeoverPanelController::onDeviceResponse(quint32 targetId, bool enabled, bool success, const QString &msg)
{
    if (!success)
    {
        pending_ = false;
        active_ = false;
        firstFrameReceived_ = false;
        firstFrameTimer_->stop();
        displayTimer_->stop();
        pendingJpeg_.clear();
        emit panelHideRequested();
        emit toastRequested(msg.isEmpty() ? QStringLiteral("图传接管执行失败") : msg);
        return;
    }

    pending_ = false;
    active_ = enabled;
    targetId_ = targetId;
    firstFrameReceived_ = false;

    if (enabled)
    {
        firstFrameTimer_->start();
        displayTimer_->stop();
        pendingJpeg_.clear();
        pendingFrameSize_ = QSize();
        lastMetaUpdateMs_ = 0;
        emit panelClearRequested();
        emit panelShowRequested(targetId_, frequencyKhz_, buildMetaText());
        emit toastRequested(QStringLiteral("图传接管已开启"));
        return;
    }

    firstFrameTimer_->stop();
    displayTimer_->stop();
    pendingJpeg_.clear();
    emit panelHideRequested();
    targetId_ = 0;
    frequencyKhz_ = 0;
    firstFrameReceived_ = false;
    emit toastRequested(QStringLiteral("图传接管已关闭"));
}

void VideoTakeoverPanelController::onVideoFrame(const QByteArray &jpegPayload, const QSize &frameSize)
{
    if (!active_ || jpegPayload.isEmpty())
    {
        return;
    }

    if (!firstFrameReceived_)
    {
        firstFrameReceived_ = true;
        firstFrameTimer_->stop();
    }

    pendingJpeg_ = jpegPayload;
    if (frameSize.isValid())
    {
        pendingFrameSize_ = frameSize;
    }

    if (!displayTimer_->isActive())
    {
        flushPendingFrame();
        displayTimer_->start();
    }
}

void VideoTakeoverPanelController::flushPendingFrame()
{
    if (pendingJpeg_.isEmpty())
    {
        return;
    }

    const QByteArray jpegPayload = pendingJpeg_;
    const QSize frameSize = pendingFrameSize_;
    pendingJpeg_.clear();

    emit panelFrameReady(jpegPayload, frameSize);

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (lastMetaUpdateMs_ == 0 || nowMs - lastMetaUpdateMs_ >= VideoTakeover::kMetaUpdateIntervalMs)
    {
        syncVideoTakeoverPanelMeta(frameSize);
        lastMetaUpdateMs_ = nowMs;
    }
}

void VideoTakeoverPanelController::onConnectionLost()
{
    if (!active_ && !pending_)
    {
        return;
    }

    active_ = false;
    pending_ = false;
    firstFrameReceived_ = false;
    firstFrameTimer_->stop();
    displayTimer_->stop();
    pendingJpeg_.clear();
    emit panelClearRequested();
    emit panelMetaChanged(QStringLiteral("连接已断开，请重新开启图传"));
}

void VideoTakeoverPanelController::onFirstFrameTimeout()
{
    if (!active_ || firstFrameReceived_)
    {
        return;
    }

    syncVideoTakeoverPanelMeta();
    emit toastRequested(QStringLiteral("图传已开启，但3秒内未收到首帧291图像"));
}

void VideoTakeoverPanelController::syncVideoTakeoverPanelMeta(const QSize &frameSize)
{
    emit panelMetaChanged(buildMetaText(frameSize));
}

QString VideoTakeoverPanelController::buildMetaText(const QSize &frameSize) const
{
    QString statusText = QStringLiteral("目标ID：%1   频点：%2 MHz")
                             .arg(targetId_)
                             .arg(QString::number(frequencyKhz_ / 1000.0, 'f', 0));
    if (frameSize.isValid())
    {
        statusText += QStringLiteral("   更新时间：%1").arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")));
        statusText += QStringLiteral("   分辨率：%1 x %2").arg(frameSize.width()).arg(frameSize.height());
    }
    else if (active_ && !firstFrameReceived_ && firstFrameTimer_->isActive())
    {
        statusText += QStringLiteral("   状态：等待首帧");
    }
    else if (active_ && !firstFrameReceived_)
    {
        statusText += QStringLiteral("   状态：首帧超时");
    }
    else if (active_)
    {
        statusText += QStringLiteral("   状态：等待图像数据");
    }
    return statusText;
}
