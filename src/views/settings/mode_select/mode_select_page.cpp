#include "mode_select_page.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
QString requiredLabelText(const QString &labelText)
{
    return "<font color='red'>*</font> " + labelText;
}
}

ModeSelectPage::ModeSelectPage(QWidget *parent)
    : QWidget(parent), droneReportModeCombo(nullptr), jamModeCombo(nullptr), networkModeCombo(nullptr),
      wifiRemoteIdFeatureCombo(nullptr), fpvFeatureCombo(nullptr), djiParseFeatureCombo(nullptr),
      uavCategoryDisplayCombo(nullptr), dataEnableCheckBox(nullptr), droneReportModeSaveButton(nullptr),
      jamModeSaveButton(nullptr), networkModeSaveButton(nullptr), featureModeSaveButton(nullptr),
      uavCategoryDisplaySaveButton(nullptr), dataEnableSaveButton(nullptr), uavCategoryDisplayFrame(nullptr),
      dataEnableFrame(nullptr), toastWidget(nullptr), toastIconLabel(nullptr), toastTextLabel(nullptr),
      toastHideTimer(nullptr), toastOpacityEffect(nullptr), toastFadeInAnimation(nullptr), toastFadeOutAnimation(nullptr),
      droneReportModePendingTimer(nullptr), jamModePendingTimer(nullptr), networkModePendingTimer(nullptr),
      uavCategoryDisplayModePendingTimer(nullptr), dataEnablePendingTimer(nullptr), featureModePendingTimer(nullptr),
      currentUserRole(SettingsUserRole::None)
{
    setupUi();
    setUserRole(SettingsUserRole::None);
}

void ModeSelectPage::setUserRole(SettingsUserRole role)
{
    currentUserRole = role;
    const bool isRoot = currentUserRole == SettingsUserRole::Root;
    if (uavCategoryDisplayFrame)
    {
        uavCategoryDisplayFrame->setVisible(isRoot);
    }
    if (dataEnableFrame)
    {
        dataEnableFrame->setVisible(isRoot);
    }
}

void ModeSelectPage::setupUi()
{
    setObjectName("modeSelectPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background-color: #202020; color: #ffffff;");

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
    scrollArea->setWidget(createScrollableContent());

    droneReportModePendingTimer = new QTimer(this);
    jamModePendingTimer = new QTimer(this);
    networkModePendingTimer = new QTimer(this);
    uavCategoryDisplayModePendingTimer = new QTimer(this);
    dataEnablePendingTimer = new QTimer(this);
    featureModePendingTimer = new QTimer(this);
    setupPendingTimer(droneReportModePendingTimer, SectionKey::DroneReportMode, droneReportModeSaveButton);
    setupPendingTimer(jamModePendingTimer, SectionKey::JamMode, jamModeSaveButton);
    setupPendingTimer(networkModePendingTimer, SectionKey::NetworkMode, networkModeSaveButton);
    setupPendingTimer(uavCategoryDisplayModePendingTimer, SectionKey::UavCategoryDisplayMode, uavCategoryDisplaySaveButton);
    setupPendingTimer(dataEnablePendingTimer, SectionKey::DataEnable, dataEnableSaveButton);
    setupPendingTimer(featureModePendingTimer, SectionKey::FeatureMode, featureModeSaveButton);
}

QWidget *ModeSelectPage::createScrollableContent()
{
    QWidget *content = new QWidget(this);
    content->setStyleSheet("background-color: #202020;");

    QVBoxLayout *pageLayout = new QVBoxLayout(content);
    pageLayout->setContentsMargins(40, 30, 20, 30);
    pageLayout->setSpacing(20);
    pageLayout->setAlignment(Qt::AlignTop);

    QVBoxLayout *detectLayout = nullptr;
    QFrame *detectFrame = createSectionFrame("侦测模式设置", pageLayout, detectLayout);
    droneReportModeCombo = createStyledComboBox(
        detectFrame, {"协议分析模式", "频谱分析模式", "混合模式", "频段扫描模式", "组合模式"}, "混合模式");
    addSingleChoiceRow(detectLayout, detectFrame, "无人机上报模式", droneReportModeCombo);
    addSectionSaveButton(detectLayout, detectFrame, sectionKeyToString(SectionKey::DroneReportMode));

    QVBoxLayout *jamLayout = nullptr;
    QFrame *jamFrame = createSectionFrame("压制模式设置", pageLayout, jamLayout);
    jamModeCombo = createStyledComboBox(jamFrame, {"频段跟踪模式", "频段全开模式"}, "频段全开模式");
    addSingleChoiceRow(jamLayout, jamFrame, "压制模式", jamModeCombo);
    addSectionSaveButton(jamLayout, jamFrame, sectionKeyToString(SectionKey::JamMode));

    QVBoxLayout *networkLayout = nullptr;
    QFrame *networkFrame = createSectionFrame("联网模式设置", pageLayout, networkLayout);
    networkModeCombo = createStyledComboBox(networkFrame, {"有线连接", "无线连接"}, "有线连接");
    addSingleChoiceRow(networkLayout, networkFrame, "联网模式", networkModeCombo, "重启生效");
    addSectionSaveButton(networkLayout, networkFrame, sectionKeyToString(SectionKey::NetworkMode));

    QVBoxLayout *featureLayout = nullptr;
    QFrame *featureFrame = createSectionFrame("功能模式设置", pageLayout, featureLayout);
    wifiRemoteIdFeatureCombo = createStyledComboBox(featureFrame, {"关闭", "开启"}, "关闭");
    fpvFeatureCombo = createStyledComboBox(featureFrame, {"关闭", "开启"}, "关闭");
    djiParseFeatureCombo = createStyledComboBox(featureFrame, {"关闭", "开启"}, "关闭");
    addFeatureChoiceRow(featureLayout, featureFrame, "增强型WiFi/RemoteID功能检测", wifiRemoteIdFeatureCombo);
    addFeatureChoiceRow(featureLayout, featureFrame, "频谱扫描FPV功能检测", fpvFeatureCombo);
    addFeatureChoiceRow(featureLayout, featureFrame, "大疆无人机解析功能检测", djiParseFeatureCombo);
    addSectionSaveButton(featureLayout, featureFrame, sectionKeyToString(SectionKey::FeatureMode));

    QVBoxLayout *uavCategoryLayout = nullptr;
    uavCategoryDisplayFrame = createSectionFrame("无人机类别显示", pageLayout, uavCategoryLayout);
    uavCategoryDisplayCombo = createStyledComboBox(uavCategoryDisplayFrame, {"详细", "简略"}, "详细");
    addSingleChoiceRow(uavCategoryLayout, uavCategoryDisplayFrame, "无人机类别显示", uavCategoryDisplayCombo);
    addSectionSaveButton(uavCategoryLayout, uavCategoryDisplayFrame, sectionKeyToString(SectionKey::UavCategoryDisplayMode));

    QVBoxLayout *dataEnableLayout = nullptr;
    dataEnableFrame = createSectionFrame("数据使能", pageLayout, dataEnableLayout);
    dataEnableCheckBox = createStyledToggleSwitch(dataEnableFrame, false);
    addToggleChoiceRow(dataEnableLayout, dataEnableFrame, "数据使能", dataEnableCheckBox);
    addSectionSaveButton(dataEnableLayout, dataEnableFrame, QStringLiteral("dataEnable"));

    pageLayout->addStretch();
    return content;
}

void ModeSelectPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateToastPosition();
}

QFrame *ModeSelectPage::createSectionFrame(const QString &title, QVBoxLayout *pageLayout, QVBoxLayout *&sectionLayout)
{
    QFrame *sectionFrame = new QFrame(this);
    sectionFrame->setStyleSheet("QFrame { background-color: #2b2b2b; border-radius: 6px; }");

    sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setContentsMargins(20, 18, 20, 18);
    sectionLayout->setSpacing(0);

    QLabel *titleLabel = new QLabel(title, sectionFrame);
    titleLabel->setStyleSheet(sectionTitleStyle());
    sectionLayout->addWidget(titleLabel);
    pageLayout->addWidget(sectionFrame);
    return sectionFrame;
}

QComboBox *ModeSelectPage::createStyledComboBox(QFrame *parent, const QStringList &items, const QString &currentText) const
{
    QComboBox *comboBox = new QComboBox(parent);
    comboBox->addItems(items);
    comboBox->setFixedSize(200, 32);
    comboBox->setStyleSheet(comboBoxStyle());

    QLabel *arrowLabel = new QLabel("▼", comboBox);
    arrowLabel->setStyleSheet("color: #888; font-size: 10px; background: transparent;");
    arrowLabel->setAlignment(Qt::AlignCenter);
    arrowLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    arrowLabel->setGeometry(170, 0, 30, 32);

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

void ModeSelectPage::addSingleChoiceRow(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &labelText,
                                        QComboBox *comboBox, const QString &noteText)
{
    QWidget *rowWidget = new QWidget(sectionFrame);
    rowWidget->setStyleSheet("background-color: transparent;");

    QVBoxLayout *rowWrapperLayout = new QVBoxLayout(rowWidget);
    rowWrapperLayout->setContentsMargins(0, 0, 0, 0);
    rowWrapperLayout->setSpacing(0);

    QWidget *mainRowWidget = new QWidget(rowWidget);
    mainRowWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *rowLayout = new QHBoxLayout(mainRowWidget);
    rowLayout->setContentsMargins(0, 14, 0, 14);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel(requiredLabelText(labelText), mainRowWidget);
    label->setStyleSheet(formLabelStyle());
    label->setFixedWidth(220);
    rowLayout->addWidget(label);
    rowLayout->addStretch();
    rowLayout->addWidget(comboBox);

    rowWrapperLayout->addWidget(mainRowWidget);

    if (!noteText.isEmpty())
    {
        QWidget *noteWidget = new QWidget(rowWidget);
        noteWidget->setStyleSheet("background-color: transparent;");
        QHBoxLayout *noteLayout = new QHBoxLayout(noteWidget);
        noteLayout->setContentsMargins(0, -2, 0, 10);
        noteLayout->setSpacing(0);
        noteLayout->addStretch();

        QLabel *noteLabel = new QLabel(noteText, noteWidget);
        noteLabel->setStyleSheet(noteLabelStyle());
        noteLabel->setFixedWidth(200);
        noteLayout->addWidget(noteLabel);
        rowWrapperLayout->addWidget(noteWidget);
    }

    sectionLayout->addWidget(rowWidget);
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
}

void ModeSelectPage::addFeatureChoiceRow(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &labelText,
                                         QComboBox *comboBox)
{
    QWidget *rowWidget = new QWidget(sectionFrame);
    rowWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 14, 0, 14);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel(requiredLabelText(labelText), rowWidget);
    label->setStyleSheet(formLabelStyle());
    label->setFixedWidth(260);
    rowLayout->addWidget(label);
    rowLayout->addStretch();
    rowLayout->addWidget(comboBox);

    sectionLayout->addWidget(rowWidget);
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
}

void ModeSelectPage::addToggleChoiceRow(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &labelText,
                                        QCheckBox *checkBox)
{
    QWidget *rowWidget = new QWidget(sectionFrame);
    rowWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 14, 0, 14);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel(requiredLabelText(labelText), rowWidget);
    label->setStyleSheet(formLabelStyle());
    label->setFixedWidth(220);
    rowLayout->addWidget(label);
    rowLayout->addStretch();
    rowLayout->addWidget(checkBox);

    sectionLayout->addWidget(rowWidget);
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
}

void ModeSelectPage::addSectionSaveButton(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &sectionKey)
{
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 15, 0, 0);
    buttonLayout->setSpacing(0);

    QPushButton *saveButton = new QPushButton("保存", sectionFrame);
    saveButton->setProperty("sectionKey", sectionKey);
    saveButton->setFixedSize(120, 36);
    saveButton->setStyleSheet(saveButtonStyle());
    connect(saveButton, &QPushButton::clicked, this, &ModeSelectPage::handleSaveClicked);
    if (sectionKey == sectionKeyToString(SectionKey::DroneReportMode))
    {
        droneReportModeSaveButton = saveButton;
    }
    else if (sectionKey == sectionKeyToString(SectionKey::JamMode))
    {
        jamModeSaveButton = saveButton;
    }
    else if (sectionKey == sectionKeyToString(SectionKey::NetworkMode))
    {
        networkModeSaveButton = saveButton;
    }
    else if (sectionKey == sectionKeyToString(SectionKey::UavCategoryDisplayMode))
    {
        uavCategoryDisplaySaveButton = saveButton;
    }
    else if (sectionKey == sectionKeyToString(SectionKey::FeatureMode))
    {
        featureModeSaveButton = saveButton;
    }
    else if (sectionKey == QStringLiteral("dataEnable"))
    {
        dataEnableSaveButton = saveButton;
    }

    buttonLayout->addWidget(saveButton);
    buttonLayout->addStretch();
    sectionLayout->addLayout(buttonLayout);
}

QFrame *ModeSelectPage::createSeparatorLine(QFrame *parent) const
{
    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #444; max-height: 1px;");
    return line;
}

QCheckBox *ModeSelectPage::createStyledToggleSwitch(QFrame *parent, bool checked) const
{
    QCheckBox *checkBox = new QCheckBox(parent);
    checkBox->setChecked(checked);
    checkBox->setCursor(Qt::PointingHandCursor);
    checkBox->setStyleSheet("QCheckBox { spacing: 0px; }"
                            "QCheckBox::indicator { width: 50px; height: 28px; border-radius: 14px; "
                            "background-color: #5f5f5f; }"
                            "QCheckBox::indicator:unchecked { image: none; background-color: #5f5f5f; "
                            "border: 1px solid #4a4a4a; }"
                            "QCheckBox::indicator:checked { image: none; background-color: #4caf50; "
                            "border: 1px solid #459b49; }");
    return checkBox;
}

QString ModeSelectPage::comboBoxStyle() const
{
    return "QComboBox { "
           "    background-color: #1e1e1e; "
           "    color: #fff; "
           "    border: 1px solid #444; "
           "    border-radius: 3px; "
           "    padding-left: 10px; "
           "    padding-right: 30px; "
           "} "
           "QComboBox::drop-down { "
           "    subcontrol-origin: padding; "
           "    subcontrol-position: top right; "
           "    width: 30px; "
           "    border-left-width: 1px; "
           "    border-left-color: #333; "
           "    border-left-style: solid; "
           "} "
           "QComboBox::down-arrow { "
           "    image: none; "
           "} "
           "QComboBox QAbstractItemView { "
           "    background-color: #2b2b2b; "
           "    color: #fff; "
           "    selection-background-color: #444444; "
           "    border: 1px solid #444; "
           "}";
}

QString ModeSelectPage::formLabelStyle() const
{
    return "color: #cccccc; font-size: 13px;";
}

QString ModeSelectPage::sectionTitleStyle() const
{
    return "color: #ffffff; font-size: 15px; font-weight: bold; padding: 4px 0 12px 0;";
}

QString ModeSelectPage::noteLabelStyle() const
{
    return "color: #8a8a8a; font-size: 11px;";
}

QString ModeSelectPage::saveButtonStyle() const
{
    return "QPushButton { background-color: #ffffff; color: black; border: none; border-radius: 2px; font-weight: bold; "
           "font-size: 13px; }"
           "QPushButton:hover { background-color: #e0e0e0; }"
           "QPushButton:disabled { background-color: #9d9d9d; color: #2f2f2f; }";
}

void ModeSelectPage::updateDroneReportMode(uint8_t mode)
{
    if (!droneReportModeCombo)
    {
        return;
    }

    const int comboIndex = droneReportModeToComboIndex(mode);
    if (comboIndex >= 0 && comboIndex < droneReportModeCombo->count())
    {
        droneReportModeCombo->setCurrentIndex(comboIndex);
    }

    handleRecoveryQueryResult(SectionKey::DroneReportMode, QByteArray(1, static_cast<char>(mode)));
}

void ModeSelectPage::showDroneReportModeSaveResult(bool success, const QString &message)
{
    clearPendingCommand(SectionKey::DroneReportMode);
    showToastResult(success, message);
}

void ModeSelectPage::updateJamMode(uint8_t mode)
{
    if (!jamModeCombo)
    {
        return;
    }

    const int comboIndex = jamModeToComboIndex(mode);
    if (comboIndex >= 0 && comboIndex < jamModeCombo->count())
    {
        jamModeCombo->setCurrentIndex(comboIndex);
    }

    handleRecoveryQueryResult(SectionKey::JamMode, QByteArray(1, static_cast<char>(mode)));
}

void ModeSelectPage::showJamModeSaveResult(bool success, const QString &message)
{
    clearPendingCommand(SectionKey::JamMode);
    showToastResult(success, message);
}

void ModeSelectPage::updateNetworkMode(uint8_t mode)
{
    if (!networkModeCombo)
    {
        return;
    }

    const int comboIndex = networkModeToComboIndex(mode);
    if (comboIndex >= 0 && comboIndex < networkModeCombo->count())
    {
        networkModeCombo->setCurrentIndex(comboIndex);
    }

    handleRecoveryQueryResult(SectionKey::NetworkMode, QByteArray(1, static_cast<char>(mode)));
}

void ModeSelectPage::showNetworkModeSaveResult(bool success, const QString &message)
{
    clearPendingCommand(SectionKey::NetworkMode);
    showToastResult(success, message);
}

void ModeSelectPage::updateUavCategoryDisplayMode(uint8_t mode)
{
    if (!uavCategoryDisplayCombo)
    {
        return;
    }

    const int comboIndex = uavCategoryDisplayModeToComboIndex(mode);
    if (comboIndex >= 0 && comboIndex < uavCategoryDisplayCombo->count())
    {
        uavCategoryDisplayCombo->setCurrentIndex(comboIndex);
    }

    handleRecoveryQueryResult(SectionKey::UavCategoryDisplayMode, QByteArray(1, static_cast<char>(mode)));
}

void ModeSelectPage::showUavCategoryDisplayModeSaveResult(bool success, const QString &message)
{
    clearPendingCommand(SectionKey::UavCategoryDisplayMode);
    showToastResult(success, message);
}

void ModeSelectPage::updateDataEnable(uint8_t enabled)
{
    if (!dataEnableCheckBox)
    {
        return;
    }

    dataEnableCheckBox->setChecked(enabled != 0);
    handleRecoveryQueryResult(SectionKey::DataEnable, QByteArray(1, static_cast<char>(enabled != 0 ? 1 : 0)));
}

void ModeSelectPage::showDataEnableSaveResult(bool success, const QString &message)
{
    clearPendingCommand(SectionKey::DataEnable);
    showToastResult(success, message);
}

void ModeSelectPage::updateFeatureModes(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled)
{
    if (!wifiRemoteIdFeatureCombo || !fpvFeatureCombo || !djiParseFeatureCombo)
    {
        return;
    }

    const int wifiIndex = featureFlagToComboIndex(wifiRemoteIdEnabled);
    const int fpvIndex = featureFlagToComboIndex(fpvEnabled);
    const int djiIndex = featureFlagToComboIndex(djiParseEnabled);
    if (wifiIndex >= 0)
    {
        wifiRemoteIdFeatureCombo->setCurrentIndex(wifiIndex);
    }
    if (fpvIndex >= 0)
    {
        fpvFeatureCombo->setCurrentIndex(fpvIndex);
    }
    if (djiIndex >= 0)
    {
        djiParseFeatureCombo->setCurrentIndex(djiIndex);
    }

    handleRecoveryQueryResult(SectionKey::FeatureMode, currentFeatureModePayload());
}

void ModeSelectPage::showFeatureModeSaveResult(bool success, const QString &message)
{
    clearPendingCommand(SectionKey::FeatureMode);
    showToastResult(success, message);
}

void ModeSelectPage::onConnectionLost()
{
    for (const SectionKey key :
         {SectionKey::DroneReportMode, SectionKey::JamMode, SectionKey::NetworkMode, SectionKey::UavCategoryDisplayMode,
          SectionKey::DataEnable, SectionKey::FeatureMode})
    {
        PendingModeCommand &command = pendingCommand(key);
        if (!command.active)
        {
            continue;
        }

        QTimer *timer = pendingTimer(key);
        if (timer)
        {
            timer->stop();
        }

        command.waitingReconnect = true;
        command.awaitingRecoveryQuery = false;

        if (QPushButton *button = saveButton(key))
        {
            button->setEnabled(false);
            button->setText(QStringLiteral("等待重连..."));
        }
    }
}

void ModeSelectPage::onConnectionRestored()
{
    for (const SectionKey key :
         {SectionKey::DroneReportMode, SectionKey::JamMode, SectionKey::NetworkMode, SectionKey::UavCategoryDisplayMode,
          SectionKey::DataEnable, SectionKey::FeatureMode})
    {
        PendingModeCommand &command = pendingCommand(key);
        if (!command.active || !command.waitingReconnect)
        {
            continue;
        }

        command.waitingReconnect = false;
        command.awaitingRecoveryQuery = true;

        if (QPushButton *button = saveButton(key))
        {
            button->setEnabled(false);
            button->setText(QStringLiteral("恢复中..."));
        }

        if (QTimer *timer = pendingTimer(key))
        {
            timer->start(3000);
        }
        requestRecoveryQuery(key);
    }
}

void ModeSelectPage::showToastResult(bool success, const QString &message)
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

void ModeSelectPage::setSaveButtonPending(QPushButton *button, bool pending)
{
    if (!button)
    {
        return;
    }

    button->setEnabled(!pending);
    button->setText(pending ? QStringLiteral("保存中...") : QStringLiteral("保存"));
}

void ModeSelectPage::setupPendingTimer(QTimer *timer, SectionKey key, QPushButton *button)
{
    if (!timer)
    {
        return;
    }

    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this,
            [this, key, button]()
            {
                clearPendingCommand(key, false);
                setSaveButtonPending(button, false);
                showToastResult(false, QStringLiteral("设置超时"));
            });
}

void ModeSelectPage::clearPendingCommand(SectionKey key, bool resetButton)
{
    if (QTimer *timer = pendingTimer(key))
    {
        timer->stop();
    }

    PendingModeCommand &command = pendingCommand(key);
    command = PendingModeCommand();

    if (resetButton)
    {
        setSaveButtonPending(saveButton(key), false);
    }
}

void ModeSelectPage::startPendingCommand(SectionKey key, const QByteArray &targetPayload, QPushButton *button, QTimer *timer)
{
    PendingModeCommand &command = pendingCommand(key);
    command = PendingModeCommand();
    command.active = true;
    command.targetPayload = targetPayload;

    setSaveButtonPending(button, true);
    if (timer)
    {
        timer->start(3000);
    }
}

void ModeSelectPage::handleRecoveryQueryResult(SectionKey key, const QByteArray &currentPayload)
{
    PendingModeCommand &command = pendingCommand(key);
    if (!command.active || !command.awaitingRecoveryQuery)
    {
        return;
    }
    command.awaitingRecoveryQuery = false;

    if (currentPayload == command.targetPayload)
    {
        clearPendingCommand(key);
        showToastResult(true, QStringLiteral("设置成功"));
        return;
    }

    resendPendingCommand(key);
}

void ModeSelectPage::requestRecoveryQuery(SectionKey key)
{
    if (key == SectionKey::DroneReportMode)
    {
        emit requestQueryDroneReportMode();
    }
    else if (key == SectionKey::JamMode)
    {
        emit requestQueryJamMode();
    }
    else if (key == SectionKey::NetworkMode)
    {
        emit requestQueryNetworkMode();
    }
    else if (key == SectionKey::UavCategoryDisplayMode)
    {
        emit requestQueryUavCategoryDisplayMode();
    }
    else if (key == SectionKey::DataEnable)
    {
        emit requestQueryDataEnable();
    }
    else if (key == SectionKey::FeatureMode)
    {
        emit requestQueryFeatureModes();
    }
}

void ModeSelectPage::resendPendingCommand(SectionKey key)
{
    PendingModeCommand &command = pendingCommand(key);
    if (!command.active)
    {
        return;
    }

    command.resendAttempted = true;
    if (QPushButton *button = saveButton(key))
    {
        setSaveButtonPending(button, true);
    }
    if (QTimer *timer = pendingTimer(key))
    {
        timer->start(3000);
    }

    if (key == SectionKey::DroneReportMode)
    {
        emit requestSaveDroneReportMode(
            static_cast<uint8_t>(static_cast<unsigned char>(command.targetPayload.at(0))));
    }
    else if (key == SectionKey::JamMode)
    {
        emit requestSaveJamMode(static_cast<uint8_t>(static_cast<unsigned char>(command.targetPayload.at(0))));
    }
    else if (key == SectionKey::NetworkMode)
    {
        emit requestSaveNetworkMode(static_cast<uint8_t>(static_cast<unsigned char>(command.targetPayload.at(0))));
    }
    else if (key == SectionKey::UavCategoryDisplayMode)
    {
        emit requestSaveUavCategoryDisplayMode(
            static_cast<uint8_t>(static_cast<unsigned char>(command.targetPayload.at(0))));
    }
    else if (key == SectionKey::DataEnable)
    {
        emit requestSaveDataEnable(static_cast<uint8_t>(static_cast<unsigned char>(command.targetPayload.at(0))));
    }
    else if (key == SectionKey::FeatureMode && command.targetPayload.size() >= 3)
    {
        emit requestSaveFeatureModes(static_cast<uint8_t>(static_cast<unsigned char>(command.targetPayload.at(0))),
                                     static_cast<uint8_t>(static_cast<unsigned char>(command.targetPayload.at(1))),
                                     static_cast<uint8_t>(static_cast<unsigned char>(command.targetPayload.at(2))));
    }
}

ModeSelectPage::PendingModeCommand &ModeSelectPage::pendingCommand(SectionKey key)
{
    if (key == SectionKey::DroneReportMode)
    {
        return droneReportModePendingCommand;
    }
    if (key == SectionKey::JamMode)
    {
        return jamModePendingCommand;
    }
    if (key == SectionKey::NetworkMode)
    {
        return networkModePendingCommand;
    }
    if (key == SectionKey::UavCategoryDisplayMode)
    {
        return uavCategoryDisplayModePendingCommand;
    }
    if (key == SectionKey::DataEnable)
    {
        return dataEnablePendingCommand;
    }
    return featureModePendingCommand;
}

QTimer *ModeSelectPage::pendingTimer(SectionKey key) const
{
    if (key == SectionKey::DroneReportMode)
    {
        return droneReportModePendingTimer;
    }
    if (key == SectionKey::JamMode)
    {
        return jamModePendingTimer;
    }
    if (key == SectionKey::NetworkMode)
    {
        return networkModePendingTimer;
    }
    if (key == SectionKey::UavCategoryDisplayMode)
    {
        return uavCategoryDisplayModePendingTimer;
    }
    if (key == SectionKey::DataEnable)
    {
        return dataEnablePendingTimer;
    }
    return featureModePendingTimer;
}

QPushButton *ModeSelectPage::saveButton(SectionKey key) const
{
    if (key == SectionKey::DroneReportMode)
    {
        return droneReportModeSaveButton;
    }
    if (key == SectionKey::JamMode)
    {
        return jamModeSaveButton;
    }
    if (key == SectionKey::NetworkMode)
    {
        return networkModeSaveButton;
    }
    if (key == SectionKey::UavCategoryDisplayMode)
    {
        return uavCategoryDisplaySaveButton;
    }
    if (key == SectionKey::DataEnable)
    {
        return dataEnableSaveButton;
    }
    return featureModeSaveButton;
}

void ModeSelectPage::ensureToastWidget()
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

void ModeSelectPage::updateToastPosition()
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

QString ModeSelectPage::extractDisplayMessage(bool success, const QString &message) const
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

QString ModeSelectPage::sectionKeyToString(SectionKey key)
{
    switch (key)
    {
    case SectionKey::DroneReportMode:
        return QStringLiteral("droneReportMode");
    case SectionKey::JamMode:
        return QStringLiteral("jamMode");
    case SectionKey::NetworkMode:
        return QStringLiteral("networkMode");
    case SectionKey::UavCategoryDisplayMode:
        return QStringLiteral("uavCategoryDisplayMode");
    case SectionKey::DataEnable:
        return QStringLiteral("dataEnable");
    case SectionKey::FeatureMode:
        return QStringLiteral("featureMode");
    }

    return QStringLiteral("unknown");
}

uint8_t ModeSelectPage::comboIndexToDroneReportMode(QComboBox *comboBox)
{
    return comboBox ? static_cast<uint8_t>(qBound(0, comboBox->currentIndex(), 4)) : 0;
}

int ModeSelectPage::droneReportModeToComboIndex(uint8_t mode)
{
    return mode <= 4 ? static_cast<int>(mode) : -1;
}

uint8_t ModeSelectPage::comboIndexToJamMode(QComboBox *comboBox)
{
    return comboBox ? static_cast<uint8_t>(qBound(0, comboBox->currentIndex(), 1)) : 0;
}

int ModeSelectPage::jamModeToComboIndex(uint8_t mode)
{
    return mode <= 1 ? static_cast<int>(mode) : -1;
}

uint8_t ModeSelectPage::comboIndexToNetworkMode(QComboBox *comboBox)
{
    return comboBox ? static_cast<uint8_t>(qBound(0, comboBox->currentIndex(), 1)) : 0;
}

int ModeSelectPage::networkModeToComboIndex(uint8_t mode)
{
    return mode <= 1 ? static_cast<int>(mode) : -1;
}

uint8_t ModeSelectPage::comboIndexToUavCategoryDisplayMode(QComboBox *comboBox)
{
    return comboBox ? static_cast<uint8_t>(qBound(0, comboBox->currentIndex(), 1)) : 0;
}

int ModeSelectPage::uavCategoryDisplayModeToComboIndex(uint8_t mode)
{
    return mode <= 1 ? static_cast<int>(mode) : -1;
}

uint8_t ModeSelectPage::checkBoxToDataEnable(QCheckBox *checkBox)
{
    return checkBox && checkBox->isChecked() ? 1 : 0;
}

uint8_t ModeSelectPage::comboIndexToFeatureFlag(QComboBox *comboBox)
{
    return comboBox ? static_cast<uint8_t>(qBound(0, comboBox->currentIndex(), 1)) : 0;
}

int ModeSelectPage::featureFlagToComboIndex(uint8_t enabled)
{
    return enabled <= 1 ? static_cast<int>(enabled) : -1;
}

QByteArray ModeSelectPage::currentFeatureModePayload() const
{
    QByteArray payload;
    payload.reserve(3);
    payload.append(static_cast<char>(comboIndexToFeatureFlag(wifiRemoteIdFeatureCombo)));
    payload.append(static_cast<char>(comboIndexToFeatureFlag(fpvFeatureCombo)));
    payload.append(static_cast<char>(comboIndexToFeatureFlag(djiParseFeatureCombo)));
    return payload;
}

void ModeSelectPage::handleSaveClicked()
{
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    const QString sectionKey = button ? button->property("sectionKey").toString() : QString();
    if (sectionKey == sectionKeyToString(SectionKey::DroneReportMode))
    {
        startPendingCommand(SectionKey::DroneReportMode,
                            QByteArray(1, static_cast<char>(comboIndexToDroneReportMode(droneReportModeCombo))), button,
                            droneReportModePendingTimer);
        emit requestSaveDroneReportMode(
            static_cast<uint8_t>(static_cast<unsigned char>(droneReportModePendingCommand.targetPayload.at(0))));
        return;
    }
    if (sectionKey == sectionKeyToString(SectionKey::JamMode))
    {
        startPendingCommand(SectionKey::JamMode, QByteArray(1, static_cast<char>(comboIndexToJamMode(jamModeCombo))),
                            button, jamModePendingTimer);
        emit requestSaveJamMode(static_cast<uint8_t>(static_cast<unsigned char>(jamModePendingCommand.targetPayload.at(0))));
        return;
    }
    if (sectionKey == sectionKeyToString(SectionKey::NetworkMode))
    {
        startPendingCommand(SectionKey::NetworkMode,
                            QByteArray(1, static_cast<char>(comboIndexToNetworkMode(networkModeCombo))), button,
                            networkModePendingTimer);
        emit requestSaveNetworkMode(
            static_cast<uint8_t>(static_cast<unsigned char>(networkModePendingCommand.targetPayload.at(0))));
        return;
    }
    if (sectionKey == sectionKeyToString(SectionKey::UavCategoryDisplayMode))
    {
        startPendingCommand(SectionKey::UavCategoryDisplayMode,
                            QByteArray(1, static_cast<char>(comboIndexToUavCategoryDisplayMode(uavCategoryDisplayCombo))),
                            button, uavCategoryDisplayModePendingTimer);
        emit requestSaveUavCategoryDisplayMode(
            static_cast<uint8_t>(static_cast<unsigned char>(uavCategoryDisplayModePendingCommand.targetPayload.at(0))));
        return;
    }
    if (sectionKey == QStringLiteral("dataEnable"))
    {
        startPendingCommand(SectionKey::DataEnable, QByteArray(1, static_cast<char>(checkBoxToDataEnable(dataEnableCheckBox))),
                            button, dataEnablePendingTimer);
        emit requestSaveDataEnable(static_cast<uint8_t>(static_cast<unsigned char>(dataEnablePendingCommand.targetPayload.at(0))));
        return;
    }
    if (sectionKey == sectionKeyToString(SectionKey::FeatureMode))
    {
        startPendingCommand(SectionKey::FeatureMode, currentFeatureModePayload(), button, featureModePendingTimer);
        emit requestSaveFeatureModes(
            static_cast<uint8_t>(static_cast<unsigned char>(featureModePendingCommand.targetPayload.at(0))),
            static_cast<uint8_t>(static_cast<unsigned char>(featureModePendingCommand.targetPayload.at(1))),
            static_cast<uint8_t>(static_cast<unsigned char>(featureModePendingCommand.targetPayload.at(2))));
        return;
    }

    showToastResult(false, QStringLiteral("协议待接入"));
}
