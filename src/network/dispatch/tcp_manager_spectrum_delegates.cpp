#include "tcp_manager.h"
#include "services/spectrum_service.h"

bool TcpManager::dispatchSpectrumSwitchProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 66:
    case 68:
        if (spectrumService)
        {
            spectrumService->handleSpectrogramSwitchResponse(header->dataType, frameData);
        }
        return true;
    case 69:
        if (spectrumService)
        {
            spectrumService->handleSpectrumDataReport(header, frameData);
        }
        return true;
    case 219:
        if (spectrumService)
        {
            spectrumService->handleFullSpectrumSwitchResponse(frameData);
        }
        return true;
    case 220:
        if (spectrumService)
        {
            spectrumService->handleFullSpectrumReport(header, frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::openSpectrogram()
{
    if (spectrumService)
    {
        spectrumService->openSpectrogram();
    }
}

void TcpManager::closeSpectrogram()
{
    if (spectrumService)
    {
        spectrumService->closeSpectrogram();
    }
}

void TcpManager::openFullSpectrum()
{
    if (spectrumService)
    {
        spectrumService->openFullSpectrum();
    }
}

void TcpManager::closeFullSpectrum()
{
    if (spectrumService)
    {
        spectrumService->closeFullSpectrum();
    }
}
