#ifndef SETTINGS_PAGE_H
#define SETTINGS_PAGE_H

#include <QDateTime>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QWidget>
#include "settings_role.h"
#include "network/core/protocol_types.h"
#include <cstdint>

class DeviceSettingsPage;
class AlarmHistoryPage;
class AuthorizationInfoPage;
class DataCollectionPage;
class AngleCalibrationPage;
class DirectionCalibrationValuePage;
class DetectBandPage;
class FirmwareVersionPage;
class ModelLibraryPage;
class ModeSelectPage;
class PowerAmplifierPage;
class SignalSourceParamsPage;
class SystemLogPage;
class SystemFunctionPage;
class SpectrumSwitchPage;
class StrikeFrequencyPage;
class StrikeStatusPage;

class SettingsPage : public QWidget
{
    Q_OBJECT
  public:
    explicit SettingsPage(QWidget *parent = nullptr);

  private slots:
    void onLoginClicked();
    void handleLogout();

  public:
    uint8_t getCurrentLocationMode() const;
    int currentWarningRemoveTimeSeconds() const;

  private:
    void setupUi();
    void setupAuthView();
    void setupSettingsView();
    bool authenticateUser(const QString &password);
    void handlePasswordChangeRequest(SettingsUserRole role, const QString &oldPassword, const QString &newPassword);
    void resetSettingsContent();
    QLabel *createPlaceholderPage(const QString &text);
    void addSidebarPage(const QString &sidebarText, QWidget *page);
    void populateAdminSettingsPages();
    void populateRootSettingsPages();
    void showSettingsView();
    QWidget *createSidebarContainer();
    QListWidget *createSidebarList(QWidget *parent);
    QPushButton *createLogoutButton(QWidget *parent);
    void setupContentStack();
    void bindSettingsViewSignals(QPushButton *logoutBtn);
    void triggerDeviceSettingsRefresh();

    QStackedWidget *mainStack;

    // Auth View
    QWidget *authWidget;
    QLineEdit *pwdInput;
    QLabel *errorLabel;

    // Settings View
    QWidget *settingsWidget;
    QListWidget *sidebar;
    QStackedWidget *contentStack;
    DeviceSettingsPage *deviceSettingsPage;
    AlarmHistoryPage *alarmHistoryPage;
    AuthorizationInfoPage *authorizationInfoPage;
    DataCollectionPage *dataCollectionPage;
    AngleCalibrationPage *angleCalibrationPage;
    DirectionCalibrationValuePage *directionCalibrationValuePage;
    DetectBandPage *detectBandPage;
    FirmwareVersionPage *firmwareVersionPage;
    ModelLibraryPage *modelLibraryPage;
    ModeSelectPage *modeSelectPage;
    PowerAmplifierPage *powerAmplifierPage;
    SignalSourceParamsPage *signalSourceParamsPage;
    SystemLogPage *systemLogPage;
    SystemFunctionPage *systemFunctionPage;
    SpectrumSwitchPage *spectrumSwitchPage;
    StrikeFrequencyPage *strikeFrequencyPage;
    StrikeStatusPage *strikeStatusPage;
    SettingsUserRole currentUserRole;

  signals:
    void requestSaveGps(uint8_t mode, float lng, float lat, float alt);
    void requestSaveFullScan(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin, double att);
    void requestSaveDeviceIp(const QString &ip, int port, const QString &mask, const QString &route, const QString &dns);
    void requestSaveTcpServerIp(const QString &ip, int port);
    void requestSaveDetectBands(const QVector<DetectBandParam> &bands);
    void requestSaveDroneReportMode(uint8_t mode);
    void requestSaveJamMode(uint8_t mode);
    void requestSaveNetworkMode(uint8_t mode);
    void requestSaveUavCategoryDisplayMode(uint8_t mode);
    void requestSaveDataEnable(uint8_t enabled);
    void requestSaveFeatureModes(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled);
    void requestStartCompassCalibration();
    void requestFinishCompassCalibration();
    void requestConfirmCompassCalibration(uint16_t angle);
    void requestCancelCompassCalibration();
    void requestOpenSpectrogram();
    void requestCloseSpectrogram();
    void requestOpenSpectrum();
    void requestCloseSpectrum();
    void requestQueryAlarmHistory();
    void requestQueryDeviceUsageInfo();
    void requestSaveBuzzerEnabled(uint8_t enabled);
    void warningRemoveTimeChanged(int seconds);
    void requestSaveSystemTime(const QDateTime &dateTime, const QString &timezoneId);
    void requestSetScreenFlashEnabled(bool enabled);
    void requestRebootDevice();
    void requestSaveModelLibraryMode(uint8_t mode);
    void requestUpdateModelLibraryRecord(const ModelLibraryUpdateRequest &request);
    void requestSaveStrikeFrequencyBands(const StrikeFrequencyBandList &bands);
    void requestSavePowerAmplifierParams(const PowerAmplifierParamList &params);
    void requestSaveDirectionCalibrationValues(const DirectionCalibrationValueList &values);
    void requestSaveSignalSourceParams(int serialScan, const QVector<int> &scanModes, int signalMode, const QVector<int> &vcoScans);
    void requestUploadPatternFile(const PatternUploadRequest &request);
    void requestQueryGps();
    void requestQueryFullScan();
    void requestQueryDeviceIp();
    void requestQueryTcpServerIp();
    void requestQueryDetectBands();
    void requestQueryDroneReportMode();
    void requestQueryJamMode();
    void requestQueryNetworkMode();
    void requestQueryUavCategoryDisplayMode();
    void requestQueryDataEnable();
    void requestQueryFeatureModes();
    void requestQueryFirmwareVersions();
    void requestQueryStrikeFrequencyBands();
    void requestQueryStrikeStatus();
    void requestQueryPowerAmplifierParams();
    void requestQueryDirectionCalibrationValues();
    void requestQuerySignalSourceParams();
    void requestQueryBuzzerEnabled();
    void requestQueryScreenFlashEnabled();
    void requestQueryModelLibraryMode();
    void requestQueryModelLibraryRecords(int current, int size);

  public slots:
    // 提供给外部调用的接口，当页面被切走时强制退出登录
    void forceLogout();

    // 更新界面上显示的 GPS 数据 (响应 DataType=60)
    void updateGpsInfo(uint8_t mode, float lng, float lat, float alt);
    void updateGpsSaveResult(bool success, const QString &message);
    void updateFullScanSettings(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin, double att);
    void updateFullScanSaveResult(bool success, const QString &message);
    void updateDeviceIpSettings(const QString &ip, int port, const QString &mask, const QString &route,
                                const QString &dns);
    void updateDeviceIpSaveResult(bool success, const QString &message);
    void updateTcpServerIpSettings(const QString &ip, int port);
    void updateTcpServerIpSaveResult(bool success, const QString &message);
    void updateDetectBands(const QVector<DetectBandParam> &bands);
    void updateDetectBandSaveResult(bool success, const QString &message);
    void updateDroneReportMode(uint8_t mode);
    void updateDroneReportModeSaveResult(bool success, const QString &message);
    void updateJamMode(uint8_t mode);
    void updateJamModeSaveResult(bool success, const QString &message);
    void updateNetworkMode(uint8_t mode);
    void updateNetworkModeSaveResult(bool success, const QString &message);
    void updateUavCategoryDisplayMode(uint8_t mode);
    void updateUavCategoryDisplayModeSaveResult(bool success, const QString &message);
    void updateDataEnable(uint8_t enabled);
    void updateDataEnableSaveResult(bool success, const QString &message);
    void updateFeatureModes(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled);
    void updateFeatureModeSaveResult(bool success, const QString &message);
    void updateCompassCalibrationResult(uint16_t responseDataType, bool success, const QString &message);
    void updateSpectrumSwitchResult(uint16_t responseDataType, bool success, const QString &message);
    void updateSpectrumReport(const SpectrumReportData &reportData);
    void updateFullSpectrumSwitchResult(bool enabled, bool success, const QString &message);
    void updateFullSpectrumReport(const FullSpectrumReportData &reportData);
    void updateAlarmHistory(const AlarmHistoryInfo &info);
    void updateDeviceUsageInfo(const DeviceUsageInfo &info);
    void updateBuzzerEnabled(uint8_t enabled);
    void updateScreenFlashEnabled(bool enabled);
    void updateBuzzerEnabledSaveResult(bool success, const QString &message);
    void updateSystemTimeSaveResult(bool success, const QString &message);
    void updateRebootResult(bool success, const QString &message);
    void updateModelLibraryMode(uint8_t mode);
    void updateModelLibraryModeSaveResult(bool success, const QString &message);
    void updateModelLibraryRecords(const ModelLibraryPageResult &result);
    void updateModelLibraryRecordSaveResult(bool success, const QString &message);
    void updateStrikeFrequencyBands(const StrikeFrequencyBandList &bands);
    void updateStrikeFrequencySaveResult(bool success, const QString &message);
    void updatePowerAmplifierParams(const PowerAmplifierParamList &params);
    void updatePowerAmplifierSaveResult(bool success, const QString &message);
    void updateDirectionCalibrationValues(const DirectionCalibrationValueList &values);
    void updateDirectionCalibrationSaveResult(bool success, const QString &message);
    void updateStrikeStatus(const QVector<int> &switchStates);
    void updateSignalSourceParams(const SignalSourceParamsConfig &config);
    void updateSignalSourceParamsSaveResult(bool success, const QString &message);
    void updatePatternUploadResult(bool success, const QString &message);
    void updateFirmwareVersions(const QString &appVersion, const QString &fpgaVersion, const QString &gpuVersion);
    void updateFirmwareDeviceSerial(const QString &serialText);
    void onDeviceConnectionLost();
    void onDeviceConnectionRestored();
};

#endif // SETTINGS_PAGE_H
