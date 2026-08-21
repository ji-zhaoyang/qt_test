#ifndef DRONE_OPS_SERVICE_H
#define DRONE_OPS_SERVICE_H

#include "network/core/protocol_types.h"

#include <QJsonObject>
#include <QObject>
#include <QSize>
#include <functional>

class DroneOpsService : public QObject
{
    Q_OBJECT

public:
    using FrameSender = std::function<void(uint16_t, const QByteArray &)>;

    explicit DroneOpsService(QObject *parent = nullptr);

    void setFrameSender(const FrameSender &sender);

    void setDroneDirectionFinding(bool enabled, quint32 targetId);
    void setDronePrecisionStrike(bool enabled, quint32 timestamp, const QString &sn, int type, quint32 targetId);
    void setDroneWideBandJamming(bool enabled, quint32 frequencyKhz, const QString &sn, quint32 targetId);
    void setDroneVideoTakeover(bool enabled, quint32 frequencyKhz, quint32 targetId);

    void handleDroneTargetReport(const ProtocolHeader *header, const QByteArray &frameData);
    void handleDroneDirectionFindingResponse(const QByteArray &frameData);
    void handleDroneDirectionPowerReport(const ProtocolHeader *header, const QByteArray &frameData);
    void handleDronePrecisionStrikeResponse(const QByteArray &frameData);
    void handleDroneWideBandJammingResponse(const QByteArray &frameData);
    void handleDroneVideoTakeoverResponse(const QByteArray &frameData);
    void handleDroneVideoImageReport(const QByteArray &frameData);

signals:
    void droneTargetReported(const QJsonObject &targetInfo);
    void droneDirectionFindingResponse(quint32 targetId, bool enabled, bool success, const QString &msg);
    void droneDirectionPowerReported(const QJsonObject &reportData);
    void dronePrecisionStrikeResponse(quint32 targetId, bool enabled, bool success, const QString &msg);
    void droneWideBandJammingResponse(quint32 targetId, bool enabled, bool success, const QString &msg);
    void droneVideoTakeoverResponse(quint32 targetId, bool enabled, bool success, const QString &msg);
    void droneVideoImageReported(const QByteArray &jpegPayload, const QSize &frameSize);

private:
    FrameSender sendFrame_;

    bool pendingDroneDirectionFindingEnabled_ = false;
    quint32 pendingDroneDirectionFindingTargetId_ = 0;
    bool activeDroneDirectionFindingEnabled_ = false;
    quint32 activeDroneDirectionFindingTargetId_ = 0;

    bool pendingDronePrecisionStrikeEnabled_ = false;
    quint32 pendingDronePrecisionStrikeTimestamp_ = 0;
    QString pendingDronePrecisionStrikeSn_;
    int pendingDronePrecisionStrikeType_ = 0;
    quint32 pendingDronePrecisionStrikeTargetId_ = 0;

    bool pendingDroneWideBandJammingEnabled_ = false;
    quint32 pendingDroneWideBandJammingFrequencyKhz_ = 0;
    QString pendingDroneWideBandJammingSn_;
    quint32 pendingDroneWideBandJammingTargetId_ = 0;

    bool pendingDroneVideoTakeoverEnabled_ = false;
    quint32 pendingDroneVideoTakeoverTargetId_ = 0;
};

#endif // DRONE_OPS_SERVICE_H
