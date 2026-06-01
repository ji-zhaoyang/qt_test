#include "tcp_manager.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
constexpr int kPowerAmplifierChannelCount = 6;

QString parseResultMessage(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
}
} // namespace

bool TcpManager::dispatchPowerAmplifierProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 119:
        handlePowerAmplifierSetResponse(frameData);
        return true;
    case 121:
        handlePowerAmplifierQueryResponse(frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::setPowerAmplifierParams(const PowerAmplifierParamList &params)
{
    QJsonArray jsonArray;
    for (int i = 0; i < kPowerAmplifierChannelCount; ++i)
    {
        const PowerAmplifierParam param = i < params.size() ? params.at(i) : PowerAmplifierParam();
        QJsonObject item;
        item["K"] = param.k;
        item["B"] = param.b;
        item["att"] = param.att;
        jsonArray.append(item);
    }

    sendFrame(118, QJsonDocument(jsonArray).toJson(QJsonDocument::Compact));
}

void TcpManager::queryPowerAmplifierParams()
{
    qDebug() << "[TcpManager] 准备发送功放参数查询命令 (DataType=120)";
    sendFrame(120);
}

void TcpManager::handlePowerAmplifierSetResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains("RESULT:SUCCESSED");
    emit powerAmplifierParamsSetResponse(success, resultMsg);
}

void TcpManager::handlePowerAmplifierQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        qDebug() << "[TcpManager] DataType=121 查询应答为空";
        return;
    }

    const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isArray())
    {
        qDebug() << "[TcpManager] DataType=121 JSON 解析失败:" << parseError.errorString();
        return;
    }

    const QJsonArray jsonArray = jsonDoc.array();
    PowerAmplifierParamList params;
    params.reserve(kPowerAmplifierChannelCount);
    for (int i = 0; i < kPowerAmplifierChannelCount; ++i)
    {
        PowerAmplifierParam param;
        if (i < jsonArray.size() && jsonArray.at(i).isObject())
        {
            const QJsonObject item = jsonArray.at(i).toObject();
            param.k = item.value("K").toDouble();
            param.b = item.value("B").toDouble();
            param.outpower = item.value("outpower").toDouble();
            param.att = item.value("att").toDouble();
        }
        params.append(param);
    }

    QStringList resultTexts;
    resultTexts.reserve(params.size());
    for (int i = 0; i < params.size(); ++i)
    {
        const PowerAmplifierParam &param = params.at(i);
        resultTexts.append(QStringLiteral("PA%1{K=%2, B=%3, outpower=%4, att=%5}")
                               .arg(i + 1)
                               .arg(param.k, 0, 'g', 10)
                               .arg(param.b, 0, 'g', 10)
                               .arg(param.outpower, 0, 'g', 10)
                               .arg(param.att, 0, 'g', 10));
    }
    qDebug() << "[TcpManager] 功放参数查询结果:" << resultTexts.join(QStringLiteral("; "));
    emit powerAmplifierParamsQueried(params);
}
