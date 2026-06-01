#include "tcp_manager.h"
#include <QDebug>
#include <cstring>

bool TcpManager::dispatchDetectBandProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 9:
        handleDetectBandSettingResponse(header, frameData);
        return true;
    case 11:
        handleDetectBandQueryResponse(header, frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::setDetectBands(const QVector<DetectBandParam> &bands)
{
    if (bands.isEmpty() || bands.size() > 128)
    {
        qDebug() << "[TcpManager] 侦测频段设置失败，频段数量非法:" << bands.size();
        return;
    }

    qDebug() << "[TcpManager] 准备发送侦测频段设置 (DataType=8), 频段数量:" << bands.size();
    for (int i = 0; i < bands.size(); ++i)
    {
        const DetectBandParam &band = bands.at(i);
        qDebug() << "  -> band[" << i << "] freqMhz =" << band.freqMhz << " measureCount =" << band.measureCount
                 << " gain =" << band.gain;
    }

    QByteArray payload;
    payload.reserve(2 + bands.size() * 12);

    auto appendUInt16LELocal = [](QByteArray &buffer, quint16 value)
    {
        buffer.append(static_cast<char>(value & 0xFF));
        buffer.append(static_cast<char>((value >> 8) & 0xFF));
    };

    auto appendUInt32LELocal = [](QByteArray &buffer, quint32 value)
    {
        buffer.append(static_cast<char>(value & 0xFF));
        buffer.append(static_cast<char>((value >> 8) & 0xFF));
        buffer.append(static_cast<char>((value >> 16) & 0xFF));
        buffer.append(static_cast<char>((value >> 24) & 0xFF));
    };

    auto appendInt32LELocal = [&](QByteArray &buffer, qint32 value) { appendUInt32LELocal(buffer, static_cast<quint32>(value)); };

    auto appendFloatLELocal = [&](QByteArray &buffer, float value)
    {
        quint32 raw = 0;
        std::memcpy(&raw, &value, sizeof(raw));
        appendUInt32LELocal(buffer, raw);
    };

    appendUInt16LELocal(payload, static_cast<quint16>(bands.size()));

    for (const DetectBandParam &band : bands)
    {
        appendFloatLELocal(payload, band.freqMhz);
        appendInt32LELocal(payload, band.measureCount);
        appendInt32LELocal(payload, band.gain);
    }

    qDebug() << "[TcpManager] 侦测频段设置载荷组装完毕，长度:" << payload.size() << "字节";
    sendFrame(8, payload);
}

void TcpManager::queryDetectBands()
{
    qDebug() << "[TcpManager] 准备发送侦测频段查询命令 (DataType=10)...";
    sendFrame(10);
}

void TcpManager::handleDetectBandSettingResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    qDebug() << "[TcpManager] 收到侦测频段设置应答 (DataType=9), success =" << success << " message =" << resultMsg;

    emit detectBandsSetResponse(success, resultMsg);

    if (success)
    {
        qDebug() << "[TcpManager] 侦测频段设置成功，自动发起查询 (DataType=10)...";
        queryDetectBands();
    }
}

void TcpManager::handleDetectBandQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 2)
    {
        qDebug() << "[TcpManager] DataType=11 查询应答长度不足，实际:" << payloadLen;
        return;
    }

    auto readUInt16LELocal = [](const char *data)
    {
        return static_cast<quint16>(static_cast<unsigned char>(data[0])) |
               (static_cast<quint16>(static_cast<unsigned char>(data[1])) << 8);
    };

    auto readUInt32LELocal = [](const char *data)
    {
        return static_cast<quint32>(static_cast<unsigned char>(data[0])) |
               (static_cast<quint32>(static_cast<unsigned char>(data[1])) << 8) |
               (static_cast<quint32>(static_cast<unsigned char>(data[2])) << 16) |
               (static_cast<quint32>(static_cast<unsigned char>(data[3])) << 24);
    };

    auto readInt32LELocal = [&](const char *data) { return static_cast<qint32>(readUInt32LELocal(data)); };

    auto readFloatLELocal = [&](const char *data)
    {
        const quint32 raw = readUInt32LELocal(data);
        float value = 0.0f;
        std::memcpy(&value, &raw, sizeof(value));
        return value;
    };

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    const quint16 bandCount = readUInt16LELocal(payload);
    qDebug() << "[TcpManager] 收到侦测频段查询应答 (DataType=11), 频段数量:" << bandCount;
    const int expectedLen = 2 + static_cast<int>(bandCount) * 12;
    if (payloadLen < expectedLen)
    {
        qDebug() << "[TcpManager] DataType=11 查询应答长度不足，频段数量:" << bandCount << "实际:" << payloadLen
                 << "期望:" << expectedLen;
        return;
    }

    QVector<DetectBandParam> bands;
    bands.reserve(static_cast<int>(bandCount));
    const char *bandData = payload + 2;
    for (quint16 i = 0; i < bandCount; ++i)
    {
        DetectBandParam band = {};
        band.freqMhz = readFloatLELocal(bandData);
        band.measureCount = readInt32LELocal(bandData + 4);
        band.gain = readInt32LELocal(bandData + 8);
        qDebug() << "  -> band[" << i << "] freqMhz =" << band.freqMhz << " measureCount =" << band.measureCount
                 << " gain =" << band.gain;
        bands.append(band);
        bandData += 12;
    }

    emit detectBandsQueried(bands);
}
