#include "tcp_manager.h"
#include <cstring>

namespace
{
constexpr int kDirectionCalibrationValueCount = 6;
constexpr int kDirectionCalibrationPayloadLength = kDirectionCalibrationValueCount * static_cast<int>(sizeof(float));

void appendFloat32LE(QByteArray &buffer, float value)
{
    quint32 raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    buffer.append(static_cast<char>(raw & 0xFF));
    buffer.append(static_cast<char>((raw >> 8) & 0xFF));
    buffer.append(static_cast<char>((raw >> 16) & 0xFF));
    buffer.append(static_cast<char>((raw >> 24) & 0xFF));
}

float readFloat32LE(const char *data)
{
    const quint32 raw = static_cast<quint32>(static_cast<unsigned char>(data[0])) |
                        (static_cast<quint32>(static_cast<unsigned char>(data[1])) << 8) |
                        (static_cast<quint32>(static_cast<unsigned char>(data[2])) << 16) |
                        (static_cast<quint32>(static_cast<unsigned char>(data[3])) << 24);
    float value = 0.0f;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

QString parseResultMessage(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
}
} // namespace

bool TcpManager::dispatchDirectionCalibrationProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 125:
        handleDirectionCalibrationSetResponse(frameData);
        return true;
    case 127:
        handleDirectionCalibrationQueryResponse(frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::setDirectionCalibrationValues(const DirectionCalibrationValueList &values)
{
    QByteArray payload;
    payload.reserve(kDirectionCalibrationPayloadLength);
    for (int i = 0; i < kDirectionCalibrationValueCount; ++i)
    {
        appendFloat32LE(payload, i < values.size() ? values.at(i) : 0.0f);
    }

    sendFrame(124, payload);
}

void TcpManager::queryDirectionCalibrationValues()
{
    qDebug() << "[TcpManager] 准备发送测向定标值查询命令 (DataType=126)";
    sendFrame(126);
}

void TcpManager::handleDirectionCalibrationSetResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains("RESULT:SUCCESSED");
    emit directionCalibrationValuesSetResponse(success, resultMsg);
}

void TcpManager::handleDirectionCalibrationQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen != kDirectionCalibrationPayloadLength)
    {
        qDebug() << "[TcpManager] DataType=127 查询应答长度非法，实际:" << payloadLen
                 << "期望:" << kDirectionCalibrationPayloadLength;
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    DirectionCalibrationValueList values;
    values.reserve(kDirectionCalibrationValueCount);
    QStringList valueTexts;
    valueTexts.reserve(kDirectionCalibrationValueCount);
    for (int i = 0; i < kDirectionCalibrationValueCount; ++i)
    {
        const float value = readFloat32LE(payload + i * static_cast<int>(sizeof(float)));
        values.append(value);
        valueTexts.append(QStringLiteral("定标值%1=%2").arg(i + 1).arg(value, 0, 'g', 10));
    }

    qDebug() << "[TcpManager] 测向定标值查询结果:" << valueTexts.join(QStringLiteral(", "));
    emit directionCalibrationValuesQueried(values);
}
