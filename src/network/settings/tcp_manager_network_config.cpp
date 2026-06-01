#include "tcp_manager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

namespace
{
constexpr int kDeviceIpPayloadLength = 18;
constexpr int kTcpServerIpPayloadLength = 6;

void appendUInt16LE(QByteArray &buffer, quint16 value)
{
    buffer.append(static_cast<char>(value & 0xFF));
    buffer.append(static_cast<char>((value >> 8) & 0xFF));
}

quint16 readUInt16LE(const char *data)
{
    return static_cast<quint16>(static_cast<unsigned char>(data[0])) |
           (static_cast<quint16>(static_cast<unsigned char>(data[1])) << 8);
}

bool appendIpv4Bytes(QByteArray &buffer, const QString &ipText)
{
    const QStringList parts = ipText.split('.');
    if (parts.size() != 4)
    {
        return false;
    }

    for (const QString &part : parts)
    {
        bool ok = false;
        const int value = part.toInt(&ok);
        if (!ok || value < 0 || value > 255)
        {
            return false;
        }
        buffer.append(static_cast<char>(value));
    }

    return true;
}

QString ipv4BytesToString(const char *data)
{
    QStringList parts;
    parts.reserve(4);
    for (int i = 0; i < 4; ++i)
    {
        parts.append(QString::number(static_cast<int>(static_cast<unsigned char>(data[i]))));
    }
    return parts.join('.');
}

} // namespace

bool TcpManager::dispatchNetworkConfigProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 26:
        handleDeviceIpSettingResponse(header, frameData);
        return true;
    case 28:
        handleDeviceIpQueryResponse(header, frameData);
        return true;
    case 194:
        handleFullScanSettingResponse(header, frameData);
        return true;
    case 196:
        handleFullScanQueryResponse(header, frameData);
        return true;
    case 238:
        handleTcpServerIpSettingResponse(header, frameData);
        return true;
    case 240:
        handleTcpServerIpQueryResponse(header, frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::setFullScanParams(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin, double att)
{
    QJsonObject json;
    json["ssth"] = ssth;
    json["ss_jg_max"] = ssJgMax;
    json["ss_jg_min"] = ssJgMin;
    json["ss_max"] = ssMax;
    json["ss_min"] = ssMin;
    json["att"] = att;

    sendFrame(193, QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void TcpManager::queryFullScanParams()
{
    sendFrame(195);
}

void TcpManager::setDeviceIp(const QString &ip, int port, const QString &mask, const QString &route, const QString &dns)
{
    if (port < 0 || port > 65535)
    {
        qDebug() << "[TcpManager] 设备IP设置失败，端口非法:" << port;
        return;
    }

    QByteArray data;
    data.reserve(kDeviceIpPayloadLength);

    if (!appendIpv4Bytes(data, ip) || !appendIpv4Bytes(data, mask) || !appendIpv4Bytes(data, route) ||
        !appendIpv4Bytes(data, dns))
    {
        qDebug() << "[TcpManager] 设备IP设置失败，存在非法 IPv4 地址:" << ip << mask << route << dns;
        return;
    }

    QByteArray payload;
    payload.reserve(kDeviceIpPayloadLength);
    appendIpv4Bytes(payload, ip);
    appendUInt16LE(payload, static_cast<quint16>(port));
    appendIpv4Bytes(payload, mask);
    appendIpv4Bytes(payload, route);
    appendIpv4Bytes(payload, dns);
    sendFrame(25, payload);
}

void TcpManager::queryDeviceIp()
{
    sendFrame(27);
}

void TcpManager::setTcpServerIp(const QString &ip, int port)
{
    if (port < 0 || port > 65535)
    {
        qDebug() << "[TcpManager] TCP服务器IP设置失败，端口非法:" << port;
        return;
    }

    QByteArray payload;
    payload.reserve(kTcpServerIpPayloadLength);

    if (!appendIpv4Bytes(payload, ip))
    {
        qDebug() << "[TcpManager] TCP服务器IP设置失败，非法 IPv4 地址:" << ip;
        return;
    }

    appendUInt16LE(payload, static_cast<quint16>(port));
    sendFrame(237, payload);
}

void TcpManager::queryTcpServerIp()
{
    sendFrame(239);
}

void TcpManager::handleFullScanSettingResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    emit fullScanParamsSetResponse(success, resultMsg);

    if (success)
    {
        queryFullScanParams();
    }
}

void TcpManager::handleFullScanQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        qDebug() << "[TcpManager] DataType=196 查询应答为空";
        return;
    }

    const QByteArray jsonBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        qDebug() << "[TcpManager] DataType=196 JSON 解析失败:" << parseError.errorString();
        return;
    }

    const QJsonObject json = doc.object();
    emit fullScanParamsQueried(json.value("ssth").toDouble(), json.value("ss_jg_max").toDouble(),
                               json.value("ss_jg_min").toDouble(), json.value("ss_max").toDouble(),
                               json.value("ss_min").toDouble(), json.value("att").toDouble());
}

void TcpManager::handleDeviceIpSettingResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    emit deviceIpSetResponse(success, resultMsg);

    if (success)
    {
        queryDeviceIp();
    }
}

void TcpManager::handleDeviceIpQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < kDeviceIpPayloadLength)
    {
        qDebug() << "[TcpManager] DataType=28 查询应答长度不足，实际:" << payloadLen << "期望:" << kDeviceIpPayloadLength;
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    const QString ip = ipv4BytesToString(payload);
    const int port = static_cast<int>(readUInt16LE(payload + 4));
    const QString mask = ipv4BytesToString(payload + 6);
    const QString route = ipv4BytesToString(payload + 10);
    const QString dns = ipv4BytesToString(payload + 14);

    emit deviceIpQueried(ip, port, mask, route, dns);
}

void TcpManager::handleTcpServerIpSettingResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    emit tcpServerIpSetResponse(success, resultMsg);

    if (success)
    {
        queryTcpServerIp();
    }
}

void TcpManager::handleTcpServerIpQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < kTcpServerIpPayloadLength)
    {
        qDebug() << "[TcpManager] DataType=240 查询应答长度不足，实际:" << payloadLen << "期望:"
                 << kTcpServerIpPayloadLength;
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    const QString ip = ipv4BytesToString(payload);
    const int port = static_cast<int>(readUInt16LE(payload + 4));
    emit tcpServerIpQueried(ip, port);
}
