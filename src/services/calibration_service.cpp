#include "calibration_service.h"

#include "network/core/protocol_types.h"

#include <QDebug>

namespace
{
QString parseResultMessage(const QByteArray &frameData)
{
    const int msgLen =
        frameData.size() - static_cast<int>(sizeof(ProtocolHeader)) - static_cast<int>(sizeof(ProtocolTail));
    return msgLen > 0 ? QString::fromUtf8(frameData.constData() + sizeof(ProtocolHeader), msgLen).trimmed() : QString();
}

void appendUInt16LE(QByteArray &payload, uint16_t value)
{
    payload.append(static_cast<char>(value & 0xFF));
    payload.append(static_cast<char>((value >> 8) & 0xFF));
}
} // namespace

CalibrationService::CalibrationService(QObject *parent)
    : QObject(parent)
{
}

void CalibrationService::setFrameSender(const FrameSender &sender)
{
    sendFrame_ = sender;
}

void CalibrationService::startCompassCalibration()
{
    if (sendFrame_)
    {
        qDebug() << "[CalibrationService] 准备发送罗盘开始校准命令 (DataType=31)";
        sendFrame_(31, QByteArray());
    }
}

void CalibrationService::finishCompassCalibration()
{
    if (sendFrame_)
    {
        qDebug() << "[CalibrationService] 准备发送罗盘结束校准命令 (DataType=33)";
        sendFrame_(33, QByteArray());
    }
}

void CalibrationService::confirmCompassCalibration(uint16_t angle)
{
    if (!sendFrame_)
    {
        return;
    }

    QByteArray payload;
    payload.reserve(2);
    appendUInt16LE(payload, angle);
    qDebug() << "[CalibrationService] 准备发送罗盘确认校准命令 (DataType=35), angle =" << angle;
    sendFrame_(35, payload);
}

void CalibrationService::cancelCompassCalibration()
{
    if (sendFrame_)
    {
        qDebug() << "[CalibrationService] 准备发送罗盘退出校准命令 (DataType=37)";
        sendFrame_(37, QByteArray());
    }
}

void CalibrationService::handleCompassCalibrationResponse(uint16_t responseDataType, const QByteArray &frameData)
{
    const QString resultMsg = parseResultMessage(frameData);
    const bool success = resultMsg.contains(QStringLiteral("RESULT:SUCCESSED"));

    qDebug() << "[CalibrationService] 收到罗盘校准应答, dataType =" << responseDataType << "success =" << success
             << "message =" << resultMsg;

    emit compassCalibrationResponse(responseDataType, success, resultMsg);
}
