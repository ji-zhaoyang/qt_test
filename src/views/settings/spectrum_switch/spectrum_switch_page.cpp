#include "spectrum_switch_page.h"
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTimer>
#include <QToolTip>
#include <QVBoxLayout>
#include <QSizePolicy>
#include <algorithm>

SpectrumSwitchPage::SpectrumSwitchPage(QWidget *parent)
    : QWidget(parent), openSpectrogramButton(nullptr), closeSpectrogramButton(nullptr), spectrogramContainer(nullptr),
      openSpectrumButton(nullptr), closeSpectrumButton(nullptr), fullSpectrumContainer(nullptr), reportInfoLabel(nullptr),
      axisHintLabel(nullptr), fullSpectrumInfoLabel(nullptr), fullSpectrumImageLabel(nullptr), toastWidget(nullptr),
      toastIconLabel(nullptr), toastTextLabel(nullptr), toastHideTimer(nullptr), toastOpacityEffect(nullptr),
      toastFadeInAnimation(nullptr), toastFadeOutAnimation(nullptr), spectrogramVisible(false), fullSpectrumVisible(false)
{
    setupUi();
}

void SpectrumSwitchPage::setupUi()
{
    setObjectName("spectrumSwitchPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#spectrumSwitchPage { background-color: #202020; color: #ffffff; }");

    QVBoxLayout *hostLayout = new QVBoxLayout(this);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }"
                              "QScrollBar:vertical { background: #1e1e1e; width: 10px; margin: 0px; }"
                              "QScrollBar::handle:vertical { background: #555555; min-height: 30px; border-radius: 4px; }"
                              "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
                              "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");
    hostLayout->addWidget(scrollArea);

    QWidget *content = new QWidget(scrollArea);
    content->setStyleSheet("background-color: #202020;");
    scrollArea->setWidget(content);

    QVBoxLayout *pageLayout = new QVBoxLayout(content);
    pageLayout->setContentsMargins(40, 28, 20, 30);
    pageLayout->setSpacing(22);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel *pageTitle = new QLabel(QStringLiteral("频谱图开关"), content);
    pageTitle->setStyleSheet(titleStyle());
    pageLayout->addWidget(pageTitle);

    QFrame *card = new QFrame(content);
    card->setStyleSheet("QFrame { background-color: #2b2b2b; border-radius: 6px; }");
    pageLayout->addWidget(card);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 24, 24, 24);
    cardLayout->setSpacing(20);

    openSpectrogramButton = createActionButton(card, QStringLiteral("打开时频图"));
    closeSpectrogramButton = createActionButton(card, QStringLiteral("关闭时频图"));
    openSpectrumButton = createActionButton(card, QStringLiteral("打开频谱图"));
    closeSpectrumButton = createActionButton(card, QStringLiteral("关闭频谱图"));

    cardLayout->addWidget(openSpectrogramButton);
    cardLayout->addWidget(closeSpectrogramButton);
    cardLayout->addWidget(openSpectrumButton);
    cardLayout->addWidget(closeSpectrumButton);

    QWidget *buttonRow = new QWidget(card);
    buttonRow->setStyleSheet("background-color: transparent;");
    QHBoxLayout *buttonRowLayout = new QHBoxLayout(buttonRow);
    buttonRowLayout->setContentsMargins(0, 0, 0, 0);
    buttonRowLayout->setSpacing(0);
    buttonRowLayout->addStretch();
    cardLayout->addWidget(buttonRow);

    spectrogramContainer = new QWidget(card);
    spectrogramContainer->setStyleSheet("background-color: transparent;");
    QVBoxLayout *spectrogramLayout = new QVBoxLayout(spectrogramContainer);
    spectrogramLayout->setContentsMargins(0, 0, 0, 0);
    spectrogramLayout->setSpacing(12);

    reportInfoLabel = new QLabel(QStringLiteral("等待时频图数据"), spectrogramContainer);
    reportInfoLabel->setStyleSheet("color: #9aa4b2; font-size: 13px;");
    spectrogramLayout->addWidget(reportInfoLabel);

    axisHintLabel = new QLabel(QStringLiteral("横轴 = 频率，纵轴 = 时间"), spectrogramContainer);
    axisHintLabel->setStyleSheet("color: #84a3c4; font-size: 12px;");
    spectrogramLayout->addWidget(axisHintLabel);

    QWidget *previewRow = new QWidget(spectrogramContainer);
    previewRow->setStyleSheet("background-color: transparent;");
    QHBoxLayout *previewLayout = new QHBoxLayout(previewRow);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLayout->setSpacing(3);
    previewLayout->setAlignment(Qt::AlignLeft);
    for (int i = 0; i < 4; ++i)
    {
        QFrame *groupFrame = new QFrame(previewRow);
        groupFrame->setStyleSheet("QFrame { background-color: transparent; border: none; }");
        groupFrame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QVBoxLayout *groupLayout = new QVBoxLayout(groupFrame);
        groupLayout->setContentsMargins(0, 0, 0, 0);
        groupLayout->setSpacing(4);

        QLabel *groupTitleLabel = new QLabel(QStringLiteral("第%1组").arg(i + 1), groupFrame);
        groupTitleLabel->setAlignment(Qt::AlignCenter);
        groupTitleLabel->setStyleSheet("color: #ffffff; font-size: 13px; font-weight: bold;");
        groupTitleLabel->setFixedWidth(72);

        QLabel *groupImageLabel = new QLabel(QStringLiteral("等待数据"), groupFrame);
        groupImageLabel->setAlignment(Qt::AlignCenter);
        groupImageLabel->setFixedSize(72, 320);
        groupImageLabel->setStyleSheet("background-color: #0f2233; color: #84a3c4; border: none;");

        groupTitleLabels.append(groupTitleLabel);
        groupImageLabels.append(groupImageLabel);
        groupLayout->addWidget(groupTitleLabel);
        groupLayout->addWidget(groupImageLabel, 0, Qt::AlignCenter);
        previewLayout->addWidget(groupFrame);
    }
    spectrogramLayout->addWidget(previewRow);
    cardLayout->addWidget(spectrogramContainer);
    setSpectrogramVisible(false);

    fullSpectrumContainer = new QWidget(card);
    fullSpectrumContainer->setStyleSheet("background-color: transparent;");
    QVBoxLayout *fullSpectrumLayout = new QVBoxLayout(fullSpectrumContainer);
    fullSpectrumLayout->setContentsMargins(0, 0, 0, 0);
    fullSpectrumLayout->setSpacing(10);

    fullSpectrumInfoLabel = new QLabel(QStringLiteral("等待频谱图数据"), fullSpectrumContainer);
    fullSpectrumInfoLabel->setStyleSheet("color: #9aa4b2; font-size: 13px;");
    fullSpectrumLayout->addWidget(fullSpectrumInfoLabel);

    QLabel *fullAxisHintLabel = new QLabel(QStringLiteral("横轴 = 频率，纵轴 = 强度"), fullSpectrumContainer);
    fullAxisHintLabel->setStyleSheet("color: #84a3c4; font-size: 12px;");
    fullSpectrumLayout->addWidget(fullAxisHintLabel);

    fullSpectrumImageLabel = new QLabel(QStringLiteral("等待数据"), fullSpectrumContainer);
    fullSpectrumImageLabel->setAlignment(Qt::AlignCenter);
    fullSpectrumImageLabel->setFixedSize(460, 220);
    fullSpectrumImageLabel->setMouseTracking(true);
    fullSpectrumImageLabel->installEventFilter(this);
    fullSpectrumImageLabel->setStyleSheet(
        "background-color: #14283a; color: #84a3c4; border: 1px solid #1b3a55; border-radius: 4px;");
    fullSpectrumLayout->addWidget(fullSpectrumImageLabel, 0, Qt::AlignLeft);

    cardLayout->addWidget(fullSpectrumContainer);
    setFullSpectrumVisible(false);

    pageLayout->addStretch();

    connect(openSpectrogramButton, &QPushButton::clicked, this, &SpectrumSwitchPage::requestOpenSpectrogram);
    connect(closeSpectrogramButton, &QPushButton::clicked, this, &SpectrumSwitchPage::requestCloseSpectrogram);
    connect(openSpectrumButton, &QPushButton::clicked, this, &SpectrumSwitchPage::requestOpenSpectrum);
    connect(closeSpectrumButton, &QPushButton::clicked, this, &SpectrumSwitchPage::requestCloseSpectrum);
}

void SpectrumSwitchPage::showSwitchResult(uint16_t responseDataType, bool success, const QString &message)
{
    switch (responseDataType)
    {
    case 66:
        if (success)
        {
            setSpectrogramVisible(true);
            refreshSpectrumViews();
        }
        showToastResult(success, message);
        break;
    case 68:
        if (success)
        {
            setSpectrogramVisible(false);
        }
        showToastResult(success, message);
        break;
    default:
        showToastResult(success, message);
        break;
    }
}

void SpectrumSwitchPage::setSpectrogramVisible(bool visible)
{
    spectrogramVisible = visible;
    if (spectrogramContainer)
    {
        spectrogramContainer->setVisible(visible);
    }
}

void SpectrumSwitchPage::setFullSpectrumVisible(bool visible)
{
    fullSpectrumVisible = visible;
    if (fullSpectrumContainer)
    {
        fullSpectrumContainer->setVisible(visible);
    }
}

void SpectrumSwitchPage::updateSpectrumReport(const SpectrumReportData &reportData)
{
    currentSpectrumReport = reportData;
    refreshSpectrumViews();
}

void SpectrumSwitchPage::showFullSpectrumSwitchResult(bool enabled, bool success, const QString &message)
{
    if (success)
    {
        setFullSpectrumVisible(enabled);
        if (enabled)
        {
            refreshFullSpectrumView();
        }
    }
    showToastResult(success, message);
}

void SpectrumSwitchPage::updateFullSpectrumReport(const FullSpectrumReportData &reportData)
{
    currentFullSpectrumReport = reportData;
    refreshFullSpectrumView();
}

void SpectrumSwitchPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateToastPosition();
}

bool SpectrumSwitchPage::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == fullSpectrumImageLabel)
    {
        if (event->type() == QEvent::MouseMove && fullSpectrumImageLabel && !currentFullSpectrumReport.data.isEmpty())
        {
            const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            const QRect plotRect = fullSpectrumPlotRect();
            const QPoint pos = mouseEvent->pos();
            if (!plotRect.contains(pos))
            {
                QToolTip::hideText();
                return QWidget::eventFilter(watched, event);
            }

            const qreal xRatio =
                qBound<qreal>(0.0, static_cast<qreal>(pos.x() - plotRect.left()) / qMax(1, plotRect.width()), 1.0);
            const qreal yRatio =
                qBound<qreal>(0.0, static_cast<qreal>(plotRect.bottom() - pos.y()) / qMax(1, plotRect.height()), 1.0);

            const double freqSpan = qMax(1.0, currentFullSpectrumReport.endMhz - currentFullSpectrumReport.startMhz);
            const double freqMhz = currentFullSpectrumReport.startMhz + xRatio * freqSpan;
            const double axisDb = -120.0 + yRatio * 120.0;

            const int sampleIndex =
                qBound(0, static_cast<int>(xRatio * qMax(0, currentFullSpectrumReport.data.size() - 1)),
                       currentFullSpectrumReport.data.size() - 1);
            const int rawValue = currentFullSpectrumReport.data.at(sampleIndex);
            const int sampleDb = qBound(-120, rawValue > 0 ? -rawValue : rawValue, 0);

            QToolTip::showText(fullSpectrumImageLabel->mapToGlobal(pos),
                               QStringLiteral("频率: %1 MHz\n纵坐标: %2 dB\n数据点: %3 dB")
                                   .arg(freqMhz, 0, 'f', 1)
                                   .arg(axisDb, 0, 'f', 1)
                                   .arg(sampleDb),
                               fullSpectrumImageLabel);
        }
        else if (event->type() == QEvent::Leave)
        {
            QToolTip::hideText();
        }
    }

    return QWidget::eventFilter(watched, event);
}

QPushButton *SpectrumSwitchPage::createActionButton(QWidget *parent, const QString &text) const
{
    QPushButton *button = new QPushButton(text, parent);
    button->setFixedSize(460, 38);
    button->setStyleSheet(buttonStyle());
    return button;
}

void SpectrumSwitchPage::ensureToastWidget()
{
    if (toastWidget)
    {
        return;
    }

    toastWidget = new QWidget(this);
    toastWidget->setAttribute(Qt::WA_StyledBackground, true);
    toastWidget->setStyleSheet("background-color: rgba(23, 23, 23, 235); border-radius: 12px;");

    QHBoxLayout *layout = new QHBoxLayout(toastWidget);
    layout->setContentsMargins(18, 12, 18, 12);
    layout->setSpacing(10);

    toastIconLabel = new QLabel(toastWidget);
    toastIconLabel->setFixedSize(20, 20);
    toastIconLabel->setAlignment(Qt::AlignCenter);

    toastTextLabel = new QLabel(toastWidget);
    toastTextLabel->setStyleSheet("color: #ffffff; font-size: 16px; font-weight: 600;");

    layout->addWidget(toastIconLabel);
    layout->addWidget(toastTextLabel);
    toastWidget->hide();

    toastOpacityEffect = new QGraphicsOpacityEffect(toastWidget);
    toastOpacityEffect->setOpacity(0.0);
    toastWidget->setGraphicsEffect(toastOpacityEffect);

    toastFadeInAnimation = new QPropertyAnimation(toastOpacityEffect, "opacity", this);
    toastFadeInAnimation->setDuration(180);
    toastFadeInAnimation->setStartValue(0.0);
    toastFadeInAnimation->setEndValue(1.0);

    toastFadeOutAnimation = new QPropertyAnimation(toastOpacityEffect, "opacity", this);
    toastFadeOutAnimation->setDuration(220);
    toastFadeOutAnimation->setStartValue(1.0);
    toastFadeOutAnimation->setEndValue(0.0);
    connect(toastFadeOutAnimation, &QPropertyAnimation::finished, this,
            [this]()
            {
                if (toastWidget && toastOpacityEffect && toastOpacityEffect->opacity() <= 0.01)
                {
                    toastWidget->hide();
                }
            });

    toastHideTimer = new QTimer(this);
    toastHideTimer->setSingleShot(true);
    connect(toastHideTimer, &QTimer::timeout, this,
            [this]()
            {
                if (toastFadeInAnimation)
                {
                    toastFadeInAnimation->stop();
                }
                if (toastFadeOutAnimation)
                {
                    toastFadeOutAnimation->start();
                }
            });
}

void SpectrumSwitchPage::updateToastPosition()
{
    if (!toastWidget)
    {
        return;
    }

    toastWidget->adjustSize();
    const int x = qMax(0, (width() - toastWidget->width()) / 2);
    const int y = 18;
    toastWidget->move(x, y);
}

void SpectrumSwitchPage::showToastResult(bool success, const QString &message)
{
    ensureToastWidget();

    toastIconLabel->setText(success ? QStringLiteral("✓") : QStringLiteral("!"));
    toastIconLabel->setStyleSheet(success
                                      ? "background-color: #67c23a; color: #ffffff; font-size: 14px; font-weight: bold; "
                                        "border-radius: 10px;"
                                      : "background-color: #ff9f55; color: #ffffff; font-size: 14px; font-weight: bold; "
                                        "border-radius: 10px;");
    toastTextLabel->setText(extractDisplayMessage(success, message));
    updateToastPosition();

    if (toastFadeOutAnimation)
    {
        toastFadeOutAnimation->stop();
    }
    if (toastFadeInAnimation)
    {
        toastFadeInAnimation->stop();
    }
    if (toastHideTimer)
    {
        toastHideTimer->stop();
    }
    if (toastOpacityEffect)
    {
        toastOpacityEffect->setOpacity(0.0);
    }

    toastWidget->show();
    toastWidget->raise();
    if (toastFadeInAnimation)
    {
        toastFadeInAnimation->start();
    }
    if (toastHideTimer)
    {
        toastHideTimer->start(1800);
    }
}

void SpectrumSwitchPage::refreshSpectrumViews()
{
    if (!spectrogramVisible)
    {
        return;
    }

    if (reportInfoLabel)
    {
        if (currentSpectrumReport.groups.isEmpty())
        {
            reportInfoLabel->setText(QStringLiteral("等待时频图数据"));
        }
        else
        {
            reportInfoLabel->setText(
                QStringLiteral("当前分包 %1/%2，主中心频点 %3MHz")
                    .arg(currentSpectrumReport.packetIndex)
                    .arg(currentSpectrumReport.totalPacketCount)
                    .arg(currentSpectrumReport.centerFreqMhz, 0, 'f', 0));
        }
    }

    for (int i = 0; i < groupTitleLabels.size(); ++i)
    {
        QLabel *titleLabel = groupTitleLabels.at(i);
        QLabel *imageLabel = i < groupImageLabels.size() ? groupImageLabels.at(i) : nullptr;
        if (!titleLabel || !imageLabel)
        {
            continue;
        }

        if (i >= currentSpectrumReport.groups.size())
        {
            titleLabel->setText(QStringLiteral("第%1组").arg(i + 1));
            imageLabel->setPixmap(QPixmap());
            imageLabel->setText(QStringLiteral("暂无数据"));
            continue;
        }

        const SpectrumGroupData &groupData = currentSpectrumReport.groups.at(i);
        titleLabel->setText(QStringLiteral("%1MHz").arg(groupData.centerFreqMhz, 0, 'f', 0));
        imageLabel->setText(QString());
        imageLabel->setPixmap(buildHeatmapPixmap(groupData, 72, 320));
    }
}

void SpectrumSwitchPage::refreshFullSpectrumView()
{
    if (!fullSpectrumVisible)
    {
        return;
    }

    if (!fullSpectrumInfoLabel || !fullSpectrumImageLabel)
    {
        return;
    }

    if (currentFullSpectrumReport.data.isEmpty())
    {
        fullSpectrumInfoLabel->setText(QStringLiteral("等待频谱图数据"));
        fullSpectrumImageLabel->setPixmap(QPixmap());
        fullSpectrumImageLabel->setText(QStringLiteral("暂无数据"));
        return;
    }

    fullSpectrumInfoLabel->setText(QStringLiteral("频段范围 %1MHz - %2MHz，点数 %3，疑似信号点 %4 个")
                                       .arg(currentFullSpectrumReport.startMhz, 0, 'f', 0)
                                       .arg(currentFullSpectrumReport.endMhz, 0, 'f', 0)
                                       .arg(currentFullSpectrumReport.data.size())
                                       .arg(currentFullSpectrumReport.markerIndices.size()));
    fullSpectrumImageLabel->setText(QString());
    fullSpectrumImageLabel->setPixmap(buildFullSpectrumPixmap());
}

QPixmap SpectrumSwitchPage::buildHeatmapPixmap(const SpectrumGroupData &groupData, int targetWidth, int targetHeight) const
{
    const int timeCount = groupData.matrix.size();
    const int freqCount = timeCount > 0 ? groupData.matrix.first().size() : 0;
    if (timeCount <= 0 || freqCount <= 0)
    {
        return QPixmap();
    }

    // 每组独立计算色阶，但用百分位裁剪避免少量极端值把整张图拉平。
    QVector<qint16> samples;
    samples.reserve(timeCount * freqCount);
    for (const QVector<qint16> &timeRow : groupData.matrix)
    {
        for (qint16 value : timeRow)
        {
            samples.append(value);
        }
    }

    qint16 minValue = groupData.matrix.first().first();
    qint16 maxValue = minValue;
    if (!samples.isEmpty())
    {
        std::sort(samples.begin(), samples.end());
        const int lowIndex = qBound(0, samples.size() * 5 / 100, samples.size() - 1);
        const int highIndex = qBound(0, samples.size() * 95 / 100, samples.size() - 1);
        minValue = samples.at(lowIndex);
        maxValue = samples.at(highIndex);
        if (maxValue <= minValue)
        {
            minValue = samples.first();
            maxValue = samples.last();
        }
    }

    QImage image(freqCount, timeCount, QImage::Format_RGB32);
    const qreal span = qMax<qreal>(1.0, static_cast<qreal>(maxValue - minValue));
    auto smoothedValueAt = [&groupData, timeCount, freqCount](int timeIndex, int freqIndex) -> qreal
    {
        const qreal centerWeight = 4.0;
        const qreal directNeighborWeight = 1.0;
        qreal weightedSum = 0.0;
        qreal totalWeight = 0.0;
        for (int timeOffset = -1; timeOffset <= 1; ++timeOffset)
        {
            const int neighborTime = timeIndex + timeOffset;
            if (neighborTime < 0 || neighborTime >= timeCount)
            {
                continue;
            }

            const QVector<qint16> &neighborRow = groupData.matrix.at(neighborTime);
            for (int freqOffset = -1; freqOffset <= 1; ++freqOffset)
            {
                const int neighborFreq = freqIndex + freqOffset;
                if (neighborFreq < 0 || neighborFreq >= freqCount)
                {
                    continue;
                }

                if (timeOffset != 0 && freqOffset != 0)
                {
                    continue;
                }

                const qreal weight = (timeOffset == 0 && freqOffset == 0) ? centerWeight : directNeighborWeight;
                weightedSum += neighborRow.value(neighborFreq) * weight;
                totalWeight += weight;
            }
        }

        return totalWeight > 0.0 ? (weightedSum / totalWeight) : 0.0;
    };

    for (int timeIndex = 0; timeIndex < timeCount; ++timeIndex)
    {
        for (int freqIndex = 0; freqIndex < freqCount; ++freqIndex)
        {
            const qreal clampedValue = qBound<qreal>(minValue, smoothedValueAt(timeIndex, freqIndex), maxValue);
            const qreal normalized = (clampedValue - minValue) / span;
            const int red = static_cast<int>(20 + normalized * 18);
            const int green = static_cast<int>(120 + normalized * 85);
            const int blue = static_cast<int>(175 + normalized * 55);
            image.setPixelColor(freqIndex, timeIndex, QColor(red, green, qMin(255, blue)));
        }
    }

    return QPixmap::fromImage(
        image.scaled(qMax(1, targetWidth), qMax(1, targetHeight), Qt::IgnoreAspectRatio, Qt::FastTransformation));
}

QPixmap SpectrumSwitchPage::buildFullSpectrumPixmap() const
{
    if (currentFullSpectrumReport.data.isEmpty())
    {
        return QPixmap();
    }

    QVector<int> values;
    values.reserve(currentFullSpectrumReport.data.size());
    for (int rawValue : currentFullSpectrumReport.data)
    {
        const int displayValue = rawValue > 0 ? -rawValue : rawValue;
        values.append(qBound(-120, displayValue, 0));
    }

    const int widthPx = 460;
    const int heightPx = 220;
    const QRect plotRect = fullSpectrumPlotRect();

    const int minValue = -120;
    const int maxValue = 0;
    const qreal span = static_cast<qreal>(maxValue - minValue);

    QPixmap pixmap(widthPx, heightPx);
    pixmap.fill(QColor("#14283a"));
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.setPen(QPen(QColor("#516980"), 1));
    const int gridLines = 6;
    for (int i = 0; i <= gridLines; ++i)
    {
        const int y = plotRect.top() + (plotRect.height() * i) / gridLines;
        painter.drawLine(plotRect.left(), y, plotRect.right(), y);

        const qreal ratio = 1.0 - static_cast<qreal>(i) / gridLines;
        const int labelValue = static_cast<int>(minValue + ratio * span);
        painter.setPen(QColor("#8ea5ba"));
        painter.drawText(4, y + 4, QString::number(labelValue));
        painter.setPen(QPen(QColor("#516980"), 1));
    }

    const double freqSpan = qMax(1.0, currentFullSpectrumReport.endMhz - currentFullSpectrumReport.startMhz);
    const int xTickCount = 5;
    painter.setPen(QColor("#8ea5ba"));
    for (int i = 0; i <= xTickCount; ++i)
    {
        const int x = plotRect.left() + (plotRect.width() * i) / xTickCount;
        const double freqValue = currentFullSpectrumReport.startMhz + freqSpan * i / xTickCount;
        painter.drawText(x - 10, heightPx - 6, QString::number(static_cast<int>(freqValue)));
    }

    QPainterPath path;
    for (int i = 0; i < values.size(); ++i)
    {
        const qreal xRatio = values.size() == 1 ? 0.0 : static_cast<qreal>(i) / (values.size() - 1);
        const qreal yRatio = (values.at(i) - minValue) / span;
        const qreal x = plotRect.left() + xRatio * plotRect.width();
        const qreal y = plotRect.bottom() - yRatio * plotRect.height();
        if (i == 0)
        {
            path.moveTo(x, y);
        }
        else
        {
            path.lineTo(x, y);
        }
    }

    painter.setPen(QPen(QColor("#20bfff"), 2));
    painter.drawPath(path);

    painter.setPen(QPen(QColor("#ffb347"), 1));
    for (int markerIndex : currentFullSpectrumReport.markerIndices)
    {
        if (markerIndex < 0 || markerIndex >= values.size())
        {
            continue;
        }
        const qreal xRatio = values.size() == 1 ? 0.0 : static_cast<qreal>(markerIndex) / (values.size() - 1);
        const int x = static_cast<int>(plotRect.left() + xRatio * plotRect.width());
        painter.drawLine(x, plotRect.top(), x, plotRect.bottom());
    }

    return pixmap;
}

QRect SpectrumSwitchPage::fullSpectrumPlotRect() const
{
    const int widthPx = 460;
    const int heightPx = 220;
    const int leftMargin = 40;
    const int rightMargin = 14;
    const int topMargin = 12;
    const int bottomMargin = 28;
    return QRect(leftMargin, topMargin, widthPx - leftMargin - rightMargin, heightPx - topMargin - bottomMargin);
}

QString SpectrumSwitchPage::extractDisplayMessage(bool success, const QString &message) const
{
    if (message.trimmed().isEmpty())
    {
        return success ? QStringLiteral("设置成功") : QStringLiteral("设置失败");
    }

    const QString trimmed = message.trimmed();
    const int infoIndex = trimmed.indexOf("Info:");
    if (infoIndex >= 0)
    {
        const QString infoText = trimmed.mid(infoIndex + 5).trimmed();
        if (!infoText.isEmpty())
        {
            return infoText;
        }
    }

    return success ? QStringLiteral("设置成功") : trimmed;
}

QString SpectrumSwitchPage::titleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold;");
}

QString SpectrumSwitchPage::buttonStyle() const
{
    return QStringLiteral("QPushButton { background-color: #ffffff; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #e0e0e0; }");
}
