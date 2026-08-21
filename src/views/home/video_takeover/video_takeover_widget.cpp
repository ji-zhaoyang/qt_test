#include "video_takeover_widget.h"

#include "video_takeover_constants.h"

#include <QColor>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>

namespace
{
QLabel *createMetaChip(QWidget *parent)
{
    auto *chip = new QLabel(parent);
    chip->setVisible(false);
    chip->setStyleSheet(QStringLiteral("QLabel { background-color: rgba(255,255,255,0.06); color: #b8c0cc; "
                                       "padding: 3px 8px; border-radius: 4px; font-size: 11px; }"));
    return chip;
}
} // namespace

VideoTakeoverWidget::VideoTakeoverWidget(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("background-color: rgba(38, 41, 48, 0.96); "
                                 "border: 1px solid rgba(255,255,255,0.10); border-radius: 8px;"));
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(28);
    shadow->setOffset(0, 8);
    shadow->setColor(QColor(0, 0, 0, 120));
    setGraphicsEffect(shadow);
    hide();
    setupUi();
}

void VideoTakeoverWidget::setupUi()
{
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    QWidget *headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(44);
    headerWidget->setStyleSheet(QStringLiteral("background-color: rgba(255,255,255,0.03); "
                                               "border-bottom: 1px solid rgba(255,255,255,0.08);"));
    QHBoxLayout *headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(14, 0, 10, 0);
    headerLayout->setSpacing(8);

    liveDot_ = new QLabel(headerWidget);
    liveDot_->setFixedSize(8, 8);
    liveDot_->setStyleSheet(QStringLiteral("background-color: #5a6270; border-radius: 4px;"));
    headerLayout->addWidget(liveDot_);

    QLabel *titleLabel = new QLabel(QStringLiteral("图传"), headerWidget);
    titleLabel->setStyleSheet(QStringLiteral("color: #f3f5f8; font-size: 15px; font-weight: 700;"));
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch(1);

    closeButton_ = new QPushButton(QStringLiteral("×"), headerWidget);
    closeButton_->setCursor(Qt::PointingHandCursor);
    closeButton_->setFixedSize(28, 28);
    closeButton_->setToolTip(QStringLiteral("关闭图传"));
    closeButton_->setStyleSheet(
        QStringLiteral("QPushButton { background-color: transparent; color: #9ba3b4; border: none; "
                       "border-radius: 14px; font-size: 18px; font-weight: 400; padding-bottom: 2px; }"
                       "QPushButton:hover { background-color: rgba(255,255,255,0.08); color: #ffffff; }"
                       "QPushButton:pressed { background-color: rgba(255,255,255,0.04); }"));
    headerLayout->addWidget(closeButton_);
    rootLayout->addWidget(headerWidget);

    QWidget *metaBar = new QWidget(this);
    QHBoxLayout *metaLayout = new QHBoxLayout(metaBar);
    metaLayout->setContentsMargins(12, 8, 12, 4);
    metaLayout->setSpacing(6);
    idChip_ = createMetaChip(metaBar);
    freqChip_ = createMetaChip(metaBar);
    resChip_ = createMetaChip(metaBar);
    timeChip_ = createMetaChip(metaBar);
    statusChip_ = createMetaChip(metaBar);
    statusChip_->setVisible(true);
    statusChip_->setText(QStringLiteral("等待图像数据"));
    metaLayout->addWidget(idChip_);
    metaLayout->addWidget(freqChip_);
    metaLayout->addWidget(resChip_);
    metaLayout->addWidget(timeChip_);
    metaLayout->addWidget(statusChip_);
    metaLayout->addStretch(1);
    rootLayout->addWidget(metaBar);

    QWidget *bodyWidget = new QWidget(this);
    QVBoxLayout *bodyLayout = new QVBoxLayout(bodyWidget);
    bodyLayout->setContentsMargins(12, 4, 12, 12);
    bodyLayout->setSpacing(0);

    videoFrame_ = new QFrame(bodyWidget);
    videoFrame_->setStyleSheet(QStringLiteral("QFrame { background-color: #0b1016; "
                                             "border: 1px solid rgba(255,255,255,0.06); border-radius: 6px; }"));
    QVBoxLayout *videoLayout = new QVBoxLayout(videoFrame_);
    videoLayout->setContentsMargins(1, 1, 1, 1);

    imageLabel_ = new QLabel(videoFrame_);
    imageLabel_->setMinimumSize(VideoTakeover::kImageMinWidth, VideoTakeover::kImageMinHeight);
    imageLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    imageLabel_->setAlignment(Qt::AlignCenter);
    imageLabel_->setScaledContents(false);
    imageLabel_->setText(QStringLiteral("等待图像数据"));
    imageLabel_->setStyleSheet(QStringLiteral("color: #6f7888; font-size: 13px; background: transparent; border: none;"));
    videoLayout->addWidget(imageLabel_);
    bodyLayout->addWidget(videoFrame_, 1);
    rootLayout->addWidget(bodyWidget, 1);

    connect(closeButton_, &QPushButton::clicked, this,
            [this]()
            {
                emit closeRequested(sessionTargetId_, sessionFrequencyKhz_);
            });
}

void VideoTakeoverWidget::applyMetaText(const QString &metaText)
{
    const QString text = metaText.trimmed();
    idChip_->setVisible(false);
    freqChip_->setVisible(false);
    resChip_->setVisible(false);
    timeChip_->setVisible(false);
    statusChip_->setVisible(false);

    if (text.isEmpty())
    {
        statusChip_->setText(QStringLiteral("等待图像数据"));
        statusChip_->setVisible(true);
        return;
    }

    const QRegularExpression idRe(QStringLiteral("目标ID：(\\d+)"));
    const QRegularExpression freqRe(QStringLiteral("频点：([\\d.]+)\\s*MHz"));
    const QRegularExpression timeRe(QStringLiteral("更新时间：(\\d{2}:\\d{2}:\\d{2})"));
    const QRegularExpression resRe(QStringLiteral("分辨率：(\\d+)\\s*x\\s*(\\d+)"));
    const QRegularExpression statusRe(QStringLiteral("状态：([^\\s]+(?:\\s+[^\\s]+)*)"));

    const QRegularExpressionMatch idMatch = idRe.match(text);
    if (idMatch.hasMatch())
    {
        idChip_->setText(QStringLiteral("ID %1").arg(idMatch.captured(1)));
        idChip_->setVisible(true);
    }

    const QRegularExpressionMatch freqMatch = freqRe.match(text);
    if (freqMatch.hasMatch())
    {
        freqChip_->setText(QStringLiteral("%1 MHz").arg(freqMatch.captured(1)));
        freqChip_->setVisible(true);
    }

    const QRegularExpressionMatch resMatch = resRe.match(text);
    if (resMatch.hasMatch())
    {
        resChip_->setText(QStringLiteral("%1×%2").arg(resMatch.captured(1), resMatch.captured(2)));
        resChip_->setVisible(true);
    }

    const QRegularExpressionMatch timeMatch = timeRe.match(text);
    if (timeMatch.hasMatch())
    {
        timeChip_->setText(timeMatch.captured(1));
        timeChip_->setVisible(true);
    }

    const QRegularExpressionMatch statusMatch = statusRe.match(text);
    if (statusMatch.hasMatch())
    {
        statusChip_->setText(statusMatch.captured(1).trimmed());
        statusChip_->setVisible(true);
        return;
    }

    if (text.contains(QStringLiteral("正在关闭")))
    {
        statusChip_->setText(QStringLiteral("正在关闭"));
        statusChip_->setVisible(true);
        return;
    }

    if (text.contains(QStringLiteral("连接已断开")))
    {
        statusChip_->setText(QStringLiteral("连接已断开"));
        statusChip_->setVisible(true);
        return;
    }

    if (!idChip_->isVisible() && !freqChip_->isVisible())
    {
        statusChip_->setText(text);
        statusChip_->setVisible(true);
    }
}

void VideoTakeoverWidget::setLiveIndicator(bool active)
{
    if (!liveDot_)
    {
        return;
    }

    liveDot_->setStyleSheet(active ? QStringLiteral("background-color: #3ecf7a; border-radius: 4px;")
                                   : QStringLiteral("background-color: #5a6270; border-radius: 4px;"));
}

void VideoTakeoverWidget::setAnchorGeometry(const QRect &mapAreaRect)
{
    anchorRect_ = mapAreaRect;

    const int mapPaneWidth = qMax(0, mapAreaRect.width() - VideoTakeover::kWebTargetPanelWidth);
    const int availableWidth = qMax(0, mapPaneWidth - VideoTakeover::kPanelMargin * 2);
    const int availableHeight = qMax(0, mapAreaRect.height() - VideoTakeover::kPanelMargin * 2);
    const int panelWidth = qMin(availableWidth, VideoTakeover::kPanelMaxWidth);
    const int panelHeight = qMin(availableHeight, VideoTakeover::kPanelMaxHeight);
    const int panelX = mapAreaRect.x() + mapPaneWidth - panelWidth - VideoTakeover::kPanelMargin;
    const int panelY = mapAreaRect.y() + VideoTakeover::kPanelMargin;
    setGeometry(panelX, panelY, panelWidth, panelHeight);

    refreshImageCanvasSize();
    refreshImageDisplay();
    raise();
}

void VideoTakeoverWidget::refreshImageCanvasSize()
{
    if (!imageLabel_)
    {
        return;
    }

    const QSize labelSize = imageLabel_->contentsRect().size();
    if (labelSize.width() <= 0 || labelSize.height() <= 0)
    {
        return;
    }

    if (!imageCanvasSize_.isValid() || qAbs(labelSize.width() - imageCanvasSize_.width()) > 2
        || qAbs(labelSize.height() - imageCanvasSize_.height()) > 2)
    {
        imageCanvasSize_ = labelSize;
        videoDisplaySize_ = QSize();
        videoOffsetX_ = 0;
        videoOffsetY_ = 0;
    }
}

void VideoTakeoverWidget::ensureDisplayMetrics()
{
    refreshImageCanvasSize();
    if (!imageCanvasSize_.isValid() || videoDisplaySize_.isValid())
    {
        return;
    }

    const int canvasW = imageCanvasSize_.width();
    const int canvasH = imageCanvasSize_.height();

    int videoW = canvasW;
    int videoH = qMax(1, static_cast<int>(qRound(videoW * double(VideoTakeover::kSourceHeight)
                                                  / double(VideoTakeover::kSourceWidth))));
    if (videoH > canvasH)
    {
        videoH = canvasH;
        videoW = qMax(1, static_cast<int>(qRound(videoH * double(VideoTakeover::kSourceWidth)
                                                / double(VideoTakeover::kSourceHeight))));
    }

    videoDisplaySize_ = QSize(videoW, videoH);
    videoOffsetX_ = (canvasW - videoW) / 2;
    videoOffsetY_ = (canvasH - videoH) / 2;
}

void VideoTakeoverWidget::refreshImageDisplay()
{
    if (!imageLabel_)
    {
        return;
    }

    if (currentImage_.isNull())
    {
        setLiveIndicator(false);
        imageLabel_->setPixmap(QPixmap());
        if (imageLabel_->text().isEmpty())
        {
            imageLabel_->setText(QStringLiteral("等待图像数据"));
        }
        return;
    }

    ensureDisplayMetrics();
    if (!imageCanvasSize_.isValid() || !videoDisplaySize_.isValid())
    {
        return;
    }

    const QPixmap scaledPixmap = QPixmap::fromImage(currentImage_)
                                     .scaled(videoDisplaySize_, Qt::KeepAspectRatio, Qt::FastTransformation);

    QPixmap canvas(imageCanvasSize_);
    canvas.fill(QColor(QStringLiteral("#0b1016")));

    QPainter painter(&canvas);
    int drawX = videoOffsetX_;
    int drawY = videoOffsetY_;
    if (scaledPixmap.width() != videoDisplaySize_.width() || scaledPixmap.height() != videoDisplaySize_.height())
    {
        drawX += (videoDisplaySize_.width() - scaledPixmap.width()) / 2;
        drawY += (videoDisplaySize_.height() - scaledPixmap.height()) / 2;
    }
    painter.drawPixmap(drawX, drawY, scaledPixmap);

    imageLabel_->setPixmap(canvas);
    imageLabel_->setText(QString());
    setLiveIndicator(true);
}

void VideoTakeoverWidget::showSession(quint32 targetId, quint32 frequencyKhz, const QString &metaText)
{
    sessionTargetId_ = targetId;
    sessionFrequencyKhz_ = frequencyKhz;
    updateMeta(metaText);
    clearFrame();
    if (anchorRect_.isValid())
    {
        setAnchorGeometry(anchorRect_);
    }
    show();
    raise();
    ensureDisplayMetrics();
}

void VideoTakeoverWidget::hideSession()
{
    sessionTargetId_ = 0;
    sessionFrequencyKhz_ = 0;
    clearFrame();
    updateMeta(QString());
    hide();
}

void VideoTakeoverWidget::updateMeta(const QString &metaText)
{
    applyMetaText(metaText);
}

void VideoTakeoverWidget::clearFrame()
{
    imageCanvasSize_ = QSize();
    videoDisplaySize_ = QSize();
    videoOffsetX_ = 0;
    videoOffsetY_ = 0;
    currentImage_ = QImage();
    refreshImageDisplay();
}

void VideoTakeoverWidget::displayFrame(const QImage &image)
{
    currentImage_ = image;
    refreshImageDisplay();
}

void VideoTakeoverWidget::showDecodeFailed()
{
    currentImage_ = QImage();
    setLiveIndicator(false);
    if (imageLabel_)
    {
        imageLabel_->setPixmap(QPixmap());
        imageLabel_->setText(QStringLiteral("图像解码失败"));
    }
}
