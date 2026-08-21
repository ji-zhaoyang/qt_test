#include "video_frame_decode_worker.h"

#include <QImage>
#include <QMetaObject>

VideoFrameDecodeWorker::VideoFrameDecodeWorker(QObject *parent) : QObject(parent)
{
}

void VideoFrameDecodeWorker::enqueueFrame(const QByteArray &jpegPayload, const QSize &frameSize, quint64 frameToken)
{
    pendingJpeg_ = jpegPayload;
    pendingFrameSize_ = frameSize;
    pendingFrameToken_ = frameToken;

    if (!processScheduled_)
    {
        processScheduled_ = true;
        QMetaObject::invokeMethod(this, "processLatestFrame", Qt::QueuedConnection);
    }
}

void VideoFrameDecodeWorker::processLatestFrame()
{
    if (pendingJpeg_.isEmpty())
    {
        processScheduled_ = false;
        return;
    }

    const QByteArray jpegPayload = pendingJpeg_;
    const QSize frameSize = pendingFrameSize_;
    const quint64 frameToken = pendingFrameToken_;
    pendingJpeg_.clear();
    processScheduled_ = false;

    QImage image;
    const bool success = image.loadFromData(jpegPayload, "JPG") || image.loadFromData(jpegPayload);
    emit frameDecoded(image, frameSize, frameToken, success);
}
