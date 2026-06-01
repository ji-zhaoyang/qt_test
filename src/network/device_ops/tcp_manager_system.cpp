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

// 当前系统协议分发仅保留设备重启相关逻辑。
// Linux 板子本机时间设置已改由 AppController 直接通过系统命令处理，
// 不再经过 TcpManager 的设备协议链路。
bool TcpManager::dispatchSystemProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
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

void TcpManager::rebootDevice()
{
    sendFrame(29);
}
