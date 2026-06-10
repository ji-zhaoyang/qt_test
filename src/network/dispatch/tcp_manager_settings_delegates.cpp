#include "tcp_manager.h"
#include "services/settings_protocol_service.h"

bool TcpManager::dispatchGpsProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 58:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleGpsSettingResponse(frameData);
        }
        return true;
    case 60:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleGpsQueryResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::setDeviceGps(uint8_t mode, float lng, float lat, float alt)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setDeviceGps(mode, lng, lat, alt);
}

void TcpManager::queryDeviceGps()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryDeviceGps();
}

bool TcpManager::dispatchDetectBandProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 9:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleDetectBandSettingResponse(frameData);
        }
        return true;
    case 11:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleDetectBandQueryResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::setDetectBands(const QVector<DetectBandParam> &bands)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setDetectBands(bands);
}

void TcpManager::queryDetectBands()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryDetectBands();
}

bool TcpManager::dispatchModeSelectProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 62:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleDroneReportModeSettingResponse(frameData);
        }
        return true;
    case 64:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleDroneReportModeQueryResponse(frameData);
        }
        return true;
    case 131:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleSuppressionModeSettingResponse(frameData);
        }
        return true;
    case 133:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleSuppressionModeQueryResponse(frameData);
        }
        return true;
    case 182:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleUavCategoryDisplayModeSettingResponse(frameData);
        }
        return true;
    case 184:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleUavCategoryDisplayModeQueryResponse(frameData);
        }
        return true;
    case 215:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleDataEnableSettingResponse(frameData);
        }
        return true;
    case 217:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleDataEnableQueryResponse(frameData);
        }
        return true;
    case 222:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleO4ServerModeSettingResponse(frameData);
        }
        return true;
    case 224:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleO4ServerModeQueryResponse(frameData);
        }
        return true;
    case 253:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleFeatureModesQueryResponse(frameData);
        }
        return true;
    case 255:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleFeatureModesSettingResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::setDroneReportMode(uint8_t mode)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setDroneReportMode(mode);
}

void TcpManager::queryDroneReportMode()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryDroneReportMode();
}

void TcpManager::setSuppressionMode(uint8_t mode)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setSuppressionMode(mode);
}

void TcpManager::querySuppressionMode()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->querySuppressionMode();
}

void TcpManager::setO4ServerMode(uint8_t mode)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setO4ServerMode(mode);
}

void TcpManager::queryO4ServerMode()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryO4ServerMode();
}

void TcpManager::setUavCategoryDisplayMode(uint8_t mode)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setUavCategoryDisplayMode(mode);
}

void TcpManager::queryUavCategoryDisplayMode()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryUavCategoryDisplayMode();
}

void TcpManager::setDataEnable(uint8_t enabled)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setDataEnable(enabled);
}

void TcpManager::queryDataEnable()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryDataEnable();
}

void TcpManager::setFeatureModes(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setFeatureModes(wifiRemoteIdEnabled, fpvEnabled, djiParseEnabled);
}

void TcpManager::queryFeatureModes()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryFeatureModes();
}

bool TcpManager::dispatchNetworkConfigProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 26:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleDeviceIpSettingResponse(frameData);
        }
        return true;
    case 28:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleDeviceIpQueryResponse(frameData);
        }
        return true;
    case 194:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleFullScanSettingResponse(frameData);
        }
        return true;
    case 196:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleFullScanQueryResponse(frameData);
        }
        return true;
    case 238:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleTcpServerIpSettingResponse(frameData);
        }
        return true;
    case 240:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleTcpServerIpQueryResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::setFullScanParams(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin, double att)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setFullScanParams(ssth, ssJgMax, ssJgMin, ssMax, ssMin, att);
}

void TcpManager::queryFullScanParams()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryFullScanParams();
}

void TcpManager::setDeviceIp(const QString &ip, int port, const QString &mask, const QString &route, const QString &dns)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setDeviceIp(ip, port, mask, route, dns);
}

void TcpManager::queryDeviceIp()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryDeviceIp();
}

void TcpManager::setTcpServerIp(const QString &ip, int port)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setTcpServerIp(ip, port);
}

void TcpManager::queryTcpServerIp()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryTcpServerIp();
}

bool TcpManager::dispatchStrikeFrequencyProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 97:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleStrikeFrequencySettingResponse(frameData);
        }
        return true;
    case 99:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleStrikeFrequencyQueryResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::setStrikeFrequencyBands(const StrikeFrequencyBandList &bands)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setStrikeFrequencyBands(bands);
}

void TcpManager::queryStrikeFrequencyBands()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryStrikeFrequencyBands();
}

bool TcpManager::dispatchPowerAmplifierProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 119:
        if (settingsProtocolService)
        {
            settingsProtocolService->handlePowerAmplifierSettingResponse(frameData);
        }
        return true;
    case 121:
        if (settingsProtocolService)
        {
            settingsProtocolService->handlePowerAmplifierQueryResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::setPowerAmplifierParams(const PowerAmplifierParamList &params)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setPowerAmplifierParams(params);
}

void TcpManager::queryPowerAmplifierParams()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryPowerAmplifierParams();
}

bool TcpManager::dispatchDirectionCalibrationProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 125:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleDirectionCalibrationSettingResponse(frameData);
        }
        return true;
    case 127:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleDirectionCalibrationQueryResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::setDirectionCalibrationValues(const DirectionCalibrationValueList &values)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setDirectionCalibrationValues(values);
}

void TcpManager::queryDirectionCalibrationValues()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryDirectionCalibrationValues();
}

bool TcpManager::dispatchSignalSourceParamsProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 106:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleSignalSourceParamsSettingResponse(frameData);
        }
        return true;
    case 108:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleSignalSourceParamsQueryResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::setSignalSourceParams(int serialScan, const QVector<int> &scanModes, int vcoMode, const QVector<int> &vcoScans)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setSignalSourceParams(serialScan, scanModes, vcoMode, vcoScans);
}

void TcpManager::querySignalSourceParams()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->querySignalSourceParams();
}

bool TcpManager::dispatchDataCollectionProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 19:
        if (settingsProtocolService)
        {
            settingsProtocolService->handlePatternUploadResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::uploadPatternFile(const PatternUploadRequest &request)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->uploadPatternFile(request);
}

bool TcpManager::dispatchFirmwareProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 15:
        Q_UNUSED(header);
        if (settingsProtocolService)
        {
            settingsProtocolService->handleFirmwareVersionsQueryResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::queryFirmwareVersions()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryFirmwareVersions();
}

bool TcpManager::dispatchModelLibraryProtocol(const ProtocolHeader *header, const QByteArray &frameData)
{
    switch (header->dataType)
    {
    case 202:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleModelLibraryModeSettingResponse(frameData);
        }
        return true;
    case 204:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleModelLibraryModeQueryResponse(frameData);
        }
        return true;
    case 206:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleModelLibraryRecordSetResponse(frameData);
        }
        return true;
    case 208:
        if (settingsProtocolService)
        {
            settingsProtocolService->handleModelLibraryRecordsQueryResponse(frameData);
        }
        return true;
    default:
        return false;
    }
}

void TcpManager::setModelLibraryMode(uint8_t mode)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setModelLibraryMode(mode);
}

void TcpManager::queryModelLibraryMode()
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryModelLibraryMode();
}

void TcpManager::setModelLibraryRecord(const ModelLibraryUpdateRequest &request)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->setModelLibraryRecord(request);
}

void TcpManager::queryModelLibraryRecords(const ModelLibraryPageQuery &query)
{
    if (!settingsProtocolService)
    {
        return;
    }

    settingsProtocolService->queryModelLibraryRecords(query);
}
