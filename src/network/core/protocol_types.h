#ifndef PROTOCOL_TYPES_H
#define PROTOCOL_TYPES_H

#include <QMetaType>
#include <QString>
#include <QVector>
#include <cstdint>

// 为了确保网络传输时内存严丝合缝，禁用结构体对齐优化
#pragma pack(push, 1)

// 帧头 (29 bytes)
struct ProtocolHeader
{
    uint32_t startFlag; // 0xEEEEEEEE
    uint16_t version;
    uint32_t length;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t millisecond;
    uint16_t dataType;
    uint64_t packageNum;
};

// 监测设备信息负载 (74 bytes) - 旧版协议 (DataType=1)
struct DeviceInfoPayload
{
    uint32_t deviceId;
    char deviceName[20];
    float longitude;
    float latitude;
    float altitude;
    uint16_t workStatus;
    float azimuth;
    float pitch;
    char runStatus[4];
    uint32_t antennaCoverage;
    uint32_t receiverId;
    float temperature;
    float humidity;
    uint32_t signalParam3;
    uint32_t stationNum;
};

// 设备状态信息上报 (183 bytes) - DataType=2
struct DeviceStatusPayload
{
    uint32_t deviceId;                          //监测设备ID
    char deviceName[20];                        //设备名称
    char deviceType[4];                         //设备类型
    char firmwareVersion[20];                  //固件版本
    char fpgaVersion[20];                       //FPGA版本
    char gpuVersion[20];                        //GPU版本
    float longitude;                          //经度
    float latitude;                           //纬度
    int32_t altitude;                         //海拔
    int32_t azimuth;                          //方位角
    float pitch;                            //俯仰角
    int32_t temperature;                    //温度
    int32_t detectRadius;                     //检测半径
    int32_t jammingAngleRange;                //干扰角度范围
    int32_t jammingDistance;                  //干扰距离
    uint8_t communicationJammingStatus;       //通信干扰状态
    uint8_t navigationJammingStatus;          //导航干扰状态
    char bandSwitchStatus[8];                 //频段切换状态
    uint8_t detectStatus;                     //检测状态
    uint8_t powerSupplyMode;                  //电源模式
    uint8_t batteryLevel;                     //电池等级
    uint8_t fanAlarm;                         //风扇报警
    uint8_t clockAlarm;                       //时钟报警
    uint8_t receiverPllAlarm;                 //接收器PLL报警
    uint8_t transmitterPllAlarm;                //发射器PLL报警
    uint8_t eepromAlarm;                      //EEPROM报警
    uint8_t temperatureChipAlarm;             //温度芯片报警
    uint8_t compassAlarm;                     //指南针报警
    uint8_t adcAlarm;                         //ADC报警
    char pa485Alarm[8];                        //PA485报警
    char paUnderpowerAlarm[8];              //PA欠压报警
    char paOverpowerAlarm[8];               //PA过压报警
    uint8_t simCardStatus;                    //Sim卡插卡状态
    uint8_t fourGNetworkStatus;               //4G模块联网状态
    uint8_t fourGSignalQuality;               //4G模块信号质量
    uint8_t fourGNetworkType;                 //4G模块网络连接方式
    char operatorId[10];                      //运营商id
};

// 帧尾 (5 bytes)
struct ProtocolTail
{
    uint8_t checksum;
    uint32_t endFlag; // 0xAAAAAAAA
};

// GPS设置与查询负载 (DataType = 57, 60) (13 bytes)
struct GpsSettingPayload
{
    uint8_t mode;    // GPS选择模式 (0: 手动 1: 自动)
    float longitude; // 设备所在经度
    float latitude;  // 设备所在纬度
    float altitude;  // 设备所在海拔
};

struct DetectBandParam
{
    float freqMhz;
    int32_t measureCount;
    int32_t gain;
};

struct SpectrumGroupData
{
    float centerFreqMhz = 0.0f;
    QVector<QVector<qint16>> matrix; // [timeIndex][freqIndex]
};

struct SpectrumReportData
{
    uint16_t totalPacketCount = 0;
    uint16_t packetIndex = 0;
    float centerFreqMhz = 0.0f;
    QVector<SpectrumGroupData> groups;
};

struct FullSpectrumReportData
{
    double startMhz = 300.0;
    double endMhz = 6000.0;
    QVector<int> data;
    QVector<int> markerIndices;
};

struct StrikeFrequencyBandConfig
{
    int enable = 0;
    double startMhz = 0.0;
    double endMhz = 0.0;
    int att = 0;
    double minMhz = 0.0;
    double maxMhz = 0.0;
    double filterStartMhz = 0.0;
    double filterEndMhz = 0.0;
    int modify = 1;
    int group = 0;
    int range = 0;
    int navigation = 0;
};

using StrikeFrequencyBandList = QVector<StrikeFrequencyBandConfig>;

struct PowerAmplifierParam
{
    double k = 0.0;
    double b = 0.0;
    double outpower = 0.0;
    double att = 0.0;
};

using PowerAmplifierParamList = QVector<PowerAmplifierParam>;
using DirectionCalibrationValueList = QVector<float>;

struct AlarmHistoryInfo
{
    int deviceTemperature = 0;
    int fanAlarm = 0;
    int clockAlarm = 0;
    int receiverPllAlarm = 0;
    int transmitterPllAlarm = 0;
    int adcAlarm = 0;
    int eepromAlarm = 0;
    int temperatureChipAlarm = 0;
    int compassAlarm = 0;
    QVector<int> paSerialAlarms = QVector<int>(6, 0);
    QVector<int> paOverpowerStatus = QVector<int>(6, 0);
    QVector<int> paOverpowerCounts = QVector<int>(6, 0);
    QVector<int> paUnderpowerStatus = QVector<int>(6, 0);
    QVector<int> paUnderpowerCounts = QVector<int>(6, 0);
    int serverConnectionStatus = 0;
};

struct DeviceUsageInfo
{
    int limit = 0;
    int remainingTimeSeconds = 0;
    int remainingCount = 0;
};

struct SignalSourceParamsConfig
{
    int serialScan = 0;
    QVector<int> scanModes = QVector<int>(6, 0);
    int vcoMode = 0;
    QVector<int> vcoScans = QVector<int>(6, 0);
};

struct PatternUploadRequest
{
    QString ip;
    int port = 21;
    QString user;
    QString password;
    QString path;
    int time = 4;
    int type = 1;
    QString filename;
    int channel = 1;
    double freq = 0.0;
};

struct ModelLibraryFreqBand
{
    int start = 0;
    int end = 0;
};

struct ModelLibraryRecord
{
    int type = 0;
    QString name;
    int sensitivity = 1;
    int enable = 0;
    QVector<ModelLibraryFreqBand> freqbands;
};

struct ModelLibraryPageQuery
{
    int current = 1;
    int size = 10;
};

struct ModelLibraryPageResult
{
    int total = 0;
    int current = 1;
    int size = 10;
    QVector<ModelLibraryRecord> records;
};

struct ModelLibraryUpdateRequest
{
    int deleteFlag = 0;
    ModelLibraryRecord record;
};

#pragma pack(pop)

Q_DECLARE_METATYPE(DetectBandParam)
Q_DECLARE_METATYPE(QVector<DetectBandParam>)
Q_DECLARE_METATYPE(SpectrumGroupData)
Q_DECLARE_METATYPE(SpectrumReportData)
Q_DECLARE_METATYPE(FullSpectrumReportData)
Q_DECLARE_METATYPE(StrikeFrequencyBandConfig)
Q_DECLARE_METATYPE(StrikeFrequencyBandList)
Q_DECLARE_METATYPE(PowerAmplifierParam)
Q_DECLARE_METATYPE(PowerAmplifierParamList)
Q_DECLARE_METATYPE(DirectionCalibrationValueList)
Q_DECLARE_METATYPE(AlarmHistoryInfo)
Q_DECLARE_METATYPE(DeviceUsageInfo)
Q_DECLARE_METATYPE(SignalSourceParamsConfig)
Q_DECLARE_METATYPE(PatternUploadRequest)
Q_DECLARE_METATYPE(ModelLibraryFreqBand)
Q_DECLARE_METATYPE(QVector<ModelLibraryFreqBand>)
Q_DECLARE_METATYPE(ModelLibraryRecord)
Q_DECLARE_METATYPE(QVector<ModelLibraryRecord>)
Q_DECLARE_METATYPE(ModelLibraryPageQuery)
Q_DECLARE_METATYPE(ModelLibraryPageResult)
Q_DECLARE_METATYPE(ModelLibraryUpdateRequest)

#endif // PROTOCOL_TYPES_H
