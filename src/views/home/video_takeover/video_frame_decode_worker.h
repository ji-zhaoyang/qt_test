#ifndef VIDEO_FRAME_DECODE_WORKER_H
#define VIDEO_FRAME_DECODE_WORKER_H

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QSize>

class VideoFrameDecodeWorker : public QObject
{
    Q_OBJECT

  public:
    explicit VideoFrameDecodeWorker(QObject *parent = nullptr);

  public slots:
    void enqueueFrame(const QByteArray &jpegPayload, const QSize &frameSize, quint64 frameToken);

  signals:
    void frameDecoded(const QImage &image, const QSize &frameSize, quint64 frameToken, bool success);

  private slots:
    void processLatestFrame();

  private:
    QByteArray pendingJpeg_;
    QSize pendingFrameSize_;
    quint64 pendingFrameToken_ = 0;
    bool processScheduled_ = false;
};

#endif // VIDEO_FRAME_DECODE_WORKER_H
