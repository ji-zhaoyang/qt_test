#include "tcp_manager.h"
#include "services/calibration_service.h"
#include "services/device_ops_service.h"
#include "services/drone_ops_service.h"
#include "services/spectrum_service.h"
#include "services/settings_protocol_service.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <cstdlib>
#include <cstring>

namespace
{
QString socketErrorText(QAbstractSocket::SocketError error)
{
    switch (error)
    {
    case QAbstractSocket::ConnectionRefusedError:
        return "ConnectionRefusedError";
    case QAbstractSocket::RemoteHostClosedError:
        return "RemoteHostClosedError";
    case QAbstractSocket::HostNotFoundError:
        return "HostNotFoundError";
    case QAbstractSocket::SocketAccessError:
        return "SocketAccessError";
    case QAbstractSocket::SocketResourceError:
        return "SocketResourceError";
    case QAbstractSocket::SocketTimeoutError:
        return "SocketTimeoutError";
    case QAbstractSocket::DatagramTooLargeError:
        return "DatagramTooLargeError";
    case QAbstractSocket::NetworkError:
        return "NetworkError";
    case QAbstractSocket::AddressInUseError:
        return "AddressInUseError";
    case QAbstractSocket::SocketAddressNotAvailableError:
        return "SocketAddressNotAvailableError";
    case QAbstractSocket::UnsupportedSocketOperationError:
        return "UnsupportedSocketOperationError";
    case QAbstractSocket::ProxyAuthenticationRequiredError:
        return "ProxyAuthenticationRequiredError";
    case QAbstractSocket::SslHandshakeFailedError:
        return "SslHandshakeFailedError";
    case QAbstractSocket::UnfinishedSocketOperationError:
        return "UnfinishedSocketOperationError";
    case QAbstractSocket::ProxyConnectionRefusedError:
        return "ProxyConnectionRefusedError";
    case QAbstractSocket::ProxyConnectionClosedError:
        return "ProxyConnectionClosedError";
    case QAbstractSocket::ProxyConnectionTimeoutError:
        return "ProxyConnectionTimeoutError";
    case QAbstractSocket::ProxyNotFoundError:
        return "ProxyNotFoundError";
    case QAbstractSocket::ProxyProtocolError:
        return "ProxyProtocolError";
    case QAbstractSocket::OperationError:
        return "OperationError";
    case QAbstractSocket::SslInternalError:
        return "SslInternalError";
    case QAbstractSocket::SslInvalidUserDataError:
        return "SslInvalidUserDataError";
    case QAbstractSocket::TemporaryError:
        return "TemporaryError";
    case QAbstractSocket::UnknownSocketError:
    default:
        return "UnknownSocketError";
    }
}

bool shouldLogStrikeFrequencyFrame(uint16_t dataType)
{
    return dataType == 96 || dataType == 97 || dataType == 98 || dataType == 99;
}

QString strikeFrequencyFrameLabel(uint16_t dataType)
{
    switch (dataType)
    {
    case 96:
        return QStringLiteral("保存下发");
    case 97:
        return QStringLiteral("保存应答");
    case 98:
        return QStringLiteral("查询下发");
    case 99:
        return QStringLiteral("查询应答");
    default:
        return QStringLiteral("未知");
    }
}

bool isCommunicationJammingCommand(uint16_t dataType, const QByteArray &payload)
{
    if (dataType != 100)
    {
        return false;
    }

    const QByteArray compactPayload = payload.trimmed();
    return compactPayload == QByteArrayLiteral("{\"mode\":0,\"switch\":1}") ||
           compactPayload == QByteArrayLiteral("{\"mode\":0,\"switch\":0}");
}

QString communicationJammingFrameLabel(const QByteArray &payload)
{
    const QByteArray compactPayload = payload.trimmed();
    if (compactPayload == QByteArrayLiteral("{\"mode\":0,\"switch\":1}"))
    {
        return QStringLiteral("通信干扰打开");
    }
    if (compactPayload == QByteArrayLiteral("{\"mode\":0,\"switch\":0}"))
    {
        return QStringLiteral("通信干扰关闭");
    }
    return QStringLiteral("通信干扰");
}

bool isPrecisionStrikeFrame(uint16_t dataType)
{
    return dataType == 109 || dataType == 110;
}

QString precisionStrikeFrameLabel(uint16_t dataType)
{
    switch (dataType)
    {
    case 109:
        return QStringLiteral("精准打击设置");
    case 110:
        return QStringLiteral("精准打击应答");
    default:
        return QStringLiteral("精准打击");
    }
}

QString hexSliceUpper(const QByteArray &data, int offset, int length)
{
    if (offset < 0 || length <= 0 || offset + length > data.size())
    {
        return QString();
    }
    return QString::fromLatin1(data.mid(offset, length).toHex(' ').toUpper());
}

QString formatProtocolTimestampText(const ProtocolHeader *header)
{
    return QStringLiteral("%1/%2/%3 %4:%5:%6.%7")
        .arg(header->year)
        .arg(header->month, 2, 10, QChar('0'))
        .arg(header->day, 2, 10, QChar('0'))
        .arg(header->hour, 2, 10, QChar('0'))
        .arg(header->minute, 2, 10, QChar('0'))
        .arg(header->second, 2, 10, QChar('0'))
        .arg(header->millisecond, 3, 10, QChar('0'));
}

QString droneReportModeLabel(uint8_t mode)
{
    static const QStringList labels = {
        QStringLiteral("协议分析模式"),
        QStringLiteral("频谱分析模式"),
        QStringLiteral("混合模式"),
        QStringLiteral("频段扫描模式"),
        QStringLiteral("组合模式"),
    };
    if (mode < labels.size())
    {
        return labels.at(mode);
    }
    return QStringLiteral("未知(%1)").arg(mode);
}

QString gpsSettingModeLabel(uint8_t mode)
{
    switch (mode)
    {
    case 0:
        return QStringLiteral("手动");
    case 1:
        return QStringLiteral("自动");
    default:
        return QStringLiteral("未知(%1)").arg(mode);
    }
}

QString uavCategoryDisplayModeLabel(uint8_t mode)
{
    switch (mode)
    {
    case 0:
        return QStringLiteral("详细");
    case 1:
        return QStringLiteral("简略");
    default:
        return QStringLiteral("未知(%1)").arg(mode);
    }
}

void logFullScanJsonPayloadFields(const QByteArray &payload)
{
    qDebug().noquote() << QStringLiteral("载荷 JSON：%1 : %2")
                              .arg(QString::fromLatin1(payload.toHex(' ').toUpper()), QString::fromUtf8(payload));

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        return;
    }

    const QJsonObject json = doc.object();
    qDebug().noquote() << QStringLiteral("ssth（信号强度比值门限）：%1")
                              .arg(json.value(QStringLiteral("ssth")).toDouble());
    qDebug().noquote() << QStringLiteral("ss_jg_max（信号带宽最大值 MHz）：%1")
                              .arg(json.value(QStringLiteral("ss_jg_max")).toDouble());
    qDebug().noquote() << QStringLiteral("ss_jg_min（信号带宽最小值 MHz）：%1")
                              .arg(json.value(QStringLiteral("ss_jg_min")).toDouble());
    qDebug().noquote() << QStringLiteral("ss_max（跳变比值最大值）：%1")
                              .arg(json.value(QStringLiteral("ss_max")).toDouble());
    qDebug().noquote() << QStringLiteral("ss_min（跳变比值最小值）：%1")
                              .arg(json.value(QStringLiteral("ss_min")).toDouble());
    qDebug().noquote() << QStringLiteral("att（信号增益）：%1").arg(json.value(QStringLiteral("att")).toDouble());
}

QString fullSpectrumSwitchModeLabel(uint8_t mode)
{
    switch (mode)
    {
    case 0:
        return QStringLiteral("关闭");
    case 1:
        return QStringLiteral("打开");
    default:
        return QStringLiteral("未知(%1)").arg(mode);
    }
}

QString featureModeEnableLabel(uint8_t enabled)
{
    switch (enabled)
    {
    case 0:
        return QStringLiteral("关闭");
    case 1:
        return QStringLiteral("开启");
    default:
        return QStringLiteral("未知(%1)").arg(enabled);
    }
}

void logFeatureModePayloadFields(const QByteArray &payload)
{
    if (payload.size() < 3)
    {
        return;
    }

    const uint8_t wifiRemoteIdEnabled = static_cast<uint8_t>(static_cast<unsigned char>(payload.at(0)));
    const uint8_t fpvEnabled = static_cast<uint8_t>(static_cast<unsigned char>(payload.at(1)));
    const uint8_t djiParseEnabled = static_cast<uint8_t>(static_cast<unsigned char>(payload.at(2)));

    qDebug().noquote()
        << QStringLiteral("增强型WiFi/RemoteID：%1 : %2 — %3")
               .arg(hexSliceUpper(payload, 0, 1))
               .arg(wifiRemoteIdEnabled)
               .arg(featureModeEnableLabel(wifiRemoteIdEnabled));
    qDebug().noquote() << QStringLiteral("频谱扫描FPV：%1 : %2 — %3")
                              .arg(hexSliceUpper(payload, 1, 1))
                              .arg(fpvEnabled)
                              .arg(featureModeEnableLabel(fpvEnabled));
    qDebug().noquote() << QStringLiteral("大疆无人机解析：%1 : %2 — %3")
                              .arg(hexSliceUpper(payload, 2, 1))
                              .arg(djiParseEnabled)
                              .arg(featureModeEnableLabel(djiParseEnabled));
}

void logFullSpectrumJsonPayloadFields(const QByteArray &payload)
{
    qDebug().noquote() << QStringLiteral("载荷 JSON：%1 : %2")
                              .arg(QString::fromLatin1(payload.toHex(' ').toUpper()), QString::fromUtf8(payload));

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        return;
    }

    const QJsonObject json = doc.object();
    qDebug().noquote() << QStringLiteral("start（起始频段 MHz）：%1")
                              .arg(json.value(QStringLiteral("start")).toDouble());
    qDebug().noquote() << QStringLiteral("end（结束频段 MHz）：%1")
                              .arg(json.value(QStringLiteral("end")).toDouble());

    const QJsonArray dataArray = json.value(QStringLiteral("data")).toArray();
    if (dataArray.isEmpty())
    {
        qDebug().noquote() << QStringLiteral("data（频谱数据）：空数组");
    }
    else
    {
        QStringList preview;
        const int previewCount = qMin(5, dataArray.size());
        preview.reserve(previewCount);
        for (int i = 0; i < previewCount; ++i)
        {
            preview.append(QString::number(dataArray.at(i).toInt()));
        }
        qDebug().noquote() << QStringLiteral("data（频谱数据 %1 点，前 %2 个）：[%3]")
                                  .arg(dataArray.size())
                                  .arg(previewCount)
                                  .arg(preview.join(QStringLiteral(", ")));
    }

    const QJsonArray countArray = json.value(QStringLiteral("count")).toArray();
    if (countArray.isEmpty())
    {
        qDebug().noquote() << QStringLiteral("count（疑似信号点）：空数组");
    }
    else
    {
        QStringList markers;
        markers.reserve(countArray.size());
        for (const QJsonValue &value : countArray)
        {
            markers.append(QString::number(value.toInt()));
        }
        const int previewCount = qMin(8, markers.size());
        qDebug().noquote() << QStringLiteral("count（疑似信号点 %1 个）：[%2]")
                                  .arg(countArray.size())
                                  .arg(markers.mid(0, previewCount).join(QStringLiteral(", ")));
    }
}

void logProtocolFrameReferenceFormat(const char *tag, const char *direction, const QByteArray &frameData,
                                     const QString &dataTypeName)
{
    if (frameData.size() < static_cast<int>(sizeof(ProtocolHeader) + sizeof(ProtocolTail)))
    {
        return;
    }

    ProtocolHeader headerValue;
    std::memcpy(&headerValue, frameData.constData(), sizeof(headerValue));

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QByteArray payload = frameData.mid(sizeof(ProtocolHeader), payloadLen);

    ProtocolTail tailValue;
    std::memcpy(&tailValue, frameData.constData() + frameData.size() - sizeof(ProtocolTail), sizeof(tailValue));

    const QString fullHex = QString::fromLatin1(frameData.toHex(' ').toUpper());
    const QString timestampHex = hexSliceUpper(frameData, 10, 9);
    const QString packageNumHex = hexSliceUpper(frameData, 21, 8);

    qDebug().noquote() << QStringLiteral("[%1][%2] 参考格式 — %3 (dataType=%4, %5 字节)")
                              .arg(QString::fromUtf8(tag),
                                   QString::fromUtf8(direction),
                                   dataTypeName,
                                   QString::number(headerValue.dataType),
                                   QString::number(frameData.size()));
    qDebug().noquote() << QStringLiteral("完整报文：");
    qDebug().noquote() << fullHex;
    qDebug().noquote() << QStringLiteral("开始标志：%1").arg(hexSliceUpper(frameData, 0, 4));
    qDebug().noquote() << QStringLiteral("协议版本：%1 : 0x%2")
                              .arg(hexSliceUpper(frameData, 4, 2))
                              .arg(headerValue.version, 4, 16, QChar('0'));
    qDebug().noquote() << QStringLiteral("数据帧长度：%1 : %2 字节")
                              .arg(hexSliceUpper(frameData, 6, 4))
                              .arg(headerValue.length);
    qDebug().noquote() << QStringLiteral("数据帧时间戳：%1 : %2")
                              .arg(timestampHex, formatProtocolTimestampText(&headerValue));
    qDebug().noquote() << QStringLiteral("数据类型：%1 : %2 — %3")
                              .arg(hexSliceUpper(frameData, 19, 2))
                              .arg(headerValue.dataType)
                              .arg(dataTypeName);
    qDebug().noquote() << QStringLiteral("包编号：%1 : %2 号包").arg(packageNumHex).arg(headerValue.packageNum);

    if ((headerValue.dataType == 61 || headerValue.dataType == 64) && payloadLen >= 1)
    {
        const uint8_t mode = static_cast<uint8_t>(static_cast<unsigned char>(payload.at(0)));
        qDebug().noquote() << QStringLiteral("上报模式 mode：%1 : %2 — %3")
                                  .arg(hexSliceUpper(payload, 0, 1))
                                  .arg(mode)
                                  .arg(droneReportModeLabel(mode));
    }
    else if (headerValue.dataType == 63)
    {
        qDebug().noquote() << QStringLiteral("载荷：（空，查询命令无 Payload）");
    }
    else if (headerValue.dataType == 62)
    {
        qDebug().noquote() << QStringLiteral("应答结果：%1 : %2")
                                  .arg(QString::fromLatin1(payload.toHex(' ').toUpper()),
                                       QString::fromUtf8(payload));
    }
    else if (headerValue.dataType == 109)
    {
        qDebug().noquote() << QStringLiteral("载荷 JSON：%1 : %2")
                                  .arg(QString::fromLatin1(payload.toHex(' ').toUpper()),
                                       QString::fromUtf8(payload));
    }
    else if (headerValue.dataType == 110)
    {
        qDebug().noquote() << QStringLiteral("应答结果：%1 : %2")
                                  .arg(QString::fromLatin1(payload.toHex(' ').toUpper()),
                                       QString::fromUtf8(payload));
    }
    else if ((headerValue.dataType == 8 || headerValue.dataType == 11) && payloadLen >= 2)
    {
        quint16 bandCount = 0;
        std::memcpy(&bandCount, payload.constData(), sizeof(bandCount));
        qDebug().noquote() << QStringLiteral("频段数量：%1 : %2")
                                  .arg(hexSliceUpper(payload, 0, 2))
                                  .arg(bandCount);
        int offset = 2;
        for (int i = 0; i < bandCount && offset + 12 <= payloadLen; ++i)
        {
            float freqMhz = 0.0f;
            qint32 measureCount = 0;
            qint32 gain = 0;
            std::memcpy(&freqMhz, payload.constData() + offset, sizeof(freqMhz));
            std::memcpy(&measureCount, payload.constData() + offset + 4, sizeof(measureCount));
            std::memcpy(&gain, payload.constData() + offset + 8, sizeof(gain));
            qDebug().noquote()
                << QStringLiteral("频段[%1] 频率MHz：%2 : %3 MHz")
                       .arg(i)
                       .arg(hexSliceUpper(payload, offset, 4))
                       .arg(static_cast<double>(freqMhz), 0, 'f', 3);
            qDebug().noquote()
                << QStringLiteral("频段[%1] 测量次数：%2 : %3")
                       .arg(i)
                       .arg(hexSliceUpper(payload, offset + 4, 4))
                       .arg(measureCount);
            qDebug().noquote()
                << QStringLiteral("频段[%1] 增益：%2 : %3")
                       .arg(i)
                       .arg(hexSliceUpper(payload, offset + 8, 4))
                       .arg(gain);
            offset += 12;
        }
    }
    else if (headerValue.dataType == 9)
    {
        qDebug().noquote() << QStringLiteral("应答结果：%1 : %2")
                                  .arg(QString::fromLatin1(payload.toHex(' ').toUpper()),
                                       QString::fromUtf8(payload));
    }
    else if (headerValue.dataType == 10)
    {
        qDebug().noquote() << QStringLiteral("载荷：（空，查询命令无 Payload）");
    }
    else if (headerValue.dataType == 57 &&
             payloadLen >= static_cast<int>(sizeof(GpsSettingPayload)))
    {
        GpsSettingPayload gpsPayload;
        std::memcpy(&gpsPayload, payload.constData(), sizeof(gpsPayload));
        qDebug().noquote() << QStringLiteral("GPS 模式 mode：%1 : %2 — %3")
                                  .arg(hexSliceUpper(payload, 0, 1))
                                  .arg(gpsPayload.mode)
                                  .arg(gpsSettingModeLabel(gpsPayload.mode));
        qDebug().noquote() << QStringLiteral("设备所在经度 longitude：%1 : %2")
                                  .arg(hexSliceUpper(payload, 1, 4))
                                  .arg(static_cast<double>(gpsPayload.longitude), 0, 'f', 6);
        qDebug().noquote() << QStringLiteral("设备所在纬度 latitude：%1 : %2")
                                  .arg(hexSliceUpper(payload, 5, 4))
                                  .arg(static_cast<double>(gpsPayload.latitude), 0, 'f', 6);
        qDebug().noquote() << QStringLiteral("设备所在海拔 altitude：%1 : %2 米")
                                  .arg(hexSliceUpper(payload, 9, 4))
                                  .arg(static_cast<double>(gpsPayload.altitude), 0, 'f', 1);
    }
    else if (headerValue.dataType == 58)
    {
        qDebug().noquote() << QStringLiteral("应答结果：%1 : %2")
                                  .arg(QString::fromLatin1(payload.toHex(' ').toUpper()),
                                       QString::fromUtf8(payload));
    }
    else if (headerValue.dataType == 59)
    {
        qDebug().noquote() << QStringLiteral("载荷：（空，查询命令无 Payload）");
    }
    else if (headerValue.dataType == 60 &&
             payloadLen >= static_cast<int>(sizeof(GpsSettingPayload)))
    {
        GpsSettingPayload gpsPayload;
        std::memcpy(&gpsPayload, payload.constData(), sizeof(gpsPayload));
        qDebug().noquote() << QStringLiteral("GPS 模式 mode：%1 : %2 — %3")
                                  .arg(hexSliceUpper(payload, 0, 1))
                                  .arg(gpsPayload.mode)
                                  .arg(gpsSettingModeLabel(gpsPayload.mode));
        qDebug().noquote() << QStringLiteral("设备所在经度 longitude：%1 : %2")
                                  .arg(hexSliceUpper(payload, 1, 4))
                                  .arg(static_cast<double>(gpsPayload.longitude), 0, 'f', 6);
        qDebug().noquote() << QStringLiteral("设备所在纬度 latitude：%1 : %2")
                                  .arg(hexSliceUpper(payload, 5, 4))
                                  .arg(static_cast<double>(gpsPayload.latitude), 0, 'f', 6);
        qDebug().noquote() << QStringLiteral("设备所在海拔 altitude：%1 : %2 米")
                                  .arg(hexSliceUpper(payload, 9, 4))
                                  .arg(static_cast<double>(gpsPayload.altitude), 0, 'f', 1);
    }
    else if (headerValue.dataType == 181 && payloadLen >= 1)
    {
        const uint8_t mode = static_cast<uint8_t>(static_cast<unsigned char>(payload.at(0)));
        qDebug().noquote() << QStringLiteral("无人机类别显示 mode：%1 : %2 — %3")
                                  .arg(hexSliceUpper(payload, 0, 1))
                                  .arg(mode)
                                  .arg(uavCategoryDisplayModeLabel(mode));
    }
    else if (headerValue.dataType == 182)
    {
        qDebug().noquote() << QStringLiteral("应答结果：%1 : %2")
                                  .arg(QString::fromLatin1(payload.toHex(' ').toUpper()),
                                       QString::fromUtf8(payload));
    }
    else if (headerValue.dataType == 183)
    {
        qDebug().noquote() << QStringLiteral("载荷：（空，查询命令无 Payload）");
    }
    else if (headerValue.dataType == 184 && payloadLen >= 1)
    {
        const uint8_t mode = static_cast<uint8_t>(static_cast<unsigned char>(payload.at(0)));
        qDebug().noquote() << QStringLiteral("无人机类别显示 mode：%1 : %2 — %3")
                                  .arg(hexSliceUpper(payload, 0, 1))
                                  .arg(mode)
                                  .arg(uavCategoryDisplayModeLabel(mode));
        if (payloadLen > 1)
        {
            qDebug().noquote() << QStringLiteral("附加载荷：%1 : %2")
                                      .arg(QString::fromLatin1(payload.mid(1).toHex(' ').toUpper()),
                                           QString::fromUtf8(payload.mid(1)));
        }
    }
    else if ((headerValue.dataType == 193 || headerValue.dataType == 196) && payloadLen > 0)
    {
        logFullScanJsonPayloadFields(payload);
    }
    else if (headerValue.dataType == 194)
    {
        qDebug().noquote() << QStringLiteral("应答结果：%1 : %2")
                                  .arg(QString::fromLatin1(payload.toHex(' ').toUpper()),
                                       QString::fromUtf8(payload));
    }
    else if (headerValue.dataType == 195)
    {
        qDebug().noquote() << QStringLiteral("载荷：（空，查询命令无 Payload）");
    }
    else if (headerValue.dataType == 218 && payloadLen >= 1)
    {
        const uint8_t mode = static_cast<uint8_t>(static_cast<unsigned char>(payload.at(0)));
        qDebug().noquote() << QStringLiteral("模式使能 enable：%1 : %2 — %3")
                                  .arg(hexSliceUpper(payload, 0, 1))
                                  .arg(mode)
                                  .arg(fullSpectrumSwitchModeLabel(mode));
    }
    else if (headerValue.dataType == 219)
    {
        qDebug().noquote() << QStringLiteral("应答结果：%1 : %2")
                                  .arg(QString::fromLatin1(payload.toHex(' ').toUpper()),
                                       QString::fromUtf8(payload));
    }
    else if (headerValue.dataType == 220 && payloadLen > 0)
    {
        logFullSpectrumJsonPayloadFields(payload);
    }
    else if (headerValue.dataType == 252)
    {
        qDebug().noquote() << QStringLiteral("载荷：（空，查询命令无 Payload）");
    }
    else if ((headerValue.dataType == 253 || headerValue.dataType == 254) && payloadLen >= 3)
    {
        logFeatureModePayloadFields(payload);
    }
    else if (headerValue.dataType == 255)
    {
        qDebug().noquote() << QStringLiteral("应答结果：%1 : %2")
                                  .arg(QString::fromLatin1(payload.toHex(' ').toUpper()),
                                       QString::fromUtf8(payload));
    }
    else if (payloadLen > 0)
    {
        qDebug().noquote() << QStringLiteral("载荷：%1").arg(QString::fromLatin1(payload.toHex(' ').toUpper()));
    }

    qDebug().noquote() << QStringLiteral("校验和：%1").arg(hexSliceUpper(frameData, frameData.size() - 5, 1));
    qDebug().noquote() << QStringLiteral("结束标志：%1").arg(hexSliceUpper(frameData, frameData.size() - 4, 4));
}

void logPrecisionStrikeFrame(const char *direction, uint16_t dataType, const QByteArray &frameData)
{
    logProtocolFrameReferenceFormat("PrecisionStrikeFrame", direction, frameData, precisionStrikeFrameLabel(dataType));
}

bool isDroneReportModeFrame(uint16_t dataType)
{
    return dataType == 61 || dataType == 62 || dataType == 63 || dataType == 64;
}

QString droneReportModeFrameLabel(uint16_t dataType)
{
    switch (dataType)
    {
    case 61:
        return QStringLiteral("无人机上报模式设置");
    case 62:
        return QStringLiteral("上报模式设置应答");
    case 63:
        return QStringLiteral("无人机上报模式查询");
    case 64:
        return QStringLiteral("上报模式查询应答");
    default:
        return QStringLiteral("上报模式");
    }
}

void logDroneReportModeFrame(const char *direction, uint16_t dataType, const QByteArray &frameData)
{
    logProtocolFrameReferenceFormat("DroneReportModeFrame", direction, frameData, droneReportModeFrameLabel(dataType));
}

bool isDetectBandFrame(uint16_t dataType)
{
    return dataType == 8 || dataType == 9 || dataType == 10 || dataType == 11;
}

QString detectBandFrameLabel(uint16_t dataType)
{
    switch (dataType)
    {
    case 8:
        return QStringLiteral("检测频段参数设置");
    case 9:
        return QStringLiteral("检测频段设置应答");
    case 10:
        return QStringLiteral("检测频段参数查询");
    case 11:
        return QStringLiteral("检测频段查询应答");
    default:
        return QStringLiteral("检测频段");
    }
}

void logDetectBandFrame(const char *direction, uint16_t dataType, const QByteArray &frameData)
{
    logProtocolFrameReferenceFormat("DetectBandFrame", direction, frameData, detectBandFrameLabel(dataType));
}

bool isGpsSettingFrame(uint16_t dataType)
{
    return dataType == 57 || dataType == 58 || dataType == 59 || dataType == 60;
}

QString gpsSettingFrameLabel(uint16_t dataType)
{
    switch (dataType)
    {
    case 57:
        return QStringLiteral("GPS 设置");
    case 58:
        return QStringLiteral("GPS 设置应答");
    case 59:
        return QStringLiteral("GPS 查询");
    case 60:
        return QStringLiteral("GPS 查询应答");
    default:
        return QStringLiteral("GPS");
    }
}

void logGpsSettingFrame(const char *direction, uint16_t dataType, const QByteArray &frameData)
{
    logProtocolFrameReferenceFormat("GpsSettingFrame", direction, frameData, gpsSettingFrameLabel(dataType));
}

bool isUavCategoryDisplayModeFrame(uint16_t dataType)
{
    return dataType == 181 || dataType == 182 || dataType == 183 || dataType == 184;
}

QString uavCategoryDisplayModeFrameLabel(uint16_t dataType)
{
    switch (dataType)
    {
    case 181:
        return QStringLiteral("无人机类别显示设置");
    case 182:
        return QStringLiteral("无人机类别显示设置应答");
    case 183:
        return QStringLiteral("无人机类别显示查询");
    case 184:
        return QStringLiteral("无人机类别显示查询应答");
    default:
        return QStringLiteral("无人机类别显示");
    }
}

void logUavCategoryDisplayModeFrame(const char *direction, uint16_t dataType, const QByteArray &frameData)
{
    logProtocolFrameReferenceFormat("UavCategoryDisplayModeFrame", direction, frameData,
                                    uavCategoryDisplayModeFrameLabel(dataType));
}

bool isFullScanFrame(uint16_t dataType)
{
    return dataType == 193 || dataType == 194 || dataType == 195 || dataType == 196;
}

QString fullScanFrameLabel(uint16_t dataType)
{
    switch (dataType)
    {
    case 193:
        return QStringLiteral("全频扫描参数设置");
    case 194:
        return QStringLiteral("全频扫描参数设置应答");
    case 195:
        return QStringLiteral("全频扫描参数查询");
    case 196:
        return QStringLiteral("全频扫描参数查询应答");
    default:
        return QStringLiteral("全频扫描");
    }
}

void logFullScanFrame(const char *direction, uint16_t dataType, const QByteArray &frameData)
{
    logProtocolFrameReferenceFormat("FullScanFrame", direction, frameData, fullScanFrameLabel(dataType));
}

bool isFullSpectrumFrame(uint16_t dataType)
{
    return dataType == 218 || dataType == 219 || dataType == 220;
}

QString fullSpectrumFrameLabel(uint16_t dataType)
{
    switch (dataType)
    {
    case 218:
        return QStringLiteral("全频频段谱图开关");
    case 219:
        return QStringLiteral("全频频段谱图开关应答");
    case 220:
        return QStringLiteral("全频段谱图数据上报");
    default:
        return QStringLiteral("全频谱图");
    }
}

void logFullSpectrumFrame(const char *direction, uint16_t dataType, const QByteArray &frameData)
{
    logProtocolFrameReferenceFormat("FullSpectrumFrame", direction, frameData, fullSpectrumFrameLabel(dataType));
}

bool isFeatureModeFrame(uint16_t dataType)
{
    return dataType == 252 || dataType == 253 || dataType == 254 || dataType == 255;
}

QString featureModeFrameLabel(uint16_t dataType)
{
    switch (dataType)
    {
    case 252:
        return QStringLiteral("功能模式使能查询");
    case 253:
        return QStringLiteral("功能模式使能查询应答");
    case 254:
        return QStringLiteral("功能模式使能设置");
    case 255:
        return QStringLiteral("功能模式使能设置应答");
    default:
        return QStringLiteral("功能模式使能");
    }
}

void logFeatureModeFrame(const char *direction, uint16_t dataType, const QByteArray &frameData)
{
    logProtocolFrameReferenceFormat("FeatureModeFrame", direction, frameData, featureModeFrameLabel(dataType));
}

bool looksLikeHexAsciiPayload(const QByteArray &data)
{
    if (data.size() < 8)
    {
        return false;
    }

    int hexCharCount = 0;
    for (uchar ch : data)
    {
        if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t')
        {
            continue;
        }

        if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))
        {
            hexCharCount++;
            continue;
        }

        return false;
    }

    return hexCharCount > 0;
}

} // namespace

TcpManager::TcpManager(QObject *parent)
    : QObject(parent),
      droneOpsService(new DroneOpsService(this)),
      deviceOpsService(new DeviceOpsService(this)),
      calibrationService(new CalibrationService(this)),
      spectrumService(new SpectrumService(this)),
      settingsProtocolService(new SettingsProtocolService(this))
{
    qRegisterMetaType<SpectrumReportData>("SpectrumReportData");
    qRegisterMetaType<FullSpectrumReportData>("FullSpectrumReportData");
    qRegisterMetaType<StrikeFrequencyBandList>("StrikeFrequencyBandList");
    qRegisterMetaType<PowerAmplifierParamList>("PowerAmplifierParamList");
    qRegisterMetaType<DirectionCalibrationValueList>("DirectionCalibrationValueList");
    qRegisterMetaType<AlarmHistoryInfo>("AlarmHistoryInfo");
    qRegisterMetaType<DeviceUsageInfo>("DeviceUsageInfo");
    qRegisterMetaType<SignalSourceParamsConfig>("SignalSourceParamsConfig");
    qRegisterMetaType<PatternUploadRequest>("PatternUploadRequest");
    qRegisterMetaType<ModelLibraryPageResult>("ModelLibraryPageResult");
    qRegisterMetaType<ModelLibraryUpdateRequest>("ModelLibraryUpdateRequest");

    tcpSocket = new QTcpSocket(this);

    // 初始化断线重连定时器
    reconnectTimer = new QTimer(this);
    reconnectTimer->setSingleShot(true);
    reconnectTimer->setInterval(reconnectIntervalMs);
    connect(reconnectTimer, &QTimer::timeout, this, &TcpManager::onReconnectTimeout);

    // 绑定底层 socket 信号到当前类的槽函数
    connect(tcpSocket, &QTcpSocket::connected, this, &TcpManager::onSocketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &TcpManager::onSocketDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &TcpManager::onSocketReadyRead);
    connect(tcpSocket, static_cast<void (QTcpSocket::*)(QTcpSocket::SocketError)>(&QTcpSocket::error), this,
            &TcpManager::onSocketError);

    droneOpsService->setFrameSender(
        [this](uint16_t dataType, const QByteArray &payload)
        {
            sendFrame(dataType, payload);
        });
    connect(droneOpsService, &DroneOpsService::droneTargetReported, this, &TcpManager::droneTargetReported);
    connect(droneOpsService, &DroneOpsService::droneDirectionFindingResponse,
            this, &TcpManager::droneDirectionFindingResponse);
    connect(droneOpsService, &DroneOpsService::droneDirectionPowerReported,
            this, &TcpManager::droneDirectionPowerReported);
    connect(droneOpsService, &DroneOpsService::dronePrecisionStrikeResponse,
            this, &TcpManager::dronePrecisionStrikeResponse);
    connect(droneOpsService, &DroneOpsService::droneWideBandJammingResponse,
            this, &TcpManager::droneWideBandJammingResponse);
    connect(droneOpsService, &DroneOpsService::droneVideoTakeoverResponse,
            this, &TcpManager::droneVideoTakeoverResponse);
    connect(droneOpsService, &DroneOpsService::droneVideoImageReported,
            this, &TcpManager::droneVideoImageReported);

    deviceOpsService->setFrameSender(
        [this](uint16_t dataType, const QByteArray &payload)
        {
            sendFrame(dataType, payload);
        });
    connect(deviceOpsService, &DeviceOpsService::deviceJammingModeSetResponse,
            this, &TcpManager::deviceJammingModeSetResponse);
    connect(deviceOpsService, &DeviceOpsService::deviceJammingModeReported,
            this, &TcpManager::deviceJammingModeReported);
    connect(deviceOpsService, &DeviceOpsService::deviceJammingStatusQueried,
            this, &TcpManager::deviceJammingStatusQueried);
    connect(deviceOpsService, &DeviceOpsService::alarmHistoryQueried,
            this, &TcpManager::alarmHistoryQueried);
    connect(deviceOpsService, &DeviceOpsService::deviceUsageInfoQueried,
            this, &TcpManager::deviceUsageInfoQueried);
    connect(deviceOpsService, &DeviceOpsService::buzzerEnabledSetResponse,
            this, &TcpManager::buzzerEnabledSetResponse);
    connect(deviceOpsService, &DeviceOpsService::buzzerEnabledQueried,
            this, &TcpManager::buzzerEnabledQueried);
    connect(deviceOpsService, &DeviceOpsService::deviceRebootResponse,
            this, &TcpManager::deviceRebootResponse);

    calibrationService->setFrameSender(
        [this](uint16_t dataType, const QByteArray &payload)
        {
            sendFrame(dataType, payload);
        });
    connect(calibrationService, &CalibrationService::compassCalibrationResponse,
            this, &TcpManager::compassCalibrationResponse);

    spectrumService->setFrameSender(
        [this](uint16_t dataType, const QByteArray &payload)
        {
            sendFrame(dataType, payload);
        });
    connect(spectrumService, &SpectrumService::spectrogramSwitchResponse,
            this, &TcpManager::spectrogramSwitchResponse);
    connect(spectrumService, &SpectrumService::spectrumDataReported,
            this, &TcpManager::spectrumDataReported);
    connect(spectrumService, &SpectrumService::fullSpectrumSwitchResponse,
            this, &TcpManager::fullSpectrumSwitchResponse);
    connect(spectrumService, &SpectrumService::fullSpectrumReported,
            this, &TcpManager::fullSpectrumReported);

    settingsProtocolService->setFrameSender(
        [this](uint16_t dataType, const QByteArray &payload)
        {
            sendFrame(dataType, payload);
        });
    connect(settingsProtocolService, &SettingsProtocolService::deviceGpsQueried,
            this, &TcpManager::deviceGpsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::deviceGpsSetResponse,
            this, &TcpManager::deviceGpsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::detectBandsSetResponse,
            this, &TcpManager::detectBandsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::detectBandsQueried,
            this, &TcpManager::detectBandsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::droneReportModeSetResponse,
            this, &TcpManager::droneReportModeSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::droneReportModeQueried,
            this, &TcpManager::droneReportModeQueried);
    connect(settingsProtocolService, &SettingsProtocolService::suppressionModeSetResponse,
            this, &TcpManager::suppressionModeSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::suppressionModeQueried,
            this, &TcpManager::suppressionModeQueried);
    connect(settingsProtocolService, &SettingsProtocolService::o4ServerModeSetResponse,
            this, &TcpManager::o4ServerModeSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::o4ServerModeQueried,
            this, &TcpManager::o4ServerModeQueried);
    connect(settingsProtocolService, &SettingsProtocolService::uavCategoryDisplayModeSetResponse,
            this, &TcpManager::uavCategoryDisplayModeSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::uavCategoryDisplayModeQueried,
            this, &TcpManager::uavCategoryDisplayModeQueried);
    connect(settingsProtocolService, &SettingsProtocolService::dataEnableSetResponse,
            this, &TcpManager::dataEnableSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::dataEnableQueried,
            this, &TcpManager::dataEnableQueried);
    connect(settingsProtocolService, &SettingsProtocolService::featureModesSetResponse,
            this, &TcpManager::featureModesSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::featureModesQueried,
            this, &TcpManager::featureModesQueried);
    connect(settingsProtocolService, &SettingsProtocolService::fullScanParamsSetResponse,
            this, &TcpManager::fullScanParamsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::fullScanParamsQueried,
            this, &TcpManager::fullScanParamsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::deviceIpSetResponse,
            this, &TcpManager::deviceIpSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::deviceIpQueried,
            this, &TcpManager::deviceIpQueried);
    connect(settingsProtocolService, &SettingsProtocolService::tcpServerIpSetResponse,
            this, &TcpManager::tcpServerIpSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::tcpServerIpQueried,
            this, &TcpManager::tcpServerIpQueried);
    connect(settingsProtocolService, &SettingsProtocolService::modelLibraryModeSetResponse,
            this, &TcpManager::modelLibraryModeSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::modelLibraryModeQueried,
            this, &TcpManager::modelLibraryModeQueried);
    connect(settingsProtocolService, &SettingsProtocolService::modelLibraryRecordSetResponse,
            this, &TcpManager::modelLibraryRecordSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::modelLibraryRecordsQueried,
            this, &TcpManager::modelLibraryRecordsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::strikeFrequencyBandsSetResponse,
            this, &TcpManager::strikeFrequencyBandsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::strikeFrequencyBandsQueried,
            this, &TcpManager::strikeFrequencyBandsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::powerAmplifierParamsSetResponse,
            this, &TcpManager::powerAmplifierParamsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::powerAmplifierParamsQueried,
            this, &TcpManager::powerAmplifierParamsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::directionCalibrationValuesSetResponse,
            this, &TcpManager::directionCalibrationValuesSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::directionCalibrationValuesQueried,
            this, &TcpManager::directionCalibrationValuesQueried);
    connect(settingsProtocolService, &SettingsProtocolService::signalSourceParamsSetResponse,
            this, &TcpManager::signalSourceParamsSetResponse);
    connect(settingsProtocolService, &SettingsProtocolService::signalSourceParamsQueried,
            this, &TcpManager::signalSourceParamsQueried);
    connect(settingsProtocolService, &SettingsProtocolService::patternUploadResponse,
            this, &TcpManager::patternUploadResponse);
    connect(settingsProtocolService, &SettingsProtocolService::firmwareVersionsQueried,
            this, &TcpManager::firmwareVersionsQueried);
}

TcpManager::~TcpManager()
{
    if (tcpSocket->isOpen())
    {
        tcpSocket->close();
    }
}

// 连接：建连入口与重连参数维护。
void TcpManager::connectToServer(const QString &ip, quint16 port)
{
    lastConnectAttemptAt = QDateTime::currentDateTime();
    lastConnectedIp = ip;
    lastConnectedPort = port;

    if (tcpSocket->state() == QAbstractSocket::ConnectedState)
    {
        tcpSocket->disconnectFromHost();
    }

    tcpSocket->connectToHost(ip, port);
}

void TcpManager::setReconnectIntervalMs(int intervalMs)
{
    reconnectIntervalMs = qMax(200, intervalMs);
    reconnectTimer->setInterval(reconnectIntervalMs);
}

void TcpManager::sendCommand(const QString &cmd)
{
    if (tcpSocket->state() == QAbstractSocket::ConnectedState)
    {
        tcpSocket->write(cmd.toUtf8());
        qDebug() << "[TcpManager] 已发送数据:" << cmd.trimmed();
    }
    else
    {
        qDebug() << "[TcpManager] 发送失败，未连接服务器";
    }
}

// 发包：统一封装协议头、载荷、校验和与包尾。
void TcpManager::sendFrame(uint16_t dataType, const QByteArray &data)
{
    if (tcpSocket->state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "[TcpManager] 未连接服务器，无法发送命令 (DataType:" << dataType << ") 当前状态:" << socketStateText()
                 << "目标地址:" << lastConnectedIp << ":" << lastConnectedPort;
        return;
    }

    ProtocolHeader header;
    header.startFlag = 0xEEEEEEEE;
    header.version = 0x0302;
    header.length = sizeof(ProtocolHeader) + data.size() + sizeof(ProtocolTail);

    QDateTime now = QDateTime::currentDateTime();
    QDate date = now.date();
    QTime time = now.time();

    header.year = date.year();
    header.month = date.month();
    header.day = date.day();
    header.hour = time.hour();
    header.minute = time.minute();
    header.second = time.second();
    header.millisecond = time.msec();
    header.dataType = dataType;

    static uint64_t s_packageNum = 0;
    header.packageNum = ++s_packageNum;

    QByteArray frame;
    frame.append(reinterpret_cast<const char *>(&header), sizeof(ProtocolHeader));
    frame.append(data);

    // 校验和：帧头与数据部分和取低字节（不包括开始标志部分4字节）
    uint8_t checksum = 0;
    for (int i = 4; i < frame.size(); ++i)
    {
        checksum += static_cast<uint8_t>(frame.at(i));
    }

    ProtocolTail tail;
    tail.checksum = checksum;
    tail.endFlag = 0xAAAAAAAA;

    frame.append(reinterpret_cast<const char *>(&tail), sizeof(ProtocolTail));

    tcpSocket->write(frame);
    tcpSocket->flush();
    rememberSentFrame(dataType, frame.size());

    if (shouldLogStrikeFrequencyFrame(dataType))
    {
        qDebug().noquote()
            << QStringLiteral("[StrikeFrequencyFrame][TX] %1 dataType=%2 frameHex=%3")
                   .arg(strikeFrequencyFrameLabel(dataType),
                        QString::number(dataType),
                        QString::fromLatin1(frame.toHex(' ').toUpper()));
    }

    if (isCommunicationJammingCommand(dataType, data))
    {
        qDebug().noquote()
            << QStringLiteral("[CommJammingFrame][TX] %1 dataType=%2 frameHex=%3")
                   .arg(communicationJammingFrameLabel(data),
                        QString::number(dataType),
                        QString::fromLatin1(frame.toHex(' ').toUpper()));
    }

    if (isPrecisionStrikeFrame(dataType))
    {
        logPrecisionStrikeFrame("TX", dataType, frame);
    }

    if (isDroneReportModeFrame(dataType))
    {
        logDroneReportModeFrame("TX", dataType, frame);
    }

    if (isDetectBandFrame(dataType))
    {
        logDetectBandFrame("TX", dataType, frame);
    }

    if (isGpsSettingFrame(dataType))
    {
        logGpsSettingFrame("TX", dataType, frame);
    }

    if (isUavCategoryDisplayModeFrame(dataType))
    {
        logUavCategoryDisplayModeFrame("TX", dataType, frame);
    }

    if (isFullScanFrame(dataType))
    {
        logFullScanFrame("TX", dataType, frame);
    }

    if (isFullSpectrumFrame(dataType))
    {
        logFullSpectrumFrame("TX", dataType, frame);
    }

    if (isFeatureModeFrame(dataType))
    {
        logFeatureModeFrame("TX", dataType, frame);
    }

}

bool TcpManager::isConnected() const
{
    return tcpSocket->state() == QAbstractSocket::ConnectedState;
}

void TcpManager::onSocketConnected()
// 连接：底层 socket 状态变化与重连定时器管理。
{
    lastConnectedAt = QDateTime::currentDateTime();
    lastSocketError = QAbstractSocket::UnknownSocketError;
    lastSocketErrorText.clear();
    qDebug() << "[TcpManager] TCP 客户端连接成功！";
    reconnectTimer->stop(); // 连接成功，停止重连定时器
    emit connected();
}

void TcpManager::onSocketDisconnected()
{
    lastDisconnectedAt = QDateTime::currentDateTime();
    qDebug() << "[TcpManager] TCP 连接已断开！";
    emit disconnected();

    // 断开后启动重连机制
    if (!lastConnectedIp.isEmpty() && lastConnectedPort != 0)
    {
        const int delayMs = nextReconnectDelayMs();
        reconnectTimer->start(delayMs);
    }
}

void TcpManager::onReconnectTimeout()
{
    if (tcpSocket->state() == QAbstractSocket::ConnectedState)
    {
        reconnectTimer->stop();
        return;
    }

    if (lastConnectedIp.isEmpty() || lastConnectedPort == 0)
    {
        return;
    }

    if (tcpSocket->state() != QAbstractSocket::UnconnectedState)
    {
        tcpSocket->abort();
    }

    qDebug() << "[TcpManager] 正在尝试重新连接...";
    tcpSocket->connectToHost(lastConnectedIp, lastConnectedPort);
}

// 收包：读取 socket 数据并兼容调试助手发送的十六进制字符串。
void TcpManager::onSocketReadyRead()
{
    QByteArray data = tcpSocket->readAll();

    // 仅当整段数据都是调试助手发的 hex 文本时才转换；二进制 291 JPEG 里可能偶然含 "EE EE" 子串。
    QByteArray realData;
    if (looksLikeHexAsciiPayload(data))
    {
        data.replace(" ", "");
        realData = QByteArray::fromHex(data);
        qDebug().noquote() << QStringLiteral("[TcpManager] 收到 hex 文本，转换后 bytes=%1").arg(realData.size());
    }
    else
    {
        realData = data;
    }

    m_buffer.append(realData);

    parseBuffer();
}

// 收包：在缓冲区内完成粘包/半包处理、校验并提取完整协议帧。
void TcpManager::parseBuffer()
{
    // 循环解析缓冲区中的所有完整数据帧
    while (static_cast<size_t>(m_buffer.size()) >= sizeof(ProtocolHeader))
    {
        // 1. 寻找包头 0xEEEEEEEE
        int startIndex = -1;
        for (int i = 0; i <= m_buffer.size() - 4; ++i)
        {
            uint32_t flag = 0;
            std::memcpy(&flag, m_buffer.constData() + i, sizeof(flag));
            if (flag == 0xEEEEEEEE)
            {
                startIndex = i;
                break;
            }
        }

        // 如果找不到包头，保留最后 3 个字节，避免下次收包时错过被截断的帧头。
        if (startIndex == -1)
        {
            const int keepBytes = qMin(3, m_buffer.size());
            if (keepBytes > 0)
            {
                m_buffer = m_buffer.right(keepBytes);
            }
            else
            {
                m_buffer.clear();
            }
            return;
        }

        // 丢弃包头之前的垃圾数据
        if (startIndex > 0)
        {
            m_buffer.remove(0, startIndex);
        }

        // 此时 m_buffer[0] 开始一定是 0xEEEEEEEE
        if (static_cast<size_t>(m_buffer.size()) < sizeof(ProtocolHeader))
        {
            return; // 长度不够一个头，继续等数据
        }

        ProtocolHeader headerValue;
        std::memcpy(&headerValue, m_buffer.constData(), sizeof(headerValue));
        const ProtocolHeader *header = &headerValue;
        uint32_t frameLen = header->length;

        // 检查数据包长度是否合理
        if (frameLen < sizeof(ProtocolHeader) + sizeof(ProtocolTail) || frameLen > 1024 * 1024)
        {
            m_buffer.remove(0, 4); // 丢弃这个错误的包头，继续找下一个
            continue;
        }

        // 判断缓冲区数据是否已经收全了这整个包
        if (static_cast<uint32_t>(m_buffer.size()) < frameLen)
        {
            return; // 半包，等待下一次 readAll
        }

        // 先在缓冲区上校验，确认完整帧有效后再从缓冲区移除，避免错误长度把后续好包一并删掉。
        ProtocolTail tailValue;
        std::memcpy(&tailValue, m_buffer.constData() + frameLen - sizeof(ProtocolTail), sizeof(tailValue));
        if (tailValue.endFlag != 0xAAAAAAAA)
        {
            m_buffer.remove(0, 1);
            continue; // 当前帧头无效，滑动 1 字节继续找下一个
        }

        // 校验 Checksum (实现对接收帧校验和的验证)
        uint8_t calculatedChecksum = 0;
        // Checksum从 startFlag 之后开始算（第4个字节），直到 checksum 字段之前
        for (uint32_t i = 4; i < frameLen - sizeof(ProtocolTail); ++i)
        {
            calculatedChecksum += static_cast<uint8_t>(m_buffer.at(static_cast<int>(i)));
        }

        if (tailValue.checksum != calculatedChecksum)
        {
            m_buffer.remove(0, 1);
            continue; // 当前帧头无效，滑动 1 字节继续找下一个
        }

        // 提取出一个完整的数据包
        QByteArray frameData = m_buffer.left(frameLen);
        m_buffer.remove(0, frameLen); // 从缓冲区移除已处理的包

        ProtocolHeader frameHeaderValue;
        std::memcpy(&frameHeaderValue, frameData.constData(), sizeof(frameHeaderValue));
        header = &frameHeaderValue;

        if (shouldLogStrikeFrequencyFrame(header->dataType))
        {
            qDebug().noquote()
                << QStringLiteral("[StrikeFrequencyFrame][RX] %1 dataType=%2 frameHex=%3")
                       .arg(strikeFrequencyFrameLabel(header->dataType),
                            QString::number(header->dataType),
                            QString::fromLatin1(frameData.toHex(' ').toUpper()));
        }

        if (header->dataType == 101 && lastSentDataType == 100)
        {
            qDebug().noquote()
                << QStringLiteral("[CommJammingFrame][RX] 设置应答 dataType=%1 frameHex=%2")
                       .arg(QString::number(header->dataType),
                            QString::fromLatin1(frameData.toHex(' ').toUpper()));
        }

        if (isPrecisionStrikeFrame(header->dataType))
        {
            logPrecisionStrikeFrame("RX", header->dataType, frameData);
        }

        if (isDroneReportModeFrame(header->dataType))
        {
            logDroneReportModeFrame("RX", header->dataType, frameData);
        }

        if (isDetectBandFrame(header->dataType))
        {
            logDetectBandFrame("RX", header->dataType, frameData);
        }

        if (isGpsSettingFrame(header->dataType))
        {
            logGpsSettingFrame("RX", header->dataType, frameData);
        }

        if (isUavCategoryDisplayModeFrame(header->dataType))
        {
            logUavCategoryDisplayModeFrame("RX", header->dataType, frameData);
        }

        if (isFullScanFrame(header->dataType))
        {
            logFullScanFrame("RX", header->dataType, frameData);
        }

        if (isFullSpectrumFrame(header->dataType))
        {
            logFullSpectrumFrame("RX", header->dataType, frameData);
        }

        if (isFeatureModeFrame(header->dataType))
        {
            logFeatureModeFrame("RX", header->dataType, frameData);
        }

        // 业务分发
        dispatchProtocol(header, frameData);
    }
}

// 主分发：把完整协议帧路由到各个模块子分发函数。
void TcpManager::dispatchProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    rememberReceivedFrame(header->dataType, frameData.size());

    if (dispatchDeviceBaseProtocol(header, frameData) || dispatchSystemProtocol(header, frameData) ||
        dispatchAngleCalibrationProtocol(header, frameData) || dispatchSpectrumSwitchProtocol(header, frameData) ||
        dispatchStrikeFrequencyProtocol(header, frameData) || dispatchPowerAmplifierProtocol(header, frameData) ||
        dispatchDirectionCalibrationProtocol(header, frameData) ||
        dispatchSignalSourceParamsProtocol(header, frameData) ||
        dispatchDataCollectionProtocol(header, frameData) ||
        dispatchFirmwareProtocol(header, frameData) ||
        dispatchModelLibraryProtocol(header, frameData) ||
        dispatchDeviceOpsProtocol(header, frameData) ||
        dispatchGpsProtocol(header, frameData) || dispatchDetectBandProtocol(header, frameData) ||
        dispatchModeSelectProtocol(header, frameData) || dispatchNetworkConfigProtocol(header, frameData))
    {
        return;
    }

    qDebug().noquote()
        << QStringLiteral("[TcpManager] 未处理收帧 DataType=%1 FrameHex=%2")
               .arg(header->dataType)
               .arg(QString::fromLatin1(frameData.toHex(' ').toUpper()));
}

void TcpManager::onSocketError(QTcpSocket::SocketError socketError)
{
    lastSocketError = socketError;
    lastSocketErrorText = tcpSocket->errorString();
    QString errorStr = tcpSocket->errorString();
    // #region debug-point D:socket-error
    qDebug().noquote()
        << QStringLiteral("[DEBUG-D] tcp_manager.cpp:onSocketError | socket错误 | error=%1 code=%2 errorString=%3 "
                          "socketState=%4 lastSentDataType=%5 lastSentFrameLength=%6 lastReceivedDataType=%7")
               .arg(socketErrorText(socketError),
                    QString::number(static_cast<int>(socketError)),
                    errorStr,
                    socketStateText(),
                    QString::number(lastSentDataType),
                    QString::number(lastSentFrameLength),
                    QString::number(lastReceivedDataType));
    // #endregion
    qDebug() << "[TcpManager] TCP 连接错误:" << socketErrorText(socketError) << "(" << static_cast<int>(socketError)
             << ")" << errorStr;

    if (!lastConnectedIp.isEmpty() && lastConnectedPort != 0 && tcpSocket->state() != QAbstractSocket::ConnectedState)
    {
        const int delayMs = nextReconnectDelayMs();
        reconnectTimer->start(delayMs);
    }

    emit errorOccurred(errorStr);
}

void TcpManager::rememberSentFrame(uint16_t dataType, int frameLength)
{
    lastSentDataType = dataType;
    lastSentFrameLength = frameLength;
    lastSentAt = QDateTime::currentDateTime();
}

void TcpManager::rememberReceivedFrame(uint16_t dataType, int frameLength)
{
    lastReceivedDataType = dataType;
    lastReceivedFrameLength = frameLength;
    lastReceivedAt = QDateTime::currentDateTime();
}

QString TcpManager::socketStateText() const
{
    switch (tcpSocket->state())
    {
    case QAbstractSocket::UnconnectedState:
        return "UnconnectedState";
    case QAbstractSocket::HostLookupState:
        return "HostLookupState";
    case QAbstractSocket::ConnectingState:
        return "ConnectingState";
    case QAbstractSocket::ConnectedState:
        return "ConnectedState";
    case QAbstractSocket::BoundState:
        return "BoundState";
    case QAbstractSocket::ClosingState:
        return "ClosingState";
    case QAbstractSocket::ListeningState:
        return "ListeningState";
    default:
        return "UnknownState";
    }
}

int TcpManager::nextReconnectDelayMs() const
{
    const int jitterMs = 300;
    return reconnectIntervalMs + (std::rand() % (jitterMs + 1));
}
