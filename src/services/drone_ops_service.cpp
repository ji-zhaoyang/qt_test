#include "drone_ops_service.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

#include <cstring>

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

QString parseResultMessage(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
}
} // namespace

DroneOpsService::DroneOpsService(QObject *parent) : QObject(parent)
{
}

void DroneOpsService::setFrameSender(const FrameSender &sender)
{
    sendFrame_ = sender;
}

void DroneOpsService::setDroneDirectionFinding(bool enabled, quint32 targetId)
{
    if (!sendFrame_)
    {
        return;
    }

    QByteArray payload;
    payload.reserve(1 + static_cast<int>(sizeof(quint32)));
    payload.append(static_cast<char>(enabled ? 1 : 0));
    appendUInt32LE(payload, targetId);

    pendingDroneDirectionFindingEnabled_ = enabled;
    pendingDroneDirectionFindingTargetId_ = targetId;
    sendFrame_(111, payload);
}

void DroneOpsService::setDronePrecisionStrike(bool enabled, quint32 timestamp, const QString &sn, int type, quint32 targetId)
{
    if (!sendFrame_)
    {
        return;
    }

    QJsonObject payloadObject;
    payloadObject.insert(QStringLiteral("timestamp"), static_cast<qint64>(timestamp));
    payloadObject.insert(QStringLiteral("pa_switch"), enabled ? 1 : 0);
    payloadObject.insert(QStringLiteral("sn"), sn.trimmed());
    payloadObject.insert(QStringLiteral("type"), type);

    pendingDronePrecisionStrikeEnabled_ = enabled;
    pendingDronePrecisionStrikeTimestamp_ = timestamp;
    pendingDronePrecisionStrikeSn_ = sn.trimmed();
    pendingDronePrecisionStrikeType_ = type;
    pendingDronePrecisionStrikeTargetId_ = targetId;

    sendFrame_(109, QJsonDocument(payloadObject).toJson(QJsonDocument::Compact));
}

void DroneOpsService::setDroneWideBandJamming(bool enabled, quint32 frequencyKhz, const QString &sn, quint32 targetId)
{
    if (!sendFrame_)
    {
        return;
    }

    const QByteArray snBytes = sn.trimmed().toUtf8();

    QByteArray payload;
    payload.reserve(1 + static_cast<int>(sizeof(quint32)) + static_cast<int>(sizeof(quint32)) + snBytes.size());
    payload.append(static_cast<char>(enabled ? 1 : 0));
    appendUInt32LE(payload, frequencyKhz);
    appendUInt32LE(payload, static_cast<quint32>(snBytes.size()));
    payload.append(snBytes);

    pendingDroneWideBandJammingEnabled_ = enabled;
    pendingDroneWideBandJammingFrequencyKhz_ = frequencyKhz;
    pendingDroneWideBandJammingSn_ = sn.trimmed();
    pendingDroneWideBandJammingTargetId_ = targetId;

    sendFrame_(114, payload);
}

void DroneOpsService::handleDroneTargetReport(const ProtocolHeader *header, const QByteArray &frameData)
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
    json[QStringLiteral("timestamp")] = timestamp;
    json[QStringLiteral("protocolDataType")] = 56;
    json[QStringLiteral("targetUniqueId")] = targetUniqueId;
    json[QStringLiteral("targetId")] = targetId;
    json[QStringLiteral("targetInfoLength")] = targetInfoLength;

    if (targetInfoLength == 0)
    {
        json[QStringLiteral("disappeared")] = true;
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

    json[QStringLiteral("disappeared")] = false;
    json[QStringLiteral("targetName")] = targetName;
    json[QStringLiteral("longitude")] = droneLongitude;
    json[QStringLiteral("latitude")] = droneLatitude;
    json[QStringLiteral("altitudeSeaLevel")] = altitudeSeaLevel;
    json[QStringLiteral("altitudeFromTakeoff")] = altitudeFromTakeoff;
    json[QStringLiteral("azimuth")] = azimuth;
    json[QStringLiteral("distance")] = distance;
    json[QStringLiteral("controllerLongitude")] = controllerLongitude;
    json[QStringLiteral("controllerLatitude")] = controllerLatitude;
    json[QStringLiteral("frequencyKhz")] = frequencyKhz;
    json[QStringLiteral("bandwidthKhz")] = bandwidthKhz;
    json[QStringLiteral("signalStrengthDb")] = signalStrengthDb;
    json[QStringLiteral("confidence")] = static_cast<int>(confidence);
    json[QStringLiteral("identifyTimestamp")] = static_cast<qint64>(identifyTimestamp);
    json[QStringLiteral("speedMetersPerSecond")] = speedMetersPerSecond;
    json[QStringLiteral("returnLongitude")] = returnLongitude;
    json[QStringLiteral("returnLatitude")] = returnLatitude;
    json[QStringLiteral("droneType")] = static_cast<int>(droneType);
    json[QStringLiteral("droneNumber")] = static_cast<int>(droneNumber);
    emit droneTargetReported(json);
}

void DroneOpsService::handleDroneDirectionFindingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"), Qt::CaseInsensitive);
    const quint32 pendingTargetId = pendingDroneDirectionFindingTargetId_;
    const bool pendingEnabled = pendingDroneDirectionFindingEnabled_;

    if (success)
    {
        activeDroneDirectionFindingEnabled_ = pendingEnabled;
        activeDroneDirectionFindingTargetId_ = pendingEnabled ? pendingTargetId : 0;
    }

    emit droneDirectionFindingResponse(pendingTargetId,
                                       pendingEnabled,
                                       success,
                                       resultMsg);

    // 112 可能晚于第一批 113 功率上报到达，响应处理完后清掉 pending，
    // 让后续上报统一走 active 状态，避免旧 pending 长时间残留。
    pendingDroneDirectionFindingEnabled_ = false;
    pendingDroneDirectionFindingTargetId_ = 0;
}

void DroneOpsService::handleDroneDirectionPowerReport(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);
    const bool directionFindingEnabled = activeDroneDirectionFindingEnabled_ || pendingDroneDirectionFindingEnabled_;
    if (!directionFindingEnabled)
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
    const quint32 currentTargetId =
        activeDroneDirectionFindingEnabled_ ? activeDroneDirectionFindingTargetId_ : pendingDroneDirectionFindingTargetId_;
    report[QStringLiteral("targetId")] = static_cast<qint64>(currentTargetId);
    report[QStringLiteral("enabled")] = directionFindingEnabled;
    report[QStringLiteral("omniPower")] = omniPower;
    report[QStringLiteral("directionalPower")] = directionalPower;
    report[QStringLiteral("calibrationValue")] = calibrationValue;
    emit droneDirectionPowerReported(report);
}

void DroneOpsService::handleDronePrecisionStrikeResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"), Qt::CaseInsensitive);
    emit dronePrecisionStrikeResponse(pendingDronePrecisionStrikeTargetId_,
                                      pendingDronePrecisionStrikeEnabled_,
                                      success,
                                      resultMsg);
}

void DroneOpsService::handleDroneWideBandJammingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"), Qt::CaseInsensitive);
    emit droneWideBandJammingResponse(pendingDroneWideBandJammingTargetId_,
                                      pendingDroneWideBandJammingEnabled_,
                                      success,
                                      resultMsg);
}
