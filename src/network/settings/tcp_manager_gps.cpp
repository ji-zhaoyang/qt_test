#include "tcp_manager.h"
#include <QDebug>
#include <cstring>

bool TcpManager::dispatchGpsProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 58:
        handleGpsSettingResponse(header, frameData);
        return true;
    case 60:
        handleGpsQueryResponse(header, frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::setDeviceGps(uint8_t mode, float lng, float lat, float alt)
{
    qDebug() << "[TcpManager] 组装 GPS 设置数据载荷 (DataType=57)...";
    GpsSettingPayload payload;
    std::memset(&payload, 0, sizeof(payload));
    payload.mode = mode;
    payload.longitude = lng;
    payload.latitude = lat;
    payload.altitude = alt;

    QByteArray data(reinterpret_cast<const char *>(&payload), sizeof(payload));
    qDebug() << "[TcpManager] GPS 设置数据载荷组装完毕，长度:" << data.size() << "字节";
    sendFrame(57, data);
}

void TcpManager::queryDeviceGps()
{
    qDebug() << "[TcpManager] 准备发送 GPS 查询命令 (DataType=59)...";
    sendFrame(59); // 数据部分为空
}

void TcpManager::handleGpsSettingResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    uint32_t frameLen = header->length;
    // 安全处理可能没有\0结尾的字符串
    int msgLen = frameLen - sizeof(ProtocolHeader) - sizeof(ProtocolTail);
    QString resultMsg;
    if (msgLen > 0)
    {
        resultMsg = QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen);
    }
    qDebug() << "[TcpManager] 收到 GPS 经纬度设置应答 (DataType=58), 内容:" << resultMsg;
    bool success = resultMsg.contains("RESULT:SUCCESSED");
    emit deviceGpsSetResponse(success, resultMsg);

    // 严格按照协议流程：下发成功之后，立即自动发起查询操作 (DataType=59)
    if (success)
    {
        qDebug() << "[TcpManager] GPS 设置成功，严格遵守协议，立即发起查询 (DataType=59)...";
        queryDeviceGps();
    }
}

void TcpManager::handleGpsQueryResponse(const ProtocolHeader *header, const QByteArray &frameData)
{
    uint32_t frameLen = header->length;
    qDebug() << "==================================================";
    qDebug() << "[TcpManager] 收到 GPS 经纬度查询应答 (DataType=60)";

    int payloadLen = frameLen - sizeof(ProtocolHeader) - sizeof(ProtocolTail);

    if (static_cast<size_t>(payloadLen) >= sizeof(GpsSettingPayload))
    {
        const GpsSettingPayload *payload =
            reinterpret_cast<const GpsSettingPayload *>(frameData.constData() + sizeof(ProtocolHeader));

        qDebug() << "     解析结果 -> 模式:" << payload->mode << " 经度:" << payload->longitude
                 << " 纬度:" << payload->latitude << " 海拔:" << payload->altitude;

        if (payload->latitude >= -90.0f && payload->latitude <= 90.0f && payload->longitude >= -180.0f &&
            payload->longitude <= 180.0f)
        {
            emit deviceGpsQueried(payload->mode, payload->longitude, payload->latitude, payload->altitude);
        }
        else
        {
            qDebug() << "  -> 拦截: DataType=60 回传了超范围坐标数据，经度:" << payload->longitude
                     << "纬度:" << payload->latitude;
        }
    }
    else
    {
        qDebug() << "  -> 错误: Payload 长度过短(" << payloadLen << "字节)，无法作为 GpsSettingPayload 解析！";
    }
    qDebug() << "==================================================";
}
