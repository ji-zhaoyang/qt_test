#include "tcp_manager.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
QString parseResultMessage(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
}
} // namespace

bool TcpManager::dispatchStrikeFrequencyProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 97:
        handleStrikeFrequencySetResponse(frameData);
        return true;
    case 99:
        handleStrikeFrequencyQueryResponse(frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::setStrikeFrequencyBands(const StrikeFrequencyBandList &bands)
{
    QJsonArray jsonArray;
    for (const StrikeFrequencyBandConfig &band : bands)
    {
        QJsonObject item;
        item["enable"] = band.enable;
        item["start"] = band.startMhz;
        item["end"] = band.endMhz;
        item["att"] = band.att;
        jsonArray.append(item);
    }

    sendFrame(96, QJsonDocument(jsonArray).toJson(QJsonDocument::Compact));
}

void TcpManager::queryStrikeFrequencyBands()
{
    sendFrame(98);
}

void TcpManager::handleStrikeFrequencySetResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains("RESULT:SUCCESSED");
    emit strikeFrequencyBandsSetResponse(success, resultMsg);
}

void TcpManager::handleStrikeFrequencyQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        qDebug() << "[TcpManager] DataType=99 查询应答为空";
        return;
    }

    const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isArray())
    {
        qDebug() << "[TcpManager] DataType=99 JSON 解析失败:" << parseError.errorString();
        return;
    }

    StrikeFrequencyBandList bands;
    const QJsonArray jsonArray = jsonDoc.array();
    bands.reserve(jsonArray.size());
    for (const QJsonValue &value : jsonArray)
    {
        if (!value.isObject())
        {
            continue;
        }

        const QJsonObject item = value.toObject();
        StrikeFrequencyBandConfig band;
        band.enable = item.value("enable").toInt();
        band.startMhz = item.value("start").toDouble();
        band.endMhz = item.value("end").toDouble();
        band.att = item.value("att").toInt();
        bands.append(band);
    }

    emit strikeFrequencyBandsQueried(bands);
}
