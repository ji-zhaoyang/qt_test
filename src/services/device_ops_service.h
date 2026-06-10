#ifndef DEVICE_OPS_SERVICE_H
#define DEVICE_OPS_SERVICE_H

#include "network/core/protocol_types.h"

#include <QObject>
#include <functional>

class DeviceOpsService : public QObject
{
    Q_OBJECT

public:
    using FrameSender = std::function<void(uint16_t, const QByteArray &)>;

    explicit DeviceOpsService(QObject *parent = nullptr);

    void setFrameSender(const FrameSender &sender);

    void rebootDevice();
    void setDeviceJammingMode(int mode, int switchStatus);
    void queryDeviceJammingMode();
    void queryDeviceAlarmHistory();
    void queryDeviceUsageInfo();
    void setBuzzerEnabled(uint8_t enabled);
    void queryBuzzerEnabled();

    void handleDeviceJammingModeSetResponse(const QByteArray &frameData);
    void handleDeviceJammingStatusQueryResponse(const QByteArray &frameData);
    void handleDeviceJammingModeReported(const QByteArray &frameData);
    void handleBuzzerEnabledSetResponse(const QByteArray &frameData);
    void handleBuzzerEnabledQueryResponse(const QByteArray &frameData);
    void handleAlarmHistoryQueryResponse(const QByteArray &frameData);
    void handleDeviceUsageInfoQueryResponse(const QByteArray &frameData);
    void handleDeviceRebootResponse(const QByteArray &frameData);

signals:
    void deviceJammingModeSetResponse(int mode, int switchStatus, bool success, const QString &msg);
    void deviceJammingModeReported(int mode, int switchStatus);
    void deviceJammingStatusQueried(const QVector<int> &switchStates);
    void alarmHistoryQueried(const AlarmHistoryInfo &info);
    void deviceUsageInfoQueried(const DeviceUsageInfo &info);
    void buzzerEnabledSetResponse(bool success, const QString &msg);
    void buzzerEnabledQueried(uint8_t enabled);
    void deviceRebootResponse(bool success, const QString &msg);

private:
    FrameSender sendFrame_;
    int pendingDeviceJammingMode_ = -1;
    int pendingDeviceJammingSwitchStatus_ = 0;
};

#endif // DEVICE_OPS_SERVICE_H
