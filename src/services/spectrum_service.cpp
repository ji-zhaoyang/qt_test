#include "spectrum_service.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include <cmath>
#include <cstring>

namespace
{
constexpr int kSpectrumDataMatrixCount = 4 * 3000;
constexpr int kSpectrumDataBytes = kSpectrumDataMatrixCount * static_cast<int>(sizeof(qint16));
constexpr int kSpectrumFreqBins = 24;
constexpr int kSpectrumTimeBins = 125;
constexpr int kSpectrumValuesPerGroup = kSpectrumFreqBins * kSpectrumTimeBins;

QString parseResultMessage(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
}

quint16 readUInt16LE(const char *data)
{
    return static_cast<quint16>(static_cast<unsigned char>(data[0])) |
           (static_cast<quint16>(static_cast<unsigned char>(data[1])) << 8);
}

quint32 readUInt32LE(const char *data)
{
    return static_cast<quint32>(static_cast<unsigned char>(data[0])) |
           (static_cast<quint32>(static_cast<unsigned char>(data[1])) << 8) |
           (static_cast<quint32>(static_cast<unsigned char>(data[2])) << 16) |
           (static_cast<quint32>(static_cast<unsigned char>(data[3])) << 24);
}

qint32 readInt32LE(const char *data)
{
    return static_cast<qint32>(readUInt32LE(data));
}

qint16 readInt16LE(const char *data)
{
    return static_cast<qint16>(static_cast<unsigned char>(data[0]) |
                               (static_cast<unsigned short>(static_cast<unsigned char>(data[1])) << 8));
}

float readFloatLE(const char *data)
{
    const quint32 raw = readUInt32LE(data);
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

    qDebug() << "[SpectrumService] 2430MHz 时频图数据开始";
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
    qDebug() << "[SpectrumService] 2430MHz 时频图数据结束";
}

} // namespace

SpectrumService::SpectrumService(QObject *parent)
    : QObject(parent)
{
}

void SpectrumService::setFrameSender(const FrameSender &sender)
{
    sendFrame_ = sender;
}

void SpectrumService::openSpectrogram()
{
    if (sendFrame_)
    {
        qDebug() << "[SpectrumService] 准备发送打开时频图命令 (DataType=65)";
        sendFrame_(65, QByteArray());
    }
}

void SpectrumService::closeSpectrogram()
{
    if (sendFrame_)
    {
        qDebug() << "[SpectrumService] 准备发送关闭时频图命令 (DataType=67)";
        sendFrame_(67, QByteArray());
    }
}

void SpectrumService::openFullSpectrum()
{
    if (!sendFrame_)
    {
        return;
    }

    QByteArray payload;
    payload.append(static_cast<char>(1));
    pendingFullSpectrumEnabled_ = true;
    qDebug() << "[SpectrumService] 准备发送打开频谱图命令 (DataType=218), enabled = 1";
    sendFrame_(218, payload);
}

void SpectrumService::closeFullSpectrum()
{
    if (!sendFrame_)
    {
        return;
    }

    QByteArray payload;
    payload.append(static_cast<char>(0));
    pendingFullSpectrumEnabled_ = false;
    qDebug() << "[SpectrumService] 准备发送关闭频谱图命令 (DataType=218), enabled = 0";
    sendFrame_(218, payload);
}

void SpectrumService::handleSpectrogramSwitchResponse(uint16_t responseDataType, const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));

    qDebug() << "[SpectrumService] 收到时频图开关应答, dataType =" << responseDataType << "success =" << success
             << "message =" << resultMsg;

    emit spectrogramSwitchResponse(responseDataType, success, resultMsg);
}

void SpectrumService::handleSpectrumDataReport(const ProtocolHeader *header, const QByteArray &frameData)
{
    const int payloadLen = static_cast<int>(header->length) - static_cast<int>(sizeof(ProtocolHeader)) -
                           static_cast<int>(sizeof(ProtocolTail));
    const int minPayloadLen = 2 + 2 + 4 + 4 + 16;
    if (payloadLen < minPayloadLen)
    {
        qDebug() << "[SpectrumService] DataType=69 频谱图上报长度不足，Payload 长度:" << payloadLen << "期望至少:"
                 << minPayloadLen;
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    const quint16 totalPacketCount = readUInt16LE(payload);
    const quint16 packetIndex = readUInt16LE(payload + 2);
    const float centerFreqMhz = normalizeFrequencyToMhz(readFloatLE(payload + 4));
    const qint32 spectrumDataLength = readInt32LE(payload + 8);

    if (spectrumDataLength <= 0)
    {
        qDebug() << "[SpectrumService] DataType=69 频谱数据长度非法:" << spectrumDataLength;
        return;
    }

    const int expectedPayloadLen = 2 + 2 + 4 + 4 + spectrumDataLength + 16;
    if (payloadLen < expectedPayloadLen)
    {
        qDebug() << "[SpectrumService] DataType=69 频谱图上报长度不足，实际:" << payloadLen << "期望:"
                 << expectedPayloadLen << "dataLength:" << spectrumDataLength;
        return;
    }

    const char *spectrumData = payload + 12;
    const char *groupCenterFreqs = spectrumData + spectrumDataLength;
    const float groupFreq1 = normalizeFrequencyToMhz(readFloatLE(groupCenterFreqs));
    const float groupFreq2 = normalizeFrequencyToMhz(readFloatLE(groupCenterFreqs + 4));
    const float groupFreq3 = normalizeFrequencyToMhz(readFloatLE(groupCenterFreqs + 8));
    const float groupFreq4 = normalizeFrequencyToMhz(readFloatLE(groupCenterFreqs + 12));

    if (spectrumDataLength != kSpectrumDataBytes)
    {
        qDebug() << "[SpectrumService] DataType=69 频谱数据长度与协议固定值不一致，当前:" << spectrumDataLength
                 << "协议固定值:" << kSpectrumDataBytes;
    }

    const int sampleCount = spectrumDataLength / static_cast<int>(sizeof(qint16));
    if (sampleCount % kSpectrumValuesPerGroup != 0)
    {
        qDebug() << "[SpectrumService] DataType=69 频谱数据点数无法按分组维度均分，sampleCount =" << sampleCount;
        return;
    }

    QVector<qint16> values;
    values.reserve(sampleCount);
    for (int i = 0; i < sampleCount; ++i)
    {
        values.append(readInt16LE(spectrumData + i * static_cast<int>(sizeof(qint16))));
    }

    const int groupCount = sampleCount / kSpectrumValuesPerGroup;
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
    }

    emit spectrumDataReported(reportData);
}

void SpectrumService::handleFullSpectrumSwitchResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));

    qDebug() << "[SpectrumService] 收到频谱图开关应答 (DataType=219), enabled =" << pendingFullSpectrumEnabled_
             << "success =" << success << "message =" << resultMsg;

    emit fullSpectrumSwitchResponse(pendingFullSpectrumEnabled_, success, resultMsg);
}

void SpectrumService::handleFullSpectrumReport(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        qDebug() << "[SpectrumService] DataType=220 频谱图 JSON 负载为空";
        return;
    }

    const QByteArray payloadBytes(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payloadBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        qDebug() << "[SpectrumService] DataType=220 JSON 解析失败:" << parseError.errorString();
        return;
    }

    const QJsonObject obj = doc.object();
    FullSpectrumReportData reportData;
    reportData.startMhz = obj.value(QStringLiteral("start")).toDouble(300.0);
    reportData.endMhz = obj.value(QStringLiteral("end")).toDouble(6000.0);

    const QJsonArray dataArray = obj.value(QStringLiteral("data")).toArray();
    reportData.data.reserve(dataArray.size());
    for (const QJsonValue &value : dataArray)
    {
        reportData.data.append(value.toInt());
    }

    const QJsonArray countArray = obj.value(QStringLiteral("count")).toArray();
    reportData.markerIndices.reserve(countArray.size());
    for (const QJsonValue &value : countArray)
    {
        reportData.markerIndices.append(value.toInt());
    }

    emit fullSpectrumReported(reportData);
}
