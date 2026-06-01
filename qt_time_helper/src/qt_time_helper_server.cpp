#include "qt_time_helper_server.h"
#include <QDateTime>
#include <QFile>
#include <QFileDevice>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QTimeZone>

#ifndef Q_OS_WIN
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace
{
constexpr int kStartTimeoutMs = 3000;
constexpr int kFinishTimeoutMs = 10000;

QString serverName()
{
#ifdef Q_OS_WIN
    return QStringLiteral("qt_time_helper");
#else
    return QStringLiteral("/tmp/qt_time_helper.sock");
#endif
}

bool isAllowedProgramName(const QString &programName)
{
    static const QStringList allowedProgramNames = {
        QStringLiteral("qt_test"),
        QStringLiteral("qt_testApp"),
    };
    return allowedProgramNames.contains(programName);
}

bool isAllowedTimezone(const QString &timezoneId)
{
    if (timezoneId.isEmpty())
    {
        return true;
    }

    return QTimeZone::availableTimeZoneIds().contains(timezoneId.toUtf8());
}

bool runSystemCommand(const QString &program, const QStringList &arguments, QString *errorMessage)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForStarted(kStartTimeoutMs))
    {
        if (errorMessage)
        {
            *errorMessage = process.errorString();
        }
        return false;
    }

    if (!process.waitForFinished(kFinishTimeoutMs))
    {
        process.kill();
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("命令执行超时");
        }
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        QString output = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        if (output.isEmpty())
        {
            output = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
        }
        if (output.isEmpty())
        {
            output = QStringLiteral("%1 执行失败").arg(program);
        }
        if (errorMessage)
        {
            *errorMessage = output;
        }
        return false;
    }

    return true;
}
} // namespace

QtTimeHelperServer::QtTimeHelperServer(QObject *parent) : QObject(parent), server(new QLocalServer(this))
{
    connect(server, &QLocalServer::newConnection, this, &QtTimeHelperServer::onNewConnection);
}

bool QtTimeHelperServer::start(QString *errorMessage)
{
#ifndef Q_OS_WIN
    QFile::remove(serverName());
#endif

    if (!server->listen(serverName()))
    {
        if (errorMessage)
        {
            *errorMessage = server->errorString();
        }
        return false;
    }

#ifndef Q_OS_WIN
    QFile::setPermissions(serverName(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner |
                                            QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ExeGroup |
                                            QFileDevice::ReadOther | QFileDevice::WriteOther | QFileDevice::ExeOther);
#endif
    return true;
}

bool QtTimeHelperServer::isPeerAllowed(QLocalSocket *socket, QString *errorMessage) const
{
    if (!socket)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("连接无效");
        }
        return false;
    }

#ifdef Q_OS_WIN
    Q_UNUSED(errorMessage);
    return true;
#else
    struct ucred peerCredentials
    {
    };
    socklen_t credSize = sizeof(peerCredentials);
    const int fd = socket->socketDescriptor();
    if (fd < 0 || getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &peerCredentials, &credSize) != 0)
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("无法识别调用方身份");
        }
        return false;
    }

    const QString executablePath = QFile::symLinkTarget(QStringLiteral("/proc/%1/exe").arg(peerCredentials.pid));
    const QString programName = executablePath.section('/', -1);
    if (!isAllowedProgramName(programName))
    {
        if (errorMessage)
        {
            *errorMessage = QStringLiteral("当前调用方未被允许：%1").arg(programName.isEmpty() ? QStringLiteral("unknown")
                                                                                                : programName);
        }
        return false;
    }

    return true;
#endif
}

void QtTimeHelperServer::onNewConnection()
{
    while (QLocalSocket *socket = server->nextPendingConnection())
    {
        connect(socket, &QLocalSocket::readyRead, this, &QtTimeHelperServer::onSocketReadyRead);
        connect(socket, &QLocalSocket::disconnected, this, &QtTimeHelperServer::onSocketDisconnected);
    }
}

void QtTimeHelperServer::onSocketReadyRead()
{
    QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender());
    if (!socket)
    {
        return;
    }

    QByteArray requestBytes = socket->readAll();
    while (socket->waitForReadyRead(10))
    {
        requestBytes += socket->readAll();
    }

    const int newlineIndex = requestBytes.indexOf('\n');
    if (newlineIndex >= 0)
    {
        requestBytes = requestBytes.left(newlineIndex);
    }
    requestBytes = requestBytes.trimmed();

    if (!processRequest(socket, requestBytes))
    {
        socket->disconnectFromServer();
    }
}

void QtTimeHelperServer::onSocketDisconnected()
{
    if (QLocalSocket *socket = qobject_cast<QLocalSocket *>(sender()))
    {
        socket->deleteLater();
    }
}

void QtTimeHelperServer::writeResponse(QLocalSocket *socket, bool success, const QString &message)
{
    if (!socket)
    {
        return;
    }

    QJsonObject response;
    response.insert(QStringLiteral("success"), success);
    response.insert(QStringLiteral("message"), message);
    const QByteArray payload = QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n';
    socket->write(payload);
    socket->flush();
    socket->waitForBytesWritten(kStartTimeoutMs);
}

bool QtTimeHelperServer::processRequest(QLocalSocket *socket, const QByteArray &requestBytes)
{
    QString peerError;
    if (!isPeerAllowed(socket, &peerError))
    {
        writeResponse(socket, false, peerError);
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(requestBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        writeResponse(socket, false, QStringLiteral("请求格式错误"));
        return false;
    }

    const QJsonObject request = doc.object();
    const QString action = request.value(QStringLiteral("action")).toString().trimmed();
    if (action == QStringLiteral("ping"))
    {
        return handlePing(socket);
    }

    if (action != QStringLiteral("set_system_time"))
    {
        writeResponse(socket, false, QStringLiteral("不支持的操作"));
        return false;
    }

    const QString dateTimeText = request.value(QStringLiteral("datetime")).toString().trimmed();
    const QString timezoneId = request.value(QStringLiteral("timezone")).toString().trimmed();
    return handleSetSystemTime(socket, dateTimeText, timezoneId);
}

bool QtTimeHelperServer::handlePing(QLocalSocket *socket)
{
    writeResponse(socket, true, QStringLiteral("qt_time_helper 在线"));
    return false;
}

bool QtTimeHelperServer::handleSetSystemTime(QLocalSocket *socket, const QString &dateTimeText, const QString &timezoneId)
{
    const QDateTime dateTime = QDateTime::fromString(dateTimeText, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    if (!dateTime.isValid())
    {
        writeResponse(socket, false, QStringLiteral("时间格式不正确"));
        return false;
    }

    if (dateTime.date().year() < 2020 || dateTime.date().year() > 2100)
    {
        writeResponse(socket, false, QStringLiteral("时间超出允许范围"));
        return false;
    }

    if (!isAllowedTimezone(timezoneId))
    {
        writeResponse(socket, false, QStringLiteral("时区不在白名单中"));
        return false;
    }

    QString errorMessage;
    if (!timezoneId.isEmpty() &&
        !runSystemCommand(QStringLiteral("timedatectl"), QStringList() << QStringLiteral("set-timezone") << timezoneId,
                          &errorMessage))
    {
        writeResponse(socket, false, QStringLiteral("设置时区失败：%1").arg(errorMessage));
        return false;
    }

    if (!runSystemCommand(QStringLiteral("timedatectl"),
                          QStringList() << QStringLiteral("set-time")
                                        << dateTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                          &errorMessage) &&
        !runSystemCommand(QStringLiteral("date"),
                          QStringList() << QStringLiteral("-s")
                                        << dateTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                          &errorMessage))
    {
        writeResponse(socket, false, QStringLiteral("设置时间失败：%1").arg(errorMessage));
        return false;
    }

    runSystemCommand(QStringLiteral("hwclock"), QStringList() << QStringLiteral("-w"), nullptr);
    writeResponse(socket, true, QStringLiteral("本机时间设置成功"));
    return false;
}
