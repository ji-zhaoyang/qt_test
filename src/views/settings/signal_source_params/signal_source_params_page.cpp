#include "signal_source_params_page.h"
#include <QComboBox>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QIntValidator>
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
constexpr int kFieldWidth = 156;
constexpr int kSaveButtonWidth = 152;

QString requiredLabelText(const QString &text)
{
    return QStringLiteral("<font color='#ff6a55'>*</font> ") + text;
}
} // namespace

SignalSourceParamsPage::SignalSourceParamsPage(QWidget *parent)
    : QWidget(parent), serialScanPeriodEdit(nullptr), channel1ModeComboBox(nullptr), channel2ModeComboBox(nullptr),
      channel3ModeComboBox(nullptr), channel4ModeComboBox(nullptr), channel5ModeComboBox(nullptr),
      channel6ModeComboBox(nullptr), signalModeComboBox(nullptr), channel1PeriodEdit(nullptr), channel2PeriodEdit(nullptr),
      channel3PeriodEdit(nullptr), channel4PeriodEdit(nullptr), channel5PeriodEdit(nullptr), channel6PeriodEdit(nullptr),
      saveButton(nullptr), toastWidget(nullptr), toastIconLabel(nullptr), toastTextLabel(nullptr), toastHideTimer(nullptr),
      toastOpacityEffect(nullptr), toastFadeInAnimation(nullptr), toastFadeOutAnimation(nullptr)
{
    setupUi();
}

void SignalSourceParamsPage::setupUi()
{
    setObjectName("signalSourceParamsPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#signalSourceParamsPage { background-color: #202020; color: #ffffff; }");

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

    QFrame *footerFrame = new QFrame(this);
    footerFrame->setStyleSheet("QFrame { background-color: #202020; }");
    QHBoxLayout *footerLayout = new QHBoxLayout(footerFrame);
    footerLayout->setContentsMargins(12, 12, 18, 14);
    footerLayout->setSpacing(0);
    footerLayout->addStretch();
    saveButton = createPrimaryButton(footerFrame, QStringLiteral("保存"), kSaveButtonWidth);
    connect(saveButton, &QPushButton::clicked, this, &SignalSourceParamsPage::handleSaveClicked);
    footerLayout->addWidget(saveButton, 0, Qt::AlignRight | Qt::AlignBottom);
    hostLayout->addWidget(footerFrame);

    QWidget *content = new QWidget(scrollArea);
    content->setStyleSheet("background-color: #202020;");
    scrollArea->setWidget(content);

    QVBoxLayout *pageLayout = new QVBoxLayout(content);
    pageLayout->setContentsMargins(8, 10, 8, 18);
    pageLayout->setSpacing(12);
    pageLayout->setAlignment(Qt::AlignTop);

    QVBoxLayout *sectionLayout = nullptr;
    QFrame *sectionFrame = createSectionFrame(QStringLiteral("信源参数"), pageLayout, sectionLayout);

    serialScanPeriodEdit = createStyledLineEdit(sectionFrame, QStringLiteral("16384"), kFieldWidth);
    serialScanPeriodEdit->setValidator(new QIntValidator(0, 16384, serialScanPeriodEdit));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("串行扫描周期设置"), serialScanPeriodEdit, true));
    sectionLayout->addWidget(createNoteRow(sectionFrame, QStringLiteral("0-16384")));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));

    const QStringList modeItems = {QStringLiteral("并行"), QStringLiteral("串行")};
    channel1ModeComboBox = createStyledComboBox(sectionFrame, modeItems, kFieldWidth, QStringLiteral("并行"));
    channel2ModeComboBox = createStyledComboBox(sectionFrame, modeItems, kFieldWidth, QStringLiteral("并行"));
    channel3ModeComboBox = createStyledComboBox(sectionFrame, modeItems, kFieldWidth, QStringLiteral("并行"));
    channel4ModeComboBox = createStyledComboBox(sectionFrame, modeItems, kFieldWidth, QStringLiteral("并行"));
    channel5ModeComboBox = createStyledComboBox(sectionFrame, modeItems, kFieldWidth, QStringLiteral("并行"));
    channel6ModeComboBox = createStyledComboBox(sectionFrame, modeItems, kFieldWidth, QStringLiteral("并行"));

    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第一路扫描模式设置"), channel1ModeComboBox, true));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第二路扫描模式设置"), channel2ModeComboBox, true));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第三路扫描模式设置"), channel3ModeComboBox, true));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第四路扫描模式设置"), channel4ModeComboBox, true));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第五路扫描模式设置"), channel5ModeComboBox, true));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第六路扫描模式设置"), channel6ModeComboBox, true));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));

    signalModeComboBox = createStyledComboBox(sectionFrame, QStringList() << QStringLiteral("信源一") << QStringLiteral("信源二"),
                                              kFieldWidth, QStringLiteral("信源一"));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("信号模式选择"), signalModeComboBox, true));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));

    channel1PeriodEdit = createStyledLineEdit(sectionFrame, QStringLiteral("20"), kFieldWidth);
    channel2PeriodEdit = createStyledLineEdit(sectionFrame, QStringLiteral("20"), kFieldWidth);
    channel3PeriodEdit = createStyledLineEdit(sectionFrame, QStringLiteral("20"), kFieldWidth);
    channel4PeriodEdit = createStyledLineEdit(sectionFrame, QStringLiteral("20"), kFieldWidth);
    channel5PeriodEdit = createStyledLineEdit(sectionFrame, QStringLiteral("20"), kFieldWidth);
    channel6PeriodEdit = createStyledLineEdit(sectionFrame, QStringLiteral("20"), kFieldWidth);

    for (QLineEdit *edit : {channel1PeriodEdit, channel2PeriodEdit, channel3PeriodEdit, channel4PeriodEdit, channel5PeriodEdit,
                            channel6PeriodEdit})
    {
        edit->setValidator(new QIntValidator(0, 20, edit));
    }

    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第一路扫描周期(us)"), channel1PeriodEdit, true));
    sectionLayout->addWidget(createNoteRow(sectionFrame, QStringLiteral("0-20")));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第二路扫描周期(us)"), channel2PeriodEdit, true));
    sectionLayout->addWidget(createNoteRow(sectionFrame, QStringLiteral("0-20")));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第三路扫描周期(us)"), channel3PeriodEdit, true));
    sectionLayout->addWidget(createNoteRow(sectionFrame, QStringLiteral("0-20")));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第四路扫描周期(us)"), channel4PeriodEdit, true));
    sectionLayout->addWidget(createNoteRow(sectionFrame, QStringLiteral("0-20")));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第五路扫描周期(us)"), channel5PeriodEdit, true));
    sectionLayout->addWidget(createNoteRow(sectionFrame, QStringLiteral("0-20")));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
    sectionLayout->addWidget(createFormRow(sectionFrame, QStringLiteral("第六路扫描周期(us)"), channel6PeriodEdit, true));
    sectionLayout->addWidget(createNoteRow(sectionFrame, QStringLiteral("0-20")));
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
}

void SignalSourceParamsPage::updateSignalSourceParams(const SignalSourceParamsConfig &config)
{
    serialScanPeriodEdit->setText(QString::number(config.serialScan));
    const QList<QComboBox *> modeBoxes = {channel1ModeComboBox, channel2ModeComboBox, channel3ModeComboBox,
                                          channel4ModeComboBox, channel5ModeComboBox, channel6ModeComboBox};
    for (int i = 0; i < modeBoxes.size(); ++i)
    {
        const int scanMode = i < config.scanModes.size() ? config.scanModes.at(i) : 0;
        modeBoxes.at(i)->setCurrentIndex(qBound(0, scanMode, 1));
    }

    signalModeComboBox->setCurrentIndex(qBound(0, config.vcoMode, 1));
    const QList<QLineEdit *> periodEdits = {channel1PeriodEdit, channel2PeriodEdit, channel3PeriodEdit,
                                            channel4PeriodEdit, channel5PeriodEdit, channel6PeriodEdit};
    for (int i = 0; i < periodEdits.size(); ++i)
    {
        const int vcoScan = i < config.vcoScans.size() ? config.vcoScans.at(i) : 0;
        periodEdits.at(i)->setText(QString::number(vcoScan));
    }
}

void SignalSourceParamsPage::showSaveResult(bool success, const QString &message)
{
    showToastResult(success, message);
}

void SignalSourceParamsPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateToastPosition();
}

QFrame *SignalSourceParamsPage::createSectionFrame(const QString &title, QVBoxLayout *pageLayout, QVBoxLayout *&sectionLayout)
{
    QFrame *sectionFrame = new QFrame(this);
    sectionFrame->setStyleSheet("QFrame { background-color: #2a2d33; border-radius: 0px; }");

    sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setContentsMargins(0, 0, 0, 18);
    sectionLayout->setSpacing(0);

    QLabel *titleLabel = new QLabel(title, sectionFrame);
    titleLabel->setStyleSheet(sectionTitleStyle());
    sectionLayout->addWidget(titleLabel);
    pageLayout->addWidget(sectionFrame);
    return sectionFrame;
}

QWidget *SignalSourceParamsPage::createFormRow(QFrame *parent, const QString &labelText, QWidget *fieldWidget, bool required)
{
    QWidget *rowWidget = new QWidget(parent);
    rowWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(18, 16, 18, 16);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel(required ? requiredLabelText(labelText) : labelText, rowWidget);
    label->setTextFormat(Qt::RichText);
    label->setStyleSheet(formLabelStyle());
    label->setFixedWidth(260);
    rowLayout->addWidget(label);
    rowLayout->addStretch();
    rowLayout->addWidget(fieldWidget, 0, Qt::AlignRight | Qt::AlignVCenter);
    return rowWidget;
}

QWidget *SignalSourceParamsPage::createNoteRow(QFrame *parent, const QString &noteText) const
{
    QWidget *noteWidget = new QWidget(parent);
    noteWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *noteLayout = new QHBoxLayout(noteWidget);
    noteLayout->setContentsMargins(0, -6, 18, 8);
    noteLayout->setSpacing(0);
    noteLayout->addStretch();

    QLabel *noteLabel = new QLabel(noteText, noteWidget);
    noteLabel->setStyleSheet(noteLabelStyle());
    noteLayout->addWidget(noteLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    return noteWidget;
}

QFrame *SignalSourceParamsPage::createSeparatorLine(QFrame *parent) const
{
    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #3a3a3a; max-height: 1px;");
    return line;
}

QComboBox *SignalSourceParamsPage::createStyledComboBox(QFrame *parent, const QStringList &items, int width,
                                                        const QString &currentText) const
{
    QComboBox *comboBox = new QComboBox(parent);
    comboBox->addItems(items);
    comboBox->setFixedSize(width, 32);
    comboBox->setStyleSheet("QComboBox { background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; "
                            "border-radius: 2px; padding-left: 10px; padding-right: 28px; font-size: 14px; }"
                            "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; "
                            "border-left: 1px solid #2a2a2a; }"
                            "QComboBox::down-arrow { image: none; }"
                            "QComboBox QAbstractItemView { background-color: #202225; color: #ffffff; "
                            "selection-background-color: #3a3a3a; border: 1px solid #2e2e2e; }");
    if (!currentText.isEmpty())
    {
        const int index = comboBox->findText(currentText);
        if (index >= 0)
        {
            comboBox->setCurrentIndex(index);
        }
    }
    return comboBox;
}

QLineEdit *SignalSourceParamsPage::createStyledLineEdit(QFrame *parent, const QString &text, int width) const
{
    QLineEdit *lineEdit = new QLineEdit(text, parent);
    lineEdit->setFixedSize(width, 32);
    lineEdit->setStyleSheet("QLineEdit { background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; "
                            "border-radius: 2px; padding: 0 10px; font-size: 14px; }");
    return lineEdit;
}

bool SignalSourceParamsPage::buildSavePayload(int &serialScan, QVector<int> &scanModes, int &signalMode, QVector<int> &vcoScans,
                                              QString &errorMessage) const
{
    bool ok = false;
    serialScan = serialScanPeriodEdit->text().trimmed().toInt(&ok);
    if (!ok || serialScan < 0 || serialScan > 16384)
    {
        errorMessage = QStringLiteral("串行扫描周期需在 0-16384 之间。");
        return false;
    }

    scanModes = {channel1ModeComboBox->currentIndex(), channel2ModeComboBox->currentIndex(), channel3ModeComboBox->currentIndex(),
                 channel4ModeComboBox->currentIndex(), channel5ModeComboBox->currentIndex(), channel6ModeComboBox->currentIndex()};

    signalMode = signalModeComboBox->currentIndex();
    vcoScans.clear();
    const QList<QLineEdit *> edits = {channel1PeriodEdit, channel2PeriodEdit, channel3PeriodEdit,
                                      channel4PeriodEdit, channel5PeriodEdit, channel6PeriodEdit};
    for (int i = 0; i < edits.size(); ++i)
    {
        bool periodOk = false;
        const int value = edits.at(i)->text().trimmed().toInt(&periodOk);
        if (!periodOk || value < 0 || value > 20)
        {
            errorMessage = QStringLiteral("第 %1 路扫描周期需在 0-20 之间。").arg(i + 1);
            return false;
        }
        vcoScans.append(value);
    }

    return true;
}

void SignalSourceParamsPage::handleSaveClicked()
{
    int serialScan = 0;
    int signalMode = 0;
    QVector<int> scanModes;
    QVector<int> vcoScans;
    QString errorMessage;
    if (!buildSavePayload(serialScan, scanModes, signalMode, vcoScans, errorMessage))
    {
        showToastResult(false, errorMessage);
        return;
    }

    emit requestSaveSignalSourceParams(serialScan, scanModes, signalMode, vcoScans);
}

void SignalSourceParamsPage::ensureToastWidget()
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

void SignalSourceParamsPage::updateToastPosition()
{
    if (!toastWidget)
    {
        return;
    }

    toastWidget->adjustSize();
    const int x = qMax(0, (width() - toastWidget->width()) / 2);
    toastWidget->move(x, 18);
}

void SignalSourceParamsPage::showToastResult(bool success, const QString &message)
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

QString SignalSourceParamsPage::extractDisplayMessage(bool success, const QString &message) const
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

QPushButton *SignalSourceParamsPage::createPrimaryButton(QFrame *parent, const QString &text, int width) const
{
    QPushButton *button = new QPushButton(text, parent);
    button->setFixedSize(width, 34);
    button->setStyleSheet("QPushButton { background-color: #f0f0f0; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #dfdfdf; }");
    return button;
}

QString SignalSourceParamsPage::sectionTitleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold; padding: 0 0 10px 8px;");
}

QString SignalSourceParamsPage::formLabelStyle() const
{
    return QStringLiteral("color: #e6e6e6; font-size: 14px;");
}

QString SignalSourceParamsPage::noteLabelStyle() const
{
    return QStringLiteral("color: #9da2ab; font-size: 12px;");
}
