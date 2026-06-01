#include "tcp_manager.h"
#include <QDebug>

namespace
{
constexpr int kFirmwareVersionRecordCount = 3;
constexpr int kFirmwareVersionRecordSize = 15;

QString readFirmwareVersionString(const char *data, int maxLen)
{
    int len = 0;
    while (len < maxLen && data[len] != '\0')
    {
        ++len;
    }
    return QString::fromUtf8(data, len).trimmed();
}
} // namespace

bool TcpManager::dispatchFirmwareProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 15:
        handleFirmwareVersionQueryResponse(header, frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::queryFirmwareVersions()
{
    qDebug() << "[TcpManager] 准备发送固件版本查询 (DataType=14)...";
    sendFrame(14);
}

void TcpManager::handleFirmwareVersionQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    Q_UNUSED(header);

    const int payloadLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const int expectedLen = kFirmwareVersionRecordCount * kFirmwareVersionRecordSize;
    if (payloadLen < expectedLen)
    {
        qDebug() << "[TcpManager] DataType=15 版本查询应答长度不足，实际:" << payloadLen << "预期至少:" << expectedLen;
        return;
    }

    const char *payload = frameData.constData() + sizeof(ProtocolHeader);
    const QString appVersion = readFirmwareVersionString(payload + 1, 14);
    const QString fpgaVersion = readFirmwareVersionString(payload + 16, 14);
    const QString gpuVersion = readFirmwareVersionString(payload + 31, 14);

    qDebug() << "[TcpManager] 收到固件版本查询应答 (DataType=15)";
    qDebug() << "[TcpManager] 固件版本解析:"
             << "App =" << appVersion
             << ", Fpga =" << fpgaVersion
             << ", Gpu =" << gpuVersion;

    emit firmwareVersionsQueried(appVersion, fpgaVersion, gpuVersion);
}
