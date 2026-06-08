#include "tcp_manager.h"
#include <QJsonObject>
#include <QDebug>

namespace
{
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

QString parseResultMessage(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
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
} // namespace

bool TcpManager::dispatchDataCollectionProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 19:
        handlePatternUploadResponse(frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::uploadPatternFile(const PatternUploadRequest &request)
{
    // #region debug-point B:upload-pattern-request
    qDebug().noquote()
        << QStringLiteral("[DEBUG-B] tcp_manager_data_collection.cpp:uploadPatternFile | "
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
    // #endregion

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

    // #region debug-point B:upload-pattern-json
    qDebug().noquote()
        << QStringLiteral("[DEBUG-B] tcp_manager_data_collection.cpp:uploadPatternFile | DataType=18 JSON=%1").arg(payload);
    // #endregion

    sendFrame(18, payload.toUtf8());
}

void TcpManager::handlePatternUploadResponse(const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"), Qt::CaseInsensitive);
    qDebug().noquote()
        << QStringLiteral("[DEBUG-19] tcp_manager_data_collection.cpp:handlePatternUploadResponse | rawResponse=%1")
               .arg(resultMsg);
    qDebug().noquote()
        << QStringLiteral("[DEBUG-19] tcp_manager_data_collection.cpp:handlePatternUploadResponse | success=%1 display=%2")
               .arg(success ? QStringLiteral("true") : QStringLiteral("false"), buildPatternUploadDisplayMessage(resultMsg));
    emit patternUploadResponse(success, buildPatternUploadDisplayMessage(resultMsg));
}
