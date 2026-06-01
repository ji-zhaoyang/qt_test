#ifndef DEVICE_STATUS_H
#define DEVICE_STATUS_H

#include <QString>

enum class DeviceConnectionState
{
    Disconnected,
    Connecting,
    Connected,
    Error
};

struct DeviceStatus
{
    DeviceConnectionState connectionState = DeviceConnectionState::Disconnected;
    QString lastError;

    bool isConnected() const
    {
        return connectionState == DeviceConnectionState::Connected;
    }
};

#endif // DEVICE_STATUS_H
