#ifndef VIDEO_FRAME_PIPELINE_H
#define VIDEO_FRAME_PIPELINE_H

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QSize>

class QThread;
class VideoFrameDecodeWorker;

class VideoFramePipeline : public QObject
{
    Q_OBJECT

  public:
    explicit VideoFramePipeline(QObject *parent = nullptr);
    ~VideoFramePipeline() override;

    void submitFrame(const QByteArray &jpegPayload, const QSize &frameSize);
    void invalidatePending();

  signals:
    void frameReady(const QImage &image, const QSize &frameSize);
    void decodeFailed();

  private slots:
    void onFrameDecoded(const QImage &image, const QSize &frameSize, quint64 frameToken, bool success);

  private:
    QThread *decodeThread_ = nullptr;
    VideoFrameDecodeWorker *decodeWorker_ = nullptr;
    quint64 decodeRequestId_ = 0;
    quint64 minDisplayFrameId_ = 0;
    quint64 lastDisplayedFrameId_ = 0;
};

#endif // VIDEO_FRAME_PIPELINE_H
