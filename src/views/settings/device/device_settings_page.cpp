#include "device_settings_page.h"
#include "map_picker_dialog.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLocale>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
QString lineEditStyle(bool enabled)
{
    if (enabled)
    {
        return "QLineEdit { background-color: #1e1e1e; color: #fff; border: 1px solid #444; border-radius: 3px; "
               "padding-left: 10px; }";
    }

    return "QLineEdit { background-color: #333333; color: #888888; border: 1px solid #444; border-radius: 3px; "
           "padding-left: 10px; }";
}

QString spinBoxStyle(bool enabled)
{
    if (enabled)
    {
        return "QDoubleSpinBox { background: transparent; color: #fff; border: none; padding-left: 10px; padding-right: 4px; }";
    }

    return "QDoubleSpinBox { background: transparent; color: #888888; border: none; padding-left: 10px; padding-right: 4px; }";
}

QString spinBoxContainerStyle(bool enabled)
{
    if (enabled)
    {
        return "background-color: #1e1e1e; border: 1px solid #444; border-radius: 3px;";
    }

    return "background-color: #333333; border: 1px solid #444; border-radius: 3px;";
}

QString spinButtonPanelStyle(bool enabled)
{
    if (enabled)
    {
        return "background-color: #232323; border-left: 1px solid #3a3a3a; border-top-right-radius: 3px; "
               "border-bottom-right-radius: 3px;";
    }

    return "background-color: #2c2c2c; border-left: 1px solid #3a3a3a; border-top-right-radius: 3px; "
           "border-bottom-right-radius: 3px;";
}

QString spinArrowButtonStyle()
{
    return "QToolButton { background: transparent; border: none; padding: 0; }"
           "QToolButton:hover { background-color: rgba(255, 255, 255, 0.04); }"
           "QToolButton:pressed { background-color: rgba(0, 0, 0, 0.18); }";
}

QString actionButtonStyle(bool enabled)
{
    if (enabled)
    {
        return "QPushButton { background-color: #ffffff; color: black; border: none; border-radius: 2px; font-weight: "
               "bold; font-size: 13px; }"
               "QPushButton:hover { background-color: #e0e0e0; }";
    }

    return "QPushButton { background-color: #555555; color: #aaaaaa; border: none; border-radius: 2px; font-weight: "
           "bold; font-size: 13px; }";
}

QString locationModeComboStyle()
{
    return "QComboBox { "
           "    background-color: #1e1e1e; "
           "    color: #fff; "
           "    border: 1px solid #444; "
           "    border-radius: 3px; "
           "    padding-left: 10px; "
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

QString formLabelStyle()
{
    return "color: #cccccc; font-size: 13px;";
}

bool isNumericString(const QString &value)
{
    bool ok = false;
    QLocale::c().toDouble(value, &ok);
    return ok;
}

int inferDecimals(const QString &value)
{
    const int dotIndex = value.indexOf('.');
    if (dotIndex < 0)
    {
        return 0;
    }

    return qMin(6, value.size() - dotIndex - 1);
}
} // namespace

DeviceSettingsPage::DeviceSettingsPage(SettingsUserRole role, QWidget *parent)
    : QWidget(parent), lngInputContainer(nullptr), latInputContainer(nullptr), altInputContainer(nullptr),
      scanSaveBtn(nullptr), rootAdvancedFrame(nullptr), mapPickerDialog(nullptr), userRole(role), scanSsthInput(nullptr),
      scanSsJgMaxInput(nullptr), scanSsJgMinInput(nullptr), scanSsMaxInput(nullptr), scanSsMinInput(nullptr),
      scanAttInput(nullptr), deviceIpInput(nullptr), devicePortInput(nullptr), devicePortInputContainer(nullptr),
      deviceMaskInput(nullptr), deviceRouteInput(nullptr), deviceDnsInput(nullptr), remoteIpInput(nullptr),
      remotePortInput(nullptr), remotePortInputContainer(nullptr), toastWidget(nullptr), toastIconLabel(nullptr),
      toastTextLabel(nullptr), toastHideTimer(nullptr), toastOpacityEffect(nullptr), toastFadeInAnimation(nullptr),
      toastFadeOutAnimation(nullptr)
{
    setupUi();
}

// ===== UI 搭建开始：页面结构、控件创建、布局与样式 =====
void DeviceSettingsPage::setupUi()
{
    QVBoxLayout *pageLayout = nullptr;
    setupScrollablePage(pageLayout);
    setupPageTitle(pageLayout);
    setupCommonSettingsSection(pageLayout);
    setupRootAdvancedSection(pageLayout);
    bindSignals();
    applyRolePermissions();
}

void DeviceSettingsPage::setupScrollablePage(QVBoxLayout *&pageLayout)
{
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

    QWidget *scrollContent = new QWidget(scrollArea);
    scrollArea->setWidget(scrollContent);

    pageLayout = new QVBoxLayout(scrollContent);
    pageLayout->setContentsMargins(40, 30, 20, 30);
    pageLayout->setSpacing(20);
    pageLayout->setAlignment(Qt::AlignTop);
}

void DeviceSettingsPage::setupPageTitle(QVBoxLayout *pageLayout)
{
    QLabel *pageTitle = new QLabel("经纬度设置", this);
    pageTitle->setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold; padding-left: 20px;");
    pageLayout->addWidget(pageTitle);
}

void DeviceSettingsPage::setupCommonSettingsSection(QVBoxLayout *pageLayout)
{
    QFrame *formFrame = new QFrame(this);
    formFrame->setStyleSheet("QFrame { background-color: #2b2b2b; border-radius: 6px; }");

    QVBoxLayout *formLayout = new QVBoxLayout(formFrame);
    formLayout->setContentsMargins(20, 20, 20, 20);
    formLayout->setSpacing(15);

    setupModuleModeDisplayRow(formFrame, formLayout);
    formLayout->addWidget(createSeparatorLine(formFrame));
    setupLocationModeRow(formFrame, formLayout);
    formLayout->addWidget(createSeparatorLine(formFrame));
    setupCoordinateRows(formFrame, formLayout);
    setupDeviceActionButtons(formFrame, formLayout);

    pageLayout->addWidget(formFrame);
}

void DeviceSettingsPage::setupModuleModeDisplayRow(QFrame *formFrame, QVBoxLayout *formLayout)
{
    QHBoxLayout *rowLayout = new QHBoxLayout();
    QLabel *nameLabel = new QLabel("定位模块工作模式", formFrame);
    nameLabel->setStyleSheet("color: #aaaaaa; font-size: 13px;");

    QLabel *valueLabel = new QLabel("北斗 + GPS + 格洛纳斯", formFrame);
    valueLabel->setStyleSheet("color: #aaaaaa; font-size: 13px;");
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    rowLayout->addWidget(nameLabel);
    rowLayout->addWidget(valueLabel);
    formLayout->addLayout(rowLayout);
}

QFrame *DeviceSettingsPage::createSeparatorLine(QFrame *parent)
{
    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #444; max-height: 1px;");
    return line;
}

QHBoxLayout *DeviceSettingsPage::createFormRow(QFrame *parent, const QString &labelText, QWidget *inputWidget,
                                               const QString &suffix)
{
    QHBoxLayout *row = new QHBoxLayout();
    QLabel *label = new QLabel("<font color='red'>*</font> " + labelText, parent);
    label->setStyleSheet(formLabelStyle());
    label->setFixedWidth(150);

    row->addWidget(label);
    row->addStretch();

    inputWidget->setFixedWidth(200);
    inputWidget->setFixedHeight(32);
    row->addWidget(inputWidget);

    if (!suffix.isEmpty())
    {
        QLabel *suffixLabel = new QLabel(suffix, parent);
        suffixLabel->setStyleSheet("color: #cccccc; font-size: 13px; margin-left: 10px;");
        row->addWidget(suffixLabel);
    }

    return row;
}

void DeviceSettingsPage::addRootInputRow(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &labelText,
                                         QWidget *inputWidget, const QString &suffix, const QString &noteText)
{
    QFrame *rowFrame = new QFrame(sectionFrame);
    rowFrame->setStyleSheet("QFrame { background-color: transparent; border-radius: 0px; }");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowFrame);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel("<font color='red'>*</font> " + labelText, rowFrame);
    label->setStyleSheet(formLabelStyle());
    label->setFixedWidth(150);
    rowLayout->addWidget(label);
    rowLayout->addStretch();

    QVBoxLayout *inputArea = new QVBoxLayout();
    inputArea->setContentsMargins(0, 0, 0, 0);
    inputArea->setSpacing(4);

    QHBoxLayout *inputRow = new QHBoxLayout();
    inputRow->setContentsMargins(0, 0, 0, 0);
    inputRow->setSpacing(10);
    inputRow->addStretch();
    inputRow->addWidget(inputWidget);

    if (!suffix.isEmpty())
    {
        QLabel *suffixLabel = new QLabel(suffix, rowFrame);
        suffixLabel->setStyleSheet("color: #cccccc; font-size: 12px;");
        inputRow->addWidget(suffixLabel);
    }

    inputArea->addLayout(inputRow);

    if (!noteText.isEmpty())
    {
        QLabel *noteLabel = new QLabel(noteText, rowFrame);
        noteLabel->setStyleSheet("color: #8a8a8a; font-size: 11px;");
        noteLabel->setFixedWidth(200);

        QHBoxLayout *noteRow = new QHBoxLayout();
        noteRow->setContentsMargins(0, 0, 0, 0);
        noteRow->setSpacing(0);
        noteRow->addStretch();
        noteRow->addWidget(noteLabel);

        inputArea->addLayout(noteRow);
    }

    rowLayout->addLayout(inputArea);
    sectionLayout->addWidget(rowFrame);
    sectionLayout->addWidget(createSeparatorLine(sectionFrame));
}

QDoubleSpinBox *DeviceSettingsPage::createNumericSpinBox(double defaultValue, int decimals, QFrame *parent)
{
    QDoubleSpinBox *input = new QDoubleSpinBox(parent);
    input->setLocale(QLocale::c());
    input->setButtonSymbols(QAbstractSpinBox::NoButtons);
    input->setDecimals(decimals);
    input->setRange(-9999999.0, 9999999.0);
    input->setSingleStep(1.0);
    input->setValue(defaultValue);
    input->setStyleSheet(spinBoxStyle(true));
    return input;
}

QWidget *DeviceSettingsPage::createSpinBoxContainer(QDoubleSpinBox *spinBox, QWidget *parent)
{
    QWidget *container = new QWidget(parent);
    container->setFixedWidth(200);
    container->setFixedHeight(32);
    container->setStyleSheet(spinBoxContainerStyle(true));
    QHBoxLayout *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    spinBox->setParent(container);
    layout->addWidget(spinBox, 1);

    QWidget *buttonPanel = new QWidget(container);
    buttonPanel->setFixedWidth(18);
    buttonPanel->setStyleSheet(spinButtonPanelStyle(true));

    QVBoxLayout *buttonLayout = new QVBoxLayout(buttonPanel);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(0);

    const QString upArrowPath = QCoreApplication::applicationDirPath() + "/assets/web/images/spin_up.svg";
    const QString downArrowPath = QCoreApplication::applicationDirPath() + "/assets/web/images/spin_down.svg";

    QToolButton *upBtn = new QToolButton(buttonPanel);
    upBtn->setIcon(QIcon(upArrowPath));
    upBtn->setIconSize(QSize(8, 5));
    upBtn->setFixedHeight(16);
    upBtn->setStyleSheet(spinArrowButtonStyle() + "QToolButton { border-bottom: 1px solid #3a3a3a; }");

    QToolButton *downBtn = new QToolButton(buttonPanel);
    downBtn->setIcon(QIcon(downArrowPath));
    downBtn->setIconSize(QSize(8, 5));
    downBtn->setFixedHeight(16);
    downBtn->setStyleSheet(spinArrowButtonStyle());

    connect(upBtn, &QToolButton::clicked, spinBox, QOverload<>::of(&QDoubleSpinBox::stepUp));
    connect(downBtn, &QToolButton::clicked, spinBox, QOverload<>::of(&QDoubleSpinBox::stepDown));

    buttonLayout->addWidget(upBtn);
    buttonLayout->addWidget(downBtn);
    layout->addWidget(buttonPanel);

    return container;
}

QPushButton *DeviceSettingsPage::createActionButton(const QString &text, QFrame *parent)
{
    QPushButton *button = new QPushButton(text, parent);
    button->setFixedSize(120, 36);
    button->setStyleSheet(btnStyle);
    return button;
}

void DeviceSettingsPage::setupLocationModeRow(QFrame *formFrame, QVBoxLayout *formLayout)
{
    locationModeCombo = new QComboBox(formFrame);
    locationModeCombo->addItems({"手动", "自动"});
    locationModeCombo->setStyleSheet(locationModeComboStyle());

    QLabel *arrowLabel = new QLabel("▼", locationModeCombo);
    arrowLabel->setStyleSheet("color: #888; font-size: 10px; background: transparent;");
    arrowLabel->setAlignment(Qt::AlignCenter);
    arrowLabel->setGeometry(170, 0, 30, 32);

    formLayout->addLayout(createFormRow(formFrame, "定位模式", locationModeCombo));
}

void DeviceSettingsPage::setupCoordinateRows(QFrame *formFrame, QVBoxLayout *formLayout)
{
    lngInput = createNumericSpinBox(120.08946, 6, formFrame);
    lngInputContainer = createSpinBoxContainer(lngInput, formFrame);
    formLayout->addLayout(createFormRow(formFrame, "设备所在经度", lngInputContainer));
    formLayout->addWidget(createSeparatorLine(formFrame));

    latInput = createNumericSpinBox(30.342264, 6, formFrame);
    latInputContainer = createSpinBoxContainer(latInput, formFrame);
    formLayout->addLayout(createFormRow(formFrame, "设备所在纬度", latInputContainer));
    formLayout->addWidget(createSeparatorLine(formFrame));

    altInput = createNumericSpinBox(100.0, 0, formFrame);
    altInputContainer = createSpinBoxContainer(altInput, formFrame);
    formLayout->addLayout(createFormRow(formFrame, "设备所在海拔高度", altInputContainer, "米"));
}

void DeviceSettingsPage::setupDeviceActionButtons(QFrame *formFrame, QVBoxLayout *formLayout)
{
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 15, 0, 0);

    btnStyle = actionButtonStyle(true);
    saveBtn = createActionButton("保存", formFrame);
    pickMapBtn = createActionButton("地图选点", formFrame);
    lockPosBtn = createActionButton("锁定位置", formFrame);

    buttonLayout->addWidget(saveBtn);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(pickMapBtn);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(lockPosBtn);
    buttonLayout->addStretch();

    formLayout->addLayout(buttonLayout);
}

void DeviceSettingsPage::setupRootAdvancedSection(QVBoxLayout *pageLayout)
{
    rootAdvancedFrame = new QFrame(this);
    rootAdvancedFrame->setStyleSheet("QFrame { background-color: transparent; border-radius: 6px; }");

    QVBoxLayout *advancedLayout = new QVBoxLayout(rootAdvancedFrame);
    advancedLayout->setContentsMargins(0, 0, 0, 0);
    advancedLayout->setSpacing(20);

    setupScanSettingsSection(advancedLayout);
    setupDeviceIpSection(advancedLayout);
    setupRemotePlatformSection(advancedLayout);

    pageLayout->addWidget(rootAdvancedFrame);
}

QVBoxLayout *DeviceSettingsPage::createRootSectionFrame(const QString &title, QVBoxLayout *advancedLayout)
{
    QFrame *sectionFrame = new QFrame(rootAdvancedFrame);
    sectionFrame->setStyleSheet("QFrame { background-color: #2b2b2b; border-radius: 6px; }");

    QVBoxLayout *sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setContentsMargins(20, 20, 20, 20);
    sectionLayout->setSpacing(15);

    QLabel *sectionTitle = new QLabel(title, sectionFrame);
    sectionTitle->setStyleSheet("color: #ffffff; font-size: 15px; font-weight: bold; padding: 4px 0 10px 0;");
    sectionLayout->addWidget(sectionTitle);

    advancedLayout->addWidget(sectionFrame);
    return sectionLayout;
}

QWidget *DeviceSettingsPage::createRootInput(const QString &defaultValue, QFrame *parent)
{
    if (isNumericString(defaultValue))
    {
        bool ok = false;
        const double value = QLocale::c().toDouble(defaultValue, &ok);
        if (ok)
        {
            QDoubleSpinBox *input = createNumericSpinBox(value, inferDecimals(defaultValue), parent);
            return createSpinBoxContainer(input, parent);
        }
    }

    QLineEdit *input = new QLineEdit(defaultValue, parent);
    input->setFixedWidth(200);
    input->setFixedHeight(32);
    input->setStyleSheet(lineEditStyle(true));
    return input;
}

void DeviceSettingsPage::addRootRow(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &labelText,
                                    const QString &value, const QString &suffix, const QString &noteText)
{
    QWidget *input = createRootInput(value, sectionFrame);
    addRootInputRow(sectionLayout, sectionFrame, labelText, input, suffix, noteText);
}

void DeviceSettingsPage::addSectionSaveButton(QVBoxLayout *sectionLayout, QFrame *sectionFrame)
{
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 15, 0, 0);
    buttonLayout->setSpacing(0);

    QPushButton *sectionSaveBtn = new QPushButton("保存", sectionFrame);
    sectionSaveBtn->setFixedSize(110, 32);
    sectionSaveBtn->setStyleSheet(actionButtonStyle(true));

    buttonLayout->addWidget(sectionSaveBtn);
    buttonLayout->addStretch();
    sectionLayout->addLayout(buttonLayout);
}

void DeviceSettingsPage::setupScanSettingsSection(QVBoxLayout *advancedLayout)
{
    QVBoxLayout *scanLayout = createRootSectionFrame("全频扫描设置", advancedLayout);
    QFrame *scanSectionFrame = qobject_cast<QFrame *>(scanLayout->parent());

    scanSsthInput = createNumericSpinBox(100.0, 0, scanSectionFrame);
    scanSsJgMaxInput = createNumericSpinBox(7.0, 0, scanSectionFrame);
    scanSsJgMinInput = createNumericSpinBox(3.0, 0, scanSectionFrame);
    scanSsMaxInput = createNumericSpinBox(180.0, 0, scanSectionFrame);
    scanSsMinInput = createNumericSpinBox(60.0, 0, scanSectionFrame);
    scanAttInput = createNumericSpinBox(40.0, 0, scanSectionFrame);

    addRootInputRow(scanLayout, scanSectionFrame, "信号灵敏比值门限",
                    createSpinBoxContainer(scanSsthInput, scanSectionFrame));
    addRootInputRow(scanLayout, scanSectionFrame, "信号带宽最大值（MHz）",
                    createSpinBoxContainer(scanSsJgMaxInput, scanSectionFrame));
    addRootInputRow(scanLayout, scanSectionFrame, "信号带宽最小值（MHz）",
                    createSpinBoxContainer(scanSsJgMinInput, scanSectionFrame));
    addRootInputRow(scanLayout, scanSectionFrame, "跳变比值最大值",
                    createSpinBoxContainer(scanSsMaxInput, scanSectionFrame));
    addRootInputRow(scanLayout, scanSectionFrame, "跳变比值最小值",
                    createSpinBoxContainer(scanSsMinInput, scanSectionFrame));
    addRootInputRow(scanLayout, scanSectionFrame, "信号增益", createSpinBoxContainer(scanAttInput, scanSectionFrame));

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 15, 0, 0);
    buttonLayout->setSpacing(0);

    scanSaveBtn = new QPushButton("保存", scanSectionFrame);
    scanSaveBtn->setFixedSize(110, 32);
    scanSaveBtn->setStyleSheet(actionButtonStyle(true));

    buttonLayout->addWidget(scanSaveBtn);
    buttonLayout->addStretch();
    scanLayout->addLayout(buttonLayout);
}

void DeviceSettingsPage::setupDeviceIpSection(QVBoxLayout *advancedLayout)
{
    QVBoxLayout *deviceIpLayout = createRootSectionFrame("设备IP设置", advancedLayout);
    QFrame *deviceIpSectionFrame = qobject_cast<QFrame *>(deviceIpLayout->parent());

    deviceIpInput = new QLineEdit("10.0.76.190", deviceIpSectionFrame);
    deviceIpInput->setFixedWidth(200);
    deviceIpInput->setFixedHeight(32);
    deviceIpInput->setStyleSheet(lineEditStyle(true));

    devicePortInput = createNumericSpinBox(5555.0, 0, deviceIpSectionFrame);
    devicePortInput->setRange(0, 65535);
    devicePortInputContainer = createSpinBoxContainer(devicePortInput, deviceIpSectionFrame);

    deviceMaskInput = new QLineEdit("255.255.255.0", deviceIpSectionFrame);
    deviceMaskInput->setFixedWidth(200);
    deviceMaskInput->setFixedHeight(32);
    deviceMaskInput->setStyleSheet(lineEditStyle(true));

    deviceRouteInput = new QLineEdit("10.0.76.1", deviceIpSectionFrame);
    deviceRouteInput->setFixedWidth(200);
    deviceRouteInput->setFixedHeight(32);
    deviceRouteInput->setStyleSheet(lineEditStyle(true));

    deviceDnsInput = new QLineEdit("114.114.114.114", deviceIpSectionFrame);
    deviceDnsInput->setFixedWidth(200);
    deviceDnsInput->setFixedHeight(32);
    deviceDnsInput->setStyleSheet(lineEditStyle(true));

    addRootInputRow(deviceIpLayout, deviceIpSectionFrame, "设备ip", deviceIpInput, QString(), "主口设备通信主参数");
    addRootInputRow(deviceIpLayout, deviceIpSectionFrame, "设备端口", devicePortInputContainer, QString(),
                    "1024至65535的整型数");
    addRootInputRow(deviceIpLayout, deviceIpSectionFrame, "设备网关", deviceRouteInput);
    addRootInputRow(deviceIpLayout, deviceIpSectionFrame, "设备掩码", deviceMaskInput);
    addRootInputRow(deviceIpLayout, deviceIpSectionFrame, "设备DNS", deviceDnsInput);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 15, 0, 0);
    buttonLayout->setSpacing(0);

    deviceIpSaveBtn = new QPushButton("保存", deviceIpSectionFrame);
    deviceIpSaveBtn->setFixedSize(110, 32);
    deviceIpSaveBtn->setStyleSheet(actionButtonStyle(true));

    buttonLayout->addWidget(deviceIpSaveBtn);
    buttonLayout->addStretch();
    deviceIpLayout->addLayout(buttonLayout);
}

void DeviceSettingsPage::setupRemotePlatformSection(QVBoxLayout *advancedLayout)
{
    QVBoxLayout *remoteLayout = createRootSectionFrame("远程平台设置", advancedLayout);
    QFrame *remoteSectionFrame = qobject_cast<QFrame *>(remoteLayout->parent());

    remoteIpInput = new QLineEdit("10.0.100.211", remoteSectionFrame);
    remoteIpInput->setFixedWidth(200);
    remoteIpInput->setFixedHeight(32);
    remoteIpInput->setStyleSheet(lineEditStyle(true));

    remotePortInput = createNumericSpinBox(9999.0, 0, remoteSectionFrame);
    remotePortInput->setRange(0, 65535);
    remotePortInputContainer = createSpinBoxContainer(remotePortInput, remoteSectionFrame);

    addRootInputRow(remoteLayout, remoteSectionFrame, "远程ip设置", remoteIpInput, QString(), "主口主下发");
    addRootInputRow(remoteLayout, remoteSectionFrame, "端口号", remotePortInputContainer);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 15, 0, 0);
    buttonLayout->setSpacing(0);

    remotePlatformSaveBtn = new QPushButton("保存", remoteSectionFrame);
    remotePlatformSaveBtn->setFixedSize(110, 32);
    remotePlatformSaveBtn->setStyleSheet(actionButtonStyle(true));

    buttonLayout->addWidget(remotePlatformSaveBtn);
    buttonLayout->addStretch();
    remoteLayout->addLayout(buttonLayout);
}

// ===== UI 搭建结束：下面开始是页面交互、权限控制和业务逻辑 =====
void DeviceSettingsPage::applyLocationModeState(bool isManual)
{
    lngInput->setEnabled(isManual);
    latInput->setEnabled(isManual);
    altInput->setEnabled(isManual);
    pickMapBtn->setEnabled(isManual);

    lngInput->setStyleSheet(spinBoxStyle(isManual));
    latInput->setStyleSheet(spinBoxStyle(isManual));
    altInput->setStyleSheet(spinBoxStyle(isManual));
    if (lngInputContainer)
    {
        lngInputContainer->setStyleSheet(spinBoxContainerStyle(isManual));
    }
    if (latInputContainer)
    {
        latInputContainer->setStyleSheet(spinBoxContainerStyle(isManual));
    }
    if (altInputContainer)
    {
        altInputContainer->setStyleSheet(spinBoxContainerStyle(isManual));
    }
    pickMapBtn->setStyleSheet(isManual ? btnStyle : actionButtonStyle(false));
}

void DeviceSettingsPage::applyRolePermissions()
{
    if (!rootAdvancedFrame)
    {
        return;
    }

    rootAdvancedFrame->setVisible(userRole == SettingsUserRole::Root);
}

void DeviceSettingsPage::bindSignals()
{
    connect(locationModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index)
            {
                applyLocationModeState(index == 0);
            });

    connect(pickMapBtn, &QPushButton::clicked, this, &DeviceSettingsPage::onPickMapClicked);

    connect(lockPosBtn, &QPushButton::clicked, this,
            [this]()
            {
                locationModeCombo->setCurrentIndex(0);
                saveBtn->click();
            });

    connect(saveBtn, &QPushButton::clicked, this,
            [this]()
            {
                uint8_t mode = locationModeCombo->currentIndex();
                float lng = static_cast<float>(lngInput->value());
                float lat = static_cast<float>(latInput->value());
                float alt = static_cast<float>(altInput->value());

                qDebug() << "[DeviceSettingsPage] 点击了保存按钮，准备下发 GPS 设置:";
                qDebug() << "  -> 模式:" << mode << " 经度:" << lng << " 纬度:" << lat << " 海拔:" << alt;

                emit requestSaveGps(mode, lng, lat, alt);
            });

    if (scanSaveBtn)
    {
        connect(scanSaveBtn, &QPushButton::clicked, this,
                [this]()
                {
                    emit requestSaveFullScan(scanSsthInput->value(), scanSsJgMaxInput->value(), scanSsJgMinInput->value(),
                                             scanSsMaxInput->value(), scanSsMinInput->value(), scanAttInput->value());
                });
    }

    if (deviceIpSaveBtn)
    {
        connect(deviceIpSaveBtn, &QPushButton::clicked, this,
                [this]()
                {
                    emit requestSaveDeviceIp(deviceIpInput->text().trimmed(), static_cast<int>(devicePortInput->value()),
                                             deviceMaskInput->text().trimmed(), deviceRouteInput->text().trimmed(),
                                             deviceDnsInput->text().trimmed());
                });
    }

    if (remotePlatformSaveBtn)
    {
        connect(remotePlatformSaveBtn, &QPushButton::clicked, this,
                [this]()
                {
                    emit requestSaveTcpServerIp(remoteIpInput->text().trimmed(),
                                                static_cast<int>(remotePortInput->value()));
                });
    }

    locationModeCombo->setCurrentIndex(0);
    applyLocationModeState(true);
}

void DeviceSettingsPage::setUserRole(SettingsUserRole role)
{
    userRole = role;
    applyRolePermissions();
}

void DeviceSettingsPage::updateGpsInfo(uint8_t mode, float lng, float lat, float alt)
{
    if (!lngInput->hasFocus() && !latInput->hasFocus() && !altInput->hasFocus())
    {
        locationModeCombo->setCurrentIndex(mode);
        lngInput->setValue(lng);
        latInput->setValue(lat);
        altInput->setValue(alt);
    }
}

void DeviceSettingsPage::updateFullScanSettings(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin,
                                                double att)
{
    if (!scanSsthInput || !scanSsJgMaxInput || !scanSsJgMinInput || !scanSsMaxInput || !scanSsMinInput || !scanAttInput)
    {
        return;
    }

    scanSsthInput->setValue(ssth);
    scanSsJgMaxInput->setValue(ssJgMax);
    scanSsJgMinInput->setValue(ssJgMin);
    scanSsMaxInput->setValue(ssMax);
    scanSsMinInput->setValue(ssMin);
    scanAttInput->setValue(att);
}

void DeviceSettingsPage::updateDeviceIpSettings(const QString &ip, int port, const QString &mask, const QString &route,
                                                const QString &dns)
{
    if (!deviceIpInput || !devicePortInput || !deviceMaskInput || !deviceRouteInput || !deviceDnsInput)
    {
        return;
    }

    deviceIpInput->setText(ip);
    devicePortInput->setValue(port);
    deviceMaskInput->setText(mask);
    deviceRouteInput->setText(route);
    deviceDnsInput->setText(dns);
}

void DeviceSettingsPage::updateTcpServerIpSettings(const QString &ip, int port)
{
    if (!remoteIpInput || !remotePortInput)
    {
        return;
    }

    remoteIpInput->setText(ip);
    remotePortInput->setValue(port);
}

void DeviceSettingsPage::showSaveResult(bool success, const QString &message)
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

void DeviceSettingsPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateToastPosition();
}

uint8_t DeviceSettingsPage::currentLocationMode() const
{
    return locationModeCombo->currentIndex() == 0 ? 0 : 1;
}

void DeviceSettingsPage::onPickMapClicked()
{
    float currentLng = static_cast<float>(lngInput->value());
    float currentLat = static_cast<float>(latInput->value());

    if (!mapPickerDialog)
    {
        mapPickerDialog = new MapPickerDialog(window());
        connect(mapPickerDialog, &MapPickerDialog::locationConfirmed, this,
                [this](float newLng, float newLat)
                {
                    locationModeCombo->setCurrentIndex(0);
                    lngInput->setValue(newLng);
                    latInput->setValue(newLat);
                    qDebug() << "[DeviceSettingsPage] 地图选点完成:" << newLng << "," << newLat << "并自动切为手动模式";
                });
    }

    mapPickerDialog->setInitialLocation(currentLng, currentLat);
    mapPickerDialog->showOverlay();
}

void DeviceSettingsPage::ensureToastWidget()
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

void DeviceSettingsPage::updateToastPosition()
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

QString DeviceSettingsPage::extractDisplayMessage(bool success, const QString &message) const
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
