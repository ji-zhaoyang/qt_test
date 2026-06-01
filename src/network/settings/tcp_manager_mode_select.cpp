#include "tcp_manager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
constexpr bool kUseJsonForO4ServerModeSetPayload = false;
constexpr bool kAllowJsonFallbackForO4ServerModeQuery = true;

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
        if (json.contains("mode"))
        {
            const int modeValue = json.value("mode").toInt(-1);
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

} // namespace

bool TcpManager::dispatchModeSelectProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 62:
        handleDroneReportModeSettingResponse(header, frameData);
        return true;
    case 64:
        handleDroneReportModeQueryResponse(header, frameData);
        return true;
    case 131:
        handleSuppressionModeSettingResponse(header, frameData);
        return true;
    case 133:
        handleSuppressionModeQueryResponse(header, frameData);
        return true;
    case 182:
        handleUavCategoryDisplayModeSettingResponse(header, frameData);
        return true;
    case 184:
        handleUavCategoryDisplayModeQueryResponse(header, frameData);
        return true;
    case 215:
        handleDataEnableSettingResponse(header, frameData);
        return true;
    case 217:
        handleDataEnableQueryResponse(header, frameData);
        return true;
    case 222:
        handleO4ServerModeSettingResponse(header, frameData);
        return true;
    case 224:
        handleO4ServerModeQueryResponse(header, frameData);
        return true;
    case 253:
        handleFeatureModesQueryResponse(header, frameData);
        return true;
    case 255:
        handleFeatureModesSettingResponse(header, frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::setDroneReportMode(uint8_t mode)
{
    if (mode > 4)
    {
        qDebug() << "[TcpManager] 无人机上报模式设置失败，模式值非法:" << mode;
        return;
    }

    qDebug() << "[TcpManager] 准备发送无人机上报模式设置 (DataType=61), mode =" << mode;
    QByteArray payload;
    payload.append(static_cast<char>(mode));
    sendFrame(61, payload);
}

void TcpManager::queryDroneReportMode()
{
    qDebug() << "[TcpManager] 准备发送无人机上报模式查询命令 (DataType=63)...";
    sendFrame(63);
}

void TcpManager::setSuppressionMode(uint8_t mode)
{
    if (mode > 1)
    {
        qDebug() << "[TcpManager] 压制模式设置失败，模式值非法:" << mode;
        return;
    }

    QJsonObject json;
    json["mode"] = static_cast<int>(mode);
    const QByteArray payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
    sendFrame(130, payload);
}

void TcpManager::querySuppressionMode()
{
    qDebug() << "[TcpManager] 准备发送压制模式查询命令 (DataType=132)...";
    sendFrame(132);
}

void TcpManager::setO4ServerMode(uint8_t mode)
{
    if (mode > 1)
    {
        qDebug() << "[TcpManager] O4服务器模式设置失败，模式值非法:" << mode;
        return;
    }

    QByteArray payload;
    if (kUseJsonForO4ServerModeSetPayload)
    {
        QJsonObject json;
        json["mode"] = static_cast<int>(mode);
        payload = QJsonDocument(json).toJson(QJsonDocument::Compact);
        qDebug() << "[TcpManager] 准备发送O4服务器模式设置 (DataType=221), mode =" << mode << "payloadType=JSON";
    }
    else
    {
        payload.append(static_cast<char>(mode));
        qDebug() << "[TcpManager] 准备发送O4服务器模式设置 (DataType=221), mode =" << mode << "payloadType=Byte";
    }

    sendFrame(221, payload);
}

void TcpManager::queryO4ServerMode()
{
    sendFrame(223);
}

void TcpManager::setUavCategoryDisplayMode(uint8_t mode)
{
    if (mode > 1)
    {
        qDebug() << "[TcpManager] 无人机类别显示设置失败，模式值非法:" << mode;
        return;
    }

    QByteArray payload;
    payload.append(static_cast<char>(mode));
    qDebug() << "[TcpManager] 准备发送无人机类别显示设置 (DataType=181), mode =" << mode;
    sendFrame(181, payload);
}

void TcpManager::queryUavCategoryDisplayMode()
{
    qDebug() << "[TcpManager] 准备发送无人机类别显示查询命令 (DataType=183)...";
    sendFrame(183);
}

void TcpManager::setDataEnable(uint8_t enabled)
{
    if (enabled > 1)
    {
        qDebug() << "[TcpManager] 数据使能设置失败，开关值非法:" << enabled;
        return;
    }

    QByteArray payload;
    payload.append(static_cast<char>(enabled));
    qDebug() << "[TcpManager] 准备发送数据使能设置 (DataType=214), enabled =" << enabled;
    sendFrame(214, payload);
}

void TcpManager::queryDataEnable()
{
    qDebug() << "[TcpManager] 准备发送数据使能查询命令 (DataType=216)...";
    sendFrame(216);
}

void TcpManager::setFeatureModes(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled)
{
    if (wifiRemoteIdEnabled > 1 || fpvEnabled > 1 || djiParseEnabled > 1)
    {
        qDebug() << "[TcpManager] 功能模式设置失败，存在非法开关值:" << wifiRemoteIdEnabled << fpvEnabled
                 << djiParseEnabled;
        return;
    }

    QByteArray payload;
    payload.reserve(3);
    payload.append(static_cast<char>(wifiRemoteIdEnabled));
    payload.append(static_cast<char>(fpvEnabled));
    payload.append(static_cast<char>(djiParseEnabled));
    qDebug() << "[TcpManager] 准备发送功能模式设置 (DataType=254), flags =" << wifiRemoteIdEnabled << fpvEnabled
             << djiParseEnabled;
    sendFrame(254, payload);
}

void TcpManager::queryFeatureModes()
{
    qDebug() << "[TcpManager] 准备发送功能模式查询命令 (DataType=252)...";
    sendFrame(252);
}

void TcpManager::handleDroneReportModeSettingResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    qDebug() << "[TcpManager] 收到无人机上报模式设置应答 (DataType=62), success =" << success
             << " message =" << resultMsg;

    emit droneReportModeSetResponse(success, resultMsg);

    if (success)
    {
        qDebug() << "[TcpManager] 无人机上报模式设置成功，自动发起查询 (DataType=63)...";
        queryDroneReportMode();
    }
}

void TcpManager::handleDroneReportModeQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 1)
    {
        qDebug() << "[TcpManager] DataType=64 查询应答长度不足，实际:" << payloadLen;
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    uint8_t mode = 0;
    if (!parseModeValue(payloadBytes, 4, mode))
    {
        qDebug() << "[TcpManager] DataType=64 查询应答无法解析 mode，Payload =" << QString::fromUtf8(payloadBytes)
                 << " Hex =" << payloadBytes.toHex(' ').toUpper();
        return;
    }

    qDebug() << "[TcpManager] 收到无人机上报模式查询应答 (DataType=64), mode =" << mode;

    emit droneReportModeQueried(mode);
}

void TcpManager::handleSuppressionModeSettingResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    qDebug() << "[TcpManager] 收到压制模式设置应答 (DataType=131), success =" << success << " message =" << resultMsg;

    emit suppressionModeSetResponse(success, resultMsg);

    if (success)
    {
        qDebug() << "[TcpManager] 压制模式设置成功，自动发起查询 (DataType=132)...";
        querySuppressionMode();
    }
}

void TcpManager::handleSuppressionModeQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 1)
    {
        qDebug() << "[TcpManager] DataType=133 查询应答长度不足，实际:" << payloadLen;
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    uint8_t mode = 0;
    if (!parseModeValue(payloadBytes, 1, mode))
    {
        qDebug() << "[TcpManager] DataType=133 查询应答无法解析 mode，Payload =" << QString::fromUtf8(payloadBytes)
                 << " Hex =" << payloadBytes.toHex(' ').toUpper();
        return;
    }

    qDebug() << "[TcpManager] 收到压制模式查询应答 (DataType=133), mode =" << mode;

    emit suppressionModeQueried(mode);
}

void TcpManager::handleO4ServerModeSettingResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), qMax(0, msgLen));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    qDebug().noquote()
        << QStringLiteral("[TcpManager] O4设置应答原始 PayloadHex=%1 PayloadUtf8=%2")
               .arg(QString::fromLatin1(payloadBytes.toHex(' ').toUpper()))
               .arg(QString::fromUtf8(payloadBytes));
    qDebug() << "[TcpManager] 收到O4服务器模式设置应答 (DataType=222), success =" << success << " message =" << resultMsg;

    emit o4ServerModeSetResponse(success, resultMsg);

    if (success)
    {
        qDebug() << "[TcpManager] O4服务器模式设置成功，自动发起查询 (DataType=223)...";
        queryO4ServerMode();
    }
}

void TcpManager::handleO4ServerModeQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 1)
    {
        qDebug() << "[TcpManager] DataType=224 查询应答长度不足，实际:" << payloadLen;
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    uint8_t mode = 0;
    QString parsePath = QStringLiteral("invalid");

    if (!payloadBytes.isEmpty())
    {
        const uint8_t byteMode = static_cast<uint8_t>(static_cast<unsigned char>(payloadBytes.at(0)));
        if (byteMode <= 1)
        {
            mode = byteMode;
            parsePath = QStringLiteral("byte");
        }
    }

    if (parsePath == QStringLiteral("invalid") && kAllowJsonFallbackForO4ServerModeQuery &&
        parseJsonModeValue(payloadBytes, 1, mode))
    {
        parsePath = QStringLiteral("json");
    }

    if (parsePath == QStringLiteral("invalid"))
    {
        qDebug().noquote()
            << QStringLiteral("[TcpManager] DataType=224 查询应答无法解析 mode，PayloadHex=%1 PayloadUtf8=%2")
                   .arg(QString::fromLatin1(payloadBytes.toHex(' ').toUpper()))
                   .arg(QString::fromUtf8(payloadBytes));
        return;
    }

    qDebug().noquote()
        << QStringLiteral("[TcpManager] 收到O4服务器模式查询应答 (DataType=224), mode=%1 parsePath=%2 PayloadHex=%3")
               .arg(mode)
               .arg(parsePath)
               .arg(QString::fromLatin1(payloadBytes.toHex(' ').toUpper()));

    emit o4ServerModeQueried(mode);
}

void TcpManager::handleUavCategoryDisplayModeSettingResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    qDebug() << "[TcpManager] 收到无人机类别显示设置应答 (DataType=182), success =" << success
             << " message =" << resultMsg;

    emit uavCategoryDisplayModeSetResponse(success, resultMsg);

    if (success)
    {
        qDebug() << "[TcpManager] 无人机类别显示设置成功，自动发起查询 (DataType=183)...";
        queryUavCategoryDisplayMode();
    }
}

void TcpManager::handleUavCategoryDisplayModeQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 1)
    {
        qDebug() << "[TcpManager] DataType=184 查询应答长度不足，实际:" << payloadLen;
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    uint8_t mode = 0;
    if (!parseModeValue(payloadBytes, 1, mode))
    {
        qDebug() << "[TcpManager] DataType=184 查询应答无法解析 mode，Payload =" << QString::fromUtf8(payloadBytes)
                 << " Hex =" << payloadBytes.toHex(' ').toUpper();
        return;
    }

    qDebug() << "[TcpManager] 收到无人机类别显示查询应答 (DataType=184), mode =" << mode;

    emit uavCategoryDisplayModeQueried(mode);
}

void TcpManager::handleDataEnableSettingResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    qDebug() << "[TcpManager] 收到数据使能设置应答 (DataType=215), success =" << success << " message =" << resultMsg;

    emit dataEnableSetResponse(success, resultMsg);

    if (success)
    {
        qDebug() << "[TcpManager] 数据使能设置成功，自动发起查询 (DataType=216)...";
        queryDataEnable();
    }
}

void TcpManager::handleDataEnableQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 1)
    {
        qDebug() << "[TcpManager] DataType=217 查询应答长度不足，实际:" << payloadLen;
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    uint8_t enabled = 0;
    if (!parseModeValue(payloadBytes, 1, enabled))
    {
        qDebug() << "[TcpManager] DataType=217 查询应答无法解析 enabled，Payload =" << QString::fromUtf8(payloadBytes)
                 << " Hex =" << payloadBytes.toHex(' ').toUpper();
        return;
    }

    qDebug() << "[TcpManager] 收到数据使能查询应答 (DataType=217), enabled =" << enabled;

    emit dataEnableQueried(enabled);
}

void TcpManager::handleFeatureModesSettingResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    qDebug() << "[TcpManager] 收到功能模式设置应答 (DataType=255), success =" << success << " message =" << resultMsg;

    emit featureModesSetResponse(success, resultMsg);

    if (success)
    {
        qDebug() << "[TcpManager] 功能模式设置成功，自动发起查询 (DataType=252)...";
        queryFeatureModes();
    }
}

void TcpManager::handleFeatureModesQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    if (payloadLen < 3)
    {
        qDebug() << "[TcpManager] DataType=253 查询应答长度不足，实际:" << payloadLen;
        return;
    }

    const QByteArray payloadBytes = frameData.mid(sizeof(ProtocolHeader), payloadLen);
    const uint8_t wifiRemoteIdEnabled = static_cast<uint8_t>(static_cast<unsigned char>(payloadBytes.at(0)));
    const uint8_t fpvEnabled = static_cast<uint8_t>(static_cast<unsigned char>(payloadBytes.at(1)));
    const uint8_t djiParseEnabled = static_cast<uint8_t>(static_cast<unsigned char>(payloadBytes.at(2)));
    if (wifiRemoteIdEnabled > 1 || fpvEnabled > 1 || djiParseEnabled > 1)
    {
        qDebug() << "[TcpManager] DataType=253 查询应答存在非法开关值，Payload ="
                 << payloadBytes.toHex(' ').toUpper();
        return;
    }

    qDebug() << "[TcpManager] 收到功能模式查询应答 (DataType=253), flags =" << wifiRemoteIdEnabled << fpvEnabled
             << djiParseEnabled;

    emit featureModesQueried(wifiRemoteIdEnabled, fpvEnabled, djiParseEnabled);
}
