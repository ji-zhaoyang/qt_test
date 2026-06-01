#include "angle_calibration_page.h"
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

AngleCalibrationPage::AngleCalibrationPage(QWidget *parent)
    : QWidget(parent), startCheckButton(nullptr), finishRotateButton(nullptr), confirmCalibrationButton(nullptr),
      cancelCalibrationButton(nullptr), angleInput(nullptr), toastWidget(nullptr), toastIconLabel(nullptr),
      toastTextLabel(nullptr), toastHideTimer(nullptr), toastOpacityEffect(nullptr), toastFadeInAnimation(nullptr),
      toastFadeOutAnimation(nullptr)
{
    setupUi();
}

void AngleCalibrationPage::setupUi()
{
    setObjectName("angleCalibrationPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#angleCalibrationPage { background-color: #202020; color: #ffffff; }");

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
    pageLayout->setSpacing(20);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel *pageTitle = new QLabel(QStringLiteral("角度校准"), content);
    pageTitle->setStyleSheet(titleStyle());
    pageLayout->addWidget(pageTitle);

    QFrame *card = new QFrame(content);
    card->setStyleSheet("QFrame { background-color: #2b2b2b; border-radius: 6px; }");
    pageLayout->addWidget(card);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(24, 20, 24, 24);
    cardLayout->setSpacing(18);

    QLabel *sectionTitle = new QLabel(QStringLiteral("罗盘设置步骤"), card);
    sectionTitle->setStyleSheet(sectionTitleStyle());
    cardLayout->addWidget(sectionTitle);

    cardLayout->addWidget(createStepLabel(card, QStringLiteral("1、把设备垂直放在安装架上或者水平的桌面上")));

    QWidget *step2Row = new QWidget(card);
    step2Row->setStyleSheet("background-color: transparent;");
    QVBoxLayout *step2Layout = new QVBoxLayout(step2Row);
    step2Layout->setContentsMargins(0, 0, 0, 0);
    step2Layout->setSpacing(12);
    step2Layout->addWidget(createStepLabel(step2Row, QStringLiteral("2、点击开始检查")));
    startCheckButton = createActionButton(step2Row, QStringLiteral("开始校准"));
    step2Layout->addWidget(startCheckButton, 0, Qt::AlignLeft);
    cardLayout->addWidget(step2Row);

    cardLayout->addWidget(createStepLabel(
        card, QStringLiteral("3、提示开始校准后，将针1~3后开始旋转设备(让显示器朝上下左右前后面匀速转半圈)")));

    QWidget *step4Row = new QWidget(card);
    step4Row->setStyleSheet("background-color: transparent;");
    QVBoxLayout *step4Layout = new QVBoxLayout(step4Row);
    step4Layout->setContentsMargins(0, 0, 0, 0);
    step4Layout->setSpacing(12);
    step4Layout->addWidget(createStepLabel(step4Row, QStringLiteral("4、旋转结束后，点击旋转结束确认")));
    finishRotateButton = createActionButton(step4Row, QStringLiteral("旋转结束确认"));
    step4Layout->addWidget(finishRotateButton, 0, Qt::AlignLeft);
    cardLayout->addWidget(step4Row);

    QWidget *step5Row = new QWidget(card);
    step5Row->setStyleSheet("background-color: transparent;");
    QVBoxLayout *step5Layout = new QVBoxLayout(step5Row);
    step5Layout->setContentsMargins(0, 0, 0, 0);
    step5Layout->setSpacing(12);
    step5Layout->addWidget(createStepLabel(step5Row, QStringLiteral("5、输入罗盘参数值，例如(170)，点击确认校准")));

    QWidget *inputRow = new QWidget(step5Row);
    inputRow->setStyleSheet("background-color: transparent;");
    QHBoxLayout *inputLayout = new QHBoxLayout(inputRow);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(14);

    QLabel *parameterLabel = new QLabel(QStringLiteral("罗盘参数"), inputRow);
    parameterLabel->setStyleSheet(sectionTitleStyle());
    inputLayout->addWidget(parameterLabel);

    inputLayout->addStretch();

    angleInput = new QLineEdit(inputRow);
    angleInput->setPlaceholderText(QStringLiteral("0-359的整数"));
    angleInput->setFixedSize(150, 34);
    angleInput->setValidator(new QIntValidator(0, 359, angleInput));
    angleInput->setStyleSheet(inputStyle());
    inputLayout->addWidget(angleInput);
    step5Layout->addWidget(inputRow);

    confirmCalibrationButton = createActionButton(step5Row, QStringLiteral("确认校准"));
    step5Layout->addWidget(confirmCalibrationButton, 0, Qt::AlignLeft);
    cardLayout->addWidget(step5Row);

    QFrame *separator = new QFrame(card);
    separator->setFrameShape(QFrame::HLine);
    separator->setStyleSheet("background-color: #444444; max-height: 1px;");
    cardLayout->addWidget(separator);

    cardLayout->addWidget(createStepLabel(
        card, QStringLiteral("6、确认设备当前正常角度，例如设备当前正常角度是60度，但是当页面显示设备角度为80度")));
    cardLayout->addWidget(createStepLabel(
        card, QStringLiteral("7、修改罗盘参数值，例如之前输入的值为170，而设备显示角度和正常角度差值为20度，需要罗盘参数值为190，并点击确认校准，观察设备显示角度是否正常")));
    cardLayout->addWidget(createStepLabel(card, QStringLiteral("8、如果角度不对，重复6-7步骤，直到角度正确")));

    cancelCalibrationButton = createActionButton(card, QStringLiteral("取消校准"));
    cardLayout->addWidget(cancelCalibrationButton, 0, Qt::AlignLeft);

    pageLayout->addStretch();

    connect(startCheckButton, &QPushButton::clicked, this, &AngleCalibrationPage::requestStartCalibration);
    connect(finishRotateButton, &QPushButton::clicked, this, &AngleCalibrationPage::requestFinishCalibration);
    connect(cancelCalibrationButton, &QPushButton::clicked, this, &AngleCalibrationPage::requestCancelCalibration);
    connect(confirmCalibrationButton, &QPushButton::clicked, this,
            [this]()
            {
                const QString angleText = angleInput ? angleInput->text().trimmed() : QString();
                bool ok = false;
                const int angle = angleText.toInt(&ok);
                if (!ok || angle < 0 || angle > 359)
                {
                    QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入 0-359 的整数。"));
                    return;
                }

                emit requestConfirmCalibration(static_cast<uint16_t>(angle));
            });
}

void AngleCalibrationPage::showCalibrationResult(uint16_t responseDataType, bool success, const QString &message)
{
    switch (responseDataType)
    {
    case 32:
        showOperationMessage(QStringLiteral("开始校准"), success, message);
        break;
    case 34:
        showOperationMessage(QStringLiteral("旋转结束确认"), success, message);
        break;
    case 36:
        showOperationMessage(QStringLiteral("确认校准"), success, message);
        break;
    case 38:
        showOperationMessage(QStringLiteral("取消校准"), success, message);
        break;
    default:
        showOperationMessage(QStringLiteral("角度校准"), success, message);
        break;
    }
}

void AngleCalibrationPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateToastPosition();
}

QWidget *AngleCalibrationPage::createStepLabel(QWidget *parent, const QString &text) const
{
    QLabel *label = new QLabel(text, parent);
    label->setWordWrap(true);
    label->setStyleSheet(stepTextStyle());
    return label;
}

QPushButton *AngleCalibrationPage::createActionButton(QWidget *parent, const QString &text) const
{
    QPushButton *button = new QPushButton(text, parent);
    button->setFixedSize(128, 38);
    button->setStyleSheet(buttonStyle());
    return button;
}

void AngleCalibrationPage::ensureToastWidget()
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

void AngleCalibrationPage::updateToastPosition()
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

void AngleCalibrationPage::showOperationMessage(const QString &title, bool success, const QString &message)
{
    Q_UNUSED(title);

    ensureToastWidget();

    const QString displayText = extractDisplayMessage(success, message);
    toastIconLabel->setText(success ? QStringLiteral("✓") : QStringLiteral("!"));
    toastIconLabel->setStyleSheet(success
                                      ? "background-color: #67c23a; color: #ffffff; font-size: 14px; font-weight: bold; "
                                        "border-radius: 10px;"
                                      : "background-color: #ff9f55; color: #ffffff; font-size: 14px; font-weight: bold; "
                                        "border-radius: 10px;");
    toastTextLabel->setText(displayText);
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

QString AngleCalibrationPage::extractDisplayMessage(bool success, const QString &message) const
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

QString AngleCalibrationPage::titleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold;");
}

QString AngleCalibrationPage::sectionTitleStyle() const
{
    return QStringLiteral("color: #e6e6e6; font-size: 14px; font-weight: bold;");
}

QString AngleCalibrationPage::stepTextStyle() const
{
    return QStringLiteral("color: #d8d8d8; font-size: 13px; line-height: 1.9;");
}

QString AngleCalibrationPage::inputStyle() const
{
    return QStringLiteral("QLineEdit { background-color: #111111; color: #ffffff; border: 1px solid #3d3d3d; "
                          "border-radius: 2px; padding: 0 10px; font-size: 14px; }");
}

QString AngleCalibrationPage::buttonStyle() const
{
    return QStringLiteral("QPushButton { background-color: #ffffff; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #e0e0e0; }");
}
