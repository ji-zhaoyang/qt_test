#include "device_ops_service.h"

#include <QJsonArray>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

namespace
{
constexpr int kAlarmHistoryPayloadLength = 79;

QString parseResultMessage(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
}

int readInt32LE(const char *data)
{
    const quint32 raw = static_cast<quint32>(static_cast<unsigned char>(data[0])) |
                        (static_cast<quint32>(static_cast<unsigned char>(data[1])) << 8) |
                        (static_cast<quint32>(static_cast<unsigned char>(data[2])) << 16) |
                        (static_cast<quint32>(static_cast<unsigned char>(data[3])) << 24);
    return static_cast<int>(raw);
}

int readJsonInt(const QJsonObject &object, const QStringList &keys, int defaultValue = 0)
{
    for (const QString &key : keys)
    {
        const QJsonValue value = object.value(key);
        if (!value.isUndefined() && !value.isNull())
        {
            return value.toInt(defaultValue);
        }
    }
    return defaultValue;
}
} // namespace

DeviceOpsService::DeviceOpsService(QObject *parent) : QObject(parent)
{
}

void DeviceOpsService::setFrameSender(const FrameSender &sender)
{
    sendFrame_ = sender;
}

void DeviceOpsService::rebootDevice()
{
    if (!sendFrame_)
    {
        return;
    }

    sendFrame_(29, QByteArray());
}

void DeviceOpsService::setDeviceJammingMode(int mode, int switchStatus)
{
    if (!sendFrame_)
    {
        return;
    }

    QJsonObject json;
    json[QStringLiteral("mode")] = mode;
    json[QStringLiteral("switch")] = switchStatus;
    pendingDeviceJammingMode_ = mode;
    pendingDeviceJammingSwitchStatus_ = switchStatus;
    sendFrame_(100, QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void DeviceOpsService::queryDeviceJammingMode()
{
    if (!sendFrame_)
    {
        return;
    }

    sendFrame_(102, QByteArray());
}

void DeviceOpsService::queryDeviceAlarmHistory()
{
    if (!sendFrame_)
    {
        return;
    }

    sendFrame_(116, QByteArray());
}

void DeviceOpsService::queryDeviceUsageInfo()
{
    if (!sendFrame_)
    {
        return;
    }

    sendFrame_(136, QByteArray());
}

void DeviceOpsService::setBuzzerEnabled(uint8_t enabled)
{
    if (!sendFrame_)
    {
        return;
    }

    QByteArray data(1, 0);
    data[0] = static_cast<char>(enabled == 0 ? 0 : 1);
    sendFrame_(92, data);
}

void DeviceOpsService::queryBuzzerEnabled()
{
    if (!sendFrame_)
    {
        return;
    }

    sendFrame_(94, QByteArray());
}

void DeviceOpsService::handleDeviceJammingModeSetResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit deviceJammingModeSetResponse(pendingDeviceJammingMode_, pendingDeviceJammingSwitchStatus_, success, resultMsg);
}

void DeviceOpsService::handleDeviceJammingStatusQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        qDebug() << "[TcpManager] DataType=103 查询应答为空";
        return;
    }

    const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
    {
        qDebug() << "[TcpManager] DataType=103 JSON 解析失败:" << parseError.errorString();
        return;
    }

    const QJsonObject jsonObject = jsonDoc.object();
    QVector<int> switchStates;
    switchStates.reserve(12);
    for (int i = 1; i <= 12; ++i)
    {
        const QString key = QStringLiteral("switch%1").arg(i);
        switchStates.append(jsonObject.value(key).toInt());
    }

    emit deviceJammingStatusQueried(switchStates);
}

void DeviceOpsService::handleDeviceJammingModeReported(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        qDebug() << "[TcpManager] DataType=104 状态上报为空";
        return;
    }

    const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qDebug() << "[TcpManager] DataType=104 JSON 解析失败:" << parseError.errorString();
        return;
    }

    QJsonObject jsonObject;
    if (jsonDoc.isObject())
    {
        jsonObject = jsonDoc.object();
    }
    else if (jsonDoc.isArray() && !jsonDoc.array().isEmpty() && jsonDoc.array().first().isObject())
    {
        jsonObject = jsonDoc.array().first().toObject();
    }
    else
    {
        qDebug() << "[TcpManager] DataType=104 JSON 结构不支持";
        return;
    }

    const int mode = readJsonInt(jsonObject, {QStringLiteral("mode")}, -1);
    const int switchStatus = readJsonInt(jsonObject, {QStringLiteral("switch")}, -1);
    if (mode < 0 || switchStatus < 0)
    {
        qDebug() << "[TcpManager] DataType=104 缺少 mode/switch 字段";
        return;
    }

    emit deviceJammingModeReported(mode, switchStatus);
}

void DeviceOpsService::handleBuzzerEnabledSetResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit buzzerEnabledSetResponse(success, resultMsg);
}

void DeviceOpsService::handleBuzzerEnabledQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        qDebug() << "[TcpManager] DataType=95 查询应答为空";
        return;
    }

    const uint8_t enabled =
        static_cast<uint8_t>(static_cast<unsigned char>(frameData.constData()[sizeof(ProtocolHeader)]));
    emit buzzerEnabledQueried(enabled);
}

void DeviceOpsService::handleAlarmHistoryQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen != kAlarmHistoryPayloadLength)
    {
        qDebug() << "[TcpManager] DataType=117 查询应答长度非法，实际:" << payloadLen
                 << "期望:" << kAlarmHistoryPayloadLength;
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    int offset = 0;
    AlarmHistoryInfo info;
    info.deviceTemperature = readInt32LE(payload + offset);
    offset += 4;

    info.fanAlarm = static_cast<unsigned char>(payload[offset++]);
    info.clockAlarm = static_cast<unsigned char>(payload[offset++]);
    info.receiverPllAlarm = static_cast<unsigned char>(payload[offset++]);
    info.transmitterPllAlarm = static_cast<unsigned char>(payload[offset++]);
    info.adcAlarm = static_cast<unsigned char>(payload[offset++]);
    info.eepromAlarm = static_cast<unsigned char>(payload[offset++]);
    info.temperatureChipAlarm = static_cast<unsigned char>(payload[offset++]);
    info.compassAlarm = static_cast<unsigned char>(payload[offset++]);

    for (int i = 0; i < info.paSerialAlarms.size(); ++i)
    {
        info.paSerialAlarms[i] = static_cast<unsigned char>(payload[offset++]);
    }
    for (int i = 0; i < info.paOverpowerStatus.size(); ++i)
    {
        info.paOverpowerStatus[i] = static_cast<unsigned char>(payload[offset++]);
        info.paOverpowerCounts[i] = readInt32LE(payload + offset);
        offset += 4;
    }
    for (int i = 0; i < info.paUnderpowerStatus.size(); ++i)
    {
        info.paUnderpowerStatus[i] = static_cast<unsigned char>(payload[offset++]);
        info.paUnderpowerCounts[i] = readInt32LE(payload + offset);
        offset += 4;
    }
    info.serverConnectionStatus = static_cast<unsigned char>(payload[offset++]);

    emit alarmHistoryQueried(info);
}

void DeviceOpsService::handleDeviceUsageInfoQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        qDebug() << "[TcpManager] DataType=137 查询应答为空";
        return;
    }

    const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
    {
        qDebug() << "[TcpManager] DataType=137 JSON 解析失败:" << parseError.errorString();
        return;
    }

    const QJsonObject jsonObject = jsonDoc.object();
    DeviceUsageInfo info;
    info.limit = jsonObject.value(QStringLiteral("limit")).toInt();
    info.remainingTimeSeconds = jsonObject.value(QStringLiteral("time")).toInt();
    info.remainingCount = jsonObject.value(QStringLiteral("num")).toInt();

    emit deviceUsageInfoQueried(info);
}

void DeviceOpsService::handleDeviceRebootResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit deviceRebootResponse(success, resultMsg);
}
