#include "tcp_manager.h"
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
constexpr int kSignalSourceChannelCount = 6;

QString parseResultMessage(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
}
} // namespace

bool TcpManager::dispatchSignalSourceParamsProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 106:
        handleSignalSourceParamsSetResponse(frameData);
        return true;
    case 108:
        handleSignalSourceParamsQueryResponse(frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::setSignalSourceParams(int serialScan, const QVector<int> &scanModes, int vcoMode, const QVector<int> &vcoScans)
{
    QJsonObject json;
    json["SerialScan"] = serialScan;
    for (int i = 0; i < 6; ++i)
    {
        json[QStringLiteral("ScanMode%1").arg(i + 1)] = i < scanModes.size() ? scanModes.at(i) : 0;
        json[QStringLiteral("VcoScan%1").arg(i + 1)] = i < vcoScans.size() ? vcoScans.at(i) : 0;
    }
    json["VcoMode"] = vcoMode;

    sendFrame(105, QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void TcpManager::querySignalSourceParams()
{
    sendFrame(107);
}

void TcpManager::handleSignalSourceParamsSetResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains("RESULT:SUCCESSED");
    emit signalSourceParamsSetResponse(success, resultMsg);
}

void TcpManager::handleSignalSourceParamsQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        qDebug() << "[TcpManager] DataType=108 查询应答为空";
        return;
    }

    const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
    {
        qDebug() << "[TcpManager] DataType=108 JSON 解析失败:" << parseError.errorString();
        return;
    }

    const QJsonObject jsonObject = jsonDoc.object();
    SignalSourceParamsConfig config;
    config.serialScan = jsonObject.value("SerialScan").toInt();
    config.vcoMode = jsonObject.value("VcoMode").toInt();
    config.scanModes.fill(0, kSignalSourceChannelCount);
    config.vcoScans.fill(0, kSignalSourceChannelCount);
    for (int i = 0; i < kSignalSourceChannelCount; ++i)
    {
        const QString scanModeKey = QStringLiteral("ScanMode%1").arg(i + 1);
        const QString vcoScanKey = QStringLiteral("VcoScan%1").arg(i + 1);
        config.scanModes[i] = jsonObject.contains(scanModeKey) ? jsonObject.value(scanModeKey).toInt()
                                                               : jsonObject.value("ScanMode").toInt();
        config.vcoScans[i] = jsonObject.contains(vcoScanKey) ? jsonObject.value(vcoScanKey).toInt()
                                                             : jsonObject.value("VcoScan").toInt();
    }

    emit signalSourceParamsQueried(config);
}
