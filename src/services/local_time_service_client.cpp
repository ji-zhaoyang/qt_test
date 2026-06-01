#include "local_time_service_client.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>

namespace
{
constexpr int kConnectTimeoutMs = 3000;
constexpr int kIoTimeoutMs = 10000;

QString parseResponseMessage(const QByteArray &responseBytes, bool *ok)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        if (ok)
        {
            *ok = false;
        }
        return QStringLiteral("本地时间服务返回了无法解析的响应");
    }

    const QJsonObject obj = doc.object();
    if (ok)
    {
        *ok = obj.value(QStringLiteral("success")).toBool(false);
    }
    return obj.value(QStringLiteral("message")).toString();
}
} // namespace

LocalTimeServiceClient::LocalTimeServiceClient(QObject *parent) : QObject(parent)
{
}

QString LocalTimeServiceClient::serverName()
{
#ifdef Q_OS_WIN
    return QStringLiteral("qt_time_helper");
#else
    return QStringLiteral("/tmp/qt_time_helper.sock");
#endif
}

LocalTimeServiceResult LocalTimeServiceClient::checkAvailability() const
{
    QJsonObject request;
    request.insert(QStringLiteral("action"), QStringLiteral("ping"));
    LocalTimeServiceResult result = sendRequest(request);
    if (!result.success && result.message.isEmpty())
    {
        result.message = QStringLiteral("qt_time_helper 未启动");
    }
    return result;
}

LocalTimeServiceResult LocalTimeServiceClient::setSystemTime(const QDateTime &dateTime, const QString &timezoneId) const
{
    QJsonObject request;
    request.insert(QStringLiteral("action"), QStringLiteral("set_system_time"));
    request.insert(QStringLiteral("datetime"), dateTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
    request.insert(QStringLiteral("timezone"), timezoneId.trimmed());
    return sendRequest(request);
}

LocalTimeServiceResult LocalTimeServiceClient::sendRequest(const QJsonObject &request) const
{
    LocalTimeServiceResult result;

    QLocalSocket socket;
    socket.connectToServer(serverName());
    if (!socket.waitForConnected(kConnectTimeoutMs))
    {
        switch (socket.error())
        {
        case QLocalSocket::ServerNotFoundError:
            result.message = QStringLiteral("qt_time_helper 未启动");
            break;
        case QLocalSocket::SocketAccessError:
            result.message = QStringLiteral("无权限访问 qt_time_helper socket");
            break;
        default:
            result.message = QStringLiteral("连接 qt_time_helper 失败：%1").arg(socket.errorString());
            break;
        }
        return result;
    }

    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    if (socket.write(payload) != payload.size() || !socket.waitForBytesWritten(kIoTimeoutMs))
    {
        result.message = QStringLiteral("发送本地时间服务请求失败：%1").arg(socket.errorString());
        return result;
    }

    if (!socket.waitForReadyRead(kIoTimeoutMs))
    {
        result.message = QStringLiteral("等待本地时间服务响应超时");
        return result;
    }

    QByteArray responseBytes = socket.readAll();
    while (socket.waitForReadyRead(100))
    {
        responseBytes += socket.readAll();
    }

    const int newlineIndex = responseBytes.indexOf('\n');
    if (newlineIndex >= 0)
    {
        responseBytes = responseBytes.left(newlineIndex);
    }
    responseBytes = responseBytes.trimmed();

    bool success = false;
    const QString message = parseResponseMessage(responseBytes, &success);
    result.success = success;
    result.message = message.isEmpty() ? (success ? QStringLiteral("本机时间设置成功") : QStringLiteral("本机时间设置失败"))
                                       : message;
    return result;
}
