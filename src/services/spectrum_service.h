#ifndef SPECTRUM_SERVICE_H
#define SPECTRUM_SERVICE_H

#include "network/core/protocol_types.h"

#include <QObject>
#include <functional>

class SpectrumService : public QObject
{
    Q_OBJECT

public:
    using FrameSender = std::function<void(uint16_t, const QByteArray &)>;

    explicit SpectrumService(QObject *parent = nullptr);

    void setFrameSender(const FrameSender &sender);

    void openSpectrogram();
    void closeSpectrogram();
    void openFullSpectrum();
    void closeFullSpectrum();

    void handleSpectrogramSwitchResponse(uint16_t responseDataType, const QByteArray &frameData);
    void handleSpectrumDataReport(const ProtocolHeader *header, const QByteArray &frameData);
    void handleFullSpectrumSwitchResponse(const QByteArray &frameData);
    void handleFullSpectrumReport(const ProtocolHeader *header, const QByteArray &frameData);

signals:
    void spectrogramSwitchResponse(uint16_t responseDataType, bool success, const QString &msg);
    void spectrumDataReported(const SpectrumReportData &reportData);
    void fullSpectrumSwitchResponse(bool enabled, bool success, const QString &msg);
    void fullSpectrumReported(const FullSpectrumReportData &reportData);

private:
    FrameSender sendFrame_;
    bool pendingFullSpectrumEnabled_ = false;
};

#endif // SPECTRUM_SERVICE_H
