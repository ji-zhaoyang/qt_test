#include "strike_frequency_page.h"
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleValidator>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
QString buildRangeHint(const StrikeFrequencyBandConfig &band)
{
    const QString baseRange =
        QStringLiteral("%1-%2").arg(QString::number(band.minMhz, 'f', 0), QString::number(band.maxMhz, 'f', 0));
    return band.navigation == 1 ? QStringLiteral("导航干扰频段: %1").arg(baseRange) : baseRange;
}
} // namespace

StrikeFrequencyPage::StrikeFrequencyPage(QWidget *parent)
    : QWidget(parent), saveButton(nullptr), toastWidget(nullptr), toastIconLabel(nullptr), toastTextLabel(nullptr),
      toastHideTimer(nullptr), toastOpacityEffect(nullptr), toastFadeInAnimation(nullptr), toastFadeOutAnimation(nullptr)
{
    setupUi();
}

void StrikeFrequencyPage::setupUi()
{
    setObjectName("strikeFrequencyPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#strikeFrequencyPage { background-color: #202020; color: #ffffff; }");

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
    pageLayout->setContentsMargins(24, 10, 24, 18);
    pageLayout->setSpacing(10);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel *pageTitle = new QLabel(QStringLiteral("打击频率设置"), content);
    pageTitle->setStyleSheet(titleStyle());
    pageLayout->addWidget(pageTitle);

    QFrame *card = new QFrame(content);
    card->setStyleSheet("QFrame { background-color: #2b2b2b; border-radius: 6px; }");
    pageLayout->addWidget(card);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    QWidget *headerRow = new QWidget(card);
    headerRow->setStyleSheet("background-color: #1f1f1f;");
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(10, 12, 10, 12);
    headerLayout->setSpacing(16);

    auto addHeaderLabel = [this, headerLayout](const QString &text, int width)
    {
        QLabel *label = new QLabel(text);
        label->setFixedWidth(width);
        label->setStyleSheet(headerTextStyle());
        headerLayout->addWidget(label, 0, Qt::AlignVCenter);
    };

    addHeaderLabel(QStringLiteral("启用"), 92);
    addHeaderLabel(QStringLiteral("序号"), 68);
    addHeaderLabel(QStringLiteral("起始频点"), 300);
    addHeaderLabel(QStringLiteral("结束频点"), 300);
    addHeaderLabel(QStringLiteral("衰减"), 280);
    headerLayout->addStretch();
    cardLayout->addWidget(headerRow);

    const QList<StrikeFrequencyBandConfig> defaultBands = {
        {0, 360, 365, 0, 360, 800, 0, 0, 1, 1, 0, 0},   {0, 400, 480, 0, 360, 800, 0, 0, 1, 1, 0, 0},
        {0, 830, 950, 0, 800, 1150, 0, 0, 1, 2, 0, 0},  {0, 1150, 1210, 0, 1150, 1240, 0, 0, 1, 2, 0, 0},
        {0, 1360, 1480, 0, 1300, 1550, 0, 0, 1, 3, 0, 0}, {1, 1550, 1620, 0, 1550, 1740, 0, 0, 1, 3, 0, 1},
        {0, 2400, 2500, 0, 2310, 2750, 0, 0, 1, 4, 0, 0}, {1, 2400, 2500, 0, 2310, 2750, 0, 0, 1, 4, 0, 0},
        {0, 5100, 5105, 0, 5100, 5540, 0, 0, 1, 5, 0, 0}, {0, 5125, 5250, 0, 5100, 5540, 0, 0, 1, 5, 0, 0},
        {0, 5725, 5850, 0, 5540, 5980, 0, 0, 1, 6, 0, 0}, {0, 5725, 5850, 0, 5540, 5980, 0, 0, 1, 6, 0, 0},
    };

    rowWidgets.reserve(defaultBands.size());
    for (int rowIndex = 0; rowIndex < defaultBands.size(); ++rowIndex)
    {
        QFrame *rowFrame = new QFrame(card);
        rowFrame->setStyleSheet("QFrame { background-color: #101010; border-top: 1px solid #292929; }");
        QHBoxLayout *rowLayout = new QHBoxLayout(rowFrame);
        rowLayout->setContentsMargins(10, 8, 10, 6);
        rowLayout->setSpacing(16);

        RowWidgets row;
        row.metadata = defaultBands.at(rowIndex);

        QWidget *toggleCell = new QWidget(rowFrame);
        toggleCell->setFixedWidth(92);
        QHBoxLayout *toggleLayout = new QHBoxLayout(toggleCell);
        toggleLayout->setContentsMargins(0, 0, 0, 0);
        toggleLayout->setSpacing(0);
        toggleLayout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        row.enabledCheckBox = new QCheckBox(toggleCell);
        row.enabledCheckBox->setStyleSheet(toggleStyle());
        toggleLayout->addWidget(row.enabledCheckBox);
        rowLayout->addWidget(toggleCell, 0, Qt::AlignVCenter);

        row.indexLabel = new QLabel(QString::number(rowIndex + 1), rowFrame);
        row.indexLabel->setFixedWidth(68);
        row.indexLabel->setStyleSheet(rowIndexStyle());
        rowLayout->addWidget(row.indexLabel, 0, Qt::AlignVCenter);

        auto createFrequencyCell =
            [this, rowFrame](QLineEdit *&edit, QLabel *&hintLabel, const QString &value, const QString &hintText)
        {
            QWidget *cell = new QWidget(rowFrame);
            cell->setFixedWidth(300);
            QVBoxLayout *cellLayout = new QVBoxLayout(cell);
            cellLayout->setContentsMargins(0, 0, 0, 0);
            cellLayout->setSpacing(4);

            edit = new QLineEdit(value, cell);
            edit->setFixedSize(300, 28);
            edit->setValidator(new QDoubleValidator(0.0, 9999.0, 2, edit));
            edit->setStyleSheet(inputStyle());
            cellLayout->addWidget(edit);

            hintLabel = new QLabel(hintText, cell);
            hintLabel->setStyleSheet(rangeTextStyle());
            cellLayout->addWidget(hintLabel);
            return cell;
        };

        rowLayout->addWidget(createFrequencyCell(row.startEdit, row.startHintLabel, formatFrequencyValue(row.metadata.startMhz),
                                                 buildRangeHint(row.metadata)),
                             0, Qt::AlignTop);
        rowLayout->addWidget(createFrequencyCell(row.endEdit, row.endHintLabel, formatFrequencyValue(row.metadata.endMhz),
                                                 buildRangeHint(row.metadata)),
                             0, Qt::AlignTop);

        row.attComboBox = new QComboBox(rowFrame);
        row.attComboBox->addItems(
            QStringList() << QStringLiteral("加强模式") << QStringLiteral("正常模式") << QStringLiteral("节能模式"));
        row.attComboBox->setFixedSize(280, 28);
        row.attComboBox->setStyleSheet(comboBoxStyle());
        rowLayout->addWidget(row.attComboBox, 0, Qt::AlignTop);
        rowLayout->addStretch();

        cardLayout->addWidget(rowFrame);
        rowWidgets.append(row);
        applyBandToRow(rowIndex, row.metadata);
    }

    QWidget *footerRow = new QWidget(content);
    footerRow->setStyleSheet("background-color: transparent;");
    QHBoxLayout *footerLayout = new QHBoxLayout(footerRow);
    footerLayout->setContentsMargins(0, 2, 0, 0);
    footerLayout->setSpacing(12);

    QLabel *footerText = new QLabel(QStringLiteral("请按每路频段限制值填写"), footerRow);
    footerText->setStyleSheet(footerTextStyle());
    footerLayout->addWidget(footerText, 0, Qt::AlignLeft | Qt::AlignVCenter);
    footerLayout->addStretch();

    saveButton = new QPushButton(QStringLiteral("保存"), footerRow);
    saveButton->setFixedSize(110, 34);
    saveButton->setStyleSheet(buttonStyle());
    connect(saveButton, &QPushButton::clicked, this, &StrikeFrequencyPage::handleSaveClicked);
    footerLayout->addWidget(saveButton, 0, Qt::AlignRight | Qt::AlignVCenter);

    pageLayout->addWidget(footerRow);
    pageLayout->addStretch();
}

void StrikeFrequencyPage::updateStrikeFrequencyBands(const StrikeFrequencyBandList &bands)
{
    for (int i = 0; i < rowWidgets.size(); ++i)
    {
        StrikeFrequencyBandConfig mergedBand = rowWidgets.at(i).metadata;
        if (i < bands.size())
        {
            const StrikeFrequencyBandConfig &band = bands.at(i);
            mergedBand.enable = band.enable;
            mergedBand.startMhz = band.startMhz;
            mergedBand.endMhz = band.endMhz;
            mergedBand.att = band.att;
        }

        applyBandToRow(i, mergedBand);
    }
}

void StrikeFrequencyPage::showStrikeFrequencySaveResult(bool success, const QString &message)
{
    showToastResult(success, message);
}

void StrikeFrequencyPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateToastPosition();
}

void StrikeFrequencyPage::applyBandToRow(int rowIndex, const StrikeFrequencyBandConfig &band)
{
    if (rowIndex < 0 || rowIndex >= rowWidgets.size())
    {
        return;
    }

    RowWidgets &row = rowWidgets[rowIndex];
    row.metadata = band;
    if (row.enabledCheckBox)
    {
        row.enabledCheckBox->setChecked(band.enable == 1);
    }
    if (row.startEdit)
    {
        row.startEdit->setText(formatFrequencyValue(band.startMhz));
    }
    if (row.endEdit)
    {
        row.endEdit->setText(formatFrequencyValue(band.endMhz));
    }
    if (row.startHintLabel)
    {
        row.startHintLabel->setText(buildRangeHint(band));
    }
    if (row.endHintLabel)
    {
        row.endHintLabel->setText(buildRangeHint(band));
    }
    if (row.attComboBox)
    {
        row.attComboBox->setCurrentIndex(qBound(0, band.att, 2));
    }

    const bool editable = true;
    if (row.enabledCheckBox)
    {
        row.enabledCheckBox->setEnabled(editable);
    }
    if (row.startEdit)
    {
        row.startEdit->setEnabled(editable);
    }
    if (row.endEdit)
    {
        row.endEdit->setEnabled(editable);
    }
    if (row.attComboBox)
    {
        row.attComboBox->setEnabled(editable);
    }
}

bool StrikeFrequencyPage::buildSavePayload(StrikeFrequencyBandList &bands, QString &errorMessage) const
{
    bands.clear();
    for (int rowIndex = 0; rowIndex < rowWidgets.size(); ++rowIndex)
    {
        const RowWidgets &row = rowWidgets.at(rowIndex);
        bool startOk = false;
        bool endOk = false;
        const double startMhz = row.startEdit ? row.startEdit->text().trimmed().toDouble(&startOk) : 0.0;
        const double endMhz = row.endEdit ? row.endEdit->text().trimmed().toDouble(&endOk) : 0.0;
        if (!startOk || !endOk)
        {
            errorMessage = QStringLiteral("第 %1 行频点格式无效。").arg(rowIndex + 1);
            return false;
        }
        if (startMhz > endMhz)
        {
            errorMessage = QStringLiteral("第 %1 行起始频点不能大于结束频点。").arg(rowIndex + 1);
            return false;
        }

        const double minMhz = row.metadata.minMhz > 0.0 ? row.metadata.minMhz : startMhz;
        const double maxMhz = row.metadata.maxMhz > 0.0 ? row.metadata.maxMhz : endMhz;
        if (startMhz < minMhz || endMhz > maxMhz)
        {
            errorMessage = QStringLiteral("第 %1 行频点范围需在 %2-%3 MHz 内。")
                               .arg(rowIndex + 1)
                               .arg(formatFrequencyValue(minMhz))
                               .arg(formatFrequencyValue(maxMhz));
            return false;
        }

        StrikeFrequencyBandConfig band = row.metadata;
        band.enable = row.enabledCheckBox && row.enabledCheckBox->isChecked() ? 1 : 0;
        band.startMhz = startMhz;
        band.endMhz = endMhz;
        band.att = row.attComboBox ? row.attComboBox->currentIndex() : 0;
        bands.append(band);
    }

    return true;
}

void StrikeFrequencyPage::handleSaveClicked()
{
    StrikeFrequencyBandList bands;
    QString errorMessage;
    if (!buildSavePayload(bands, errorMessage))
    {
        showToastResult(false, errorMessage);
        return;
    }

    emit requestSaveStrikeFrequencyBands(bands);
}

void StrikeFrequencyPage::ensureToastWidget()
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

void StrikeFrequencyPage::updateToastPosition()
{
    if (!toastWidget)
    {
        return;
    }

    toastWidget->adjustSize();
    const int x = qMax(0, (width() - toastWidget->width()) / 2);
    toastWidget->move(x, 18);
}

void StrikeFrequencyPage::showToastResult(bool success, const QString &message)
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

QString StrikeFrequencyPage::extractDisplayMessage(bool success, const QString &message) const
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

QString StrikeFrequencyPage::formatFrequencyValue(double value) const
{
    if (qFuzzyIsNull(value))
    {
        return QStringLiteral("0");
    }

    const QString text = QString::number(value, 'f', 2);
    QString trimmed = text;
    while (trimmed.contains('.') && (trimmed.endsWith('0') || trimmed.endsWith('.')))
    {
        trimmed.chop(1);
    }
    return trimmed;
}

QString StrikeFrequencyPage::titleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold; padding-left: 2px;");
}

QString StrikeFrequencyPage::headerTextStyle() const
{
    return QStringLiteral("color: #f0f0f0; font-size: 14px; font-weight: bold;");
}

QString StrikeFrequencyPage::rowIndexStyle() const
{
    return QStringLiteral("color: #f0f0f0; font-size: 14px;");
}

QString StrikeFrequencyPage::rangeTextStyle() const
{
    return QStringLiteral("color: #9ea4ad; font-size: 12px;");
}

QString StrikeFrequencyPage::inputStyle() const
{
    return QStringLiteral("QLineEdit { background-color: #0f0f0f; color: #ffffff; border: 1px solid #323232; "
                          "border-radius: 2px; padding: 0 10px; font-size: 14px; }"
                          "QLineEdit:focus { border: 1px solid #5d87b8; }");
}

QString StrikeFrequencyPage::comboBoxStyle() const
{
    return QStringLiteral("QComboBox { background-color: #0f0f0f; color: #ffffff; border: 1px solid #323232; "
                          "border-radius: 2px; padding-left: 10px; padding-right: 28px; font-size: 14px; }"
                          "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; "
                          "border-left-width: 1px; border-left-color: #2a2a2a; border-left-style: solid; }"
                          "QComboBox::down-arrow { image: none; }"
                          "QComboBox QAbstractItemView { background-color: #2b2b2b; color: #ffffff; "
                          "selection-background-color: #3d2416; selection-color: #ffffff; border: 1px solid #444444; "
                          "outline: 0; }");
}

QString StrikeFrequencyPage::toggleStyle() const
{
    return QStringLiteral("QCheckBox { spacing: 0px; }"
                          "QCheckBox::indicator { width: 36px; height: 20px; border-radius: 10px; }"
                          "QCheckBox::indicator:unchecked { image: none; background-color: #7b7b7b; border: 1px solid #666666; }"
                          "QCheckBox::indicator:checked { image: none; background-color: #ef9a53; border: 1px solid #dc8741; }");
}

QString StrikeFrequencyPage::footerTextStyle() const
{
    return QStringLiteral("color: #d8d8d8; font-size: 13px;");
}

QString StrikeFrequencyPage::buttonStyle() const
{
    return QStringLiteral("QPushButton { background-color: #ffffff; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #e0e0e0; }");
}
