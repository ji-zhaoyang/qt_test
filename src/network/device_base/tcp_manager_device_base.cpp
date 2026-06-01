#include "tcp_manager.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

namespace
{
QString readFixedUtf8String(const char *data, int maxLen)
{
    int len = 0;
    while (len < maxLen && data[len] != '\0')
    {
        ++len;
    }
    return QString::fromUtf8(data, len).trimmed();
}

QJsonArray byteFlagsToJsonArray(const char *data, int length)
{
    QJsonArray array;
    for (int i = 0; i < length; ++i)
    {
        array.append(static_cast<int>(static_cast<unsigned char>(data[i])));
    }
    return array;
}

QString simCardStatusText(uint8_t value)
{
    switch (value)
    {
    case 0:
        return "模块异常";
    case 1:
        return "模块正常未插卡";
    case 2:
        return "模块正常已插卡";
    default:
        return QString("未知(%1)").arg(value);
    }
}
} // namespace

bool TcpManager::dispatchDeviceBaseProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 1:
        handleDeviceInfo(header, frameData);
        return true;
    case 2:
        handleDeviceStatusInfo(header, frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::handleDeviceInfo(const ProtocolHeader *header, const QByteArray &frameData)
{
    uint32_t frameLen = header->length;
    // 只要长度不小于结构体，就可以兼容（处理硬件实际发 108 字节的情况）
    if (frameLen >= sizeof(ProtocolHeader) + sizeof(DeviceInfoPayload) + sizeof(ProtocolTail))
    {
        const DeviceInfoPayload *payload =
            reinterpret_cast<const DeviceInfoPayload *>(frameData.constData() + sizeof(ProtocolHeader));

        QJsonObject json;
        json["timestamp"] = QString("%1/%2/%3 %4:%5:%6:%7")
                                .arg(header->year)
                                .arg(header->month, 2, 10, QChar('0'))
                                .arg(header->day, 2, 10, QChar('0'))
                                .arg(header->hour, 2, 10, QChar('0'))
                                .arg(header->minute, 2, 10, QChar('0'))
                                .arg(header->second, 2, 10, QChar('0'))
                                .arg(header->millisecond, 3, 10, QChar('0'));
        json["deviceId"] = static_cast<qint64>(payload->deviceId);
        // 安全读取：防止硬件未发送\0导致内存越界读取
        int nameLen = 0;
        while (nameLen < 20 && payload->deviceName[nameLen] != '\0')
        {
            nameLen++;
        }
        json["deviceName"] = QString::fromUtf8(payload->deviceName, nameLen).trimmed();
        json["longitude"] = payload->longitude;
        json["latitude"] = payload->latitude;
        json["altitude"] = payload->altitude;
        json["workStatus"] = payload->workStatus;
        json["azimuth"] = payload->azimuth;
        json["pitch"] = payload->pitch;
        json["temperature"] = payload->temperature;
        json["humidity"] = payload->humidity;
        json["signalParam3"] = static_cast<qint64>(payload->signalParam3);
        json["stationNum"] = static_cast<qint64>(payload->stationNum);

        emit deviceInfoParsed(json);
    }
    else
    {
        qDebug() << "[TcpManager] DataType=1 数据包长度不匹配 Payload！当前帧长度:" << frameLen
                 << "，期望最小长度:" << (sizeof(ProtocolHeader) + sizeof(DeviceInfoPayload) + sizeof(ProtocolTail));
    }
}

void TcpManager::handleDeviceStatusInfo(const ProtocolHeader *header, const QByteArray &frameData)
{
    const uint32_t frameLen = header->length;
    const int payloadLen = frameLen - sizeof(ProtocolHeader) - sizeof(ProtocolTail);

    if (payloadLen < static_cast<int>(sizeof(DeviceStatusPayload)))
    {
        qDebug() << "[TcpManager] DataType=2 数据包长度不匹配 Payload！当前帧长度:" << frameLen
                 << "，Payload 长度:" << payloadLen << "，期望最小长度:"
                 << static_cast<int>(sizeof(DeviceStatusPayload));
        return;
    }

    const DeviceStatusPayload *payload =
        reinterpret_cast<const DeviceStatusPayload *>(frameData.constData() + sizeof(ProtocolHeader));
    const QString timestamp = QString("%1/%2/%3 %4:%5:%6:%7")
                                  .arg(header->year)
                                  .arg(header->month, 2, 10, QChar('0'))
                                  .arg(header->day, 2, 10, QChar('0'))
                                  .arg(header->hour, 2, 10, QChar('0'))
                                  .arg(header->minute, 2, 10, QChar('0'))
                                  .arg(header->second, 2, 10, QChar('0'))
                                  .arg(header->millisecond, 3, 10, QChar('0'));
    const QString deviceName = readFixedUtf8String(payload->deviceName, sizeof(payload->deviceName));
    const QString deviceType = readFixedUtf8String(payload->deviceType, sizeof(payload->deviceType));
    const QString firmwareVersion =
        readFixedUtf8String(payload->firmwareVersion, sizeof(payload->firmwareVersion));
    const QString fpgaVersion = readFixedUtf8String(payload->fpgaVersion, sizeof(payload->fpgaVersion));
    const QString gpuVersion = readFixedUtf8String(payload->gpuVersion, sizeof(payload->gpuVersion));
    const QString operatorId = readFixedUtf8String(payload->operatorId, sizeof(payload->operatorId));

    QJsonObject json;
    json["timestamp"] = timestamp;
    json["protocolDataType"] = 2;
    json["deviceId"] = static_cast<qint64>(payload->deviceId);
    json["deviceName"] = deviceName;
    json["deviceType"] = deviceType;
    json["firmwareVersion"] = firmwareVersion;
    json["fpgaVersion"] = fpgaVersion;
    json["gpuVersion"] = gpuVersion;
    json["longitude"] = payload->longitude;
    json["latitude"] = payload->latitude;
    json["altitude"] = payload->altitude;
    json["azimuth"] = payload->azimuth;
    json["pitch"] = payload->pitch;
    json["temperature"] = payload->temperature;
    json["detectRadius"] = payload->detectRadius;
    json["jammingAngleRange"] = payload->jammingAngleRange;
    json["jammingDistance"] = payload->jammingDistance;
    json["communicationJammingStatus"] = static_cast<int>(payload->communicationJammingStatus);
    json["navigationJammingStatus"] = static_cast<int>(payload->navigationJammingStatus);
    json["bandSwitchStatus"] = byteFlagsToJsonArray(payload->bandSwitchStatus, sizeof(payload->bandSwitchStatus));
    json["detectStatus"] = static_cast<int>(payload->detectStatus);
    json["powerSupplyMode"] = static_cast<int>(payload->powerSupplyMode);
    json["batteryLevel"] = static_cast<int>(payload->batteryLevel);
    json["fanAlarm"] = static_cast<int>(payload->fanAlarm);
    json["clockAlarm"] = static_cast<int>(payload->clockAlarm);
    json["receiverPllAlarm"] = static_cast<int>(payload->receiverPllAlarm);
    json["transmitterPllAlarm"] = static_cast<int>(payload->transmitterPllAlarm);
    json["eepromAlarm"] = static_cast<int>(payload->eepromAlarm);
    json["temperatureChipAlarm"] = static_cast<int>(payload->temperatureChipAlarm);
    json["compassAlarm"] = static_cast<int>(payload->compassAlarm);
    json["adcAlarm"] = static_cast<int>(payload->adcAlarm);
    json["pa485Alarm"] = byteFlagsToJsonArray(payload->pa485Alarm, sizeof(payload->pa485Alarm));
    json["paUnderpowerAlarm"] =
        byteFlagsToJsonArray(payload->paUnderpowerAlarm, sizeof(payload->paUnderpowerAlarm));
    json["paOverpowerAlarm"] = byteFlagsToJsonArray(payload->paOverpowerAlarm, sizeof(payload->paOverpowerAlarm));
    json["simCardStatus"] = static_cast<int>(payload->simCardStatus);
    json["simCardStatusText"] = simCardStatusText(payload->simCardStatus);
    json["fourGNetworkStatus"] = static_cast<int>(payload->fourGNetworkStatus);
    json["fourGSignalQuality"] = static_cast<int>(payload->fourGSignalQuality);
    json["fourGNetworkType"] = static_cast<int>(payload->fourGNetworkType);
    json["operatorId"] = operatorId;

    emit deviceInfoParsed(json);
}
