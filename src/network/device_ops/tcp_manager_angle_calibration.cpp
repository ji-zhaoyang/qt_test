#include "tcp_manager.h"
#include <QDebug>

namespace
{
void appendUInt16LE(QByteArray &payload, uint16_t value)
{
    payload.append(static_cast<char>(value & 0xFF));
    payload.append(static_cast<char>((value >> 8) & 0xFF));
}
} // namespace

bool TcpManager::dispatchAngleCalibrationProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 32:
    case 34:
    case 36:
    case 38:
        handleCompassCalibrationResponse(header->dataType, frameData);
        return true;
    default:
        return false;
    }
}

void TcpManager::startCompassCalibration()
{
    qDebug() << "[TcpManager] 准备发送罗盘开始校准命令 (DataType=31)";
    sendFrame(31);
}

void TcpManager::finishCompassCalibration()
{
    qDebug() << "[TcpManager] 准备发送罗盘结束校准命令 (DataType=33)";
    sendFrame(33);
}

void TcpManager::confirmCompassCalibration(uint16_t angle)
{
    QByteArray payload;
    payload.reserve(2);
    // 协议字段长度为 2 字节，这里按现有项目其他数值字段约定使用小端编码。
    appendUInt16LE(payload, angle);
    qDebug() << "[TcpManager] 准备发送罗盘确认校准命令 (DataType=35), angle =" << angle;
    sendFrame(35, payload);
}

void TcpManager::cancelCompassCalibration()
{
    qDebug() << "[TcpManager] 准备发送罗盘退出校准命令 (DataType=37)";
    sendFrame(37);
}

void TcpManager::handleCompassCalibrationResponse(uint16_t responseDataType, const QByteArray &frameData)
{
    const int msgLen = frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    const QString resultMsg =
        msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
    const bool success = resultMsg.contains("RESULT:SUCCESSED");

    qDebug() << "[TcpManager] 收到罗盘校准应答, dataType =" << responseDataType << "success =" << success
             << "message =" << resultMsg;

    emit compassCalibrationResponse(responseDataType, success, resultMsg);
}
