#ifndef CALIBRATION_SERVICE_H
#define CALIBRATION_SERVICE_H

#include <QObject>
#include <functional>

class CalibrationService : public QObject
{
    Q_OBJECT

public:
    using FrameSender = std::function<void(uint16_t, const QByteArray &)>;

    explicit CalibrationService(QObject *parent = nullptr);

    void setFrameSender(const FrameSender &sender);

    void startCompassCalibration();
    void finishCompassCalibration();
    void confirmCompassCalibration(uint16_t angle);
    void cancelCompassCalibration();

    void handleCompassCalibrationResponse(uint16_t responseDataType, const QByteArray &frameData);

signals:
    void compassCalibrationResponse(uint16_t responseDataType, bool success, const QString &msg);

private:
    FrameSender sendFrame_;
};

#endif // CALIBRATION_SERVICE_H
