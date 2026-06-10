#include "settings_page.h"
#include "alarm_history/alarm_history_page.h"
#include "angle_calibration/angle_calibration_page.h"
#include "authorization_info/authorization_info_page.h"
#include "data_collection/data_collection_page.h"
#include "direction_calibration_value/direction_calibration_value_page.h"
#include "detect_band/detect_band_page.h"
#include "device/device_settings_page.h"
#include "firmware_version/firmware_version_page.h"
#include "model_library/model_library_page.h"
#include "mode_select/mode_select_page.h"
#include "power_amplifier/power_amplifier_page.h"
#include "signal_source_params/signal_source_params_page.h"
#include "strike_frequency/strike_frequency_page.h"
#include "strike_status/strike_status_page.h"
#include "spectrum_switch/spectrum_switch_page.h"
#include "system_log/system_log_page.h"
#include "system_function/system_function_page.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QDebug>
#include <QHBoxLayout>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>

namespace
{
constexpr auto kAuthGroup = "auth";
constexpr auto kAdminPasswordKey = "admin_password_hash";
constexpr auto kRootPasswordKey = "root_password_hash";

template <typename Func>
void forwardToDeviceSettingsPage(DeviceSettingsPage *page, Func &&func)
{
    if (!page)
    {
        return;
    }

    func(page);
}

QString hashPassword(const QString &password)
{
    return QString::fromLatin1(QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex());
}

QString authConfigFilePath()
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (configDir.isEmpty())
    {
        configDir = QCoreApplication::applicationDirPath();
    }

    QDir dir(configDir);
    dir.mkpath(QStringLiteral("."));
    return dir.filePath(QStringLiteral("auth.ini"));
}

QString passwordKeyForRole(SettingsUserRole role)
{
    return role == SettingsUserRole::Root ? QString::fromLatin1(kRootPasswordKey)
                                          : QString::fromLatin1(kAdminPasswordKey);
}

QString defaultPasswordForRole(SettingsUserRole role)
{
    return role == SettingsUserRole::Root ? QStringLiteral("root123") : QStringLiteral("admin");
}

QString storedPasswordHashForRole(SettingsUserRole role)
{
    QSettings settings(authConfigFilePath(), QSettings::IniFormat);
    settings.beginGroup(QString::fromLatin1(kAuthGroup));
    const QString storedHash = settings.value(passwordKeyForRole(role)).toString().trimmed();
    settings.endGroup();
    return storedHash.isEmpty() ? hashPassword(defaultPasswordForRole(role)) : storedHash;
}

bool verifyPasswordForRole(SettingsUserRole role, const QString &password)
{
    return !password.isEmpty() && storedPasswordHashForRole(role) == hashPassword(password);
}

bool savePasswordForRole(SettingsUserRole role, const QString &password)
{
    QSettings settings(authConfigFilePath(), QSettings::IniFormat);
    settings.beginGroup(QString::fromLatin1(kAuthGroup));
    settings.setValue(passwordKeyForRole(role), hashPassword(password));
    settings.endGroup();
    settings.sync();
    return settings.status() == QSettings::NoError;
}
} // namespace

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent), deviceSettingsPage(nullptr), alarmHistoryPage(nullptr), authorizationInfoPage(nullptr),
      dataCollectionPage(nullptr), angleCalibrationPage(nullptr), directionCalibrationValuePage(nullptr), detectBandPage(nullptr),
      firmwareVersionPage(nullptr), modelLibraryPage(nullptr), modeSelectPage(nullptr), powerAmplifierPage(nullptr),
      signalSourceParamsPage(nullptr), systemLogPage(nullptr), systemFunctionPage(nullptr), spectrumSwitchPage(nullptr),
      strikeFrequencyPage(nullptr), strikeStatusPage(nullptr), currentUserRole(SettingsUserRole::None)
{
    // 强制设置为原生窗口，解决 Linux/X11 下与 WebEngine 混合的 Z-Order 遮挡问题
    setAttribute(Qt::WA_NativeWindow);
    setStyleSheet("background-color: #2b2b2b; color: white;");
    setupUi();
}

void SettingsPage::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    mainStack = new QStackedWidget(this);
    layout->addWidget(mainStack);

    setupAuthView();
    setupSettingsView();

    mainStack->addWidget(authWidget);
    mainStack->addWidget(settingsWidget);
    mainStack->setCurrentWidget(authWidget);
}

void SettingsPage::setupAuthView()
{
    authWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(authWidget);
    layout->setAlignment(Qt::AlignCenter);

    // 锁图标
    QLabel *lockIcon = new QLabel("🔒", authWidget);
    lockIcon->setAlignment(Qt::AlignCenter);
    lockIcon->setStyleSheet("font-size: 32px; color: #b0b0b0;");

    // 提示文字
    QLabel *lockText = new QLabel("设置已锁定", authWidget);
    lockText->setAlignment(Qt::AlignCenter);
    lockText->setStyleSheet("font-size: 14px; color: #b0b0b0; margin-bottom: 10px;");

    // 密码输入框
    pwdInput = new QLineEdit(authWidget);
    pwdInput->setEchoMode(QLineEdit::Normal); // 修改为 Normal，使输入的密码明文可见
    // pwdInput->setPlaceholderText("请输入密码");
    pwdInput->setFixedSize(200, 35);
    pwdInput->setStyleSheet("QLineEdit { background-color: #1e1e1e; color: white; border: 1px solid #444; "
                            "border-radius: 2px; padding: 0 10px; }");

    // 确定按钮
    QPushButton *loginBtn = new QPushButton("确  定", authWidget); // 增加文字之间的间距，避免文字粘连和显示不全
    // 增加按钮的高度，从 35 改为 40
    loginBtn->setFixedSize(200, 40);
    // 去除 Qt CSS 不支持的 letter-spacing 属性
    loginBtn->setStyleSheet("QPushButton { background-color: #ffffff; color: black; border: none; border-radius: 2px; "
                            "font-weight: bold; font-size: 14px; margin-top: 5px; padding: 2px 0px; }"
                            "QPushButton:hover { background-color: #e0e0e0; }");

    // 错误提示
    errorLabel = new QLabel("", authWidget);
    errorLabel->setAlignment(Qt::AlignCenter);
    errorLabel->setStyleSheet("color: #ff4c4c; font-size: 12px; margin-top: 5px;");

    layout->addWidget(lockIcon);
    layout->addWidget(lockText);
    layout->addWidget(pwdInput);
    layout->addWidget(loginBtn);
    layout->addWidget(errorLabel);

    connect(loginBtn, &QPushButton::clicked, this, &SettingsPage::onLoginClicked);
    connect(pwdInput, &QLineEdit::returnPressed, this, &SettingsPage::onLoginClicked);
}

void SettingsPage::setupSettingsView()
{
    settingsWidget = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(settingsWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *sidebarContainer = createSidebarContainer();
    QPushButton *logoutBtn = createLogoutButton(sidebarContainer);
    mainLayout->addWidget(sidebarContainer);

    setupContentStack();
    mainLayout->addWidget(contentStack, 1);

    bindSettingsViewSignals(logoutBtn);
}

QWidget *SettingsPage::createSidebarContainer()
{
    QWidget *sidebarContainer = new QWidget(settingsWidget);
    sidebarContainer->setFixedWidth(220);
    sidebarContainer->setStyleSheet("QWidget { background-color: #2b2b2b; border-right: 1px solid #1e1e1e; }");

    QVBoxLayout *sidebarLayout = new QVBoxLayout(sidebarContainer);
    sidebarLayout->setContentsMargins(0, 20, 0, 20);
    sidebarLayout->setSpacing(5);

    sidebar = createSidebarList(sidebarContainer);
    sidebarLayout->addWidget(sidebar, 1);
    sidebarLayout->addSpacing(10);
    return sidebarContainer;
}

QListWidget *SettingsPage::createSidebarList(QWidget *parent)
{
    QListWidget *sidebarList = new QListWidget(parent);
    sidebarList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebarList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    sidebarList->setStyleSheet(
        "QListWidget { background: transparent; border: none; outline: none; }"
        "QListWidget::item { color: #888888; padding: 12px 20px; font-size: 13px; }"
        "QListWidget::item:hover { color: #ffffff; background-color: #3a3a3a; }"
        "QListWidget::item:selected { color: #ffffff; background-color: #444444; border-left: 3px solid #ff9900; }");
    sidebarList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    return sidebarList;
}

QPushButton *SettingsPage::createLogoutButton(QWidget *parent)
{
    QPushButton *logoutBtn = new QPushButton("退出登录", parent);
    logoutBtn->setFixedSize(180, 40);
    logoutBtn->setStyleSheet("QPushButton { background-color: #d32f2f; color: white; border: none; border-radius: 4px; "
                             "font-weight: bold; margin-left: 20px; }"
                             "QPushButton:hover { background-color: #f44336; }");

    QVBoxLayout *sidebarLayout = qobject_cast<QVBoxLayout *>(parent->layout());
    sidebarLayout->addWidget(logoutBtn);
    return logoutBtn;
}

void SettingsPage::setupContentStack()
{
    contentStack = new QStackedWidget(settingsWidget);
    contentStack->setStyleSheet("QStackedWidget { background-color: #202020; }");

    deviceSettingsPage = new DeviceSettingsPage(SettingsUserRole::None, contentStack);
    alarmHistoryPage = new AlarmHistoryPage(contentStack);
    authorizationInfoPage = new AuthorizationInfoPage(contentStack);
    dataCollectionPage = new DataCollectionPage(contentStack);
    angleCalibrationPage = new AngleCalibrationPage(contentStack);
    directionCalibrationValuePage = new DirectionCalibrationValuePage(contentStack);
    detectBandPage = new DetectBandPage(contentStack);
    firmwareVersionPage = new FirmwareVersionPage(contentStack);
    modelLibraryPage = new ModelLibraryPage(contentStack);
    modeSelectPage = new ModeSelectPage(contentStack);
    powerAmplifierPage = new PowerAmplifierPage(contentStack);
    signalSourceParamsPage = new SignalSourceParamsPage(contentStack);
    systemLogPage = new SystemLogPage(contentStack);
    systemFunctionPage = new SystemFunctionPage(contentStack);
    spectrumSwitchPage = new SpectrumSwitchPage(contentStack);
    strikeFrequencyPage = new StrikeFrequencyPage(contentStack);
    strikeStatusPage = new StrikeStatusPage(contentStack);
    connect(deviceSettingsPage, &DeviceSettingsPage::requestSaveGps, this, &SettingsPage::requestSaveGps);
    connect(deviceSettingsPage, &DeviceSettingsPage::requestSaveFullScan, this, &SettingsPage::requestSaveFullScan);
    connect(deviceSettingsPage, &DeviceSettingsPage::requestSaveDeviceIp, this, &SettingsPage::requestSaveDeviceIp);
    connect(deviceSettingsPage, &DeviceSettingsPage::requestSaveTcpServerIp, this, &SettingsPage::requestSaveTcpServerIp);
    connect(dataCollectionPage, &DataCollectionPage::requestUploadPatternFile, this, &SettingsPage::requestUploadPatternFile);
    connect(detectBandPage, &DetectBandPage::requestSaveDetectBands, this, &SettingsPage::requestSaveDetectBands);
    connect(modeSelectPage, &ModeSelectPage::requestSaveDroneReportMode, this, &SettingsPage::requestSaveDroneReportMode);
    connect(modeSelectPage, &ModeSelectPage::requestSaveJamMode, this, &SettingsPage::requestSaveJamMode);
    connect(modeSelectPage, &ModeSelectPage::requestSaveNetworkMode, this, &SettingsPage::requestSaveNetworkMode);
    connect(modeSelectPage, &ModeSelectPage::requestSaveUavCategoryDisplayMode, this,
            &SettingsPage::requestSaveUavCategoryDisplayMode);
    connect(modeSelectPage, &ModeSelectPage::requestSaveDataEnable, this, &SettingsPage::requestSaveDataEnable);
    connect(modeSelectPage, &ModeSelectPage::requestSaveFeatureModes, this, &SettingsPage::requestSaveFeatureModes);
    connect(modeSelectPage, &ModeSelectPage::requestQueryDroneReportMode, this, &SettingsPage::requestQueryDroneReportMode);
    connect(modeSelectPage, &ModeSelectPage::requestQueryJamMode, this, &SettingsPage::requestQueryJamMode);
    connect(modeSelectPage, &ModeSelectPage::requestQueryNetworkMode, this, &SettingsPage::requestQueryNetworkMode);
    connect(modeSelectPage, &ModeSelectPage::requestQueryUavCategoryDisplayMode, this,
            &SettingsPage::requestQueryUavCategoryDisplayMode);
    connect(modeSelectPage, &ModeSelectPage::requestQueryDataEnable, this, &SettingsPage::requestQueryDataEnable);
    connect(modeSelectPage, &ModeSelectPage::requestQueryFeatureModes, this, &SettingsPage::requestQueryFeatureModes);
    connect(angleCalibrationPage, &AngleCalibrationPage::requestStartCalibration, this,
            &SettingsPage::requestStartCompassCalibration);
    connect(angleCalibrationPage, &AngleCalibrationPage::requestFinishCalibration, this,
            &SettingsPage::requestFinishCompassCalibration);
    connect(angleCalibrationPage, &AngleCalibrationPage::requestConfirmCalibration, this,
            &SettingsPage::requestConfirmCompassCalibration);
    connect(angleCalibrationPage, &AngleCalibrationPage::requestCancelCalibration, this,
            &SettingsPage::requestCancelCompassCalibration);
    connect(spectrumSwitchPage, &SpectrumSwitchPage::requestOpenSpectrogram, this, &SettingsPage::requestOpenSpectrogram);
    connect(spectrumSwitchPage, &SpectrumSwitchPage::requestCloseSpectrogram, this, &SettingsPage::requestCloseSpectrogram);
    connect(spectrumSwitchPage, &SpectrumSwitchPage::requestOpenSpectrum, this, &SettingsPage::requestOpenSpectrum);
    connect(spectrumSwitchPage, &SpectrumSwitchPage::requestCloseSpectrum, this, &SettingsPage::requestCloseSpectrum);
    connect(strikeFrequencyPage, &StrikeFrequencyPage::requestQueryStrikeFrequencyBands, this,
            &SettingsPage::requestQueryStrikeFrequencyBands);
    connect(strikeFrequencyPage, &StrikeFrequencyPage::requestSaveStrikeFrequencyBands, this,
            &SettingsPage::requestSaveStrikeFrequencyBands);
    connect(powerAmplifierPage, &PowerAmplifierPage::requestQueryPowerAmplifierParams, this,
            &SettingsPage::requestQueryPowerAmplifierParams);
    connect(powerAmplifierPage, &PowerAmplifierPage::requestSavePowerAmplifierParams, this,
            &SettingsPage::requestSavePowerAmplifierParams);
    connect(directionCalibrationValuePage, &DirectionCalibrationValuePage::requestQueryDirectionCalibrationValues, this,
            &SettingsPage::requestQueryDirectionCalibrationValues);
    connect(directionCalibrationValuePage, &DirectionCalibrationValuePage::requestSaveDirectionCalibrationValues, this,
            &SettingsPage::requestSaveDirectionCalibrationValues);
    connect(signalSourceParamsPage, &SignalSourceParamsPage::requestQuerySignalSourceParams, this,
            &SettingsPage::requestQuerySignalSourceParams);
    connect(signalSourceParamsPage, &SignalSourceParamsPage::requestSaveSignalSourceParams, this,
            &SettingsPage::requestSaveSignalSourceParams);
    connect(systemFunctionPage, &SystemFunctionPage::requestSaveBuzzerEnabled, this, &SettingsPage::requestSaveBuzzerEnabled);
    connect(systemFunctionPage, &SystemFunctionPage::warningRemoveTimeChanged, this, &SettingsPage::warningRemoveTimeChanged);
    connect(systemFunctionPage, &SystemFunctionPage::requestSaveSystemTime, this, &SettingsPage::requestSaveSystemTime);
    connect(systemFunctionPage, &SystemFunctionPage::requestQueryBuzzerEnabled, this, &SettingsPage::requestQueryBuzzerEnabled);
    connect(systemFunctionPage, &SystemFunctionPage::requestQueryScreenFlashEnabled, this,
            &SettingsPage::requestQueryScreenFlashEnabled);
    connect(systemFunctionPage, &SystemFunctionPage::requestSetScreenFlashEnabled, this,
            &SettingsPage::requestSetScreenFlashEnabled);
    connect(systemFunctionPage, &SystemFunctionPage::requestRebootDevice, this, &SettingsPage::requestRebootDevice);
    connect(systemFunctionPage, &SystemFunctionPage::requestChangePassword, this,
            [this](SettingsUserRole role, const QString &oldPassword, const QString &newPassword)
            {
                handlePasswordChangeRequest(role, oldPassword, newPassword);
            });
    connect(modelLibraryPage, &ModelLibraryPage::requestSaveModelLibraryMode, this, &SettingsPage::requestSaveModelLibraryMode);
    connect(modelLibraryPage, &ModelLibraryPage::requestQueryModelLibraryMode, this, &SettingsPage::requestQueryModelLibraryMode);
    connect(modelLibraryPage, &ModelLibraryPage::requestQueryModelLibraryRecords, this,
            &SettingsPage::requestQueryModelLibraryRecords);
    connect(modelLibraryPage, &ModelLibraryPage::requestUpdateModelLibraryRecord, this,
            &SettingsPage::requestUpdateModelLibraryRecord);
    contentStack->addWidget(deviceSettingsPage);
    contentStack->addWidget(detectBandPage);
    contentStack->addWidget(modeSelectPage);
    contentStack->addWidget(firmwareVersionPage);
}

int SettingsPage::currentWarningRemoveTimeSeconds() const
{
    return systemFunctionPage ? systemFunctionPage->warningRemoveTimeSeconds() : 20;
}

void SettingsPage::bindSettingsViewSignals(QPushButton *logoutBtn)
{
    auto triggerSidebarRefresh = [this](QWidget *currentWidget)
    {
        if (currentWidget == deviceSettingsPage)
        {
            triggerDeviceSettingsRefresh();
        }
        else if (currentWidget == detectBandPage)
        {
            emit requestQueryDetectBands();
        }
        else if (currentWidget == modeSelectPage)
        {
            emit requestQueryDroneReportMode();
            emit requestQueryJamMode();
            emit requestQueryNetworkMode();
            emit requestQueryUavCategoryDisplayMode();
            emit requestQueryDataEnable();
            emit requestQueryFeatureModes();
        }
        else if (currentWidget == firmwareVersionPage)
        {
            emit requestQueryFirmwareVersions();
        }
        else if (currentWidget == alarmHistoryPage)
        {
            emit requestQueryAlarmHistory();
        }
        else if (currentWidget == authorizationInfoPage)
        {
            emit requestQueryDeviceUsageInfo();
        }
        else if (currentWidget == systemFunctionPage)
        {
            emit requestQueryBuzzerEnabled();
            emit requestQueryScreenFlashEnabled();
        }
        else if (currentWidget == modelLibraryPage)
        {
            emit requestQueryModelLibraryMode();
            emit requestQueryModelLibraryRecords(1, 10);
        }
        else if (currentWidget == strikeFrequencyPage)
        {
            emit requestQueryStrikeFrequencyBands();
        }
        else if (currentWidget == strikeStatusPage)
        {
            emit requestQueryStrikeStatus();
        }
        else if (currentWidget == powerAmplifierPage)
        {
            emit requestQueryPowerAmplifierParams();
        }
        else if (currentWidget == directionCalibrationValuePage)
        {
            emit requestQueryDirectionCalibrationValues();
        }
        else if (currentWidget == signalSourceParamsPage)
        {
            emit requestQuerySignalSourceParams();
        }
    };

    connect(sidebar, &QListWidget::currentRowChanged, this,
            [this, triggerSidebarRefresh](int currentRow)
            {
                contentStack->setCurrentIndex(currentRow);
                triggerSidebarRefresh(contentStack->widget(currentRow));
            });
    connect(sidebar, &QListWidget::itemClicked, this,
            [this, triggerSidebarRefresh](QListWidgetItem *item)
            {
                if (!item)
                {
                    return;
                }

                triggerSidebarRefresh(contentStack->widget(sidebar->row(item)));
            });
    connect(logoutBtn, &QPushButton::clicked, this, &SettingsPage::forceLogout);
}

void SettingsPage::updateGpsInfo(uint8_t mode, float lng, float lat, float alt)
{
    forwardToDeviceSettingsPage(deviceSettingsPage,
                                [=](DeviceSettingsPage *page) { page->updateGpsInfo(mode, lng, lat, alt); });
}

void SettingsPage::updateGpsSaveResult(bool success, const QString &message)
{
    forwardToDeviceSettingsPage(deviceSettingsPage,
                                [&](DeviceSettingsPage *page) { page->showSaveResult(success, message); });
}

void SettingsPage::updateFullScanSettings(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin,
                                          double att)
{
    forwardToDeviceSettingsPage(
        deviceSettingsPage,
        [=](DeviceSettingsPage *page) { page->updateFullScanSettings(ssth, ssJgMax, ssJgMin, ssMax, ssMin, att); });
}

void SettingsPage::updateFullScanSaveResult(bool success, const QString &message)
{
    forwardToDeviceSettingsPage(deviceSettingsPage,
                                [&](DeviceSettingsPage *page) { page->showSaveResult(success, message); });
}

void SettingsPage::updateDeviceIpSettings(const QString &ip, int port, const QString &mask, const QString &route,
                                          const QString &dns)
{
    forwardToDeviceSettingsPage(deviceSettingsPage, [&](DeviceSettingsPage *page)
                                { page->updateDeviceIpSettings(ip, port, mask, route, dns); });
}

void SettingsPage::updateDeviceIpSaveResult(bool success, const QString &message)
{
    forwardToDeviceSettingsPage(deviceSettingsPage,
                                [&](DeviceSettingsPage *page) { page->showSaveResult(success, message); });
}

void SettingsPage::updateTcpServerIpSettings(const QString &ip, int port)
{
    forwardToDeviceSettingsPage(deviceSettingsPage,
                                [&](DeviceSettingsPage *page) { page->updateTcpServerIpSettings(ip, port); });
}

void SettingsPage::updateTcpServerIpSaveResult(bool success, const QString &message)
{
    forwardToDeviceSettingsPage(deviceSettingsPage,
                                [&](DeviceSettingsPage *page) { page->showSaveResult(success, message); });
}

void SettingsPage::updateDetectBands(const QVector<DetectBandParam> &bands)
{
    if (!detectBandPage)
    {
        return;
    }

    detectBandPage->updateDetectBands(bands);
}

void SettingsPage::updateDetectBandSaveResult(bool success, const QString &message)
{
    if (!detectBandPage)
    {
        return;
    }

    detectBandPage->showSaveResult(success, message);
}

void SettingsPage::updateDeviceUsageInfo(const DeviceUsageInfo &info)
{
    if (!authorizationInfoPage)
    {
        return;
    }

    authorizationInfoPage->updateDeviceUsageInfo(info);
}

void SettingsPage::updateModelLibraryMode(uint8_t mode)
{
    if (!modelLibraryPage)
    {
        return;
    }

    modelLibraryPage->updateModelLibraryMode(mode);
}

void SettingsPage::updateModelLibraryModeSaveResult(bool success, const QString &message)
{
    if (!modelLibraryPage)
    {
        return;
    }

    modelLibraryPage->showSaveResult(success, message);
}

void SettingsPage::updateModelLibraryRecords(const ModelLibraryPageResult &result)
{
    if (!modelLibraryPage)
    {
        return;
    }

    modelLibraryPage->updateModelLibraryRecords(result);
}

void SettingsPage::updateModelLibraryRecordSaveResult(bool success, const QString &message)
{
    if (!modelLibraryPage)
    {
        return;
    }

    modelLibraryPage->showRecordSaveResult(success, message);
}

void SettingsPage::updateDroneReportMode(uint8_t mode)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->updateDroneReportMode(mode);
}

void SettingsPage::updateDroneReportModeSaveResult(bool success, const QString &message)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->showDroneReportModeSaveResult(success, message);
}

void SettingsPage::updateJamMode(uint8_t mode)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->updateJamMode(mode);
}

void SettingsPage::updateJamModeSaveResult(bool success, const QString &message)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->showJamModeSaveResult(success, message);
}

void SettingsPage::updateNetworkMode(uint8_t mode)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->updateNetworkMode(mode);
}

void SettingsPage::updateNetworkModeSaveResult(bool success, const QString &message)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->showNetworkModeSaveResult(success, message);
}

void SettingsPage::updateUavCategoryDisplayMode(uint8_t mode)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->updateUavCategoryDisplayMode(mode);
}

void SettingsPage::updateUavCategoryDisplayModeSaveResult(bool success, const QString &message)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->showUavCategoryDisplayModeSaveResult(success, message);
}

void SettingsPage::updateDataEnable(uint8_t enabled)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->updateDataEnable(enabled);
}

void SettingsPage::updateDataEnableSaveResult(bool success, const QString &message)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->showDataEnableSaveResult(success, message);
}

void SettingsPage::updateFeatureModes(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->updateFeatureModes(wifiRemoteIdEnabled, fpvEnabled, djiParseEnabled);
}

void SettingsPage::updateFeatureModeSaveResult(bool success, const QString &message)
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->showFeatureModeSaveResult(success, message);
}

void SettingsPage::updateCompassCalibrationResult(uint16_t responseDataType, bool success, const QString &message)
{
    if (!angleCalibrationPage)
    {
        return;
    }

    angleCalibrationPage->showCalibrationResult(responseDataType, success, message);
}

void SettingsPage::updateSpectrumSwitchResult(uint16_t responseDataType, bool success, const QString &message)
{
    if (!spectrumSwitchPage)
    {
        return;
    }

    spectrumSwitchPage->showSwitchResult(responseDataType, success, message);
}

void SettingsPage::updateSpectrumReport(const SpectrumReportData &reportData)
{
    if (!spectrumSwitchPage)
    {
        return;
    }

    spectrumSwitchPage->updateSpectrumReport(reportData);
}

void SettingsPage::updateFullSpectrumSwitchResult(bool enabled, bool success, const QString &message)
{
    if (!spectrumSwitchPage)
    {
        return;
    }

    spectrumSwitchPage->showFullSpectrumSwitchResult(enabled, success, message);
}

void SettingsPage::updateFullSpectrumReport(const FullSpectrumReportData &reportData)
{
    if (!spectrumSwitchPage)
    {
        return;
    }

    spectrumSwitchPage->updateFullSpectrumReport(reportData);
}

void SettingsPage::updateAlarmHistory(const AlarmHistoryInfo &info)
{
    if (!alarmHistoryPage)
    {
        return;
    }

    alarmHistoryPage->updateAlarmHistory(info);
}

void SettingsPage::updateStrikeFrequencyBands(const StrikeFrequencyBandList &bands)
{
    if (!strikeFrequencyPage)
    {
        return;
    }

    strikeFrequencyPage->updateStrikeFrequencyBands(bands);
}

void SettingsPage::updateBuzzerEnabled(uint8_t enabled)
{
    if (!systemFunctionPage)
    {
        return;
    }

    systemFunctionPage->updateBuzzerEnabled(enabled);
}

void SettingsPage::updateScreenFlashEnabled(bool enabled)
{
    if (!systemFunctionPage)
    {
        return;
    }

    systemFunctionPage->updateScreenFlashEnabled(enabled);
}

void SettingsPage::updateBuzzerEnabledSaveResult(bool success, const QString &message)
{
    if (!systemFunctionPage)
    {
        return;
    }

    systemFunctionPage->showAlarmSaveResult(success, message);
    if (success)
    {
        emit requestQueryBuzzerEnabled();
    }
}

void SettingsPage::updateSystemTimeSaveResult(bool success, const QString &message)
{
    if (!systemFunctionPage)
    {
        return;
    }

    systemFunctionPage->showSystemTimeSaveResult(success, message);
}

void SettingsPage::updateRebootResult(bool success, const QString &message)
{
    if (!systemFunctionPage)
    {
        return;
    }

    systemFunctionPage->showRebootResult(success, message);
}

void SettingsPage::updateStrikeFrequencySaveResult(bool success, const QString &message)
{
    if (!strikeFrequencyPage)
    {
        return;
    }

    strikeFrequencyPage->showStrikeFrequencySaveResult(success, message);
}

void SettingsPage::updatePowerAmplifierParams(const PowerAmplifierParamList &params)
{
    if (!powerAmplifierPage)
    {
        return;
    }

    powerAmplifierPage->updatePowerAmplifierParams(params);
}

void SettingsPage::updatePowerAmplifierSaveResult(bool success, const QString &message)
{
    if (!powerAmplifierPage)
    {
        return;
    }

    powerAmplifierPage->showSaveResult(success, message);
}

void SettingsPage::updateDirectionCalibrationValues(const DirectionCalibrationValueList &values)
{
    if (!directionCalibrationValuePage)
    {
        return;
    }

    directionCalibrationValuePage->updateDirectionCalibrationValues(values);
}

void SettingsPage::updateDirectionCalibrationSaveResult(bool success, const QString &message)
{
    if (!directionCalibrationValuePage)
    {
        return;
    }

    directionCalibrationValuePage->showSaveResult(success, message);
}

void SettingsPage::updateStrikeStatus(const QVector<int> &switchStates)
{
    if (!strikeStatusPage)
    {
        return;
    }

    strikeStatusPage->updateStrikeStatus(switchStates);
}

void SettingsPage::updateSignalSourceParams(const SignalSourceParamsConfig &config)
{
    if (!signalSourceParamsPage)
    {
        return;
    }

    signalSourceParamsPage->updateSignalSourceParams(config);
}

void SettingsPage::updateSignalSourceParamsSaveResult(bool success, const QString &message)
{
    if (!signalSourceParamsPage)
    {
        return;
    }

    signalSourceParamsPage->showSaveResult(success, message);
}

void SettingsPage::updatePatternUploadResult(bool success, const QString &message)
{
    if (!dataCollectionPage)
    {
        return;
    }

    dataCollectionPage->showPatternUploadResult(success, message);
}

void SettingsPage::updateFirmwareVersions(const QString &appVersion, const QString &fpgaVersion, const QString &gpuVersion)
{
    if (!firmwareVersionPage)
    {
        return;
    }

    firmwareVersionPage->updateFirmwareVersions(appVersion, fpgaVersion, gpuVersion);
}

void SettingsPage::updateFirmwareDeviceSerial(const QString &serialText)
{
    if (!firmwareVersionPage)
    {
        return;
    }

    firmwareVersionPage->updateDeviceSerial(serialText);
}

void SettingsPage::onDeviceConnectionLost()
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->onConnectionLost();
}

void SettingsPage::onDeviceConnectionRestored()
{
    if (!modeSelectPage)
    {
        return;
    }

    modeSelectPage->onConnectionRestored();
}

uint8_t SettingsPage::getCurrentLocationMode() const
{
    return deviceSettingsPage ? deviceSettingsPage->currentLocationMode() : 0;
}

bool SettingsPage::authenticateUser(const QString &password)
{
    if (verifyPasswordForRole(SettingsUserRole::Root, password))
    {
        currentUserRole = SettingsUserRole::Root;
        errorLabel->clear();
        return true;
    }

    if (verifyPasswordForRole(SettingsUserRole::Admin, password))
    {
        currentUserRole = SettingsUserRole::Admin;
        errorLabel->clear();
        return true;
    }

    errorLabel->setText("密码错误，请重试");
    return false;
}

void SettingsPage::handlePasswordChangeRequest(SettingsUserRole role, const QString &oldPassword, const QString &newPassword)
{
    if (!systemFunctionPage)
    {
        return;
    }

    if (role != SettingsUserRole::Admin && role != SettingsUserRole::Root)
    {
        systemFunctionPage->showPasswordChangeResult(false, QStringLiteral("请选择要修改的密码类型"));
        return;
    }

    if (currentUserRole != SettingsUserRole::Root && role == SettingsUserRole::Root)
    {
        systemFunctionPage->showPasswordChangeResult(false, QStringLiteral("当前账号无权修改高级管理员密码"));
        return;
    }

    if (!verifyPasswordForRole(role, oldPassword))
    {
        systemFunctionPage->showPasswordChangeResult(false, QStringLiteral("旧密码输入错误"));
        return;
    }

    if (!savePasswordForRole(role, newPassword))
    {
        systemFunctionPage->showPasswordChangeResult(false, QStringLiteral("密码保存失败，请检查配置目录权限"));
        return;
    }

    const QString successMessage = role == SettingsUserRole::Root ? QStringLiteral("高级管理员密码修改成功，下次登录生效")
                                                                  : QStringLiteral("普通管理员密码修改成功，下次登录生效");
    systemFunctionPage->showPasswordChangeResult(true, successMessage);
}

void SettingsPage::resetSettingsContent()
{
    sidebar->clear();

    for (int i = contentStack->count() - 1; i >= 0; --i)
    {
        QWidget *widget = contentStack->widget(i);
        if (widget == dataCollectionPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == spectrumSwitchPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == angleCalibrationPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == strikeFrequencyPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == strikeStatusPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == alarmHistoryPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == authorizationInfoPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == modelLibraryPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == systemFunctionPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == signalSourceParamsPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == directionCalibrationValuePage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == powerAmplifierPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget == systemLogPage)
        {
            contentStack->removeWidget(widget);
            continue;
        }

        if (widget != deviceSettingsPage && widget != detectBandPage && widget != modeSelectPage &&
            widget != firmwareVersionPage)
        {
            contentStack->removeWidget(widget);
            widget->deleteLater();
        }
    }
}

QLabel *SettingsPage::createPlaceholderPage(const QString &text)
{
    QLabel *placeholder = new QLabel(text, this);
    placeholder->setAlignment(Qt::AlignCenter);
    placeholder->setStyleSheet("color: white; font-size: 18px;");
    return placeholder;
}

void SettingsPage::addSidebarPage(const QString &sidebarText, QWidget *page)
{
    sidebar->addItem(sidebarText);
    if (page && contentStack->indexOf(page) < 0)
    {
        contentStack->addWidget(page);
    }
}

void SettingsPage::populateAdminSettingsPages()
{
    addSidebarPage("  设备设置", deviceSettingsPage);
    addSidebarPage("  侦测频段", detectBandPage);
    addSidebarPage("  模式选择", modeSelectPage);
    addSidebarPage("  固件版本号", firmwareVersionPage);
    addSidebarPage("  角度校准", angleCalibrationPage);
    addSidebarPage("  打击状态", strikeStatusPage);
    addSidebarPage("  告警历史查询", alarmHistoryPage);
    addSidebarPage("  机型库", modelLibraryPage);
    addSidebarPage("  系统功能", systemFunctionPage);
    addSidebarPage("  系统日志", systemLogPage);
}

void SettingsPage::populateRootSettingsPages()
{
    addSidebarPage("  设备设置", deviceSettingsPage);
    addSidebarPage("  侦测频段", detectBandPage);
    addSidebarPage("  模式选择", modeSelectPage);
    addSidebarPage("  固件版本号", firmwareVersionPage);
    addSidebarPage("  数据采集", dataCollectionPage);
    addSidebarPage("  频谱图开关", spectrumSwitchPage);
    addSidebarPage("  角度校准", angleCalibrationPage);
    addSidebarPage("  打击频率设置", strikeFrequencyPage);
    addSidebarPage("  打击状态", strikeStatusPage);
    addSidebarPage("  信源参数", signalSourceParamsPage);
    addSidebarPage("  功放设置", powerAmplifierPage);
    addSidebarPage("  测向定标值设置", directionCalibrationValuePage);
    addSidebarPage("  告警历史查询", alarmHistoryPage);
    addSidebarPage("  机型库", modelLibraryPage);
    addSidebarPage("  授权信息", authorizationInfoPage);
    addSidebarPage("  系统功能", systemFunctionPage);
    addSidebarPage("  系统日志", systemLogPage);
}

void SettingsPage::showSettingsView()
{
    sidebar->setCurrentRow(0);
    pwdInput->clear();
    mainStack->setCurrentWidget(settingsWidget);
}

void SettingsPage::triggerDeviceSettingsRefresh()
{
    emit requestQueryGps();
    emit requestQueryFullScan();
    emit requestQueryDeviceIp();
    emit requestQueryTcpServerIp();
}

void SettingsPage::onLoginClicked()
{
    if (!authenticateUser(pwdInput->text()))
    {
        return;
    }

    resetSettingsContent();
    deviceSettingsPage->setUserRole(currentUserRole);
    if (modeSelectPage)
    {
        modeSelectPage->setUserRole(currentUserRole);
    }
    if (systemFunctionPage)
    {
        systemFunctionPage->setUserRole(currentUserRole);
    }
    if (currentUserRole == SettingsUserRole::Root)
    {
        populateRootSettingsPages();
    }
    else
    {
        populateAdminSettingsPages();
    }

    showSettingsView();
}

void SettingsPage::forceLogout()
{
    currentUserRole = SettingsUserRole::None;
    if (deviceSettingsPage)
    {
        deviceSettingsPage->setUserRole(currentUserRole);
    }
    if (modeSelectPage)
    {
        modeSelectPage->setUserRole(currentUserRole);
    }
    if (systemFunctionPage)
    {
        systemFunctionPage->setUserRole(currentUserRole);
    }
    pwdInput->clear();
    errorLabel->clear();
    mainStack->setCurrentWidget(authWidget);
}

void SettingsPage::handleLogout()
{
    // 处理退出登录的逻辑：清空当前权限，并强制返回到认证页面
    forceLogout();
}
