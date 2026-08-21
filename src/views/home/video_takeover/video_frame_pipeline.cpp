#include "video_frame_pipeline.h"

#include "video_frame_decode_worker.h"

#include <QMetaObject>
#include <QThread>

VideoFramePipeline::VideoFramePipeline(QObject *parent) : QObject(parent)
{
    decodeThread_ = new QThread(this);
    decodeWorker_ = new VideoFrameDecodeWorker();
    decodeWorker_->moveToThread(decodeThread_);
    connect(decodeThread_, &QThread::finished, decodeWorker_, &QObject::deleteLater);
    connect(decodeWorker_, &VideoFrameDecodeWorker::frameDecoded, this, &VideoFramePipeline::onFrameDecoded,
            Qt::QueuedConnection);
    decodeThread_->start();
}

VideoFramePipeline::~VideoFramePipeline()
{
    if (decodeThread_)
    {
        decodeThread_->quit();
        decodeThread_->wait();
    }
}

void VideoFramePipeline::submitFrame(const QByteArray &jpegPayload, const QSize &frameSize)
{
    if (jpegPayload.isEmpty())
    {
        return;
    }

    const quint64 frameToken = ++decodeRequestId_;
    QMetaObject::invokeMethod(decodeWorker_, "enqueueFrame", Qt::QueuedConnection, Q_ARG(QByteArray, jpegPayload),
                              Q_ARG(QSize, frameSize), Q_ARG(quint64, frameToken));
}

void VideoFramePipeline::invalidatePending()
{
    minDisplayFrameId_ = decodeRequestId_ + 1;
    lastDisplayedFrameId_ = 0;
}

void VideoFramePipeline::onFrameDecoded(const QImage &image, const QSize &frameSize, quint64 frameToken, bool success)
{
    if (frameToken < minDisplayFrameId_)
    {
        return;
    }

    if (frameToken <= lastDisplayedFrameId_)
    {
        return;
    }

    if (!success || image.isNull())
    {
        if (frameToken == decodeRequestId_)
        {
            emit decodeFailed();
        }
        return;
    }

    lastDisplayedFrameId_ = frameToken;
    emit frameReady(image, frameSize);
}
