#include "tcp_manager.h"

namespace
{
QString parseResultMessage(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
}
} // namespace

bool TcpManager::dispatchSystemProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 17:
    {
        const QString resultMsg = parseResultMessage(frameData);
        const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
        emit systemTimeSetResponse(success, resultMsg);
        return true;
    }
    case 30:
    {
        const QString resultMsg = parseResultMessage(frameData);
        const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));
        emit deviceRebootResponse(success, resultMsg);
        return true;
    }
    default:
        return false;
    }
}

void TcpManager::setSystemTime()
{
    setSystemTime(QDateTime::currentDateTime());
}

void TcpManager::rebootDevice()
{
    sendFrame(29);
}
