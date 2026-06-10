#ifndef SETTINGS_PROTOCOL_SERVICE_H
#define SETTINGS_PROTOCOL_SERVICE_H

#include "network/core/protocol_types.h"

#include <QObject>
#include <functional>

class SettingsProtocolService : public QObject
{
    Q_OBJECT

public:
    using FrameSender = std::function<void(uint16_t, const QByteArray &)>;

    explicit SettingsProtocolService(QObject *parent = nullptr);

    void setFrameSender(const FrameSender &sender);

    void setDeviceGps(uint8_t mode, float lng, float lat, float alt);
    void queryDeviceGps();
    void setDetectBands(const QVector<DetectBandParam> &bands);
    void queryDetectBands();
    void setDroneReportMode(uint8_t mode);
    void queryDroneReportMode();
    void setSuppressionMode(uint8_t mode);
    void querySuppressionMode();
    void setO4ServerMode(uint8_t mode);
    void queryO4ServerMode();
    void setUavCategoryDisplayMode(uint8_t mode);
    void queryUavCategoryDisplayMode();
    void setDataEnable(uint8_t enabled);
    void queryDataEnable();
    void setFeatureModes(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled);
    void queryFeatureModes();
    void setFullScanParams(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin, double att);
    void queryFullScanParams();
    void setDeviceIp(const QString &ip, int port, const QString &mask, const QString &route, const QString &dns);
    void queryDeviceIp();
    void setTcpServerIp(const QString &ip, int port);
    void queryTcpServerIp();
    void setModelLibraryMode(uint8_t mode);
    void queryModelLibraryMode();
    void setModelLibraryRecord(const ModelLibraryUpdateRequest &request);
    void queryModelLibraryRecords(const ModelLibraryPageQuery &query);
    void setStrikeFrequencyBands(const StrikeFrequencyBandList &bands);
    void queryStrikeFrequencyBands();
    void setPowerAmplifierParams(const PowerAmplifierParamList &params);
    void queryPowerAmplifierParams();
    void setDirectionCalibrationValues(const DirectionCalibrationValueList &values);
    void queryDirectionCalibrationValues();
    void setSignalSourceParams(int serialScan, const QVector<int> &scanModes, int vcoMode, const QVector<int> &vcoScans);
    void querySignalSourceParams();
    void uploadPatternFile(const PatternUploadRequest &request);
    void queryFirmwareVersions();

    void handleGpsSettingResponse(const QByteArray &frameData);
    void handleGpsQueryResponse(const QByteArray &frameData);
    void handleDetectBandSettingResponse(const QByteArray &frameData);
    void handleDetectBandQueryResponse(const QByteArray &frameData);
    void handleDroneReportModeSettingResponse(const QByteArray &frameData);
    void handleDroneReportModeQueryResponse(const QByteArray &frameData);
    void handleSuppressionModeSettingResponse(const QByteArray &frameData);
    void handleSuppressionModeQueryResponse(const QByteArray &frameData);
    void handleO4ServerModeSettingResponse(const QByteArray &frameData);
    void handleO4ServerModeQueryResponse(const QByteArray &frameData);
    void handleUavCategoryDisplayModeSettingResponse(const QByteArray &frameData);
    void handleUavCategoryDisplayModeQueryResponse(const QByteArray &frameData);
    void handleDataEnableSettingResponse(const QByteArray &frameData);
    void handleDataEnableQueryResponse(const QByteArray &frameData);
    void handleFeatureModesSettingResponse(const QByteArray &frameData);
    void handleFeatureModesQueryResponse(const QByteArray &frameData);
    void handleFullScanSettingResponse(const QByteArray &frameData);
    void handleFullScanQueryResponse(const QByteArray &frameData);
    void handleDeviceIpSettingResponse(const QByteArray &frameData);
    void handleDeviceIpQueryResponse(const QByteArray &frameData);
    void handleTcpServerIpSettingResponse(const QByteArray &frameData);
    void handleTcpServerIpQueryResponse(const QByteArray &frameData);
    void handleModelLibraryModeSettingResponse(const QByteArray &frameData);
    void handleModelLibraryModeQueryResponse(const QByteArray &frameData);
    void handleModelLibraryRecordSetResponse(const QByteArray &frameData);
    void handleModelLibraryRecordsQueryResponse(const QByteArray &frameData);
    void handleStrikeFrequencySettingResponse(const QByteArray &frameData);
    void handleStrikeFrequencyQueryResponse(const QByteArray &frameData);
    void handlePowerAmplifierSettingResponse(const QByteArray &frameData);
    void handlePowerAmplifierQueryResponse(const QByteArray &frameData);
    void handleDirectionCalibrationSettingResponse(const QByteArray &frameData);
    void handleDirectionCalibrationQueryResponse(const QByteArray &frameData);
    void handleSignalSourceParamsSettingResponse(const QByteArray &frameData);
    void handleSignalSourceParamsQueryResponse(const QByteArray &frameData);
    void handlePatternUploadResponse(const QByteArray &frameData);
    void handleFirmwareVersionsQueryResponse(const QByteArray &frameData);

signals:
    void deviceGpsQueried(uint8_t mode, float lng, float lat, float alt);
    void deviceGpsSetResponse(bool success, const QString &msg);
    void detectBandsSetResponse(bool success, const QString &msg);
    void detectBandsQueried(const QVector<DetectBandParam> &bands);
    void droneReportModeSetResponse(bool success, const QString &msg);
    void droneReportModeQueried(uint8_t mode);
    void suppressionModeSetResponse(bool success, const QString &msg);
    void suppressionModeQueried(uint8_t mode);
    void o4ServerModeSetResponse(bool success, const QString &msg);
    void o4ServerModeQueried(uint8_t mode);
    void uavCategoryDisplayModeSetResponse(bool success, const QString &msg);
    void uavCategoryDisplayModeQueried(uint8_t mode);
    void dataEnableSetResponse(bool success, const QString &msg);
    void dataEnableQueried(uint8_t enabled);
    void featureModesSetResponse(bool success, const QString &msg);
    void featureModesQueried(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled);
    void fullScanParamsSetResponse(bool success, const QString &msg);
    void fullScanParamsQueried(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin, double att);
    void deviceIpSetResponse(bool success, const QString &msg);
    void deviceIpQueried(const QString &ip, int port, const QString &mask, const QString &route, const QString &dns);
    void tcpServerIpSetResponse(bool success, const QString &msg);
    void tcpServerIpQueried(const QString &ip, int port);
    void modelLibraryModeSetResponse(bool success, const QString &msg);
    void modelLibraryModeQueried(uint8_t mode);
    void modelLibraryRecordSetResponse(bool success, const QString &msg);
    void modelLibraryRecordsQueried(const ModelLibraryPageResult &result);
    void strikeFrequencyBandsSetResponse(bool success, const QString &msg);
    void strikeFrequencyBandsQueried(const StrikeFrequencyBandList &bands);
    void powerAmplifierParamsSetResponse(bool success, const QString &msg);
    void powerAmplifierParamsQueried(const PowerAmplifierParamList &params);
    void directionCalibrationValuesSetResponse(bool success, const QString &msg);
    void directionCalibrationValuesQueried(const DirectionCalibrationValueList &values);
    void signalSourceParamsSetResponse(bool success, const QString &msg);
    void signalSourceParamsQueried(const SignalSourceParamsConfig &config);
    void patternUploadResponse(bool success, const QString &msg);
    void firmwareVersionsQueried(const QString &appVersion, const QString &fpgaVersion, const QString &gpuVersion);

private:
    FrameSender sendFrame_;
};

#endif // SETTINGS_PROTOCOL_SERVICE_H
