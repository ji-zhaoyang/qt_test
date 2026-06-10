#include "tcp_manager.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <cmath>
#include <cstring>
#include <limits>

namespace
{
void appendUInt32LE(QByteArray &buffer, quint32 value)
{
    buffer.append(static_cast<char>(value & 0xFF));
    buffer.append(static_cast<char>((value >> 8) & 0xFF));
    buffer.append(static_cast<char>((value >> 16) & 0xFF));
    buffer.append(static_cast<char>((value >> 24) & 0xFF));
}

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

template <typename T>
bool readScalarLocal(const QByteArray &payload, int &offset, T &value)
{
    if (offset < 0 || payload.size() - offset < static_cast<int>(sizeof(T)))
    {
        return false;
    }

    std::memcpy(&value, payload.constData() + offset, sizeof(T));
    offset += static_cast<int>(sizeof(T));
    return true;
}

QString buildProtocolTimestamp(const ProtocolHeader *header)
{
    return QString("%1/%2/%3 %4:%5:%6:%7")
        .arg(header->year)
        .arg(header->month, 2, 10, QChar('0'))
        .arg(header->day, 2, 10, QChar('0'))
        .arg(header->hour, 2, 10, QChar('0'))
        .arg(header->minute, 2, 10, QChar('0'))
        .arg(header->second, 2, 10, QChar('0'))
        .arg(header->millisecond, 3, 10, QChar('0'));
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
    case 56:
        handleDroneTargetReport(header, frameData);
        return true;
    case 112:
        handleDroneDirectionFindingResponse(frameData);
        return true;
    case 113:
        handleDroneDirectionPowerReport(header, frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::setDroneDirectionFinding(bool enabled, quint32 targetId)
{
    QByteArray payload;
    payload.reserve(1 + static_cast<int>(sizeof(quint32)));
    payload.append(static_cast<char>(enabled ? 1 : 0));
    appendUInt32LE(payload, targetId);

    pendingDroneDirectionFindingEnabled = enabled;
    pendingDroneDirectionFindingTargetId = targetId;
    sendFrame(111, payload);
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

void TcpManager::handleDroneTargetReport(const ProtocolHeader *header, const QByteArray &frameData)
{
    const int payloadLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 24)
    {
        qDebug() << "[TcpManager] DataType=56 负载长度不足，Payload 长度:" << payloadLen;
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    int offset = 0;

    const QString timestamp = buildProtocolTimestamp(header);
    const QString targetUniqueId = readFixedUtf8String(payloadBytes.constData(), 16);
    offset += 16;

    int32_t targetId = 0;
    int32_t targetInfoLength = 0;
    if (!readScalarLocal(payloadBytes, offset, targetId) || !readScalarLocal(payloadBytes, offset, targetInfoLength))
    {
        qDebug() << "[TcpManager] DataType=56 读取目标基础字段失败";
        return;
    }

    if (targetInfoLength < 0 || targetInfoLength > payloadBytes.size() - offset)
    {
        qDebug() << "[TcpManager] DataType=56 目标信息长度非法，targetInfoLength =" << targetInfoLength
                 << "remaining =" << (payloadBytes.size() - offset);
        return;
    }

    QJsonObject json;
    json["timestamp"] = timestamp;
    json["protocolDataType"] = 56;
    json["targetUniqueId"] = targetUniqueId;
    json["targetId"] = targetId;
    json["targetInfoLength"] = targetInfoLength;

    if (targetInfoLength == 0)
    {
        json["disappeared"] = true;
        emit droneTargetReported(json);
        return;
    }

    const QByteArray targetInfoBytes = payloadBytes.mid(offset, targetInfoLength);
    const QString targetName = QString::fromUtf8(targetInfoBytes).trimmed();
    offset += targetInfoLength;

    float droneLongitude = 0.0f;
    float droneLatitude = 0.0f;
    int32_t altitudeSeaLevel = 0;
    int32_t altitudeFromTakeoff = 0;
    int32_t azimuth = -1;
    int32_t distance = 0;
    float controllerLongitude = 0.0f;
    float controllerLatitude = 0.0f;
    double frequencyKhz = 0.0;
    double bandwidthKhz = 0.0;
    double signalStrengthDb = 0.0;
    uint8_t confidence = 0;
    uint32_t identifyTimestamp = 0;
    double speedMetersPerSecond = 0.0;
    float returnLongitude = 0.0f;
    float returnLatitude = 0.0f;
    uint8_t droneNumber = 0;
    uint16_t droneType = 0;

    if (!readScalarLocal(payloadBytes, offset, droneLongitude) ||
        !readScalarLocal(payloadBytes, offset, droneLatitude) ||
        !readScalarLocal(payloadBytes, offset, altitudeSeaLevel) ||
        !readScalarLocal(payloadBytes, offset, altitudeFromTakeoff) ||
        !readScalarLocal(payloadBytes, offset, azimuth) ||
        !readScalarLocal(payloadBytes, offset, distance) ||
        !readScalarLocal(payloadBytes, offset, controllerLongitude) ||
        !readScalarLocal(payloadBytes, offset, controllerLatitude) ||
        !readScalarLocal(payloadBytes, offset, frequencyKhz) ||
        !readScalarLocal(payloadBytes, offset, bandwidthKhz) ||
        !readScalarLocal(payloadBytes, offset, signalStrengthDb) ||
        !readScalarLocal(payloadBytes, offset, confidence) ||
        !readScalarLocal(payloadBytes, offset, identifyTimestamp) ||
        !readScalarLocal(payloadBytes, offset, speedMetersPerSecond) ||
        !readScalarLocal(payloadBytes, offset, returnLongitude) ||
        !readScalarLocal(payloadBytes, offset, returnLatitude) ||
        !readScalarLocal(payloadBytes, offset, droneNumber) ||
        !readScalarLocal(payloadBytes, offset, droneType))
    {
        qDebug() << "[TcpManager] DataType=56 解析目标详情失败，targetId =" << targetId
                 << "payloadHex =" << payloadBytes.toHex(' ').toUpper();
        return;
    }

    if (payloadBytes.size() - offset < 14)
    {
        qDebug() << "[TcpManager] DataType=56 预留字节长度不足，remaining =" << (payloadBytes.size() - offset);
        return;
    }

    json["disappeared"] = false;
    json["targetName"] = targetName;
    json["longitude"] = droneLongitude;
    json["latitude"] = droneLatitude;
    json["altitudeSeaLevel"] = altitudeSeaLevel;
    json["altitudeFromTakeoff"] = altitudeFromTakeoff;
    json["azimuth"] = azimuth;
    json["distance"] = distance;
    json["controllerLongitude"] = controllerLongitude;
    json["controllerLatitude"] = controllerLatitude;
    json["frequencyKhz"] = frequencyKhz;
    json["bandwidthKhz"] = bandwidthKhz;
    json["signalStrengthDb"] = signalStrengthDb;
    json["confidence"] = static_cast<int>(confidence);
    json["identifyTimestamp"] = static_cast<qint64>(identifyTimestamp);
    json["speedMetersPerSecond"] = speedMetersPerSecond;
    json["returnLongitude"] = returnLongitude;
    json["returnLatitude"] = returnLatitude;
    json["droneType"] = static_cast<int>(droneType);
    json["droneNumber"] = static_cast<int>(droneNumber);

    emit droneTargetReported(json);
}

void TcpManager::handleDroneDirectionFindingResponse(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"), Qt::CaseInsensitive);

    if (success)
    {
        activeDroneDirectionFindingEnabled = pendingDroneDirectionFindingEnabled;
        activeDroneDirectionFindingTargetId = pendingDroneDirectionFindingEnabled ? pendingDroneDirectionFindingTargetId : 0;
    }

    emit droneDirectionFindingResponse(pendingDroneDirectionFindingTargetId,
                                       pendingDroneDirectionFindingEnabled,
                                       success,
                                       resultMsg);
}

void TcpManager::handleDroneDirectionPowerReport(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);
    if (!activeDroneDirectionFindingEnabled)
    {
        return;
    }

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const int expectedPayloadLen = static_cast<int>(sizeof(double) * 3);
    if (payloadLen < expectedPayloadLen)
    {
        qDebug() << "[TcpManager] DataType=113 测向功率上报长度不足，实际:" << payloadLen << "期望:" << expectedPayloadLen;
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    int offset = 0;
    double omniPower = 0.0;
    double directionalPower = 0.0;
    double calibrationValue = 0.0;
    if (!readScalarLocal(payloadBytes, offset, omniPower) ||
        !readScalarLocal(payloadBytes, offset, directionalPower) ||
        !readScalarLocal(payloadBytes, offset, calibrationValue))
    {
        qDebug() << "[TcpManager] DataType=113 测向功率上报解析失败";
        return;
    }

    QJsonObject report;
    report["targetId"] = static_cast<qint64>(activeDroneDirectionFindingTargetId);
    report["enabled"] = activeDroneDirectionFindingEnabled;
    report["omniPower"] = omniPower;
    report["directionalPower"] = directionalPower;
    report["calibrationValue"] = calibrationValue;
    emit droneDirectionPowerReported(report);
}
