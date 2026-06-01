#ifndef TCP_MANAGER_H
#define TCP_MANAGER_H

#include <QDateTime>
#include <QJsonObject>
#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include "protocol_types.h"

class TcpManager : public QObject
{
    Q_OBJECT
public:
    explicit TcpManager(QObject *parent = nullptr);
    ~TcpManager();

    void connectToServer(const QString &ip, quint16 port);
    void setReconnectIntervalMs(int intervalMs);
    void sendCommand(const QString &cmd); // 发送字符串（已弃用或作后备）
    bool isConnected() const;

    // 协议封装方法
    void sendFrame(uint16_t dataType, const QByteArray &data = QByteArray());

    // 具体的业务下发接口
    void rebootDevice();                                              // DataType 29
    void startCompassCalibration();                                   // DataType 31
    void finishCompassCalibration();                                  // DataType 33
    void confirmCompassCalibration(uint16_t angle);                   // DataType 35
    void cancelCompassCalibration();                                  // DataType 37
    void openSpectrogram();                                           // DataType 65
    void closeSpectrogram();                                          // DataType 67
    void openFullSpectrum();                                          // DataType 218 payload=1
    void closeFullSpectrum();                                         // DataType 218 payload=0
    void setStrikeFrequencyBands(const StrikeFrequencyBandList &bands); // DataType 96
    void queryStrikeFrequencyBands();                                  // DataType 98
    void setPowerAmplifierParams(const PowerAmplifierParamList &params); // DataType 118
    void queryPowerAmplifierParams();                                   // DataType 120
    void setDirectionCalibrationValues(const DirectionCalibrationValueList &values); // DataType 124
    void queryDirectionCalibrationValues();                                            // DataType 126
    void setSignalSourceParams(int serialScan, const QVector<int> &scanModes, int vcoMode, const QVector<int> &vcoScans);
    void querySignalSourceParams();
    void setDeviceJammingMode(int mode, int switchStatus);            // DataType 100
    void queryDeviceJammingMode();                                    // DataType 102
    void queryDeviceAlarmHistory();                                   // DataType 116
    void queryDeviceUsageInfo();                                      // DataType 136
    void setBuzzerEnabled(uint8_t enabled);                           // DataType 92
    void queryBuzzerEnabled();                                        // DataType 94
    void setModelLibraryMode(uint8_t mode);                           // DataType 201
    void queryModelLibraryMode();                                     // DataType 203
    void setModelLibraryRecord(const ModelLibraryUpdateRequest &request); // DataType 205
    void queryModelLibraryRecords(const ModelLibraryPageQuery &query);    // DataType 207
    void queryJammingBands();                                         // DataType 98
    void queryFirmwareVersions();                                     // DataType 14
    void setDeviceGps(uint8_t mode, float lng, float lat, float alt); // DataType 57
    void queryDeviceGps();                                            // DataType 59
    void setDetectBands(const QVector<DetectBandParam> &bands);       // DataType 8
    void queryDetectBands();                                          // DataType 10
    void setDroneReportMode(uint8_t mode);                            // DataType 61
    void queryDroneReportMode();                                      // DataType 63
    void setSuppressionMode(uint8_t mode);                            // DataType 130
    void querySuppressionMode();                                      // DataType 132
    void setO4ServerMode(uint8_t mode);                               // DataType 221
    void queryO4ServerMode();                                         // DataType 223
    void setUavCategoryDisplayMode(uint8_t mode);                     // DataType 181
    void queryUavCategoryDisplayMode();                               // DataType 183
    void setDataEnable(uint8_t enabled);                              // DataType 214
    void queryDataEnable();                                           // DataType 216
    void setFeatureModes(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled); // DataType 254
    void queryFeatureModes();                                                               // DataType 252
    void setFullScanParams(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin, double att);
    void queryFullScanParams();
    void setDeviceIp(const QString &ip, int port, const QString &mask, const QString &route, const QString &dns);
    void queryDeviceIp();
    void setTcpServerIp(const QString &ip, int port);
    void queryTcpServerIp();

signals:
    // 自定义信号，抛出给 MainWindow 调度
    void connected();
    void disconnected();
    void errorOccurred(const QString &errorStr);
    void dataReceived(const QString &msg);
    void deviceInfoParsed(const QJsonObject &deviceInfo);                 // 新增解析后的数据信号
    void firmwareVersionsQueried(const QString &appVersion, const QString &fpgaVersion, const QString &gpuVersion);
    void deviceGpsQueried(uint8_t mode, float lng, float lat, float alt); // DataType 60 解析后的信号
    void deviceGpsSetResponse(bool success, const QString &msg);          // DataType 58 响应信号
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
    void compassCalibrationResponse(uint16_t responseDataType, bool success, const QString &msg);
    void spectrogramSwitchResponse(uint16_t responseDataType, bool success, const QString &msg);
    void spectrumDataReported(const SpectrumReportData &reportData);
    void fullSpectrumSwitchResponse(bool enabled, bool success, const QString &msg);
    void fullSpectrumReported(const FullSpectrumReportData &reportData);
    void strikeFrequencyBandsSetResponse(bool success, const QString &msg);
    void strikeFrequencyBandsQueried(const StrikeFrequencyBandList &bands);
    void powerAmplifierParamsSetResponse(bool success, const QString &msg);
    void powerAmplifierParamsQueried(const PowerAmplifierParamList &params);
    void directionCalibrationValuesSetResponse(bool success, const QString &msg);
    void directionCalibrationValuesQueried(const DirectionCalibrationValueList &values);
    void alarmHistoryQueried(const AlarmHistoryInfo &info);
    void deviceUsageInfoQueried(const DeviceUsageInfo &info);
    void buzzerEnabledSetResponse(bool success, const QString &msg);
    void buzzerEnabledQueried(uint8_t enabled);
    void deviceRebootResponse(bool success, const QString &msg);
    void modelLibraryModeSetResponse(bool success, const QString &msg);
    void modelLibraryModeQueried(uint8_t mode);
    void modelLibraryRecordSetResponse(bool success, const QString &msg);
    void modelLibraryRecordsQueried(const ModelLibraryPageResult &result);
    void deviceJammingStatusQueried(const QVector<int> &switchStates);
    void signalSourceParamsSetResponse(bool success, const QString &msg);
    void signalSourceParamsQueried(const SignalSourceParamsConfig &config);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QTcpSocket::SocketError socketError);
    void onReconnectTimeout();

private:
    QTcpSocket *tcpSocket;
    QByteArray m_buffer; // 用于处理TCP粘包/半包的缓冲区
    QTimer *reconnectTimer;
    QString lastConnectedIp;
    quint16 lastConnectedPort = 0;
    uint16_t lastSentDataType = 0;
    uint16_t lastReceivedDataType = 0;
    int lastSentFrameLength = 0;
    int lastReceivedFrameLength = 0;
    bool pendingFullSpectrumEnabled = false;
    int reconnectIntervalMs = 1000;
    QDateTime lastConnectAttemptAt;
    QDateTime lastConnectedAt;
    QDateTime lastDisconnectedAt;
    QDateTime lastSentAt;
    QDateTime lastReceivedAt;
    QTcpSocket::SocketError lastSocketError = QAbstractSocket::UnknownSocketError;
    QString lastSocketErrorText;

    void parseBuffer();
    void dispatchProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchDeviceBaseProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchSystemProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchAngleCalibrationProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchSpectrumSwitchProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchStrikeFrequencyProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchPowerAmplifierProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchDirectionCalibrationProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchSignalSourceParamsProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchFirmwareProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchDeviceOpsProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchGpsProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchDetectBandProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchModeSelectProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    bool dispatchNetworkConfigProtocol(const ProtocolHeader *header, const QByteArray &frameData);
    void handleCompassCalibrationResponse(uint16_t responseDataType, const QByteArray &frameData);
    void handleDeviceInfo(const ProtocolHeader *header, const QByteArray &frameData);
    void handleDeviceStatusInfo(const ProtocolHeader *header, const QByteArray &frameData);
    void handleFirmwareVersionQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleGpsSettingResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleGpsQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleDetectBandSettingResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleDetectBandQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleDroneReportModeSettingResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleDroneReportModeQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleSuppressionModeSettingResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleSuppressionModeQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleO4ServerModeSettingResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleO4ServerModeQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleUavCategoryDisplayModeSettingResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleUavCategoryDisplayModeQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleDataEnableSettingResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleDataEnableQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleFeatureModesSettingResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleFeatureModesQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleFullScanSettingResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleFullScanQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleDeviceIpSettingResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleDeviceIpQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleTcpServerIpSettingResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleTcpServerIpQueryResponse(const ProtocolHeader *header, const QByteArray &frameData);
    void handleSpectrogramSwitchResponse(uint16_t responseDataType, const QByteArray &frameData);
    void handleSpectrumDataReport(const ProtocolHeader *header, const QByteArray &frameData);
    void handleFullSpectrumSwitchResponse(bool enabled, const QByteArray &frameData);
    void handleFullSpectrumReport(const ProtocolHeader *header, const QByteArray &frameData);
    void handleStrikeFrequencySetResponse(const QByteArray &frameData);
    void handleStrikeFrequencyQueryResponse(const QByteArray &frameData);
    void handlePowerAmplifierSetResponse(const QByteArray &frameData);
    void handlePowerAmplifierQueryResponse(const QByteArray &frameData);
    void handleDirectionCalibrationSetResponse(const QByteArray &frameData);
    void handleDirectionCalibrationQueryResponse(const QByteArray &frameData);
    void handleSignalSourceParamsSetResponse(const QByteArray &frameData);
    void handleSignalSourceParamsQueryResponse(const QByteArray &frameData);
    void rememberSentFrame(uint16_t dataType, int frameLength);
    void rememberReceivedFrame(uint16_t dataType, int frameLength);
    QString socketStateText() const;
    int nextReconnectDelayMs() const;
};

#endif // TCP_MANAGER_H
