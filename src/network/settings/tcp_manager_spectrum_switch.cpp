#include "tcp_manager.h"
#include <QDebug>
#include <QStringList>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cstring>

namespace
{
constexpr int kSpectrumDataMatrixCount = 4 * 3000;
constexpr int kSpectrumDataBytes = kSpectrumDataMatrixCount * static_cast<int>(sizeof(qint16));
constexpr int kSpectrumFreqBins = 24;
constexpr int kSpectrumTimeBins = 125;
constexpr int kSpectrumValuesPerGroup = kSpectrumFreqBins * kSpectrumTimeBins;

quint16 readUInt16LELocal(const char *data)
{
    return static_cast<quint16>(static_cast<unsigned char>(data[0])) |
           (static_cast<quint16>(static_cast<unsigned char>(data[1])) << 8);
}

quint32 readUInt32LELocal(const char *data)
{
    return static_cast<quint32>(static_cast<unsigned char>(data[0])) |
           (static_cast<quint32>(static_cast<unsigned char>(data[1])) << 8) |
           (static_cast<quint32>(static_cast<unsigned char>(data[2])) << 16) |
           (static_cast<quint32>(static_cast<unsigned char>(data[3])) << 24);
}

qint32 readInt32LELocal(const char *data)
{
    return static_cast<qint32>(readUInt32LELocal(data));
}

qint16 readInt16LELocal(const char *data)
{
    return static_cast<qint16>(static_cast<unsigned char>(data[0]) |
                               (static_cast<unsigned short>(static_cast<unsigned char>(data[1])) << 8));
}

float readFloatLELocal(const char *data)
{
    const quint32 raw = readUInt32LELocal(data);
    float value = 0.0f;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
}

float normalizeFrequencyToMhz(float rawValue)
{
    float normalized = rawValue;
    while (std::abs(normalized) >= 10000.0f)
    {
        normalized /= 1000.0f;
    }
    return normalized;
}

void logTargetGroupDataIfNeeded(const SpectrumGroupData &groupData)
{
    if (std::abs(groupData.centerFreqMhz - 2430.0f) > 0.5f)
    {
        return;
    }

    qDebug() << "[TcpManager] 2430MHz 时频图数据开始";
    for (int timeIndex = 0; timeIndex < groupData.matrix.size(); ++timeIndex)
    {
        const QVector<qint16> &timeRow = groupData.matrix.at(timeIndex);
        QStringList parts;
        parts.reserve(timeRow.size());
        for (qint16 value : timeRow)
        {
            parts.append(QString::number(value));
        }
        qDebug().noquote() << QStringLiteral("  时刻%1: %2").arg(timeIndex + 1, 3, 10, QChar('0')).arg(parts.join(' '));
    }
    qDebug() << "[TcpManager] 2430MHz 时频图数据结束";
}
} // namespace

bool TcpManager::dispatchSpectrumSwitchProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 66:
    case 68:
        handleSpectrogramSwitchResponse(header->dataType, frameData);
        return true;
    case 69:
        handleSpectrumDataReport(header, frameData);
        return true;
    case 219:
        handleFullSpectrumSwitchResponse(true, frameData);
        return true;
    case 220:
        handleFullSpectrumReport(header, frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::openSpectrogram()
{
    qDebug() << "[TcpManager] 准备发送打开时频图命令 (DataType=65)";
    sendFrame(65);
}

void TcpManager::closeSpectrogram()
{
    qDebug() << "[TcpManager] 准备发送关闭时频图命令 (DataType=67)";
    sendFrame(67);
}

void TcpManager::openFullSpectrum()
{
    QByteArray payload;
    payload.append(static_cast<char>(1));
    pendingFullSpectrumEnabled = true;
    qDebug() << "[TcpManager] 准备发送打开频谱图命令 (DataType=218), enabled = 1";
    sendFrame(218, payload);
}

void TcpManager::closeFullSpectrum()
{
    QByteArray payload;
    payload.append(static_cast<char>(0));
    pendingFullSpectrumEnabled = false;
    qDebug() << "[TcpManager] 准备发送关闭频谱图命令 (DataType=218), enabled = 0";
    sendFrame(218, payload);
}

void TcpManager::handleSpectrogramSwitchResponse(uint16_t responseDataType, const QByteArray &frameData)
{
    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    qDebug() << "[TcpManager] 收到时频图开关应答, dataType =" << responseDataType << "success =" << success
             << "message =" << resultMsg;

    emit spectrogramSwitchResponse(responseDataType, success, resultMsg);
}

void TcpManager::handleFullSpectrumSwitchResponse(bool enabled, const QByteArray &frameData)
{
    Q_UNUSED(enabled);
    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    qDebug() << "[TcpManager] 收到频谱图开关应答 (DataType=219), enabled =" << enabled << "success =" << success
             << "message =" << resultMsg;

    emit fullSpectrumSwitchResponse(pendingFullSpectrumEnabled, success, resultMsg);
}

void TcpManager::handleSpectrumDataReport(const ProtocolHeader *header, const QByteArray &frameData)
{
    const int payloadLen = static_cast<int>(header->length) - static_cast<int>(sizeof(ProtocolHeader)) -
                           static_cast<int>(sizeof(ProtocolTail));
    const int minPayloadLen = 2 + 2 + 4 + 4 + 16;
    if (payloadLen < minPayloadLen)
    {
        qDebug() << "[TcpManager] DataType=69 频谱图上报长度不足，Payload 长度:" << payloadLen << "期望至少:"
                 << minPayloadLen;
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    const quint16 totalPacketCount = readUInt16LELocal(payload);
    const quint16 packetIndex = readUInt16LELocal(payload + 2);
    const float centerFreqMhz = normalizeFrequencyToMhz(readFloatLELocal(payload + 4));
    const qint32 spectrumDataLength = readInt32LELocal(payload + 8);

    if (spectrumDataLength <= 0)
    {
        qDebug() << "[TcpManager] DataType=69 频谱数据长度非法:" << spectrumDataLength;
        return;
    }

    const int expectedPayloadLen = 2 + 2 + 4 + 4 + spectrumDataLength + 16;
    if (payloadLen < expectedPayloadLen)
    {
        qDebug() << "[TcpManager] DataType=69 频谱图上报长度不足，实际:" << payloadLen << "期望:" << expectedPayloadLen
                 << "dataLength:" << spectrumDataLength;
        return;
    }

    const char *spectrumData = payload + 12;
    const char *groupCenterFreqs = spectrumData + spectrumDataLength;
    const float groupFreq1 = normalizeFrequencyToMhz(readFloatLELocal(groupCenterFreqs));
    const float groupFreq2 = normalizeFrequencyToMhz(readFloatLELocal(groupCenterFreqs + 4));
    const float groupFreq3 = normalizeFrequencyToMhz(readFloatLELocal(groupCenterFreqs + 8));
    const float groupFreq4 = normalizeFrequencyToMhz(readFloatLELocal(groupCenterFreqs + 12));

    // qDebug() << "==================================================";
    // qDebug() << "[TcpManager] 收到频谱图数据上报 (DataType=69)";
    // qDebug() << "  -> 总包数:" << totalPacketCount;
    // qDebug() << "  -> 分包编号:" << packetIndex;
    // qDebug() << "  -> 中心频点(M):" << centerFreqMhz;
    // qDebug() << "  -> 频谱数据长度(字节):" << spectrumDataLength;
    // qDebug() << "  -> 第一组中心频点(M):" << groupFreq1;
    // qDebug() << "  -> 第二组中心频点(M):" << groupFreq2;
    // qDebug() << "  -> 第三组中心频点(M):" << groupFreq3;
    // qDebug() << "  -> 第四组中心频点(M):" << groupFreq4;

    if (spectrumDataLength != kSpectrumDataBytes)
    {
        qDebug() << "[TcpManager] DataType=69 频谱数据长度与协议固定值不一致，当前:" << spectrumDataLength
                 << "协议固定值:" << kSpectrumDataBytes;
    }

    const int sampleCount = spectrumDataLength / static_cast<int>(sizeof(qint16));
    if (sampleCount % kSpectrumValuesPerGroup != 0)
    {
        qDebug() << "[TcpManager] DataType=69 频谱数据点数无法按分组维度均分，sampleCount =" << sampleCount;
        qDebug() << "==================================================";
        return;
    }

    QVector<qint16> values;
    values.reserve(sampleCount);
    for (int i = 0; i < sampleCount; ++i)
    {
        values.append(readInt16LELocal(spectrumData + i * static_cast<int>(sizeof(qint16))));
    }

    const int groupCount = sampleCount / kSpectrumValuesPerGroup;
    // qDebug() << "  -> 解析后分组数:" << groupCount << "每组点数:" << kSpectrumValuesPerGroup
    //          << "排布: time x freq =" << kSpectrumTimeBins << "x" << kSpectrumFreqBins;

    SpectrumReportData reportData;
    reportData.totalPacketCount = totalPacketCount;
    reportData.packetIndex = packetIndex;
    reportData.centerFreqMhz = centerFreqMhz;
    reportData.groups.reserve(groupCount);

    const float groupFreqs[] = {groupFreq1, groupFreq2, groupFreq3, groupFreq4};
    for (int groupIndex = 0; groupIndex < groupCount; ++groupIndex)
    {
        SpectrumGroupData groupData;
        groupData.centerFreqMhz = groupIndex < 4 ? groupFreqs[groupIndex] : 0.0f;
        groupData.matrix.resize(kSpectrumTimeBins);

        const qint16 *groupBase = values.constData() + groupIndex * kSpectrumValuesPerGroup;
        for (int timeIndex = 0; timeIndex < kSpectrumTimeBins; ++timeIndex)
        {
            QVector<qint16> timeRow;
            timeRow.reserve(kSpectrumFreqBins);
            const qint16 *freqRow = groupBase + timeIndex * kSpectrumFreqBins;
            for (int freqIndex = 0; freqIndex < kSpectrumFreqBins; ++freqIndex)
            {
                timeRow.append(freqRow[freqIndex]);
            }
            groupData.matrix[timeIndex] = timeRow;
        }

        reportData.groups.append(groupData);
        logTargetGroupDataIfNeeded(groupData);
        // qDebug() << QStringLiteral("  -> 第%1组中心频点(M):").arg(groupIndex + 1) << groupData.centerFreqMhz
        //          << "矩阵尺寸:" << groupData.matrix.size() << "x"
        //          << (groupData.matrix.isEmpty() ? 0 : groupData.matrix.first().size());
    }

    // qDebug() << "==================================================";
    emit spectrumDataReported(reportData);
}

void TcpManager::handleFullSpectrumReport(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        qDebug() << "[TcpManager] DataType=220 频谱图 JSON 负载为空";
        return;
    }

    const QByteArray payloadBytes(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payloadBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        qDebug() << "[TcpManager] DataType=220 JSON 解析失败:" << parseError.errorString();
        return;
    }

    const QJsonObject obj = doc.object();
    FullSpectrumReportData reportData;
    reportData.startMhz = obj.value("start").toDouble(300.0);
    reportData.endMhz = obj.value("end").toDouble(6000.0);

    const QJsonArray dataArray = obj.value("data").toArray();
    reportData.data.reserve(dataArray.size());
    for (const QJsonValue &value : dataArray)
    {
        reportData.data.append(value.toInt());
    }

    const QJsonArray countArray = obj.value("count").toArray();
    reportData.markerIndices.reserve(countArray.size());
    for (const QJsonValue &value : countArray)
    {
        reportData.markerIndices.append(value.toInt());
    }

    emit fullSpectrumReported(reportData);
}
