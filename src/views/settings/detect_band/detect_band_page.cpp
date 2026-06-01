#include "detect_band_page.h"
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
constexpr int kColumnSpacing = 26;
constexpr int kFreqColumnWidth = 200;
constexpr int kMeasureColumnWidth = 200;
constexpr int kGainColumnWidth = 200;
constexpr int kActionColumnWidth = 78;
}

DetectBandPage::DetectBandPage(QWidget *parent)
    : QWidget(parent), rowsLayout(nullptr), addRowButton(nullptr), saveButton(nullptr), toastWidget(nullptr),
      toastIconLabel(nullptr), toastTextLabel(nullptr), toastHideTimer(nullptr), toastOpacityEffect(nullptr),
      toastFadeInAnimation(nullptr), toastFadeOutAnimation(nullptr)
{
    setupUi();
}

void DetectBandPage::setupUi()
{
    setObjectName("detectBandPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#detectBandPage { background-color: #202020; color: #ffffff; }");

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

    QWidget *content = createScrollContent(scrollArea);
    scrollArea->setWidget(content);

    appendBandRow(2430.0, 1, 60);
    appendBandRow(5786.0, 1, 60);
    refreshDeleteButtonState();
}

QWidget *DetectBandPage::createScrollContent(QScrollArea *scrollArea)
{
    QWidget *content = new QWidget(scrollArea);
    applyStyledBackground(content, "background-color: #202020;");

    QVBoxLayout *pageLayout = new QVBoxLayout(content);
    pageLayout->setContentsMargins(40, 30, 20, 30);
    pageLayout->setSpacing(20);
    pageLayout->setAlignment(Qt::AlignTop);

    QWidget *tableFrame = new QWidget(content);
    applyStyledBackground(tableFrame, "background-color: #2b2b2b; border-radius: 6px;");

    QVBoxLayout *tableLayout = new QVBoxLayout(tableFrame);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);

    tableLayout->addWidget(createHeaderRow(tableFrame));

    rowsLayout = new QVBoxLayout();
    rowsLayout->setContentsMargins(0, 0, 0, 0);
    rowsLayout->setSpacing(0);
    tableLayout->addLayout(rowsLayout);

    QWidget *actionRow = new QWidget(tableFrame);
    applyStyledBackground(actionRow, "background-color: transparent; border-top: 1px solid #444444;");

    QHBoxLayout *actionLayout = new QHBoxLayout(actionRow);
    actionLayout->setContentsMargins(20, 15, 20, 20);
    actionLayout->setSpacing(0);

    addRowButton = new QPushButton("+  添加下一行数据", actionRow);
    addRowButton->setFixedSize(120, 36);
    addRowButton->setStyleSheet(addRowButtonStyle());
    actionLayout->addWidget(addRowButton);
    actionLayout->addStretch();
    tableLayout->addWidget(actionRow);

    pageLayout->addWidget(tableFrame);
    pageLayout->addStretch();

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(0);
    bottomLayout->addStretch();

    saveButton = new QPushButton("保存", content);
    saveButton->setFixedSize(120, 36);
    saveButton->setStyleSheet(actionButtonStyle());
    bottomLayout->addWidget(saveButton);
    pageLayout->addLayout(bottomLayout);

    connect(addRowButton, &QPushButton::clicked, this, &DetectBandPage::addDefaultRow);
    connect(saveButton, &QPushButton::clicked, this, &DetectBandPage::handleSaveClicked);
    return content;
}

void DetectBandPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateToastPosition();
}

QWidget *DetectBandPage::createHeaderRow(QWidget *parent)
{
    QWidget *header = new QWidget(parent);
    applyStyledBackground(header, "background-color: transparent; border-bottom: 1px solid #444444;");

    QHBoxLayout *layout = new QHBoxLayout(header);
    layout->setContentsMargins(20, 20, 20, 15);
    layout->setSpacing(kColumnSpacing);

    auto createHeaderLabel = [header](const QString &text, int width)
    {
        QLabel *label = new QLabel(text, header);
        label->setFixedWidth(width);
        label->setStyleSheet("color: #ffffff; font-size: 13px; font-weight: bold;");
        return label;
    };

    layout->addWidget(createHeaderLabel("频段值(MHZ)", kFreqColumnWidth));
    layout->addWidget(createHeaderLabel("测量次数", kMeasureColumnWidth));
    layout->addWidget(createHeaderLabel("增益", kGainColumnWidth));
    layout->addWidget(createHeaderLabel("操作", kActionColumnWidth));
    layout->addStretch();

    return header;
}

QWidget *DetectBandPage::createBandRow(QWidget *parent, double freqMhz, int measureCount, int gain)
{
    QWidget *rowWidget = new QWidget(parent);
    applyStyledBackground(rowWidget, rowBackgroundStyle());

    QHBoxLayout *layout = new QHBoxLayout(rowWidget);
    layout->setContentsMargins(20, 15, 20, 12);
    layout->setSpacing(kColumnSpacing);

    QDoubleSpinBox *freqInput = createFreqInput(rowWidget, freqMhz);
    QSpinBox *measureInput = createMeasureCountInput(rowWidget, measureCount);
    QSpinBox *gainInput = createGainInput(rowWidget, gain);

    QLabel *freqHint = nullptr;
    QLabel *measureHint = nullptr;
    QLabel *gainHint = nullptr;

    layout->addWidget(createColumnCell(rowWidget, createNumericContainer(rowWidget, freqInput), &freqHint, "0-6000"));
    layout->addWidget(createColumnCell(rowWidget, createNumericContainer(rowWidget, measureInput), &measureHint, "0-255"));
    layout->addWidget(createColumnCell(rowWidget, createNumericContainer(rowWidget, gainInput), &gainHint, "0-60"));

    QPushButton *deleteButton = new QPushButton("删除", rowWidget);
    deleteButton->setFixedSize(kActionColumnWidth, 24);
    deleteButton->setCursor(Qt::PointingHandCursor);
    deleteButton->setStyleSheet(deleteButtonStyle());
    layout->addWidget(deleteButton, 0, Qt::AlignVCenter);
    layout->addStretch();

    connect(deleteButton, &QPushButton::clicked, this,
            [this, rowWidget]()
            {
                removeBandRow(rowWidget);
            });

    BandRowWidgets row = {rowWidget, freqInput, measureInput, gainInput, deleteButton, freqHint, measureHint, gainHint};
    bandRows.append(row);
    return rowWidget;
}

QWidget *DetectBandPage::createColumnCell(QWidget *parent, QWidget *fieldWidget, QLabel **hintLabel, const QString &hintText)
{
    QWidget *cell = new QWidget(parent);
    applyStyledBackground(cell, "background-color: transparent; border: none;");

    QVBoxLayout *layout = new QVBoxLayout(cell);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(3);
    layout->addWidget(fieldWidget);

    *hintLabel = new QLabel(hintText, cell);
    (*hintLabel)->setStyleSheet(hintLabelStyle());
    layout->addWidget(*hintLabel);
    return cell;
}

QWidget *DetectBandPage::createNumericContainer(QWidget *parent, QWidget *fieldWidget)
{
    QWidget *container = new QWidget(parent);
    container->setFixedSize(200, 32);
    container->setStyleSheet(inputContainerStyle());

    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(fieldWidget);
    return container;
}

QDoubleSpinBox *DetectBandPage::createFreqInput(QWidget *parent, double value) const
{
    QDoubleSpinBox *input = new QDoubleSpinBox(parent);
    input->setDecimals(0);
    input->setRange(0.0, 6000.0);
    input->setValue(value);
    input->setButtonSymbols(QAbstractSpinBox::NoButtons);
    input->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    input->setStyleSheet(inputFieldStyle());
    input->setFixedSize(kFreqColumnWidth, 32);
    return input;
}

QSpinBox *DetectBandPage::createMeasureCountInput(QWidget *parent, int value) const
{
    QSpinBox *input = new QSpinBox(parent);
    input->setRange(0, 255);
    input->setValue(value);
    input->setButtonSymbols(QAbstractSpinBox::NoButtons);
    input->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    input->setStyleSheet(inputFieldStyle());
    input->setFixedSize(kMeasureColumnWidth, 32);
    return input;
}

QSpinBox *DetectBandPage::createGainInput(QWidget *parent, int value) const
{
    QSpinBox *input = new QSpinBox(parent);
    input->setRange(0, 60);
    input->setValue(value);
    input->setButtonSymbols(QAbstractSpinBox::NoButtons);
    input->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    input->setStyleSheet(inputFieldStyle());
    input->setFixedSize(kGainColumnWidth, 32);
    return input;
}

void DetectBandPage::appendBandRow(double freqMhz, int measureCount, int gain)
{
    if (!rowsLayout)
    {
        return;
    }

    QWidget *rowWidget = createBandRow(this, freqMhz, measureCount, gain);
    rowsLayout->addWidget(rowWidget);
}

QVector<DetectBandParam> DetectBandPage::collectBands() const
{
    QVector<DetectBandParam> bands;
    bands.reserve(bandRows.size());

    for (const BandRowWidgets &row : bandRows)
    {
        DetectBandParam band = {};
        band.freqMhz = static_cast<float>(row.freqInput->value());
        band.measureCount = row.measureCountInput->value();
        band.gain = row.gainInput->value();
        bands.append(band);
    }

    return bands;
}

void DetectBandPage::clearBandRows()
{
    for (const BandRowWidgets &row : bandRows)
    {
        if (row.container)
        {
            rowsLayout->removeWidget(row.container);
            row.container->deleteLater();
        }
    }
    bandRows.clear();
}

void DetectBandPage::removeBandRow(QWidget *rowWidget)
{
    for (int i = 0; i < bandRows.size(); ++i)
    {
        if (bandRows[i].container != rowWidget)
        {
            continue;
        }

        rowsLayout->removeWidget(rowWidget);
        bandRows.remove(i);
        rowWidget->deleteLater();
        refreshDeleteButtonState();
        return;
    }
}

void DetectBandPage::refreshDeleteButtonState()
{
    for (BandRowWidgets &row : bandRows)
    {
        row.deleteButton->setEnabled(true);
        row.deleteButton->setStyleSheet(deleteButtonStyle());
    }
}

QString DetectBandPage::inputContainerStyle() const
{
    return "background-color: #1e1e1e; border: 1px solid #444; border-radius: 3px;";
}

QString DetectBandPage::inputFieldStyle() const
{
    return "QDoubleSpinBox, QSpinBox { background: transparent; color: #ffffff; border: none; font-size: 12px; "
           "padding-left: 10px; padding-right: 10px; selection-background-color: #3c3c3c; }"
           "QDoubleSpinBox::up-button, QDoubleSpinBox::down-button, QSpinBox::up-button, QSpinBox::down-button { width: 0px; border: none; }";
}

QString DetectBandPage::hintLabelStyle() const
{
    return "color: #8a8a8a; font-size: 11px; padding-left: 2px;";
}

QString DetectBandPage::actionButtonStyle() const
{
    return "QPushButton { background-color: #ffffff; color: #111111; border: none; border-radius: 2px; font-size: 13px; "
           "font-weight: bold; }"
           "QPushButton:hover { background-color: #ededed; }";
}

QString DetectBandPage::deleteButtonStyle() const
{
    return "QPushButton { background: transparent; border: none; color: #ff9f55; font-size: 12px; font-weight: bold; "
           "padding: 0; text-align: left; }"
           "QPushButton:hover { color: #ffbc82; }";
}

QString DetectBandPage::addRowButtonStyle() const
{
    return "QPushButton { background-color: #ffffff; color: #111111; border: none; border-radius: 2px; font-size: 12px; "
           "font-weight: bold; text-align: center; padding: 0; }"
           "QPushButton:hover { background-color: #e0e0e0; }";
}

QString DetectBandPage::rowBackgroundStyle() const
{
    return "background-color: transparent; border: none; border-bottom: 1px solid #444444;";
}

void DetectBandPage::applyStyledBackground(QWidget *widget, const QString &styleSheet) const
{
    if (!widget)
    {
        return;
    }

    widget->setAttribute(Qt::WA_StyledBackground, true);
    widget->setStyleSheet(styleSheet);
}

void DetectBandPage::addDefaultRow()
{
    if (bandRows.size() >= 128)
    {
        QMessageBox::warning(this, "提示", "侦测频段最多支持 128 条。");
        return;
    }

    appendBandRow(2430.0, 1, 60);
    refreshDeleteButtonState();
}

void DetectBandPage::handleSaveClicked()
{
    const QVector<DetectBandParam> bands = collectBands();
    if (bands.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请至少添加一条侦测频段数据后再保存。");
        return;
    }

    if (bands.size() > 128)
    {
        QMessageBox::warning(this, "提示", "侦测频段数量不能超过 128 条。");
        return;
    }

    emit requestSaveDetectBands(bands);
}

void DetectBandPage::updateDetectBands(const QVector<DetectBandParam> &bands)
{
    clearBandRows();
    for (const DetectBandParam &band : bands)
    {
        appendBandRow(band.freqMhz, band.measureCount, band.gain);
    }
    refreshDeleteButtonState();
}

void DetectBandPage::showSaveResult(bool success, const QString &message)
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

void DetectBandPage::ensureToastWidget()
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

void DetectBandPage::updateToastPosition()
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

QString DetectBandPage::extractDisplayMessage(bool success, const QString &message) const
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
