#include "settings_coordinator.h"

#include "network/core/tcp_manager.h"
#include "services/local_time_service_client.h"
#include "views/settings/settings_page.h"

#include <QJsonObject>

SettingsCoordinator::SettingsCoordinator(SettingsPage *settingsPage, TcpManager *tcpManager,
                                         LocalTimeServiceClient *localTimeServiceClient, QObject *parent)
    : QObject(parent), settingsPage_(settingsPage), tcpManager_(tcpManager), localTimeServiceClient_(localTimeServiceClient)
{
}

void SettingsCoordinator::setupConnections()
{
    if (!settingsPage_ || !tcpManager_ || !localTimeServiceClient_)
    {
        return;
    }

    connect(tcpManager_, &TcpManager::connected, settingsPage_, &SettingsPage::onDeviceConnectionRestored);
    connect(tcpManager_, &TcpManager::disconnected, settingsPage_, &SettingsPage::onDeviceConnectionLost);

    connect(tcpManager_, &TcpManager::firmwareVersionsQueried, settingsPage_, &SettingsPage::updateFirmwareVersions);
    connect(tcpManager_, &TcpManager::deviceGpsQueried, settingsPage_, &SettingsPage::updateGpsInfo);
    connect(tcpManager_, &TcpManager::deviceGpsSetResponse, settingsPage_, &SettingsPage::updateGpsSaveResult);
    connect(tcpManager_, &TcpManager::detectBandsSetResponse, settingsPage_, &SettingsPage::updateDetectBandSaveResult);
    connect(tcpManager_, &TcpManager::detectBandsQueried, settingsPage_, &SettingsPage::updateDetectBands);
    connect(tcpManager_, &TcpManager::droneReportModeSetResponse, settingsPage_,
            &SettingsPage::updateDroneReportModeSaveResult);
    connect(tcpManager_, &TcpManager::droneReportModeQueried, settingsPage_, &SettingsPage::updateDroneReportMode);
    connect(tcpManager_, &TcpManager::suppressionModeSetResponse, settingsPage_, &SettingsPage::updateJamModeSaveResult);
    connect(tcpManager_, &TcpManager::suppressionModeQueried, settingsPage_, &SettingsPage::updateJamMode);
    connect(tcpManager_, &TcpManager::o4ServerModeSetResponse, settingsPage_, &SettingsPage::updateNetworkModeSaveResult);
    connect(tcpManager_, &TcpManager::o4ServerModeQueried, settingsPage_, &SettingsPage::updateNetworkMode);
    connect(tcpManager_, &TcpManager::uavCategoryDisplayModeSetResponse, settingsPage_,
            &SettingsPage::updateUavCategoryDisplayModeSaveResult);
    connect(tcpManager_, &TcpManager::uavCategoryDisplayModeQueried, settingsPage_,
            &SettingsPage::updateUavCategoryDisplayMode);
    connect(tcpManager_, &TcpManager::dataEnableSetResponse, settingsPage_, &SettingsPage::updateDataEnableSaveResult);
    connect(tcpManager_, &TcpManager::dataEnableQueried, settingsPage_, &SettingsPage::updateDataEnable);
    connect(tcpManager_, &TcpManager::featureModesSetResponse, settingsPage_, &SettingsPage::updateFeatureModeSaveResult);
    connect(tcpManager_, &TcpManager::featureModesQueried, settingsPage_, &SettingsPage::updateFeatureModes);
    connect(tcpManager_, &TcpManager::compassCalibrationResponse, settingsPage_,
            &SettingsPage::updateCompassCalibrationResult);
    connect(tcpManager_, &TcpManager::spectrogramSwitchResponse, settingsPage_, &SettingsPage::updateSpectrumSwitchResult);
    connect(tcpManager_, &TcpManager::spectrumDataReported, settingsPage_, &SettingsPage::updateSpectrumReport);
    connect(tcpManager_, &TcpManager::fullSpectrumSwitchResponse, settingsPage_,
            &SettingsPage::updateFullSpectrumSwitchResult);
    connect(tcpManager_, &TcpManager::fullSpectrumReported, settingsPage_, &SettingsPage::updateFullSpectrumReport);
    connect(tcpManager_, &TcpManager::alarmHistoryQueried, settingsPage_, &SettingsPage::updateAlarmHistory);
    connect(tcpManager_, &TcpManager::deviceUsageInfoQueried, settingsPage_, &SettingsPage::updateDeviceUsageInfo);
    connect(tcpManager_, &TcpManager::buzzerEnabledQueried, settingsPage_, &SettingsPage::updateBuzzerEnabled);
    connect(tcpManager_, &TcpManager::buzzerEnabledSetResponse, settingsPage_, &SettingsPage::updateBuzzerEnabledSaveResult);
    connect(tcpManager_, &TcpManager::deviceRebootResponse, settingsPage_, &SettingsPage::updateRebootResult);
    connect(tcpManager_, &TcpManager::modelLibraryModeQueried, settingsPage_, &SettingsPage::updateModelLibraryMode);
    connect(tcpManager_, &TcpManager::modelLibraryModeSetResponse, settingsPage_,
            &SettingsPage::updateModelLibraryModeSaveResult);
    connect(tcpManager_, &TcpManager::modelLibraryRecordsQueried, settingsPage_, &SettingsPage::updateModelLibraryRecords);
    connect(tcpManager_, &TcpManager::modelLibraryRecordSetResponse, settingsPage_,
            &SettingsPage::updateModelLibraryRecordSaveResult);
    connect(tcpManager_, &TcpManager::strikeFrequencyBandsQueried, settingsPage_,
            &SettingsPage::updateStrikeFrequencyBands);
    connect(tcpManager_, &TcpManager::strikeFrequencyBandsSetResponse, settingsPage_,
            &SettingsPage::updateStrikeFrequencySaveResult);
    connect(tcpManager_, &TcpManager::powerAmplifierParamsQueried, settingsPage_,
            &SettingsPage::updatePowerAmplifierParams);
    connect(tcpManager_, &TcpManager::powerAmplifierParamsSetResponse, settingsPage_,
            &SettingsPage::updatePowerAmplifierSaveResult);
    connect(tcpManager_, &TcpManager::directionCalibrationValuesQueried, settingsPage_,
            &SettingsPage::updateDirectionCalibrationValues);
    connect(tcpManager_, &TcpManager::directionCalibrationValuesSetResponse, settingsPage_,
            &SettingsPage::updateDirectionCalibrationSaveResult);
    connect(tcpManager_, &TcpManager::deviceJammingStatusQueried, settingsPage_, &SettingsPage::updateStrikeStatus);
    connect(tcpManager_, &TcpManager::signalSourceParamsQueried, settingsPage_, &SettingsPage::updateSignalSourceParams);
    connect(tcpManager_, &TcpManager::signalSourceParamsSetResponse, settingsPage_,
            &SettingsPage::updateSignalSourceParamsSaveResult);
    connect(tcpManager_, &TcpManager::patternUploadResponse, settingsPage_, &SettingsPage::updatePatternUploadResult);
    connect(tcpManager_, &TcpManager::fullScanParamsQueried, settingsPage_, &SettingsPage::updateFullScanSettings);
    connect(tcpManager_, &TcpManager::fullScanParamsSetResponse, settingsPage_, &SettingsPage::updateFullScanSaveResult);
    connect(tcpManager_, &TcpManager::deviceIpQueried, settingsPage_, &SettingsPage::updateDeviceIpSettings);
    connect(tcpManager_, &TcpManager::deviceIpSetResponse, settingsPage_, &SettingsPage::updateDeviceIpSaveResult);
    connect(tcpManager_, &TcpManager::tcpServerIpQueried, settingsPage_, &SettingsPage::updateTcpServerIpSettings);
    connect(tcpManager_, &TcpManager::tcpServerIpSetResponse, settingsPage_, &SettingsPage::updateTcpServerIpSaveResult);

    connect(settingsPage_, &SettingsPage::requestSaveGps, tcpManager_, &TcpManager::setDeviceGps);
    connect(settingsPage_, &SettingsPage::requestSaveDetectBands, tcpManager_, &TcpManager::setDetectBands);
    connect(settingsPage_, &SettingsPage::requestSaveDroneReportMode, tcpManager_, &TcpManager::setDroneReportMode);
    connect(settingsPage_, &SettingsPage::requestSaveJamMode, tcpManager_, &TcpManager::setSuppressionMode);
    connect(settingsPage_, &SettingsPage::requestSaveNetworkMode, tcpManager_, &TcpManager::setO4ServerMode);
    connect(settingsPage_, &SettingsPage::requestSaveUavCategoryDisplayMode, tcpManager_,
            &TcpManager::setUavCategoryDisplayMode);
    connect(settingsPage_, &SettingsPage::requestSaveDataEnable, tcpManager_, &TcpManager::setDataEnable);
    connect(settingsPage_, &SettingsPage::requestSaveFeatureModes, tcpManager_, &TcpManager::setFeatureModes);
    connect(settingsPage_, &SettingsPage::requestStartCompassCalibration, tcpManager_, &TcpManager::startCompassCalibration);
    connect(settingsPage_, &SettingsPage::requestFinishCompassCalibration, tcpManager_, &TcpManager::finishCompassCalibration);
    connect(settingsPage_, &SettingsPage::requestConfirmCompassCalibration, tcpManager_,
            &TcpManager::confirmCompassCalibration);
    connect(settingsPage_, &SettingsPage::requestCancelCompassCalibration, tcpManager_, &TcpManager::cancelCompassCalibration);
    connect(settingsPage_, &SettingsPage::requestOpenSpectrogram, tcpManager_, &TcpManager::openSpectrogram);
    connect(settingsPage_, &SettingsPage::requestCloseSpectrogram, tcpManager_, &TcpManager::closeSpectrogram);
    connect(settingsPage_, &SettingsPage::requestOpenSpectrum, tcpManager_, &TcpManager::openFullSpectrum);
    connect(settingsPage_, &SettingsPage::requestCloseSpectrum, tcpManager_, &TcpManager::closeFullSpectrum);
    connect(settingsPage_, &SettingsPage::requestQueryAlarmHistory, tcpManager_, &TcpManager::queryDeviceAlarmHistory);
    connect(settingsPage_, &SettingsPage::requestQueryDeviceUsageInfo, tcpManager_, &TcpManager::queryDeviceUsageInfo);
    connect(settingsPage_, &SettingsPage::requestSaveBuzzerEnabled, tcpManager_, &TcpManager::setBuzzerEnabled);
    connect(settingsPage_, &SettingsPage::requestSaveSystemTime, this,
            [this](const QDateTime &dateTime, const QString &timezoneId)
            {
                const LocalTimeServiceResult result = localTimeServiceClient_->setSystemTime(dateTime, timezoneId);
                settingsPage_->updateSystemTimeSaveResult(result.success, result.message);
            });
    connect(settingsPage_, &SettingsPage::requestRebootDevice, tcpManager_, &TcpManager::rebootDevice);
    connect(settingsPage_, &SettingsPage::requestSaveModelLibraryMode, tcpManager_, &TcpManager::setModelLibraryMode);
    connect(settingsPage_, &SettingsPage::requestUpdateModelLibraryRecord, tcpManager_, &TcpManager::setModelLibraryRecord);
    connect(settingsPage_, &SettingsPage::requestSaveStrikeFrequencyBands, tcpManager_,
            &TcpManager::setStrikeFrequencyBands);
    connect(settingsPage_, &SettingsPage::requestSavePowerAmplifierParams, tcpManager_,
            &TcpManager::setPowerAmplifierParams);
    connect(settingsPage_, &SettingsPage::requestSaveDirectionCalibrationValues, tcpManager_,
            &TcpManager::setDirectionCalibrationValues);
    connect(settingsPage_, &SettingsPage::requestSaveSignalSourceParams, tcpManager_,
            &TcpManager::setSignalSourceParams);
    connect(settingsPage_, &SettingsPage::requestUploadPatternFile, tcpManager_, &TcpManager::uploadPatternFile);
    connect(settingsPage_, &SettingsPage::requestSaveFullScan, tcpManager_, &TcpManager::setFullScanParams);
    connect(settingsPage_, &SettingsPage::requestSaveDeviceIp, tcpManager_, &TcpManager::setDeviceIp);
    connect(settingsPage_, &SettingsPage::requestSaveTcpServerIp, tcpManager_, &TcpManager::setTcpServerIp);
    connect(settingsPage_, &SettingsPage::requestQueryGps, tcpManager_, &TcpManager::queryDeviceGps);
    connect(settingsPage_, &SettingsPage::requestQueryDetectBands, tcpManager_, &TcpManager::queryDetectBands);
    connect(settingsPage_, &SettingsPage::requestQueryDroneReportMode, tcpManager_, &TcpManager::queryDroneReportMode);
    connect(settingsPage_, &SettingsPage::requestQueryJamMode, tcpManager_, &TcpManager::querySuppressionMode);
    connect(settingsPage_, &SettingsPage::requestQueryNetworkMode, tcpManager_, &TcpManager::queryO4ServerMode);
    connect(settingsPage_, &SettingsPage::requestQueryUavCategoryDisplayMode, tcpManager_,
            &TcpManager::queryUavCategoryDisplayMode);
    connect(settingsPage_, &SettingsPage::requestQueryDataEnable, tcpManager_, &TcpManager::queryDataEnable);
    connect(settingsPage_, &SettingsPage::requestQueryFeatureModes, tcpManager_, &TcpManager::queryFeatureModes);
    connect(settingsPage_, &SettingsPage::requestQueryFirmwareVersions, tcpManager_, &TcpManager::queryFirmwareVersions);
    connect(settingsPage_, &SettingsPage::requestQueryStrikeFrequencyBands, tcpManager_,
            &TcpManager::queryStrikeFrequencyBands);
    connect(settingsPage_, &SettingsPage::requestQueryStrikeStatus, tcpManager_, &TcpManager::queryDeviceJammingMode);
    connect(settingsPage_, &SettingsPage::requestQueryPowerAmplifierParams, tcpManager_,
            &TcpManager::queryPowerAmplifierParams);
    connect(settingsPage_, &SettingsPage::requestQueryDirectionCalibrationValues, tcpManager_,
            &TcpManager::queryDirectionCalibrationValues);
    connect(settingsPage_, &SettingsPage::requestQuerySignalSourceParams, tcpManager_,
            &TcpManager::querySignalSourceParams);
    connect(settingsPage_, &SettingsPage::requestQueryBuzzerEnabled, tcpManager_, &TcpManager::queryBuzzerEnabled);
    connect(settingsPage_, &SettingsPage::requestQueryModelLibraryMode, tcpManager_, &TcpManager::queryModelLibraryMode);
    connect(settingsPage_, &SettingsPage::requestQueryModelLibraryRecords, this,
            [this](int current, int size)
            {
                ModelLibraryPageQuery query;
                query.current = current;
                query.size = size;
                tcpManager_->queryModelLibraryRecords(query);
            });
    connect(settingsPage_, &SettingsPage::requestQueryFullScan, tcpManager_, &TcpManager::queryFullScanParams);
    connect(settingsPage_, &SettingsPage::requestQueryDeviceIp, tcpManager_, &TcpManager::queryDeviceIp);
    connect(settingsPage_, &SettingsPage::requestQueryTcpServerIp, tcpManager_, &TcpManager::queryTcpServerIp);
}

void SettingsCoordinator::handleDeviceInfo(const QJsonObject &deviceInfo)
{
    if (!settingsPage_)
    {
        return;
    }

    if (deviceInfo.value(QStringLiteral("protocolDataType")).toInt() != 2)
    {
        return;
    }

    const QString deviceSerial = deviceInfo.value(QStringLiteral("deviceName")).toString().trimmed();
    settingsPage_->updateFirmwareDeviceSerial(deviceSerial);
}
