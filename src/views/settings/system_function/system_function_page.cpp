#include "system_function_page.h"
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QCalendarWidget>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QEvent>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QLocale>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRadioButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
QString requiredLabelText(const QString &text)
{
    return QStringLiteral("<font color='#ff6a55'>*</font> ") + text;
}

QString normalizeDeviceTimestamp(const QString &timestamp)
{
    const QString trimmed = timestamp.trimmed();
    if (trimmed.isEmpty())
    {
        return trimmed;
    }

    const QDateTime parsed = QDateTime::fromString(trimmed, QStringLiteral("yyyy/MM/dd HH:mm:ss:zzz"));
    if (parsed.isValid())
    {
        return parsed.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }

    QString text = trimmed;
    const int lastColon = text.lastIndexOf(':');
    if (lastColon > text.indexOf(' '))
    {
        bool ok = false;
        text.mid(lastColon + 1).toInt(&ok);
        if (ok)
        {
            text = text.left(lastColon);
        }
    }

    return text.replace('/', '-');
}
} // namespace

SystemFunctionPage::SystemFunctionPage(QWidget *parent)
    : QWidget(parent), alarmVoiceToggle(nullptr), screenFlashToggle(nullptr), alarmSaveButton(nullptr),
      rebootButton(nullptr), currentTimeValueLabel(nullptr), timezoneComboBox(nullptr), setTimeEdit(nullptr),
      syncTimeToggle(nullptr), timeSaveButton(nullptr), warningRemoveTimeEdit(nullptr), mapTypeComboBox(nullptr),
      paramSaveButton(nullptr), diskSpaceValueLabel(nullptr), uploadButton(nullptr), clearButton(nullptr),
      changePasswordFrame(nullptr), normalAdminPasswordRadio(nullptr), advancedAdminPasswordRadio(nullptr),
      oldPasswordEdit(nullptr), newPasswordEdit(nullptr), confirmPasswordEdit(nullptr), changePasswordSaveButton(nullptr),
      toastWidget(nullptr), toastIconLabel(nullptr), toastTextLabel(nullptr), toastHideTimer(nullptr),
      toastOpacityEffect(nullptr), toastFadeInAnimation(nullptr), toastFadeOutAnimation(nullptr),
      rebootConfirmOverlay(nullptr), rebootConfirmPanel(nullptr), rebootConfirmIconLabel(nullptr),
      rebootConfirmTitleLabel(nullptr), rebootConfirmMessageLabel(nullptr), rebootConfirmCancelButton(nullptr),
      rebootConfirmOkButton(nullptr), timePickerPopup(nullptr), timePickerCalendar(nullptr),
      timePickerHeaderLabel(nullptr), timePickerPrevYearButton(nullptr), timePickerPrevMonthButton(nullptr),
      timePickerNextMonthButton(nullptr), timePickerNextYearButton(nullptr),
      timePickerHourList(nullptr), timePickerMinuteList(nullptr), timePickerSecondList(nullptr),
      timePickerNowButton(nullptr), timePickerConfirmButton(nullptr),
      currentUserRole(SettingsUserRole::None)
{
    setupUi();
    setUserRole(SettingsUserRole::None);
}

void SystemFunctionPage::setUserRole(SettingsUserRole role)
{
    currentUserRole = role;
    if (changePasswordFrame)
    {
        changePasswordFrame->setVisible(currentUserRole == SettingsUserRole::Root);
    }
}

void SystemFunctionPage::setupUi()
{
    setObjectName("systemFunctionPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#systemFunctionPage { background-color: #202020; color: #ffffff; }");

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
    pageLayout->setContentsMargins(14, 10, 14, 18);
    pageLayout->setSpacing(14);
    pageLayout->setAlignment(Qt::AlignTop);

    QVBoxLayout *alarmLayout = nullptr;
    QFrame *alarmFrame = createSectionFrame(QStringLiteral("告警设置"), pageLayout, alarmLayout);
    alarmVoiceToggle = createStyledToggleSwitch(alarmFrame, false);
    alarmLayout->addWidget(createFormRow(alarmFrame, QStringLiteral("告警声音"), alarmVoiceToggle));
    alarmLayout->addWidget(createSeparatorLine(alarmFrame));
    screenFlashToggle = createStyledToggleSwitch(alarmFrame, false);
    alarmLayout->addWidget(createFormRow(alarmFrame, QStringLiteral("屏幕闪烁"), screenFlashToggle));
    alarmLayout->addWidget(createSeparatorLine(alarmFrame));
    QHBoxLayout *alarmButtonLayout = new QHBoxLayout();
    alarmButtonLayout->setContentsMargins(0, 16, 0, 0);
    alarmButtonLayout->setSpacing(0);
    alarmSaveButton = createPrimaryButton(alarmFrame, QStringLiteral("保存"));
    connect(alarmSaveButton, &QPushButton::clicked, this,
            [this]()
            {
                emit requestSetScreenFlashEnabled(screenFlashToggle && screenFlashToggle->isChecked());
                emit requestSaveBuzzerEnabled(alarmVoiceToggle->isChecked() ? 1 : 0);
            });
    alarmButtonLayout->addWidget(alarmSaveButton);
    alarmButtonLayout->addStretch();
    alarmLayout->addLayout(alarmButtonLayout);

    QVBoxLayout *operationLayout = nullptr;
    QFrame *operationFrame = createSectionFrame(QStringLiteral("系统操作"), pageLayout, operationLayout);
    QHBoxLayout *operationButtonLayout = new QHBoxLayout();
    operationButtonLayout->setContentsMargins(0, 0, 0, 0);
    operationButtonLayout->setSpacing(0);
    rebootButton = createPrimaryButton(operationFrame, QStringLiteral("设备重启"), 160);
    connect(rebootButton, &QPushButton::clicked, this,
            [this]()
            {
                showRebootConfirmOverlay();
            });
    operationButtonLayout->addWidget(rebootButton);
    operationButtonLayout->addStretch();
    operationLayout->addLayout(operationButtonLayout);

    QVBoxLayout *timeLayout = nullptr;
    QFrame *timeFrame = createSectionFrame(QStringLiteral("系统时间"), pageLayout, timeLayout);
    QLabel *tipLabel = new QLabel(QStringLiteral("设置后系统将重启"), timeFrame);
    tipLabel->setStyleSheet("background-color: #3f2a0a; color: #f0b24f; border: 1px solid #694205; border-radius: 6px; "
                            "padding: 8px 12px; font-size: 13px;");
    timeLayout->addWidget(tipLabel);
    currentTimeValueLabel = new QLabel(QStringLiteral("2026-05-22 09:34:31"), timeFrame);
    currentTimeValueLabel->setStyleSheet(readOnlyValueStyle());
    timeLayout->addWidget(createReadOnlyRow(timeFrame, QStringLiteral("系统时间"), currentTimeValueLabel));
    timeLayout->addWidget(createSeparatorLine(timeFrame));
    timezoneComboBox = createStyledComboBox(timeFrame, QStringList() << QStringLiteral("Asia/Shanghai"), 160,
                                            QStringLiteral("Asia/Shanghai"));
    timeLayout->addWidget(createFormRow(timeFrame, QStringLiteral("时区"), timezoneComboBox));
    timeLayout->addWidget(createSeparatorLine(timeFrame));
    setTimeEdit = createStyledDateTimeEdit(timeFrame);
    timeLayout->addWidget(createFormRow(timeFrame, QStringLiteral("设置时间"), setTimeEdit));
    timeLayout->addWidget(createSeparatorLine(timeFrame));
    syncTimeToggle = createStyledToggleSwitch(timeFrame, false);
    connect(syncTimeToggle, &QCheckBox::toggled, this,
            [this](bool checked)
            {
                if (!setTimeEdit || !timezoneComboBox)
                {
                    return;
                }

                if (checked)
                {
                    hideTimePickerPopup();
                }

                setTimeEdit->setEnabled(!checked);
                timezoneComboBox->setEnabled(!checked);
                setTimeEdit->setStyleSheet(
                    checked
                        ? "QDateTimeEdit { background-color: #1a1b1d; color: #7d828a; border: 1px solid #2d2d2d; "
                          "border-radius: 2px; padding: 0 10px; font-size: 14px; }"
                          "QDateTimeEdit::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; "
                          "border-left: 1px solid #2a2a2a; }"
                          "QDateTimeEdit::down-arrow { image: none; }"
                        : "QDateTimeEdit { background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; "
                          "border-radius: 2px; padding: 0 10px; font-size: 14px; }"
                          "QDateTimeEdit::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; "
                          "border-left: 1px solid #2a2a2a; }"
                          "QDateTimeEdit::down-arrow { image: none; }");
                timezoneComboBox->setStyleSheet(
                    checked
                        ? "QComboBox { background-color: #1a1b1d; color: #7d828a; border: 1px solid #2d2d2d; "
                          "border-radius: 2px; padding-left: 10px; padding-right: 28px; font-size: 14px; }"
                          "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; "
                          "border-left: 1px solid #2a2a2a; }"
                          "QComboBox::down-arrow { image: none; }"
                          "QComboBox QAbstractItemView { background-color: #202225; color: #ffffff; "
                          "selection-background-color: #3a3a3a; border: 1px solid #2e2e2e; }"
                        : "QComboBox { background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; "
                          "border-radius: 2px; padding-left: 10px; padding-right: 28px; font-size: 14px; }"
                          "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; "
                          "border-left: 1px solid #2a2a2a; }"
                          "QComboBox::down-arrow { image: none; }"
                          "QComboBox QAbstractItemView { background-color: #202225; color: #ffffff; "
                          "selection-background-color: #3a3a3a; border: 1px solid #2e2e2e; }");
            });
    timeLayout->addWidget(createFormRow(timeFrame, QStringLiteral("与计算机时间同步"), syncTimeToggle));
    timeLayout->addWidget(createSeparatorLine(timeFrame));
    QHBoxLayout *timeButtonLayout = new QHBoxLayout();
    timeButtonLayout->setContentsMargins(0, 16, 0, 0);
    timeButtonLayout->setSpacing(0);
    timeSaveButton = createPrimaryButton(timeFrame, QStringLiteral("保存"));
    connect(timeSaveButton, &QPushButton::clicked, this,
            [this]()
            {
                if (!setTimeEdit)
                {
                    return;
                }

                QDateTime targetDateTime = setTimeEdit->dateTime();
                if (syncTimeToggle && syncTimeToggle->isChecked())
                {
                    targetDateTime = QDateTime::currentDateTime();
                    setTimeEdit->setDateTime(targetDateTime);
                }

                emit requestSaveSystemTime(targetDateTime);
            });
    timeButtonLayout->addWidget(timeSaveButton);
    timeButtonLayout->addStretch();
    timeLayout->addLayout(timeButtonLayout);

    QVBoxLayout *paramLayout = nullptr;
    QFrame *paramFrame = createSectionFrame(QStringLiteral("参数设置"), pageLayout, paramLayout);
    warningRemoveTimeEdit = createStyledLineEdit(paramFrame, QStringLiteral("20"), 150);
    paramLayout->addWidget(createFormRow(paramFrame, QStringLiteral("预警消除时间(秒)"), warningRemoveTimeEdit, true));
    paramLayout->addWidget(createSeparatorLine(paramFrame));
    mapTypeComboBox =
        createStyledComboBox(paramFrame, QStringList() << QStringLiteral("天地图"), 150, QStringLiteral("天地图"));
    paramLayout->addWidget(createFormRow(paramFrame, QStringLiteral("地图类型"), mapTypeComboBox));
    paramLayout->addWidget(createNoteRow(paramFrame, QStringLiteral("改变地图类型将会刷新页面")));
    QHBoxLayout *paramButtonLayout = new QHBoxLayout();
    paramButtonLayout->setContentsMargins(0, 16, 0, 0);
    paramButtonLayout->setSpacing(0);
    paramSaveButton = createPrimaryButton(paramFrame, QStringLiteral("保存"));
    paramButtonLayout->addWidget(paramSaveButton);
    paramButtonLayout->addStretch();
    paramLayout->addLayout(paramButtonLayout);

    QVBoxLayout *offlineLayout = nullptr;
    QFrame *offlineFrame = createSectionFrame(QStringLiteral("离线地图上传"), pageLayout, offlineLayout);
    diskSpaceValueLabel = new QLabel(QStringLiteral("908.12GB/937.71GB"), offlineFrame);
    diskSpaceValueLabel->setStyleSheet(readOnlyValueStyle());
    offlineLayout->addWidget(createReadOnlyRow(offlineFrame, QStringLiteral("磁盘空间（剩余/总）"), diskSpaceValueLabel->text()));
    offlineLayout->addWidget(createSeparatorLine(offlineFrame));
    QHBoxLayout *offlineButtonLayout = new QHBoxLayout();
    offlineButtonLayout->setContentsMargins(0, 16, 0, 0);
    offlineButtonLayout->setSpacing(16);
    uploadButton = createPrimaryButton(offlineFrame, QStringLiteral("上传"), 120);
    clearButton = createPrimaryButton(offlineFrame, QStringLiteral("清空"), 120);
    offlineButtonLayout->addWidget(uploadButton);
    offlineButtonLayout->addWidget(clearButton);
    offlineButtonLayout->addStretch();
    offlineLayout->addLayout(offlineButtonLayout);

    QVBoxLayout *passwordLayout = nullptr;
    changePasswordFrame = createSectionFrame(QStringLiteral("修改密码"), pageLayout, passwordLayout);

    QWidget *passwordTypeWidget = new QWidget(changePasswordFrame);
    passwordTypeWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *passwordTypeLayout = new QHBoxLayout(passwordTypeWidget);
    passwordTypeLayout->setContentsMargins(0, 0, 0, 0);
    passwordTypeLayout->setSpacing(14);

    normalAdminPasswordRadio = new QRadioButton(QStringLiteral("普通管理员密码"), passwordTypeWidget);
    normalAdminPasswordRadio->setChecked(true);
    normalAdminPasswordRadio->setStyleSheet("QRadioButton { color: #f0f0f0; font-size: 14px; }"
                                            "QRadioButton::indicator { width: 12px; height: 12px; }");
    passwordTypeLayout->addWidget(normalAdminPasswordRadio);

    advancedAdminPasswordRadio = new QRadioButton(QStringLiteral("高级管理员密码"), passwordTypeWidget);
    advancedAdminPasswordRadio->setStyleSheet("QRadioButton { color: #f0f0f0; font-size: 14px; }"
                                              "QRadioButton::indicator { width: 12px; height: 12px; }");
    passwordTypeLayout->addWidget(advancedAdminPasswordRadio);

    passwordLayout->addWidget(createFormRow(changePasswordFrame, QStringLiteral("密码类型"), passwordTypeWidget));
    passwordLayout->addWidget(createSeparatorLine(changePasswordFrame));

    oldPasswordEdit = createStyledPasswordEdit(changePasswordFrame, QStringLiteral("请输入旧密码"), 182);
    passwordLayout->addWidget(createFormRow(changePasswordFrame, QStringLiteral("旧密码"), oldPasswordEdit, true));
    passwordLayout->addWidget(createSeparatorLine(changePasswordFrame));

    newPasswordEdit = createStyledPasswordEdit(changePasswordFrame, QStringLiteral("请输入新密码"), 182);
    passwordLayout->addWidget(createFormRow(changePasswordFrame, QStringLiteral("新密码"), newPasswordEdit, true));
    passwordLayout->addWidget(createSeparatorLine(changePasswordFrame));

    confirmPasswordEdit = createStyledPasswordEdit(changePasswordFrame, QStringLiteral("请再次输入新密码"), 182);
    passwordLayout->addWidget(createFormRow(changePasswordFrame, QStringLiteral("确认新密码"), confirmPasswordEdit, true));
    passwordLayout->addWidget(createSeparatorLine(changePasswordFrame));

    QHBoxLayout *passwordButtonLayout = new QHBoxLayout();
    passwordButtonLayout->setContentsMargins(0, 16, 0, 0);
    passwordButtonLayout->setSpacing(0);
    changePasswordSaveButton = createPrimaryButton(changePasswordFrame, QStringLiteral("保存"));
    passwordButtonLayout->addWidget(changePasswordSaveButton);
    passwordButtonLayout->addStretch();
    passwordLayout->addLayout(passwordButtonLayout);
}

void SystemFunctionPage::updateBuzzerEnabled(uint8_t enabled)
{
    if (!alarmVoiceToggle)
    {
        return;
    }

    alarmVoiceToggle->setChecked(enabled == 1);
}

void SystemFunctionPage::updateDeviceReportedTime(const QString &timestamp)
{
    if (!currentTimeValueLabel)
    {
        return;
    }

    const QString normalized = normalizeDeviceTimestamp(timestamp);
    if (!normalized.isEmpty())
    {
        currentTimeValueLabel->setText(normalized);
    }
}

void SystemFunctionPage::showAlarmSaveResult(bool success, const QString &message)
{
    showToastResult(success, message);
}

void SystemFunctionPage::showSystemTimeSaveResult(bool success, const QString &message)
{
    if (success && currentTimeValueLabel && setTimeEdit)
    {
        currentTimeValueLabel->setText(setTimeEdit->dateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    }

    showToastResult(success, message);
}

void SystemFunctionPage::showRebootResult(bool success, const QString &message)
{
    showToastResult(success, success ? QStringLiteral("重启指令已下发") : message);
}

void SystemFunctionPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateRebootConfirmGeometry();
    updateTimePickerPopupPosition();
    updateToastPosition();
}

bool SystemFunctionPage::eventFilter(QObject *watched, QEvent *event)
{
    QLineEdit *timeEditLine = setTimeEdit ? setTimeEdit->findChild<QLineEdit *>() : nullptr;
    if (setTimeEdit && setTimeEdit->isEnabled() &&
        (watched == setTimeEdit || watched == timeEditLine) &&
        event->type() == QEvent::MouseButtonPress)
    {
        showTimePickerPopup();
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

QFrame *SystemFunctionPage::createSectionFrame(const QString &title, QVBoxLayout *pageLayout, QVBoxLayout *&sectionLayout)
{
    QFrame *sectionFrame = new QFrame(this);
    sectionFrame->setStyleSheet("QFrame { background-color: #2a2d33; border-radius: 0px; }");

    sectionLayout = new QVBoxLayout(sectionFrame);
    sectionLayout->setContentsMargins(0, 0, 0, 0);
    sectionLayout->setSpacing(0);

    QLabel *titleLabel = new QLabel(title, sectionFrame);
    titleLabel->setStyleSheet(sectionTitleStyle());
    sectionLayout->addWidget(titleLabel);
    pageLayout->addWidget(sectionFrame);
    return sectionFrame;
}

QWidget *SystemFunctionPage::createFormRow(QFrame *parent, const QString &labelText, QWidget *fieldWidget, bool required)
{
    QWidget *rowWidget = new QWidget(parent);
    rowWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(14, 16, 14, 16);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel(required ? requiredLabelText(labelText) : labelText, rowWidget);
    label->setTextFormat(Qt::RichText);
    label->setStyleSheet(formLabelStyle(required));
    label->setFixedWidth(220);
    rowLayout->addWidget(label);
    rowLayout->addStretch();
    rowLayout->addWidget(fieldWidget, 0, Qt::AlignRight | Qt::AlignVCenter);
    return rowWidget;
}

QWidget *SystemFunctionPage::createReadOnlyRow(QFrame *parent, const QString &labelText, const QString &valueText)
{
    QWidget *rowWidget = new QWidget(parent);
    rowWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(14, 16, 14, 16);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel(labelText, rowWidget);
    label->setStyleSheet(formLabelStyle(false));
    label->setFixedWidth(220);
    rowLayout->addWidget(label);
    rowLayout->addStretch();

    QLabel *valueLabel = new QLabel(valueText, rowWidget);
    valueLabel->setStyleSheet(readOnlyValueStyle());
    rowLayout->addWidget(valueLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    return rowWidget;
}

QWidget *SystemFunctionPage::createReadOnlyRow(QFrame *parent, const QString &labelText, QLabel *valueLabel)
{
    QWidget *rowWidget = new QWidget(parent);
    rowWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(14, 16, 14, 16);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel(labelText, rowWidget);
    label->setStyleSheet(formLabelStyle(false));
    label->setFixedWidth(220);
    rowLayout->addWidget(label);
    rowLayout->addStretch();

    if (valueLabel)
    {
        valueLabel->setParent(rowWidget);
        valueLabel->setStyleSheet(readOnlyValueStyle());
        rowLayout->addWidget(valueLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    }

    return rowWidget;
}

QWidget *SystemFunctionPage::createNoteRow(QFrame *parent, const QString &noteText) const
{
    QWidget *noteWidget = new QWidget(parent);
    noteWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *noteLayout = new QHBoxLayout(noteWidget);
    noteLayout->setContentsMargins(0, -4, 14, 10);
    noteLayout->setSpacing(0);
    noteLayout->addStretch();

    QLabel *noteLabel = new QLabel(noteText, noteWidget);
    noteLabel->setStyleSheet(noteLabelStyle());
    noteLayout->addWidget(noteLabel, 0, Qt::AlignRight | Qt::AlignVCenter);
    return noteWidget;
}

QFrame *SystemFunctionPage::createSeparatorLine(QFrame *parent) const
{
    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #3a3a3a; max-height: 1px;");
    return line;
}

QComboBox *SystemFunctionPage::createStyledComboBox(QFrame *parent, const QStringList &items, int width,
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

QLineEdit *SystemFunctionPage::createStyledLineEdit(QFrame *parent, const QString &text, int width) const
{
    QLineEdit *lineEdit = new QLineEdit(text, parent);
    lineEdit->setFixedSize(width, 32);
    lineEdit->setStyleSheet("QLineEdit { background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; "
                            "border-radius: 2px; padding: 0 10px; font-size: 14px; }");
    return lineEdit;
}

QLineEdit *SystemFunctionPage::createStyledPasswordEdit(QFrame *parent, const QString &placeholderText, int width) const
{
    QLineEdit *lineEdit = new QLineEdit(parent);
    lineEdit->setPlaceholderText(placeholderText);
    lineEdit->setEchoMode(QLineEdit::Password);
    lineEdit->setFixedSize(width, 32);
    lineEdit->setStyleSheet("QLineEdit { background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; "
                            "border-radius: 2px; padding: 0 10px; font-size: 14px; }"
                            "QLineEdit::placeholder { color: #666666; }");
    return lineEdit;
}

QDateTimeEdit *SystemFunctionPage::createStyledDateTimeEdit(QFrame *parent) const
{
    QDateTimeEdit *dateTimeEdit = new QDateTimeEdit(parent);
    dateTimeEdit->setLocale(QLocale(QLocale::Chinese, QLocale::China));
    dateTimeEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    dateTimeEdit->setDateTime(
        QDateTime::fromString(QStringLiteral("2026-05-22 09:32:42"), QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    dateTimeEdit->setCalendarPopup(false);
    dateTimeEdit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    dateTimeEdit->setKeyboardTracking(false);
    dateTimeEdit->setReadOnly(true);
    dateTimeEdit->setFixedSize(220, 32);
    dateTimeEdit->setStyleSheet("QDateTimeEdit { background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; "
                                "border-radius: 2px; padding: 0 10px; font-size: 14px; }"
                                "QDateTimeEdit::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; "
                                "border-left: 1px solid #2a2a2a; }"
                                "QDateTimeEdit::down-arrow { image: none; }"
                                "QDateTimeEdit:read-only { background-color: #101113; }");

    dateTimeEdit->installEventFilter(const_cast<SystemFunctionPage *>(this));
    if (QLineEdit *lineEdit = dateTimeEdit->findChild<QLineEdit *>())
    {
        lineEdit->setReadOnly(true);
        lineEdit->installEventFilter(const_cast<SystemFunctionPage *>(this));
    }

    return dateTimeEdit;
}

QCheckBox *SystemFunctionPage::createStyledToggleSwitch(QFrame *parent, bool checked) const
{
    QCheckBox *checkBox = new QCheckBox(parent);
    checkBox->setChecked(checked);
    checkBox->setCursor(Qt::PointingHandCursor);
    checkBox->setStyleSheet("QCheckBox { spacing: 0px; }"
                            "QCheckBox::indicator { width: 34px; height: 20px; border-radius: 10px; }"
                            "QCheckBox::indicator:unchecked { image: none; background-color: #6d6d6d; border: 1px solid #5a5a5a; }"
                            "QCheckBox::indicator:checked { image: none; background-color: #4caf50; border: 1px solid #449846; }");
    return checkBox;
}

QPushButton *SystemFunctionPage::createPrimaryButton(QFrame *parent, const QString &text, int width) const
{
    QPushButton *button = new QPushButton(text, parent);
    button->setFixedSize(width, 32);
    button->setStyleSheet("QPushButton { background-color: #f0f0f0; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #dfdfdf; }");
    return button;
}

void SystemFunctionPage::ensureRebootConfirmOverlay()
{
    if (rebootConfirmOverlay)
    {
        return;
    }

    rebootConfirmOverlay = new QWidget(this);
    rebootConfirmOverlay->setAttribute(Qt::WA_StyledBackground, true);
    rebootConfirmOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 120);");
    rebootConfirmOverlay->hide();

    QVBoxLayout *overlayLayout = new QVBoxLayout(rebootConfirmOverlay);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->setSpacing(0);
    overlayLayout->addStretch();

    rebootConfirmPanel = new QWidget(rebootConfirmOverlay);
    rebootConfirmPanel->setAttribute(Qt::WA_StyledBackground, true);
    rebootConfirmPanel->setStyleSheet("background-color: #1f1f22; border: 1px solid #34343a; border-radius: 8px;");
    rebootConfirmPanel->setFixedSize(360, 148);

    QVBoxLayout *panelLayout = new QVBoxLayout(rebootConfirmPanel);
    panelLayout->setContentsMargins(18, 16, 18, 16);
    panelLayout->setSpacing(14);

    QHBoxLayout *titleLayout = new QHBoxLayout();
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(10);

    rebootConfirmIconLabel = new QLabel(QStringLiteral("!"), rebootConfirmPanel);
    rebootConfirmIconLabel->setAlignment(Qt::AlignCenter);
    rebootConfirmIconLabel->setFixedSize(24, 24);
    rebootConfirmIconLabel->setStyleSheet("background-color: #d89b26; color: #1f1f22; border-radius: 12px; "
                                          "font-size: 16px; font-weight: bold;");

    rebootConfirmTitleLabel = new QLabel(QStringLiteral("设备重启"), rebootConfirmPanel);
    rebootConfirmTitleLabel->setStyleSheet("color: #ffffff; font-size: 18px; font-weight: bold;");

    titleLayout->addWidget(rebootConfirmIconLabel);
    titleLayout->addWidget(rebootConfirmTitleLabel);
    titleLayout->addStretch();
    panelLayout->addLayout(titleLayout);

    rebootConfirmMessageLabel = new QLabel(QStringLiteral("确定重启设备吗？"), rebootConfirmPanel);
    rebootConfirmMessageLabel->setStyleSheet("color: #f0f0f0; font-size: 15px; font-weight: bold;");
    panelLayout->addWidget(rebootConfirmMessageLabel);
    panelLayout->addStretch();

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(12);
    buttonLayout->addStretch();

    rebootConfirmCancelButton = new QPushButton(QStringLiteral("取消"), rebootConfirmPanel);
    rebootConfirmCancelButton->setFixedSize(88, 36);
    rebootConfirmCancelButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: #ffffff; border: 1px solid #5a5a5a; "
        "border-radius: 6px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #2b2b2f; }");

    rebootConfirmOkButton = new QPushButton(QStringLiteral("确定"), rebootConfirmPanel);
    rebootConfirmOkButton->setFixedSize(88, 36);
    rebootConfirmOkButton->setStyleSheet(
        "QPushButton { background-color: #e58b3e; color: #ffffff; border: none; border-radius: 6px; "
        "font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #f09a4f; }");

    buttonLayout->addWidget(rebootConfirmCancelButton);
    buttonLayout->addWidget(rebootConfirmOkButton);
    panelLayout->addLayout(buttonLayout);

    overlayLayout->addWidget(rebootConfirmPanel, 0, Qt::AlignCenter);
    overlayLayout->addStretch();

    connect(rebootConfirmCancelButton, &QPushButton::clicked, this, &SystemFunctionPage::hideRebootConfirmOverlay);
    connect(rebootConfirmOkButton, &QPushButton::clicked, this,
            [this]()
            {
                hideRebootConfirmOverlay();
                emit requestRebootDevice();
            });

    updateRebootConfirmGeometry();
}

void SystemFunctionPage::updateRebootConfirmGeometry()
{
    if (!rebootConfirmOverlay)
    {
        return;
    }

    rebootConfirmOverlay->setGeometry(rect());
}

void SystemFunctionPage::showRebootConfirmOverlay()
{
    ensureRebootConfirmOverlay();
    updateRebootConfirmGeometry();
    rebootConfirmOverlay->show();
    rebootConfirmOverlay->raise();
}

void SystemFunctionPage::hideRebootConfirmOverlay()
{
    if (!rebootConfirmOverlay)
    {
        return;
    }

    rebootConfirmOverlay->hide();
}

void SystemFunctionPage::ensureTimePickerPopup()
{
    if (timePickerPopup)
    {
        return;
    }

    timePickerPopup = new QWidget(this);
    timePickerPopup->setObjectName(QStringLiteral("timePickerPopup"));
    timePickerPopup->setAttribute(Qt::WA_StyledBackground, true);
    timePickerPopup->setStyleSheet("#timePickerPopup { background-color: #1f1f22; border: 1px solid #34343a; border-radius: 8px; }");
    timePickerPopup->setFixedSize(560, 330);
    timePickerPopup->hide();

    QVBoxLayout *popupLayout = new QVBoxLayout(timePickerPopup);
    popupLayout->setContentsMargins(14, 14, 14, 14);
    popupLayout->setSpacing(12);

    const int calendarAreaWidth = 300;
    const int separatorWidth = 1;
    const int areaSpacing = 12;
    const int timeColumnWidth = 48;
    const int timeAreaLeftPadding = 8;
    const int rightTimeAreaWidth = timeAreaLeftPadding + timeColumnWidth * 3 + separatorWidth * 2;
    const int alignedRowWidth = calendarAreaWidth + separatorWidth + rightTimeAreaWidth + areaSpacing * 2;

    auto createHeaderButton = [this](const QString &text) -> QPushButton *
    {
        QPushButton *button = new QPushButton(text, timePickerPopup);
        button->setFixedSize(30, 24);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet("QPushButton { background-color: transparent; color: #d7dbe1; border: none; "
                              "padding: 0px; font-size: 14px; font-weight: 600; }"
                              "QPushButton:hover { color: #ffffff; background-color: #2b2b2f; border-radius: 4px; }"
                              "QPushButton:pressed { background-color: #323238; }");
        return button;
    };

    timePickerPrevYearButton = createHeaderButton(QStringLiteral("<<"));
    timePickerPrevMonthButton = createHeaderButton(QStringLiteral("<"));
    timePickerNextMonthButton = createHeaderButton(QStringLiteral(">"));
    timePickerNextYearButton = createHeaderButton(QStringLiteral(">>"));

    timePickerHeaderLabel = new QLabel(timePickerPopup);
    timePickerHeaderLabel->setAlignment(Qt::AlignCenter);
    timePickerHeaderLabel->setFixedWidth(120);
    timePickerHeaderLabel->setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold;");

    QLabel *timePreviewLabel = new QLabel(timePickerPopup);
    timePreviewLabel->setObjectName(QStringLiteral("timePickerPreviewLabel"));
    timePreviewLabel->setAlignment(Qt::AlignCenter);
    timePreviewLabel->setFixedWidth(rightTimeAreaWidth - timeAreaLeftPadding);
    timePreviewLabel->setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold;");

    QWidget *headerRowWidget = new QWidget(timePickerPopup);
    headerRowWidget->setFixedWidth(alignedRowWidth);
    headerRowWidget->setAttribute(Qt::WA_StyledBackground, true);
    headerRowWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRowWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(areaSpacing);

    QWidget *leftHeaderWidget = new QWidget(headerRowWidget);
    leftHeaderWidget->setFixedWidth(calendarAreaWidth);
    leftHeaderWidget->setAttribute(Qt::WA_StyledBackground, true);
    leftHeaderWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *leftHeaderLayout = new QHBoxLayout(leftHeaderWidget);
    leftHeaderLayout->setContentsMargins(2, 0, 2, 0);
    leftHeaderLayout->setSpacing(4);

    QFrame *headerSeparator = new QFrame(timePickerPopup);
    headerSeparator->setFixedWidth(separatorWidth);
    headerSeparator->setFixedHeight(24);
    headerSeparator->setStyleSheet("background-color: rgba(255, 255, 255, 120);");

    leftHeaderLayout->addWidget(timePickerPrevYearButton);
    leftHeaderLayout->addWidget(timePickerPrevMonthButton);
    leftHeaderLayout->addStretch();
    leftHeaderLayout->addWidget(timePickerHeaderLabel, 0, Qt::AlignCenter);
    leftHeaderLayout->addStretch();
    leftHeaderLayout->addWidget(timePickerNextMonthButton);
    leftHeaderLayout->addWidget(timePickerNextYearButton);

    QWidget *rightHeaderWidget = new QWidget(headerRowWidget);
    rightHeaderWidget->setFixedWidth(rightTimeAreaWidth);
    rightHeaderWidget->setAttribute(Qt::WA_StyledBackground, true);
    rightHeaderWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *rightHeaderLayout = new QHBoxLayout(rightHeaderWidget);
    rightHeaderLayout->setContentsMargins(timeAreaLeftPadding, 0, 0, 0);
    rightHeaderLayout->setSpacing(0);
    rightHeaderLayout->addStretch();
    rightHeaderLayout->addWidget(timePreviewLabel, 0, Qt::AlignCenter);
    rightHeaderLayout->addStretch();

    headerLayout->addWidget(leftHeaderWidget, 0, Qt::AlignLeft);
    headerLayout->addWidget(headerSeparator, 0, Qt::AlignVCenter);
    headerLayout->addWidget(rightHeaderWidget, 0, Qt::AlignLeft);
    popupLayout->addWidget(headerRowWidget, 0, Qt::AlignHCenter);

    QWidget *contentRowWidget = new QWidget(timePickerPopup);
    contentRowWidget->setFixedWidth(alignedRowWidth);
    contentRowWidget->setAttribute(Qt::WA_StyledBackground, true);
    contentRowWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *contentLayout = new QHBoxLayout(contentRowWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(areaSpacing);

    timePickerCalendar = new QCalendarWidget(timePickerPopup);
    timePickerCalendar->setLocale(QLocale(QLocale::Chinese, QLocale::China));
    timePickerCalendar->setGridVisible(false);
    timePickerCalendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    timePickerCalendar->setNavigationBarVisible(false);
    timePickerCalendar->setFixedSize(calendarAreaWidth, 222);
    timePickerCalendar->setStyleSheet("QCalendarWidget { background-color: #1f1f22; color: #ffffff; border: none; }"
                                      "QCalendarWidget QWidget#qt_calendar_navigationbar { background-color: #1f1f22; }"
                                      "QCalendarWidget QWidget#qt_calendar_calendarview { background-color: #1f1f22; "
                                      "alternate-background-color: #1f1f22; }"
                                      "QCalendarWidget QTableView { background-color: #1f1f22; alternate-background-color: #1f1f22; "
                                      "selection-background-color: #e58b3e; selection-color: #ffffff; }"
                                      "QCalendarWidget QHeaderView { background-color: #1f1f22; }"
                                      "QCalendarWidget QHeaderView::section { background-color: #1f1f22; color: #d98c84; "
                                      "border: none; padding: 6px 0; font-size: 14px; font-weight: bold; }"
                                      "QCalendarWidget QToolButton { color: #ffffff; background: transparent; min-width: 30px; "
                                      "min-height: 28px; font-size: 14px; }"
                                      "QCalendarWidget QToolButton:hover { background-color: #2b2b2f; border-radius: 4px; }"
                                      "QCalendarWidget QMenu { background-color: #1f1f22; color: #ffffff; }"
                                      "QCalendarWidget QSpinBox { background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; }"
                                      "QCalendarWidget QAbstractItemView { background-color: #1f1f22; color: #ffffff; border: none; "
                                      "selection-background-color: #e58b3e; selection-color: #ffffff; }");
    contentLayout->addWidget(timePickerCalendar);

    QFrame *calendarTimeSeparator = new QFrame(timePickerPopup);
    calendarTimeSeparator->setFixedWidth(separatorWidth);
    calendarTimeSeparator->setFixedHeight(222);
    calendarTimeSeparator->setStyleSheet("background-color: rgba(255, 255, 255, 120);");
    contentLayout->addWidget(calendarTimeSeparator);

    auto createTimeList = [this]() -> QListWidget *
    {
        QListWidget *list = new QListWidget(timePickerPopup);
        list->setFixedSize(timeColumnWidth, 222);
        list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        list->setFocusPolicy(Qt::NoFocus);
        list->setStyleSheet("QListWidget { background-color: transparent; color: #ffffff; border: none; outline: none; "
                            "font-size: 14px; }"
                            "QListWidget::viewport { background-color: transparent; border: none; }"
                            "QListWidget::item { height: 36px; }"
                            "QListWidget::item:selected { background-color: #4a2c12; color: #ffffff; border-radius: 4px; }"
                            "QScrollBar:vertical { width: 8px; background: transparent; margin: 2px 0; }"
                            "QScrollBar::handle:vertical { background: #6b6f76; border-radius: 4px; min-height: 28px; }"
                            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical, "
                            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; height: 0px; }");
        if (QScrollBar *scrollBar = list->verticalScrollBar())
        {
            scrollBar->setEnabled(true);
        }
        return list;
    };

    timePickerHourList = createTimeList();
    timePickerMinuteList = createTimeList();
    timePickerSecondList = createTimeList();

    for (int hour = 0; hour < 24; ++hour)
    {
        timePickerHourList->addItem(QStringLiteral("%1").arg(hour, 2, 10, QLatin1Char('0')));
    }
    for (int value = 0; value < 60; ++value)
    {
        const QString text = QStringLiteral("%1").arg(value, 2, 10, QLatin1Char('0'));
        timePickerMinuteList->addItem(text);
        timePickerSecondList->addItem(text);
    }

    QWidget *rightContentWidget = new QWidget(contentRowWidget);
    rightContentWidget->setFixedWidth(rightTimeAreaWidth);
    rightContentWidget->setAttribute(Qt::WA_StyledBackground, true);
    rightContentWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *timeListsLayout = new QHBoxLayout(rightContentWidget);
    timeListsLayout->setContentsMargins(timeAreaLeftPadding, 0, 0, 0);
    timeListsLayout->setSpacing(0);
    timeListsLayout->addWidget(timePickerHourList);
    QFrame *sep1 = new QFrame(rightContentWidget);
    sep1->setFixedWidth(separatorWidth);
    sep1->setStyleSheet("background-color: transparent;");
    timeListsLayout->addWidget(sep1);
    timeListsLayout->addWidget(timePickerMinuteList);
    QFrame *sep2 = new QFrame(rightContentWidget);
    sep2->setFixedWidth(separatorWidth);
    sep2->setStyleSheet("background-color: transparent;");
    timeListsLayout->addWidget(sep2);
    timeListsLayout->addWidget(timePickerSecondList);
    contentLayout->addWidget(rightContentWidget);

    popupLayout->addWidget(contentRowWidget, 0, Qt::AlignHCenter);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(12);

    timePickerNowButton = new QPushButton(QStringLiteral("此刻"), timePickerPopup);
    timePickerNowButton->setFixedSize(76, 34);
    timePickerNowButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: #59a6ff; border: none; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { color: #7db8ff; }");

    timePickerConfirmButton = new QPushButton(QStringLiteral("确定"), timePickerPopup);
    timePickerConfirmButton->setFixedSize(76, 34);
    timePickerConfirmButton->setStyleSheet(
        "QPushButton { background-color: #e58b3e; color: #ffffff; border: none; border-radius: 6px; "
        "font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #f09a4f; }");

    buttonLayout->addWidget(timePickerNowButton, 0, Qt::AlignLeft);
    buttonLayout->addStretch();
    buttonLayout->addWidget(timePickerConfirmButton, 0, Qt::AlignRight);
    popupLayout->addLayout(buttonLayout);

    connect(timePickerNowButton, &QPushButton::clicked, this,
            [this]()
            {
                if (!setTimeEdit)
                {
                    return;
                }

                setTimeEdit->setDateTime(QDateTime::currentDateTime());
                syncTimePickerSelectionFromEdit();
            });
    connect(timePickerConfirmButton, &QPushButton::clicked, this, &SystemFunctionPage::applyTimePickerSelection);
    connect(timePickerPrevYearButton, &QPushButton::clicked, timePickerCalendar, &QCalendarWidget::showPreviousYear);
    connect(timePickerPrevMonthButton, &QPushButton::clicked, timePickerCalendar, &QCalendarWidget::showPreviousMonth);
    connect(timePickerNextMonthButton, &QPushButton::clicked, timePickerCalendar, &QCalendarWidget::showNextMonth);
    connect(timePickerNextYearButton, &QPushButton::clicked, timePickerCalendar, &QCalendarWidget::showNextYear);
    connect(timePickerCalendar, &QCalendarWidget::currentPageChanged, this, &SystemFunctionPage::updateTimePickerHeader);
    connect(timePickerHourList, &QListWidget::currentRowChanged, this,
            [timePreviewLabel, this](int)
            {
                if (!timePickerHourList || !timePickerMinuteList || !timePickerSecondList)
                {
                    return;
                }
                timePreviewLabel->setText(
                    QStringLiteral("%1:%2:%3")
                        .arg(qMax(0, timePickerHourList->currentRow()), 2, 10, QLatin1Char('0'))
                        .arg(qMax(0, timePickerMinuteList->currentRow()), 2, 10, QLatin1Char('0'))
                        .arg(qMax(0, timePickerSecondList->currentRow()), 2, 10, QLatin1Char('0')));
            });
    connect(timePickerMinuteList, &QListWidget::currentRowChanged, this,
            [timePreviewLabel, this](int)
            {
                if (!timePickerHourList || !timePickerMinuteList || !timePickerSecondList)
                {
                    return;
                }
                timePreviewLabel->setText(
                    QStringLiteral("%1:%2:%3")
                        .arg(qMax(0, timePickerHourList->currentRow()), 2, 10, QLatin1Char('0'))
                        .arg(qMax(0, timePickerMinuteList->currentRow()), 2, 10, QLatin1Char('0'))
                        .arg(qMax(0, timePickerSecondList->currentRow()), 2, 10, QLatin1Char('0')));
            });
    connect(timePickerSecondList, &QListWidget::currentRowChanged, this,
            [timePreviewLabel, this](int)
            {
                if (!timePickerHourList || !timePickerMinuteList || !timePickerSecondList)
                {
                    return;
                }
                timePreviewLabel->setText(
                    QStringLiteral("%1:%2:%3")
                        .arg(qMax(0, timePickerHourList->currentRow()), 2, 10, QLatin1Char('0'))
                        .arg(qMax(0, timePickerMinuteList->currentRow()), 2, 10, QLatin1Char('0'))
                        .arg(qMax(0, timePickerSecondList->currentRow()), 2, 10, QLatin1Char('0')));
            });

    updateTimePickerHeader();
}

void SystemFunctionPage::updateTimePickerPopupPosition()
{
    if (!timePickerPopup || !setTimeEdit)
    {
        return;
    }

    const QPoint anchorBottomLeft = setTimeEdit->mapTo(this, QPoint(0, setTimeEdit->height() + 8));
    int x = anchorBottomLeft.x();
    int y = anchorBottomLeft.y();

    if (x + timePickerPopup->width() > width() - 12)
    {
        x = width() - timePickerPopup->width() - 12;
    }
    if (x < 12)
    {
        x = 12;
    }
    if (y + timePickerPopup->height() > height() - 12)
    {
        y = setTimeEdit->mapTo(this, QPoint(0, -timePickerPopup->height() - 8)).y();
    }
    if (y < 12)
    {
        y = 12;
    }

    timePickerPopup->move(x, y);
}

void SystemFunctionPage::updateTimePickerHeader()
{
    if (!timePickerCalendar || !timePickerHeaderLabel)
    {
        return;
    }

    timePickerHeaderLabel->setText(QStringLiteral("%1年 %2月").arg(timePickerCalendar->yearShown()).arg(timePickerCalendar->monthShown()));
}

void SystemFunctionPage::syncTimePickerSelectionFromEdit()
{
    if (!setTimeEdit)
    {
        return;
    }

    ensureTimePickerPopup();
    const QDateTime dateTime = setTimeEdit->dateTime();
    timePickerCalendar->setSelectedDate(dateTime.date());
    timePickerCalendar->showSelectedDate();

    auto selectRow = [](QListWidget *list, int row)
    {
        if (!list || row < 0 || row >= list->count())
        {
            return;
        }
        list->setCurrentRow(row);
        if (QListWidgetItem *item = list->item(row))
        {
            list->scrollToItem(item, QAbstractItemView::PositionAtTop);
        }
    };

    selectRow(timePickerHourList, dateTime.time().hour());
    selectRow(timePickerMinuteList, dateTime.time().minute());
    selectRow(timePickerSecondList, dateTime.time().second());
}

void SystemFunctionPage::applyTimePickerSelection()
{
    if (!setTimeEdit || !timePickerCalendar || !timePickerHourList || !timePickerMinuteList || !timePickerSecondList)
    {
        return;
    }

    const int hour = qMax(0, timePickerHourList->currentRow());
    const int minute = qMax(0, timePickerMinuteList->currentRow());
    const int second = qMax(0, timePickerSecondList->currentRow());
    const QDate selectedDate = timePickerCalendar->selectedDate();
    setTimeEdit->setDateTime(QDateTime(selectedDate, QTime(hour, minute, second)));
    hideTimePickerPopup();
}

void SystemFunctionPage::showTimePickerPopup()
{
    if (!setTimeEdit || !setTimeEdit->isEnabled())
    {
        return;
    }

    ensureTimePickerPopup();
    syncTimePickerSelectionFromEdit();
    updateTimePickerPopupPosition();
    timePickerPopup->show();
    timePickerPopup->raise();
}

void SystemFunctionPage::hideTimePickerPopup()
{
    if (!timePickerPopup)
    {
        return;
    }

    timePickerPopup->hide();
}

void SystemFunctionPage::ensureToastWidget()
{
    if (toastWidget)
    {
        return;
    }

    toastWidget = new QWidget(this);
    toastWidget->setObjectName("systemFunctionToast");
    toastWidget->setStyleSheet("#systemFunctionToast { background-color: rgba(32, 32, 32, 230); "
                               "border: 1px solid #4a4a4a; border-radius: 4px; }");
    toastWidget->hide();

    QHBoxLayout *toastLayout = new QHBoxLayout(toastWidget);
    toastLayout->setContentsMargins(14, 10, 14, 10);
    toastLayout->setSpacing(8);

    toastIconLabel = new QLabel(QStringLiteral("!"), toastWidget);
    toastIconLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold;");
    toastLayout->addWidget(toastIconLabel);

    toastTextLabel = new QLabel(toastWidget);
    toastTextLabel->setStyleSheet("color: #ffffff; font-size: 14px;");
    toastLayout->addWidget(toastTextLabel);

    toastOpacityEffect = new QGraphicsOpacityEffect(toastWidget);
    toastOpacityEffect->setOpacity(0.0);
    toastWidget->setGraphicsEffect(toastOpacityEffect);

    toastFadeInAnimation = new QPropertyAnimation(toastOpacityEffect, "opacity", this);
    toastFadeInAnimation->setDuration(160);
    toastFadeInAnimation->setStartValue(0.0);
    toastFadeInAnimation->setEndValue(1.0);

    toastFadeOutAnimation = new QPropertyAnimation(toastOpacityEffect, "opacity", this);
    toastFadeOutAnimation->setDuration(220);
    toastFadeOutAnimation->setStartValue(1.0);
    toastFadeOutAnimation->setEndValue(0.0);
    connect(toastFadeOutAnimation, &QPropertyAnimation::finished, toastWidget, &QWidget::hide);

    toastHideTimer = new QTimer(this);
    toastHideTimer->setSingleShot(true);
    connect(toastHideTimer, &QTimer::timeout, this,
            [this]()
            {
                if (!toastWidget || !toastWidget->isVisible())
                {
                    return;
                }
                toastFadeOutAnimation->stop();
                toastFadeOutAnimation->start();
            });
}

void SystemFunctionPage::updateToastPosition()
{
    if (!toastWidget)
    {
        return;
    }

    toastWidget->adjustSize();
    const int x = (width() - toastWidget->width()) / 2;
    toastWidget->move(qMax(0, x), 16);
}

void SystemFunctionPage::showToastResult(bool success, const QString &message)
{
    ensureToastWidget();
    toastHideTimer->stop();
    toastFadeOutAnimation->stop();

    toastIconLabel->setText(success ? QStringLiteral("OK") : QStringLiteral("!"));
    toastTextLabel->setText(extractDisplayMessage(success, message));
    updateToastPosition();
    toastWidget->show();
    toastWidget->raise();
    toastFadeInAnimation->stop();
    toastFadeInAnimation->start();
    toastHideTimer->start(2200);
}

QString SystemFunctionPage::extractDisplayMessage(bool success, const QString &message) const
{
    const QString trimmed = message.trimmed();
    if (success)
    {
        return trimmed.isEmpty() ? QStringLiteral("设置成功") : trimmed;
    }

    const int infoPos = trimmed.indexOf(QStringLiteral("Info:"));
    if (infoPos >= 0)
    {
        const QString infoText = trimmed.mid(infoPos + 5).trimmed();
        if (!infoText.isEmpty())
        {
            return infoText;
        }
    }

    return trimmed.isEmpty() ? QStringLiteral("设置失败") : trimmed;
}

QString SystemFunctionPage::sectionTitleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold; padding: 0 0 12px 0;");
}

QString SystemFunctionPage::formLabelStyle(bool required) const
{
    return required ? QStringLiteral("color: #e6e6e6; font-size: 14px;")
                    : QStringLiteral("color: #e6e6e6; font-size: 14px;");
}

QString SystemFunctionPage::readOnlyValueStyle() const
{
    return QStringLiteral("color: #d7e3f7; font-size: 14px;");
}

QString SystemFunctionPage::noteLabelStyle() const
{
    return QStringLiteral("color: #9da2ab; font-size: 12px;");
}
