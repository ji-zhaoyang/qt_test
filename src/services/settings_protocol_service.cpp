#include "settings_protocol_service.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>

#include <cstring>

namespace
{
constexpr bool kUseJsonForO4ServerModeSetPayload = false;
constexpr bool kAllowJsonFallbackForO4ServerModeQuery = true;
constexpr int kDeviceIpPayloadLength = 18;
constexpr int kTcpServerIpPayloadLength = 6;
constexpr int kPowerAmplifierChannelCount = 6;
constexpr int kDirectionCalibrationValueCount = 6;
constexpr int kDirectionCalibrationPayloadLength =
    kDirectionCalibrationValueCount * static_cast<int>(sizeof(float));
constexpr int kSignalSourceChannelCount = 6;
constexpr int kFirmwareVersionRecordCount = 3;
constexpr int kFirmwareVersionRecordSize = 15;

QString parseResultMessage(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
}

QString escapeJsonString(const QString &value)
{
    QString escaped = value;
    escaped.replace(QStringLiteral("\\"), QStringLiteral("\\\\"));
    escaped.replace(QStringLiteral("\""), QStringLiteral("\\\""));
    escaped.replace(QStringLiteral("\b"), QStringLiteral("\\b"));
    escaped.replace(QStringLiteral("\f"), QStringLiteral("\\f"));
    escaped.replace(QStringLiteral("\n"), QStringLiteral("\\n"));
    escaped.replace(QStringLiteral("\r"), QStringLiteral("\\r"));
    escaped.replace(QStringLiteral("\t"), QStringLiteral("\\t"));
    return escaped;
}

QString extractInfoMessage(const QString &resultMsg)
{
    const QString trimmed = resultMsg.trimmed();
    const int infoPos = trimmed.indexOf(QStringLiteral("Info:"), 0, Qt::CaseInsensitive);
    if (infoPos < 0)
    {
        return trimmed;
    }

    const QString infoText = trimmed.mid(infoPos + 5).trimmed();
    return infoText.isEmpty() ? trimmed : infoText;
}

bool containsAnyKeyword(const QString &text, const QStringList &keywords)
{
    for (const QString &keyword : keywords)
    {
        if (text.contains(keyword, Qt::CaseInsensitive))
        {
            return true;
        }
    }
    return false;
}

QString classifyPatternUploadFailure(const QString &resultMsg)
{
    const QString infoText = extractInfoMessage(resultMsg);
    const QString normalized = infoText.trimmed();
    if (normalized.isEmpty())
    {
        return QStringLiteral("设备制作失败：设备未返回具体原因");
    }

    const QStringList ftpKeywords = {QStringLiteral("ftp"), QStringLiteral("login"), QStringLiteral("socket"),
                                     QStringLiteral("connect"), QStringLiteral("upload"), QStringLiteral("port"),
                                     QStringLiteral("用户名"), QStringLiteral("密码"), QStringLiteral("连接"),
                                     QStringLiteral("认证"), QStringLiteral("目录"), QStringLiteral("路径不存在")};
    if (containsAnyKeyword(normalized, ftpKeywords))
    {
        return QStringLiteral("FTP 失败：%1").arg(normalized);
    }

    const QStringList invalidParamKeywords = {QStringLiteral("invalid"), QStringLiteral("param"), QStringLiteral("json"),
                                              QStringLiteral("format"), QStringLiteral("range"), QStringLiteral("illegal"),
                                              QStringLiteral("字段"), QStringLiteral("参数"), QStringLiteral("格式"),
                                              QStringLiteral("范围"), QStringLiteral("频率"), QStringLiteral("通道"),
                                              QStringLiteral("time"), QStringLiteral("freq"), QStringLiteral("channel"),
                                              QStringLiteral("filename"), QStringLiteral("不能为空")};
    if (containsAnyKeyword(normalized, invalidParamKeywords))
    {
        return QStringLiteral("参数非法：%1").arg(normalized);
    }

    return QStringLiteral("设备制作失败：%1").arg(normalized);
}

QString buildPatternUploadDisplayMessage(const QString &resultMsg)
{
    const QString trimmed = resultMsg.trimmed();
    if (trimmed.isEmpty())
    {
        return QStringLiteral("设备制作失败：设备未返回结果");
    }

    if (trimmed.contains(QStringLiteral("RESULT:SUCCESSED"), Qt::CaseInsensitive))
    {
        const QString infoText = extractInfoMessage(trimmed);
        return infoText.isEmpty() || infoText == trimmed
                   ? QStringLiteral("设备已受理采集任务，请到 FTP 目录确认文件是否生成")
                   : QStringLiteral("设备已受理采集任务：%1，请到 FTP 目录确认文件是否生成").arg(infoText);
    }

    return classifyPatternUploadFailure(trimmed);
}

QString readFirmwareVersionString(const char *data, int maxLen)
{
    int len = 0;
    while (len < maxLen && data[len] != '\0')
    {
        ++len;
    }
    return QString::fromUtf8(data, len).trimmed();
}

void appendUInt16LE(QByteArray &buffer, quint16 value)
{
    buffer.append(static_cast<char>(value & 0xFF));
    buffer.append(static_cast<char>((value >> 8) & 0xFF));
}

void appendUInt32LE(QByteArray &buffer, quint32 value)
{
    buffer.append(static_cast<char>(value & 0xFF));
    buffer.append(static_cast<char>((value >> 8) & 0xFF));
    buffer.append(static_cast<char>((value >> 16) & 0xFF));
    buffer.append(static_cast<char>((value >> 24) & 0xFF));
}

void appendInt32LE(QByteArray &buffer, qint32 value)
{
    appendUInt32LE(buffer, static_cast<quint32>(value));
}

void appendFloatLE(QByteArray &buffer, float value)
{
    quint32 raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    appendUInt32LE(buffer, raw);
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

float readFloatLE(const char *data)
{
    const quint32 raw = readUInt32LE(data);
    float value = 0.0f;
    std::memcpy(&value, &raw, sizeof(value));
    return value;
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

bool parseJsonModeValue(const QByteArray &payloadBytes, int maxMode, uint8_t &modeOut)
{
    if (payloadBytes.isEmpty())
    {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(payloadBytes, &parseError);
    if (parseError.error == QJsonParseError::NoError && doc.isObject())
    {
        const QJsonObject json = doc.object();
        if (json.contains(QStringLiteral("mode")))
        {
            const int modeValue = json.value(QStringLiteral("mode")).toInt(-1);
            if (modeValue >= 0 && modeValue <= maxMode)
            {
                modeOut = static_cast<uint8_t>(modeValue);
                return true;
            }
        }
    }

    return false;
}

bool parseModeValue(const QByteArray &payloadBytes, int maxMode, uint8_t &modeOut)
{
    if (parseJsonModeValue(payloadBytes, maxMode, modeOut))
    {
        return true;
    }

    if (payloadBytes.size() >= 1)
    {
        const uint8_t modeValue = static_cast<uint8_t>(static_cast<unsigned char>(payloadBytes.at(0)));
        if (modeValue <= maxMode)
        {
            modeOut = modeValue;
            return true;
        }
    }

    return false;
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

QString readJsonString(const QJsonObject &object, const QStringList &keys)
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

QJsonArray readJsonArray(const QJsonObject &object, const QStringList &keys)
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

ModelLibraryFreqBand parseModelLibraryFreqBand(const QJsonValue &value)
{
    ModelLibraryFreqBand band;
    if (value.isObject())
    {
        const QJsonObject object = value.toObject();
        band.start = readJsonInt(object, {QStringLiteral("start"), QStringLiteral("begin"),
                                          QStringLiteral("startFreq"), QStringLiteral("low")});
        band.end = readJsonInt(object, {QStringLiteral("end"), QStringLiteral("stop"),
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

void appendModelLibraryFreqBand(const QJsonValue &value, QVector<ModelLibraryFreqBand> &freqBands)
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
            const ModelLibraryFreqBand band = parseModelLibraryFreqBand(value);
            if (band.start > 0 || band.end > 0)
            {
                freqBands.append(band);
            }
            return;
        }

        for (const QJsonValue &item : array)
        {
            const ModelLibraryFreqBand band = parseModelLibraryFreqBand(item);
            if (band.start > 0 || band.end > 0)
            {
                freqBands.append(band);
            }
        }
        return;
    }

    const ModelLibraryFreqBand band = parseModelLibraryFreqBand(value);
    if (band.start > 0 || band.end > 0)
    {
        freqBands.append(band);
    }
}

QVector<ModelLibraryFreqBand> extractModelLibraryFreqBands(const QJsonObject &object)
{
    QVector<ModelLibraryFreqBand> freqBands;

    const QStringList bandKeys = {QStringLiteral("freqbands"), QStringLiteral("freqBands"), QStringLiteral("bands"),
                                  QStringLiteral("freqband"), QStringLiteral("freqBand"), QStringLiteral("band")};
    for (const QString &key : bandKeys)
    {
        appendModelLibraryFreqBand(object.value(key), freqBands);
    }

    if (!freqBands.isEmpty())
    {
        return freqBands;
    }

    appendModelLibraryFreqBand(object, freqBands);
    if (!freqBands.isEmpty())
    {
        return freqBands;
    }

    for (int i = 1; i <= 8; ++i)
    {
        ModelLibraryFreqBand band;
        band.start = readJsonInt(object, {QStringLiteral("start%1").arg(i), QStringLiteral("begin%1").arg(i),
                                          QStringLiteral("low%1").arg(i), QStringLiteral("startFreq%1").arg(i)});
        band.end = readJsonInt(object, {QStringLiteral("end%1").arg(i), QStringLiteral("stop%1").arg(i),
                                        QStringLiteral("high%1").arg(i), QStringLiteral("endFreq%1").arg(i)});
        if (band.start > 0 || band.end > 0)
        {
            freqBands.append(band);
            continue;
        }

        appendModelLibraryFreqBand(object.value(QStringLiteral("freqband%1").arg(i)), freqBands);
        appendModelLibraryFreqBand(object.value(QStringLiteral("freqBand%1").arg(i)), freqBands);
        appendModelLibraryFreqBand(object.value(QStringLiteral("band%1").arg(i)), freqBands);
    }

    return freqBands;
}

ModelLibraryRecord parseModelLibraryRecord(const QJsonObject &object)
{
    ModelLibraryRecord record;
    record.type = readJsonInt(object, {QStringLiteral("type")});
    record.name = readJsonString(object, {QStringLiteral("name"), QStringLiteral("model"),
                                          QStringLiteral("droneName"), QStringLiteral("title")});
    record.sensitivity = readJsonInt(object, {QStringLiteral("sensitivity"), QStringLiteral("threshold"),
                                              QStringLiteral("gate"), QStringLiteral("limit")}, 1);
    record.enable = readJsonInt(object, {QStringLiteral("enable"), QStringLiteral("status"), QStringLiteral("enabled")});
    record.freqbands = extractModelLibraryFreqBands(object);
    return record;
}
} // namespace

SettingsProtocolService::SettingsProtocolService(QObject *parent) : QObject(parent)
{
}

void SettingsProtocolService::setFrameSender(const FrameSender &sender)
{
    sendFrame_ = sender;
}

void SettingsProtocolService::setDeviceGps(uint8_t mode, float lng, float lat, float alt)
{
    if (!sendFrame_)
    {
        return;
    }

    GpsSettingPayload payload;
    std::memset(&payload, 0, sizeof(payload));
    payload.mode = mode;
    payload.longitude = lng;
    payload.latitude = lat;
    payload.altitude = alt;
    sendFrame_(57, QByteArray(reinterpret_cast<const char *>(&payload), sizeof(payload)));
}

void SettingsProtocolService::queryDeviceGps()
{
    if (sendFrame_)
    {
        sendFrame_(59, QByteArray());
    }
}

void SettingsProtocolService::setDetectBands(const QVector<DetectBandParam> &bands)
{
    if (!sendFrame_ || bands.isEmpty() || bands.size() > 128)
    {
        return;
    }

    QByteArray payload;
    payload.reserve(2 + bands.size() * 12);
    appendUInt16LE(payload, static_cast<quint16>(bands.size()));
    for (const DetectBandParam &band : bands)
    {
        appendFloatLE(payload, band.freqMhz);
        appendInt32LE(payload, band.measureCount);
        appendInt32LE(payload, band.gain);
    }
    sendFrame_(8, payload);
}

void SettingsProtocolService::queryDetectBands()
{
    if (sendFrame_)
    {
        sendFrame_(10, QByteArray());
    }
}

void SettingsProtocolService::setDroneReportMode(uint8_t mode)
{
    if (!sendFrame_ || mode > 4)
    {
        return;
    }

    QByteArray payload;
    payload.append(static_cast<char>(mode));
    sendFrame_(61, payload);
}

void SettingsProtocolService::queryDroneReportMode()
{
    if (sendFrame_)
    {
        sendFrame_(63, QByteArray());
    }
}

void SettingsProtocolService::setSuppressionMode(uint8_t mode)
{
    if (!sendFrame_ || mode > 1)
    {
        return;
    }

    QJsonObject json;
    json[QStringLiteral("mode")] = static_cast<int>(mode);
    sendFrame_(130, QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void SettingsProtocolService::querySuppressionMode()
{
    if (sendFrame_)
    {
        sendFrame_(132, QByteArray());
    }
}

void SettingsProtocolService::setO4ServerMode(uint8_t mode)
{
    if (!sendFrame_ || mode > 1)
    {
        return;
    }

    QByteArray payload;
    if (kUseJsonForO4ServerModeSetPayload)
    {
        QJsonObject json;
        json[QStringLiteral("mode")] = static_cast<int>(mode);
        payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
    }
    else
    {
        payload.append(static_cast<char>(mode));
    }

    sendFrame_(221, payload);
}

void SettingsProtocolService::queryO4ServerMode()
{
    if (sendFrame_)
    {
        sendFrame_(223, QByteArray());
    }
}

void SettingsProtocolService::setUavCategoryDisplayMode(uint8_t mode)
{
    if (!sendFrame_ || mode > 1)
    {
        return;
    }

    QByteArray payload;
    payload.append(static_cast<char>(mode));
    sendFrame_(181, payload);
}

void SettingsProtocolService::queryUavCategoryDisplayMode()
{
    if (sendFrame_)
    {
        sendFrame_(183, QByteArray());
    }
}

void SettingsProtocolService::setDataEnable(uint8_t enabled)
{
    if (!sendFrame_ || enabled > 1)
    {
        return;
    }

    QByteArray payload;
    payload.append(static_cast<char>(enabled));
    sendFrame_(214, payload);
}

void SettingsProtocolService::queryDataEnable()
{
    if (sendFrame_)
    {
        sendFrame_(216, QByteArray());
    }
}

void SettingsProtocolService::setFeatureModes(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled)
{
    if (!sendFrame_ || wifiRemoteIdEnabled > 1 || fpvEnabled > 1 || djiParseEnabled > 1)
    {
        return;
    }

    QByteArray payload;
    payload.reserve(3);
    payload.append(static_cast<char>(wifiRemoteIdEnabled));
    payload.append(static_cast<char>(fpvEnabled));
    payload.append(static_cast<char>(djiParseEnabled));
    sendFrame_(254, payload);
}

void SettingsProtocolService::queryFeatureModes()
{
    if (sendFrame_)
    {
        sendFrame_(252, QByteArray());
    }
}

void SettingsProtocolService::setFullScanParams(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin,
                                               double att)
{
    if (!sendFrame_)
    {
        return;
    }

    QJsonObject json;
    json[QStringLiteral("ssth")] = ssth;
    json[QStringLiteral("ss_jg_max")] = ssJgMax;
    json[QStringLiteral("ss_jg_min")] = ssJgMin;
    json[QStringLiteral("ss_max")] = ssMax;
    json[QStringLiteral("ss_min")] = ssMin;
    json[QStringLiteral("att")] = att;
    sendFrame_(193, QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void SettingsProtocolService::queryFullScanParams()
{
    if (sendFrame_)
    {
        sendFrame_(195, QByteArray());
    }
}

void SettingsProtocolService::setDeviceIp(const QString &ip, int port, const QString &mask, const QString &route,
                                          const QString &dns)
{
    if (!sendFrame_ || port < 0 || port > 65535)
    {
        return;
    }

    QByteArray payload;
    payload.reserve(kDeviceIpPayloadLength);
    if (!appendIpv4Bytes(payload, ip) || !appendIpv4Bytes(payload, mask) || !appendIpv4Bytes(payload, route) ||
        !appendIpv4Bytes(payload, dns))
    {
        return;
    }

    QByteArray withPort;
    withPort.reserve(kDeviceIpPayloadLength);
    appendIpv4Bytes(withPort, ip);
    appendUInt16LE(withPort, static_cast<quint16>(port));
    appendIpv4Bytes(withPort, mask);
    appendIpv4Bytes(withPort, route);
    appendIpv4Bytes(withPort, dns);
    sendFrame_(25, withPort);
}

void SettingsProtocolService::queryDeviceIp()
{
    if (sendFrame_)
    {
        sendFrame_(27, QByteArray());
    }
}

void SettingsProtocolService::setTcpServerIp(const QString &ip, int port)
{
    if (!sendFrame_ || port < 0 || port > 65535)
    {
        return;
    }

    QByteArray payload;
    payload.reserve(kTcpServerIpPayloadLength);
    if (!appendIpv4Bytes(payload, ip))
    {
        return;
    }

    appendUInt16LE(payload, static_cast<quint16>(port));
    sendFrame_(237, payload);
}

void SettingsProtocolService::queryTcpServerIp()
{
    if (sendFrame_)
    {
        sendFrame_(239, QByteArray());
    }
}

void SettingsProtocolService::setModelLibraryMode(uint8_t mode)
{
    if (!sendFrame_)
    {
        return;
    }

    QJsonObject json;
    json[QStringLiteral("mode")] = mode;
    sendFrame_(201, QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void SettingsProtocolService::queryModelLibraryMode()
{
    if (sendFrame_)
    {
        sendFrame_(203, QByteArray());
    }
}

void SettingsProtocolService::setModelLibraryRecord(const ModelLibraryUpdateRequest &request)
{
    if (!sendFrame_)
    {
        return;
    }

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
    sendFrame_(205, QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void SettingsProtocolService::queryModelLibraryRecords(const ModelLibraryPageQuery &query)
{
    if (!sendFrame_)
    {
        return;
    }

    QJsonObject json;
    json[QStringLiteral("current")] = qMax(1, query.current);
    json[QStringLiteral("size")] = qMax(1, query.size);
    sendFrame_(207, QJsonDocument(json).toJson(QJsonDocument::Compact));
}


void SettingsProtocolService::setStrikeFrequencyBands(const StrikeFrequencyBandList &bands)
{
    if (!sendFrame_)
    {
        return;
    }

    QJsonArray jsonArray;
    for (const StrikeFrequencyBandConfig &band : bands)
    {
        QJsonObject item;
        item[QStringLiteral("enable")] = band.enable;
        item[QStringLiteral("start")] = band.startMhz;
        item[QStringLiteral("end")] = band.endMhz;
        item[QStringLiteral("att")] = band.att;
        jsonArray.append(item);
    }

    sendFrame_(96, QJsonDocument(jsonArray).toJson(QJsonDocument::Compact));
}

void SettingsProtocolService::queryStrikeFrequencyBands()
{
    if (sendFrame_)
    {
        sendFrame_(98, QByteArray());
    }
}

void SettingsProtocolService::setPowerAmplifierParams(const PowerAmplifierParamList &params)
{
    if (!sendFrame_)
    {
        return;
    }

    QJsonArray jsonArray;
    for (int i = 0; i < kPowerAmplifierChannelCount; ++i)
    {
        const PowerAmplifierParam param = i < params.size() ? params.at(i) : PowerAmplifierParam();
        QJsonObject item;
        item[QStringLiteral("K")] = param.k;
        item[QStringLiteral("B")] = param.b;
        item[QStringLiteral("att")] = param.att;
        jsonArray.append(item);
    }

    sendFrame_(118, QJsonDocument(jsonArray).toJson(QJsonDocument::Compact));
}

void SettingsProtocolService::queryPowerAmplifierParams()
{
    if (sendFrame_)
    {
        sendFrame_(120, QByteArray());
    }
}

void SettingsProtocolService::setDirectionCalibrationValues(const DirectionCalibrationValueList &values)
{
    if (!sendFrame_)
    {
        return;
    }

    QByteArray payload;
    payload.reserve(kDirectionCalibrationPayloadLength);
    for (int i = 0; i < kDirectionCalibrationValueCount; ++i)
    {
        appendFloatLE(payload, i < values.size() ? values.at(i) : 0.0f);
    }

    sendFrame_(124, payload);
}

void SettingsProtocolService::queryDirectionCalibrationValues()
{
    if (sendFrame_)
    {
        sendFrame_(126, QByteArray());
    }
}

void SettingsProtocolService::setSignalSourceParams(int serialScan, const QVector<int> &scanModes, int vcoMode,
                                                    const QVector<int> &vcoScans)
{
    if (!sendFrame_)
    {
        return;
    }

    QJsonObject json;
    json[QStringLiteral("SerialScan")] = serialScan;
    for (int i = 0; i < kSignalSourceChannelCount; ++i)
    {
        json[QStringLiteral("ScanMode%1").arg(i + 1)] = i < scanModes.size() ? scanModes.at(i) : 0;
        json[QStringLiteral("VcoScan%1").arg(i + 1)] = i < vcoScans.size() ? vcoScans.at(i) : 0;
    }
    json[QStringLiteral("VcoMode")] = vcoMode;

    sendFrame_(105, QJsonDocument(json).toJson(QJsonDocument::Compact));
}

void SettingsProtocolService::querySignalSourceParams()
{
    if (sendFrame_)
    {
        sendFrame_(107, QByteArray());
    }
}

void SettingsProtocolService::uploadPatternFile(const PatternUploadRequest &request)
{
    if (!sendFrame_)
    {
        return;
    }

    qDebug().noquote()
        << QStringLiteral("[DEBUG-B] settings_protocol_service.cpp:uploadPatternFile | "
                          "准备组装 DataType=18 | ip=%1 port=%2 user=%3 path=%4 time=%5 channel=%6 type=%7 filename=\"%8\" freq=%9")
               .arg(request.ip.trimmed(),
                    QString::number(request.port),
                    request.user.trimmed(),
                    request.path.trimmed(),
                    QString::number(request.time),
                    QString::number(request.channel),
                    QString::number(request.type),
                    request.filename.trimmed(),
                    QString::number(request.freq, 'f', 1));

    const QString payload = QStringLiteral("{"
                                           "\"ip\":\"%1\","
                                           "\"port\":%2,"
                                           "\"user\":\"%3\","
                                           "\"password\":\"%4\","
                                           "\"path\":\"%5\","
                                           "\"time\":%6,"
                                           "\"channel\":%7,"
                                           "\"type\":%8,"
                                           "\"filename\":\"%9\","
                                           "\"freq\":%10"
                                           "}")
                                .arg(escapeJsonString(request.ip.trimmed()))
                                .arg(request.port)
                                .arg(escapeJsonString(request.user.trimmed()))
                                .arg(escapeJsonString(request.password))
                                .arg(escapeJsonString(request.path.trimmed()))
                                .arg(request.time)
                                .arg(request.channel)
                                .arg(request.type)
                                .arg(escapeJsonString(request.filename.trimmed()))
                                .arg(QString::number(request.freq, 'f', 1));

    qDebug().noquote()
        << QStringLiteral("[DEBUG-B] settings_protocol_service.cpp:uploadPatternFile | DataType=18 JSON=%1").arg(payload);

    sendFrame_(18, payload.toUtf8());
}

void SettingsProtocolService::queryFirmwareVersions()
{
    if (sendFrame_)
    {
        sendFrame_(14, QByteArray());
    }
}

void SettingsProtocolService::handleGpsSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit deviceGpsSetResponse(success, resultMsg);
    if (success)
    {
        queryDeviceGps();
    }
}

void SettingsProtocolService::handleGpsQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < static_cast<int>(sizeof(GpsSettingPayload)))
    {
        return;
    }

    const GpsSettingPayload *payload =
        reinterpret_cast<const GpsSettingPayload *>(frameData.constData() + sizeof(ProtocolHeader));
    if (payload->latitude >= -90.0f && payload->latitude <= 90.0f && payload->longitude >= -180.0f &&
        payload->longitude <= 180.0f)
    {
        emit deviceGpsQueried(payload->mode, payload->longitude, payload->latitude, payload->altitude);
    }
}

void SettingsProtocolService::handleDetectBandSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit detectBandsSetResponse(success, resultMsg);
    if (success)
    {
        queryDetectBands();
    }
}

void SettingsProtocolService::handleDetectBandQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 2)
    {
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    const quint16 bandCount = readUInt16LE(payload);
    const int expectedLen = 2 + static_cast<int>(bandCount) * 12;
    if (payloadLen < expectedLen)
    {
        return;
    }

    QVector<DetectBandParam> bands;
    bands.reserve(static_cast<int>(bandCount));
    const char *bandData = payload + 2;
    for (quint16 i = 0; i < bandCount; ++i)
    {
        DetectBandParam band = {};
        band.freqMhz = readFloatLE(bandData);
        band.measureCount = readInt32LE(bandData + 4);
        band.gain = readInt32LE(bandData + 8);
        bands.append(band);
        bandData += 12;
    }

    emit detectBandsQueried(bands);
}

void SettingsProtocolService::handleDroneReportModeSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit droneReportModeSetResponse(success, resultMsg);
    if (success)
    {
        queryDroneReportMode();
    }
}

void SettingsProtocolService::handleDroneReportModeQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 1)
    {
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    uint8_t mode = 0;
    if (parseModeValue(payloadBytes, 4, mode))
    {
        emit droneReportModeQueried(mode);
    }
}

void SettingsProtocolService::handleSuppressionModeSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit suppressionModeSetResponse(success, resultMsg);
    if (success)
    {
        querySuppressionMode();
    }
}

void SettingsProtocolService::handleSuppressionModeQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 1)
    {
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    uint8_t mode = 0;
    if (parseModeValue(payloadBytes, 1, mode))
    {
        emit suppressionModeQueried(mode);
    }
}

void SettingsProtocolService::handleO4ServerModeSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit o4ServerModeSetResponse(success, resultMsg);
    if (success)
    {
        queryO4ServerMode();
    }
}

void SettingsProtocolService::handleO4ServerModeQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 1)
    {
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    uint8_t mode = 0;
    bool ok = false;
    if (!payloadBytes.isEmpty())
    {
        const uint8_t byteMode = static_cast<uint8_t>(static_cast<unsigned char>(payloadBytes.at(0)));
        if (byteMode <= 1)
        {
            mode = byteMode;
            ok = true;
        }
    }
    if (!ok && kAllowJsonFallbackForO4ServerModeQuery)
    {
        ok = parseJsonModeValue(payloadBytes, 1, mode);
    }
    if (ok)
    {
        emit o4ServerModeQueried(mode);
    }
}

void SettingsProtocolService::handleUavCategoryDisplayModeSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit uavCategoryDisplayModeSetResponse(success, resultMsg);
    if (success)
    {
        queryUavCategoryDisplayMode();
    }
}

void SettingsProtocolService::handleUavCategoryDisplayModeQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 1)
    {
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    uint8_t mode = 0;
    if (parseModeValue(payloadBytes, 1, mode))
    {
        emit uavCategoryDisplayModeQueried(mode);
    }
}

void SettingsProtocolService::handleDataEnableSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit dataEnableSetResponse(success, resultMsg);
    if (success)
    {
        queryDataEnable();
    }
}

void SettingsProtocolService::handleDataEnableQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 1)
    {
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    uint8_t enabled = 0;
    if (parseModeValue(payloadBytes, 1, enabled))
    {
        emit dataEnableQueried(enabled);
    }
}

void SettingsProtocolService::handleFeatureModesSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit featureModesSetResponse(success, resultMsg);
    if (success)
    {
        queryFeatureModes();
    }
}

void SettingsProtocolService::handleFeatureModesQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 3)
    {
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    const uint8_t wifiRemoteIdEnabled = static_cast<uint8_t>(static_cast<unsigned char>(payloadBytes.at(0)));
    const uint8_t fpvEnabled = static_cast<uint8_t>(static_cast<unsigned char>(payloadBytes.at(1)));
    const uint8_t djiParseEnabled = static_cast<uint8_t>(static_cast<unsigned char>(payloadBytes.at(2)));
    if (wifiRemoteIdEnabled <= 1 && fpvEnabled <= 1 && djiParseEnabled <= 1)
    {
        emit featureModesQueried(wifiRemoteIdEnabled, fpvEnabled, djiParseEnabled);
    }
}

void SettingsProtocolService::handleFullScanSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit fullScanParamsSetResponse(success, resultMsg);
    if (success)
    {
        queryFullScanParams();
    }
}

void SettingsProtocolService::handleFullScanQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        return;
    }

    const QByteArray jsonBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        return;
    }

    const QJsonObject json = doc.object();
    emit fullScanParamsQueried(json.value(QStringLiteral("ssth")).toDouble(),
                               json.value(QStringLiteral("ss_jg_max")).toDouble(),
                               json.value(QStringLiteral("ss_jg_min")).toDouble(),
                               json.value(QStringLiteral("ss_max")).toDouble(),
                               json.value(QStringLiteral("ss_min")).toDouble(),
                               json.value(QStringLiteral("att")).toDouble());
}

void SettingsProtocolService::handleDeviceIpSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit deviceIpSetResponse(success, resultMsg);
    if (success)
    {
        queryDeviceIp();
    }
}

void SettingsProtocolService::handleDeviceIpQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < kDeviceIpPayloadLength)
    {
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    emit deviceIpQueried(ipv4BytesToString(payload), static_cast<int>(readUInt16LE(payload + 4)),
                         ipv4BytesToString(payload + 6), ipv4BytesToString(payload + 10),
                         ipv4BytesToString(payload + 14));
}

void SettingsProtocolService::handleTcpServerIpSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit tcpServerIpSetResponse(success, resultMsg);
    if (success)
    {
        queryTcpServerIp();
    }
}

void SettingsProtocolService::handleTcpServerIpQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < kTcpServerIpPayloadLength)
    {
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    emit tcpServerIpQueried(ipv4BytesToString(payload), static_cast<int>(readUInt16LE(payload + 4)));
}

void SettingsProtocolService::handleModelLibraryModeSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit modelLibraryModeSetResponse(success, resultMsg);
}

void SettingsProtocolService::handleModelLibraryModeQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        return;
    }

    const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
    {
        return;
    }

    emit modelLibraryModeQueried(static_cast<uint8_t>(jsonDoc.object().value(QStringLiteral("mode")).toInt()));
}

void SettingsProtocolService::handleModelLibraryRecordSetResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit modelLibraryRecordSetResponse(success, resultMsg);
}

void SettingsProtocolService::handleModelLibraryRecordsQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        return;
    }

    const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
    {
        return;
    }

    const QJsonObject jsonObject = jsonDoc.object();
    ModelLibraryPageResult result;
    result.total = readJsonInt(jsonObject, {QStringLiteral("total"), QStringLiteral("sum")});
    result.current = qMax(1, readJsonInt(jsonObject, {QStringLiteral("current"), QStringLiteral("start")}, 1));
    result.size = qMax(1, readJsonInt(jsonObject, {QStringLiteral("size"), QStringLiteral("pageSize"),
                                                   QStringLiteral("limit")}, 10));

    const QJsonArray records = readJsonArray(
        jsonObject, {QStringLiteral("records"), QStringLiteral("data"), QStringLiteral("rows"), QStringLiteral("list")});
    result.records.reserve(records.size());
    for (const QJsonValue &recordValue : records)
    {
        if (recordValue.isObject())
        {
            result.records.append(parseModelLibraryRecord(recordValue.toObject()));
        }
    }

    if (result.total <= 0 && !result.records.isEmpty())
    {
        result.total = result.records.size();
    }

    emit modelLibraryRecordsQueried(result);
}

void SettingsProtocolService::handleStrikeFrequencySettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit strikeFrequencyBandsSetResponse(success, resultMsg);
}

void SettingsProtocolService::handleStrikeFrequencyQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        return;
    }

    const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isArray())
    {
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
        band.enable = item.value(QStringLiteral("enable")).toInt();
        band.startMhz = item.value(QStringLiteral("start")).toDouble();
        band.endMhz = item.value(QStringLiteral("end")).toDouble();
        band.att = item.value(QStringLiteral("att")).toInt();
        bands.append(band);
    }

    emit strikeFrequencyBandsQueried(bands);
}

void SettingsProtocolService::handlePowerAmplifierSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit powerAmplifierParamsSetResponse(success, resultMsg);
}

void SettingsProtocolService::handlePowerAmplifierQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        return;
    }

    const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isArray())
    {
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
            param.k = item.value(QStringLiteral("K")).toDouble();
            param.b = item.value(QStringLiteral("B")).toDouble();
            param.outpower = item.value(QStringLiteral("outpower")).toDouble();
            param.att = item.value(QStringLiteral("att")).toDouble();
        }
        params.append(param);
    }

    emit powerAmplifierParamsQueried(params);
}

void SettingsProtocolService::handleDirectionCalibrationSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit directionCalibrationValuesSetResponse(success, resultMsg);
}

void SettingsProtocolService::handleDirectionCalibrationQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen != kDirectionCalibrationPayloadLength)
    {
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    DirectionCalibrationValueList values;
    values.reserve(kDirectionCalibrationValueCount);
    for (int i = 0; i < kDirectionCalibrationValueCount; ++i)
    {
        values.append(readFloatLE(payload + i * static_cast<int>(sizeof(float))));
    }

    emit directionCalibrationValuesQueried(values);
}

void SettingsProtocolService::handleSignalSourceParamsSettingResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
    emit signalSourceParamsSetResponse(success, resultMsg);
}

void SettingsProtocolService::handleSignalSourceParamsQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen <= 0)
    {
        return;
    }

    const QByteArray payload(frameData.constData() + sizeof(ProtocolHeader), payloadLen);
    QJsonParseError parseError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject())
    {
        return;
    }

    const QJsonObject jsonObject = jsonDoc.object();
    SignalSourceParamsConfig config;
    config.serialScan = jsonObject.value(QStringLiteral("SerialScan")).toInt();
    config.vcoMode = jsonObject.value(QStringLiteral("VcoMode")).toInt();
    config.scanModes.fill(0, kSignalSourceChannelCount);
    config.vcoScans.fill(0, kSignalSourceChannelCount);
    for (int i = 0; i < kSignalSourceChannelCount; ++i)
    {
        const QString scanModeKey = QStringLiteral("ScanMode%1").arg(i + 1);
        const QString vcoScanKey = QStringLiteral("VcoScan%1").arg(i + 1);
        config.scanModes[i] =
            jsonObject.contains(scanModeKey) ? jsonObject.value(scanModeKey).toInt()
                                             : jsonObject.value(QStringLiteral("ScanMode")).toInt();
        config.vcoScans[i] =
            jsonObject.contains(vcoScanKey) ? jsonObject.value(vcoScanKey).toInt()
                                            : jsonObject.value(QStringLiteral("VcoScan")).toInt();
    }

    emit signalSourceParamsQueried(config);
}

void SettingsProtocolService::handlePatternUploadResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"), Qt::CaseInsensitive);
    qDebug().noquote()
        << QStringLiteral("[DEBUG-19] settings_protocol_service.cpp:handlePatternUploadResponse | rawResponse=%1")
               .arg(resultMsg);
    qDebug().noquote()
        << QStringLiteral("[DEBUG-19] settings_protocol_service.cpp:handlePatternUploadResponse | success=%1 display=%2")
               .arg(success ? QStringLiteral("true") : QStringLiteral("false"), buildPatternUploadDisplayMessage(resultMsg));
    emit patternUploadResponse(success, buildPatternUploadDisplayMessage(resultMsg));
}

void SettingsProtocolService::handleFirmwareVersionsQueryResponse(const QByteArray &frameData)
{
    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const int expectedLen = kFirmwareVersionRecordCount * kFirmwareVersionRecordSize;
    if (payloadLen < expectedLen)
    {
        qDebug() << "[SettingsProtocolService] DataType=15 版本查询应答长度不足，实际:" << payloadLen
                 << "预期至少:" << expectedLen;
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    const QString appVersion = readFirmwareVersionString(payload + 1, 14);
    const QString fpgaVersion = readFirmwareVersionString(payload + 16, 14);
    const QString gpuVersion = readFirmwareVersionString(payload + 31, 14);

    emit firmwareVersionsQueried(appVersion, fpgaVersion, gpuVersion);
}
