#ifndef VIDEO_TAKEOVER_WIDGET_H
#define VIDEO_TAKEOVER_WIDGET_H

#include <QImage>
#include <QRect>
#include <QSize>
#include <QWidget>

class QFrame;
class QLabel;
class QPushButton;

class VideoTakeoverWidget : public QWidget
{
    Q_OBJECT

  public:
    explicit VideoTakeoverWidget(QWidget *parent = nullptr);

    void setAnchorGeometry(const QRect &mapAreaRect);
    void showSession(quint32 targetId, quint32 frequencyKhz, const QString &metaText);
    void hideSession();
    void updateMeta(const QString &metaText);
    void clearFrame();
    void displayFrame(const QImage &image);
    void showDecodeFailed();

  signals:
    void closeRequested(quint32 targetId, quint32 frequencyKhz);

  private:
    void setupUi();
    void applyMetaText(const QString &metaText);
    void setLiveIndicator(bool active);
    void refreshImageCanvasSize();
    void ensureDisplayMetrics();
    void refreshImageDisplay();

    QLabel *liveDot_ = nullptr;
    QLabel *idChip_ = nullptr;
    QLabel *freqChip_ = nullptr;
    QLabel *resChip_ = nullptr;
    QLabel *timeChip_ = nullptr;
    QLabel *statusChip_ = nullptr;
    QLabel *imageLabel_ = nullptr;
    QPushButton *closeButton_ = nullptr;
    QFrame *videoFrame_ = nullptr;
    quint32 sessionTargetId_ = 0;
    quint32 sessionFrequencyKhz_ = 0;
    QRect anchorRect_;
    QImage currentImage_;
    QSize imageCanvasSize_;
    QSize videoDisplaySize_;
    int videoOffsetX_ = 0;
    int videoOffsetY_ = 0;
};

#endif // VIDEO_TAKEOVER_WIDGET_H
