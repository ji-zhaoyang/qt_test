#include "tcp_manager.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <QtMath>
#include <cstring>

namespace
{
constexpr int kAlarmHistoryPayloadLength = 79;

void appendUInt32LELocal(QByteArray &buffer, quint32 value)
{
    buffer.append(static_cast<char>(value & 0xFF));
    buffer.append(static_cast<char>((value >> 8) & 0xFF));
    buffer.append(static_cast<char>((value >> 16) & 0xFF));
    buffer.append(static_cast<char>((value >> 24) & 0xFF));
}

int readInt32LELocal(const char *data)
{
    const quint32 raw = static_cast<quint32>(static_cast<unsigned char>(data[0])) |
                        (static_cast<quint32>(static_cast<unsigned char>(data[1])) << 8) |
                        (static_cast<quint32>(static_cast<unsigned char>(data[2])) << 16) |
                        (static_cast<quint32>(static_cast<unsigned char>(data[3])) << 24);
    return static_cast<int>(raw);
}

QString parseResultMessageLocal(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
}

int readJsonIntLocal(const QJsonObject &object, const QStringList &keys, int defaultValue = 0)
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

QString readJsonStringLocal(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys)
    {
        const QJsonValue value = object.value(key);
        if (!value.isUndefined() && !value.isNull())
        {
            return value.toString().trimmed();
        }
    }
    return QString();
}

QJsonArray readJsonArrayLocal(const QJsonObject &object, const QStringList &keys)
{
    for (const QString &key : keys)
    {
        const QJsonValue value = object.value(key);
        if (value.isArray())
        {
            return value.toArray();
        }
    }
    return QJsonArray();
}

ModelLibraryFreqBand parseModelLibraryFreqBandLocal(const QJsonValue &value)
{
    ModelLibraryFreqBand band;
    if (value.isObject())
    {
        const QJsonObject object = value.toObject();
        band.start = readJsonIntLocal(object, {QStringLiteral("start"), QStringLiteral("begin"),
                                               QStringLiteral("startFreq"), QStringLiteral("low")});
        band.end =
            readJsonIntLocal(object, {QStringLiteral("end"), QStringLiteral("stop"),
                                      QStringLiteral("endFreq"), QStringLiteral("high")});
    }
    else if (value.isArray())
    {
        const QJsonArray array = value.toArray();
        if (array.size() >= 2)
        {
            band.start = array.at(0).toInt();
            band.end = array.at(1).toInt();
        }
    }
    else if (value.isString())
    {
        const QString text = value.toString().trimmed();
        const QRegularExpressionMatch match =
            QRegularExpression(QStringLiteral("(\\d+(?:\\.\\d+)?)\\D+(\\d+(?:\\.\\d+)?)")).match(text);
        if (match.hasMatch())
        {
            band.start = qRound(match.captured(1).toDouble());
            band.end = qRound(match.captured(2).toDouble());
        }
    }
    return band;
}

void appendModelLibraryFreqBandLocal(const QJsonValue &value, QVector<ModelLibraryFreqBand> &freqBands)
{
    if (value.isUndefined() || value.isNull())
    {
        return;
    }

    if (value.isArray())
    {
        const QJsonArray array = value.toArray();
        if (array.size() >= 2 && !array.at(0).isObject() && !array.at(0).isArray())
        {
            const ModelLibraryFreqBand band = parseModelLibraryFreqBandLocal(value);
            if (band.start > 0 || band.end > 0)
            {
                freqBands.append(band);
            }
            return;
        }

        for (const QJsonValue &item : array)
        {
            const ModelLibraryFreqBand band = parseModelLibraryFreqBandLocal(item);
            if (band.start > 0 || band.end > 0)
            {
                freqBands.append(band);
            }
        }
        return;
    }

    const ModelLibraryFreqBand band = parseModelLibraryFreqBandLocal(value);
    if (band.start > 0 || band.end > 0)
    {
        freqBands.append(band);
    }
}

QVector<ModelLibraryFreqBand> extractModelLibraryFreqBandsLocal(const QJsonObject &object)
{
    QVector<ModelLibraryFreqBand> freqBands;

    const QStringList bandKeys = {QStringLiteral("freqbands"), QStringLiteral("freqBands"), QStringLiteral("bands"),
                                  QStringLiteral("freqband"), QStringLiteral("freqBand"), QStringLiteral("band")};
    for (const QString &key : bandKeys)
    {
        appendModelLibraryFreqBandLocal(object.value(key), freqBands);
    }

    if (!freqBands.isEmpty())
    {
        return freqBands;
    }

    appendModelLibraryFreqBandLocal(object, freqBands);
    if (!freqBands.isEmpty())
    {
        return freqBands;
    }

    for (int i = 1; i <= 8; ++i)
    {
        ModelLibraryFreqBand band;
        band.start =
            readJsonIntLocal(object, {QStringLiteral("start%1").arg(i), QStringLiteral("begin%1").arg(i),
                                      QStringLiteral("low%1").arg(i), QStringLiteral("startFreq%1").arg(i)});
        band.end = readJsonIntLocal(object, {QStringLiteral("end%1").arg(i), QStringLiteral("stop%1").arg(i),
                                             QStringLiteral("high%1").arg(i), QStringLiteral("endFreq%1").arg(i)});
        if (band.start > 0 || band.end > 0)
        {
            freqBands.append(band);
            continue;
        }

        appendModelLibraryFreqBandLocal(object.value(QStringLiteral("freqband%1").arg(i)), freqBands);
        appendModelLibraryFreqBandLocal(object.value(QStringLiteral("freqBand%1").arg(i)), freqBands);
        appendModelLibraryFreqBandLocal(object.value(QStringLiteral("band%1").arg(i)), freqBands);
    }

    return freqBands;
}

ModelLibraryRecord parseModelLibraryRecordLocal(const QJsonObject &object)
{
    ModelLibraryRecord record;
    record.type = readJsonIntLocal(object, {QStringLiteral("type")});
    record.name = readJsonStringLocal(object, {QStringLiteral("name"), QStringLiteral("model"),
                                               QStringLiteral("droneName"), QStringLiteral("title")});
    record.sensitivity =
        readJsonIntLocal(object, {QStringLiteral("sensitivity"), QStringLiteral("threshold"),
                                  QStringLiteral("gate"), QStringLiteral("limit")}, 1);
    record.enable =
        readJsonIntLocal(object, {QStringLiteral("enable"), QStringLiteral("status"), QStringLiteral("enabled")});
    record.freqbands = extractModelLibraryFreqBandsLocal(object);
    return record;
}
} // namespace

void TcpManager::setDronePrecisionStrike(bool enabled, quint32 timestamp, const QString &sn, int type, quint32 targetId)
{
    QJsonObject payloadObject;
    payloadObject.insert(QStringLiteral("timestamp"), static_cast<qint64>(timestamp));
    payloadObject.insert(QStringLiteral("pa_switch"), enabled ? 1 : 0);
    payloadObject.insert(QStringLiteral("sn"), sn.trimmed());
    payloadObject.insert(QStringLiteral("type"), type);

    pendingDronePrecisionStrikeEnabled = enabled;
    pendingDronePrecisionStrikeTimestamp = timestamp;
    pendingDronePrecisionStrikeSn = sn.trimmed();
    pendingDronePrecisionStrikeType = type;
    pendingDronePrecisionStrikeTargetId = targetId;

    sendFrame(109, QJsonDocument(payloadObject).toJson(QJsonDocument::Compact));
}

void TcpManager::setDroneWideBandJamming(bool enabled, quint32 frequencyKhz, const QString &sn, quint32 targetId)
{
    const QByteArray snBytes = sn.trimmed().toUtf8();

    QByteArray payload;
    payload.reserve(1 + static_cast<int>(sizeof(quint32)) + static_cast<int>(sizeof(quint32)) + snBytes.size());
    payload.append(static_cast<char>(enabled ? 1 : 0));
    appendUInt32LELocal(payload, frequencyKhz);
    appendUInt32LELocal(payload, static_cast<quint32>(snBytes.size()));
    payload.append(snBytes);

    pendingDroneWideBandJammingEnabled = enabled;
    pendingDroneWideBandJammingFrequencyKhz = frequencyKhz;
    pendingDroneWideBandJammingSn = sn.trimmed();
    pendingDroneWideBandJammingTargetId = targetId;

    sendFrame(114, payload);
}

bool TcpManager::dispatchDeviceOpsProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 101:
    {
        const QString resultMsg = parseResultMessageLocal(frameData);
        const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
        emit deviceJammingModeSetResponse(pendingDeviceJammingMode, pendingDeviceJammingSwitchStatus, success, resultMsg);
        return true;
    }
    case 103:
    {
        const int payloadLen =
            frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
        if (payloadLen <= 0)
        {
            qDebug() << "[TcpManager] DataType=103 查询应答为空";
            return true;
        }

        const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
        QJsonParseError parseError;
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
        {
            qDebug() << "[TcpManager] DataType=103 JSON 解析失败:" << parseError.errorString();
            return true;
        }

        const QJsonObject jsonObject = jsonDoc.object();
        QVector<int> switchStates;
        switchStates.reserve(12);
        for (int i = 1; i <= 12; ++i)
        {
            const QString key = QStringLiteral("switch%1").arg(i);
            switchStates.append(jsonObject.value(key).toInt());
        }

        QStringList stateTexts;
        stateTexts.reserve(switchStates.size());
        for (int i = 0; i < switchStates.size(); ++i)
        {
            stateTexts.append(QStringLiteral("%1路=%2").arg(i + 1).arg(switchStates.at(i) == 1 ? QStringLiteral("开启")
                                                                                              : QStringLiteral("关闭")));
        }
        qDebug() << "[TcpManager] 打击状态查询结果:" << stateTexts.join(QStringLiteral(", "));

        emit deviceJammingStatusQueried(switchStates);
        return true;
    }
    case 104:
    {
        const int payloadLen =
            frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
        if (payloadLen <= 0)
        {
            qDebug() << "[TcpManager] DataType=104 状态上报为空";
            return true;
        }

        const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
        QJsonParseError parseError;
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError)
        {
            qDebug() << "[TcpManager] DataType=104 JSON 解析失败:" << parseError.errorString();
            return true;
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
            return true;
        }

        const int mode = readJsonIntLocal(jsonObject, {QStringLiteral("mode")}, -1);
        const int switchStatus = readJsonIntLocal(jsonObject, {QStringLiteral("switch")}, -1);
        if (mode < 0 || switchStatus < 0)
        {
            qDebug() << "[TcpManager] DataType=104 缺少 mode/switch 字段";
            return true;
        }

        emit deviceJammingModeReported(mode, switchStatus);
        return true;
    }
    case 110:
    {
        const QString resultMsg = parseResultMessageLocal(frameData);
        const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"), Qt::CaseInsensitive);
        emit dronePrecisionStrikeResponse(pendingDronePrecisionStrikeTargetId,
                                          pendingDronePrecisionStrikeEnabled,
                                          success,
                                          resultMsg);
        return true;
    }
    case 115:
    {
        const QString resultMsg = parseResultMessageLocal(frameData);
        const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"), Qt::CaseInsensitive);
        emit droneWideBandJammingResponse(pendingDroneWideBandJammingTargetId,
                                          pendingDroneWideBandJammingEnabled,
                                          success,
                                          resultMsg);
        return true;
    }
    case 56:
        return true;
    case 93:
    {
        const QString resultMsg = parseResultMessageLocal(frameData);
        const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
        emit buzzerEnabledSetResponse(success, resultMsg);
        return true;
    }
    case 95:
    {
        const int payloadLen =
            frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
        if (payloadLen <= 0)
        {
            qDebug() << "[TcpManager] DataType=95 查询应答为空";
            return true;
        }

        const uint8_t enabled =
            static_cast<uint8_t>(static_cast<unsigned char>(frameData.constData()[sizeof(ProtocolHeader)]));
        qDebug() << "[TcpManager] 蜂鸣器开关查询结果: enabled =" << enabled;
        emit buzzerEnabledQueried(enabled);
        return true;
    }
    case 117:
    {
        const int payloadLen =
            frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
        if (payloadLen != kAlarmHistoryPayloadLength)
        {
            qDebug() << "[TcpManager] DataType=117 查询应答长度非法，实际:" << payloadLen
                     << "期望:" << kAlarmHistoryPayloadLength;
            return true;
        }

        const char *payload = frameData.constData() + sizeof(ProtocolHeader);
        int offset = 0;
        AlarmHistoryInfo info;
        info.deviceTemperature = readInt32LELocal(payload + offset);
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
            info.paOverpowerCounts[i] = readInt32LELocal(payload + offset);
            offset += 4;
        }
        for (int i = 0; i < info.paUnderpowerStatus.size(); ++i)
        {
            info.paUnderpowerStatus[i] = static_cast<unsigned char>(payload[offset++]);
            info.paUnderpowerCounts[i] = readInt32LELocal(payload + offset);
            offset += 4;
        }
        info.serverConnectionStatus = static_cast<unsigned char>(payload[offset++]);

        QStringList baseAlarmTexts;
        baseAlarmTexts << QStringLiteral("设备温度=%1").arg(info.deviceTemperature)
                       << QStringLiteral("设备风扇告警状态=%1").arg(info.fanAlarm)
                       << QStringLiteral("设备时钟告警状态=%1").arg(info.clockAlarm)
                       << QStringLiteral("设备接收锁相环告警状态=%1").arg(info.receiverPllAlarm)
                       << QStringLiteral("设备发送锁相环告警状态=%1").arg(info.transmitterPllAlarm)
                       << QStringLiteral("设备ADC芯片告警状态=%1").arg(info.adcAlarm)
                       << QStringLiteral("设备eeprom芯片告警状态=%1").arg(info.eepromAlarm)
                       << QStringLiteral("设备温度芯片告警状态=%1").arg(info.temperatureChipAlarm)
                       << QStringLiteral("设备电子罗盘芯片告警状态=%1").arg(info.compassAlarm);

        QStringList paSerialTexts;
        for (int i = 0; i < info.paSerialAlarms.size(); ++i)
        {
            paSerialTexts << QStringLiteral("PA%1串口告警状态=%2").arg(i + 1).arg(info.paSerialAlarms.at(i));
        }

        QStringList paOverpowerTexts;
        for (int i = 0; i < info.paOverpowerStatus.size(); ++i)
        {
            paOverpowerTexts << QStringLiteral("PA%1过功率告警状态=%2").arg(i + 1).arg(info.paOverpowerStatus.at(i))
                             << QStringLiteral("PA%1过功率告警历史次数=%2").arg(i + 1).arg(info.paOverpowerCounts.at(i));
        }

        QStringList paUnderpowerTexts;
        for (int i = 0; i < info.paUnderpowerStatus.size(); ++i)
        {
            paUnderpowerTexts << QStringLiteral("PA%1欠功率告警状态=%2").arg(i + 1).arg(info.paUnderpowerStatus.at(i))
                              << QStringLiteral("PA%1欠功率告警历史次数=%2").arg(i + 1).arg(info.paUnderpowerCounts.at(i));
        }

        qDebug() << "[TcpManager] 告警历史查询结果-设备告警:" << baseAlarmTexts.join(QStringLiteral(", "));
        qDebug() << "[TcpManager] 告警历史查询结果-PA串口:" << paSerialTexts.join(QStringLiteral(", "));
        qDebug() << "[TcpManager] 告警历史查询结果-PA过功率:" << paOverpowerTexts.join(QStringLiteral(", "));
        qDebug() << "[TcpManager] 告警历史查询结果-PA欠功率:" << paUnderpowerTexts.join(QStringLiteral(", "));
        qDebug() << "[TcpManager] 告警历史查询结果-服务器连接状态:" << info.serverConnectionStatus;

        emit alarmHistoryQueried(info);
        return true;
    }
    case 137:
    {
        const int payloadLen =
            frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
        if (payloadLen <= 0)
        {
            qDebug() << "[TcpManager] DataType=137 查询应答为空";
            return true;
        }

        const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
        QJsonParseError parseError;
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
        {
            qDebug() << "[TcpManager] DataType=137 JSON 解析失败:" << parseError.errorString();
            return true;
        }

        const QJsonObject jsonObject = jsonDoc.object();
        DeviceUsageInfo info;
        info.limit = jsonObject.value(QStringLiteral("limit")).toInt();
        info.remainingTimeSeconds = jsonObject.value(QStringLiteral("time")).toInt();
        info.remainingCount = jsonObject.value(QStringLiteral("num")).toInt();

        qDebug() << "[TcpManager] 设备使用时间查询结果:"
                 << QStringLiteral("limit=%1, time=%2, num=%3")
                        .arg(info.limit)
                        .arg(info.remainingTimeSeconds)
                        .arg(info.remainingCount);

        emit deviceUsageInfoQueried(info);
        return true;
    }
    case 202:
    {
        const QString resultMsg = parseResultMessageLocal(frameData);
        const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
        emit modelLibraryModeSetResponse(success, resultMsg);
        return true;
    }
    case 204:
    {
        const int payloadLen =
            frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
        if (payloadLen <= 0)
        {
            qDebug() << "[TcpManager] DataType=204 查询应答为空";
            return true;
        }

        const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
        QJsonParseError parseError;
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
        {
            qDebug() << "[TcpManager] DataType=204 JSON 解析失败:" << parseError.errorString();
            return true;
        }

        const uint8_t mode = static_cast<uint8_t>(jsonDoc.object().value(QStringLiteral("mode")).toInt());
        qDebug() << "[TcpManager] 机型库工作模式查询结果: mode =" << mode;
        emit modelLibraryModeQueried(mode);
        return true;
    }
    case 206:
    {
        const QString resultMsg = parseResultMessageLocal(frameData);
        const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
        emit modelLibraryRecordSetResponse(success, resultMsg);
        return true;
    }
    case 208:
    {
        const int payloadLen =
            frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
        if (payloadLen <= 0)
        {
            qDebug() << "[TcpManager] DataType=208 查询应答为空";
            return true;
        }

        const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
        QJsonParseError parseError;
        const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
        {
            qDebug() << "[TcpManager] DataType=208 JSON 解析失败:" << parseError.errorString();
            return true;
        }

        const QJsonObject jsonObject = jsonDoc.object();
        ModelLibraryPageResult result;
        result.total = readJsonIntLocal(jsonObject, {QStringLiteral("total"), QStringLiteral("sum")});
        result.current = qMax(1, readJsonIntLocal(jsonObject, {QStringLiteral("current"), QStringLiteral("start")}, 1));
        result.size =
            qMax(1, readJsonIntLocal(jsonObject, {QStringLiteral("size"), QStringLiteral("pageSize"),
                                                 QStringLiteral("limit")}, 10));

        const QJsonArray records = readJsonArrayLocal(
            jsonObject, {QStringLiteral("records"), QStringLiteral("data"), QStringLiteral("rows"), QStringLiteral("list")});
        result.records.reserve(records.size());
        for (const QJsonValue &recordValue : records)
        {
            if (!recordValue.isObject())
            {
                continue;
            }
            result.records.append(parseModelLibraryRecordLocal(recordValue.toObject()));
        }

        if (result.total <= 0 && !result.records.isEmpty())
        {
            result.total = result.records.size();
        }

        qDebug() << "[TcpManager] 机型库列表查询结果: total =" << result.total << ", current =" << result.current
                 << ", size =" << result.size << ", records =" << result.records.size();
        emit modelLibraryRecordsQueried(result);
        return true;
    }
    default:
        return false;
    }
}

void TcpManager::setDeviceJammingMode(int mode, int switchStatus)
{
    QJsonObject json;
    json["mode"] = mode;           // 0：驱离 1：迫降
    json["switch"] = switchStatus; // 0：关闭 1：开启
    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    pendingDeviceJammingMode = mode;
    pendingDeviceJammingSwitchStatus = switchStatus;
    sendFrame(100, data);
}

void TcpManager::queryDeviceJammingMode()
{
    qDebug() << "[TcpManager] 准备发送打击状态查询命令 (DataType=102)";
    sendFrame(102);
}

void TcpManager::queryDeviceAlarmHistory()
{
    qDebug() << "[TcpManager] 准备发送告警历史查询命令 (DataType=116)";
    sendFrame(116);
}

void TcpManager::queryDeviceUsageInfo()
{
    qDebug() << "[TcpManager] 准备发送设备使用时间查询命令 (DataType=136)";
    sendFrame(136);
}

void TcpManager::setBuzzerEnabled(uint8_t enabled)
{
    QByteArray data(1, 0);
    data[0] = static_cast<char>(enabled == 0 ? 0 : 1);
    qDebug() << "[TcpManager] 准备发送蜂鸣器开关设置命令 (DataType=92), enabled =" << static_cast<int>(data[0]);
    sendFrame(92, data);
}

void TcpManager::queryBuzzerEnabled()
{
    qDebug() << "[TcpManager] 准备发送蜂鸣器开关查询命令 (DataType=94)";
    sendFrame(94);
}

void TcpManager::setModelLibraryMode(uint8_t mode)
{
    QJsonObject json;
    json[QStringLiteral("mode")] = mode;
    qDebug() << "[TcpManager] 准备发送机型库工作模式设置命令 (DataType=201), mode =" << mode;
    sendFrame(201, QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void TcpManager::queryModelLibraryMode()
{
    qDebug() << "[TcpManager] 准备发送机型库工作模式查询命令 (DataType=203)";
    sendFrame(203);
}

void TcpManager::setModelLibraryRecord(const ModelLibraryUpdateRequest &request)
{
    QJsonArray freqBands;
    for (const ModelLibraryFreqBand &band : request.record.freqbands)
    {
        QJsonObject bandObject;
        bandObject[QStringLiteral("moduleId")] = QJsonValue::Null;
        bandObject[QStringLiteral("start")] = band.start;
        bandObject[QStringLiteral("end")] = band.end;
        bandObject[QStringLiteral("padb")] = QJsonValue::Null;
        freqBands.append(bandObject);
    }

    QJsonObject json;
    json[QStringLiteral("enable")] = request.record.enable;
    json[QStringLiteral("type")] = request.record.type;
    json[QStringLiteral("name")] = request.record.name;
    json[QStringLiteral("freqband")] = freqBands;
    json[QStringLiteral("delete")] = request.deleteFlag;
    json[QStringLiteral("sensitivity")] = request.record.sensitivity;

    const QByteArray payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
    qDebug() << "[TcpManager] 准备发送机型库记录设置命令 (DataType=205), enable =" << request.record.enable
             << ", type =" << request.record.type << ", name =" << request.record.name << ", delete ="
             << request.deleteFlag << ", sensitivity =" << request.record.sensitivity << ", freqbands ="
             << request.record.freqbands.size();
    sendFrame(205, payload);
}

void TcpManager::queryModelLibraryRecords(const ModelLibraryPageQuery &query)
{
    QJsonObject json;
    json[QStringLiteral("current")] = qMax(1, query.current);
    json[QStringLiteral("size")] = qMax(1, query.size);
    const QByteArray payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
    qDebug() << "[TcpManager] 准备发送机型库列表查询命令 (DataType=207), current =" << json.value(QStringLiteral("current")).toInt()
             << ", size =" << json.value(QStringLiteral("size")).toInt();
    sendFrame(207, payload);
}

void TcpManager::queryJammingBands()
{
    sendFrame(98);
}
