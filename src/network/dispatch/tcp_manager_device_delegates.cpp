#include "tcp_manager.h"
#include "services/calibration_service.h"
#include "services/device_ops_service.h"
#include "services/drone_ops_service.h"

void TcpManager::setDronePrecisionStrike(bool enabled, quint32 timestamp, const QString &sn, int type, quint32 targetId)
{
    if (!droneOpsService)
    {
        return;
    }

    droneOpsService->setDronePrecisionStrike(enabled, timestamp, sn, type, targetId);
}

void TcpManager::setDroneWideBandJamming(bool enabled, quint32 frequencyKhz, const QString &sn, quint32 targetId)
{
    if (!droneOpsService)
    {
        return;
    }

    droneOpsService->setDroneWideBandJamming(enabled, frequencyKhz, sn, targetId);
}

bool TcpManager::dispatchDeviceOpsProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 101:
        if (deviceOpsService)
        {
            deviceOpsService->handleDeviceJammingModeSetResponse(frameData);
        }
        return true;
    case 103:
        if (deviceOpsService)
        {
            deviceOpsService->handleDeviceJammingStatusQueryResponse(frameData);
        }
        return true;
    case 104:
        if (deviceOpsService)
        {
            deviceOpsService->handleDeviceJammingModeReported(frameData);
        }
        return true;
    case 110:
        if (droneOpsService)
        {
            droneOpsService->handleDronePrecisionStrikeResponse(frameData);
        }
        return true;
    case 115:
        if (droneOpsService)
        {
            droneOpsService->handleDroneWideBandJammingResponse(frameData);
        }
        return true;
    case 93:
        if (deviceOpsService)
        {
            deviceOpsService->handleBuzzerEnabledSetResponse(frameData);
        }
        return true;
    case 95:
        if (deviceOpsService)
        {
            deviceOpsService->handleBuzzerEnabledQueryResponse(frameData);
        }
        return true;
    case 117:
        if (deviceOpsService)
        {
            deviceOpsService->handleAlarmHistoryQueryResponse(frameData);
        }
        return true;
    case 137:
        if (deviceOpsService)
        {
            deviceOpsService->handleDeviceUsageInfoQueryResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::setDeviceJammingMode(int mode, int switchStatus)
{
    if (!deviceOpsService)
    {
        return;
    }

    deviceOpsService->setDeviceJammingMode(mode, switchStatus);
}

void TcpManager::queryDeviceJammingMode()
{
    if (!deviceOpsService)
    {
        return;
    }

    deviceOpsService->queryDeviceJammingMode();
}

void TcpManager::queryDeviceAlarmHistory()
{
    if (!deviceOpsService)
    {
        return;
    }

    deviceOpsService->queryDeviceAlarmHistory();
}

void TcpManager::queryDeviceUsageInfo()
{
    if (!deviceOpsService)
    {
        return;
    }

    deviceOpsService->queryDeviceUsageInfo();
}

void TcpManager::setBuzzerEnabled(uint8_t enabled)
{
    if (!deviceOpsService)
    {
        return;
    }

    deviceOpsService->setBuzzerEnabled(enabled);
}

void TcpManager::queryBuzzerEnabled()
{
    if (!deviceOpsService)
    {
        return;
    }

    deviceOpsService->queryBuzzerEnabled();
}

// 当前系统协议分发仅保留设备重启相关逻辑。
// Linux 板子本机时间设置已改由 AppController 直接通过系统命令处理，
// 不再经过 TcpManager 的设备协议链路。
bool TcpManager::dispatchSystemProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 30:
        if (deviceOpsService)
        {
            deviceOpsService->handleDeviceRebootResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::rebootDevice()
{
    if (!deviceOpsService)
    {
        return;
    }

    deviceOpsService->rebootDevice();
}

bool TcpManager::dispatchAngleCalibrationProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 32:
    case 34:
    case 36:
    case 38:
        if (calibrationService)
        {
            calibrationService->handleCompassCalibrationResponse(header->dataType, frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::startCompassCalibration()
{
    if (!calibrationService)
    {
        return;
    }

    calibrationService->startCompassCalibration();
}

void TcpManager::finishCompassCalibration()
{
    if (!calibrationService)
    {
        return;
    }

    calibrationService->finishCompassCalibration();
}

void TcpManager::confirmCompassCalibration(uint16_t angle)
{
    if (!calibrationService)
    {
        return;
    }

    calibrationService->confirmCompassCalibration(angle);
}

void TcpManager::cancelCompassCalibration()
{
    if (!calibrationService)
    {
        return;
    }

    calibrationService->cancelCompassCalibration();
}
