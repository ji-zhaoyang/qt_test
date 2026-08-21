#include "home_coordinator.h"

#include "network/core/tcp_manager.h"
#include "repositories/stats_repository.h"
#include "views/home/home_page.h"
#include "views/home/video_takeover/video_takeover_facade.h"
#include "views/settings/settings_page.h"

#include <QJsonObject>

HomeCoordinator::HomeCoordinator(HomePage *homePage, SettingsPage *settingsPage, TcpManager *tcpManager,
                                 StatsRepository *statsRepository, QObject *parent)
    : QObject(parent), homePage_(homePage), settingsPage_(settingsPage), tcpManager_(tcpManager),
      statsRepository_(statsRepository)
{
}

void HomeCoordinator::setupConnections()
{
    if (!homePage_ || !settingsPage_ || !tcpManager_)
    {
        return;
    }

    connect(tcpManager_, &TcpManager::droneDirectionFindingResponse,
            homePage_, &HomePage::updateDroneDirectionFindingResponse);
    connect(tcpManager_, &TcpManager::droneDirectionPowerReported,
            homePage_, &HomePage::updateDroneDirectionPowerReport);
    connect(tcpManager_, &TcpManager::dronePrecisionStrikeResponse,
            homePage_, &HomePage::updateDronePrecisionStrikeResponse);
    connect(tcpManager_, &TcpManager::droneWideBandJammingResponse,
            homePage_, &HomePage::updateDroneWideBandJammingResponse);
    if (VideoTakeoverFacade *videoTakeover = homePage_->videoTakeoverFacade())
    {
        connect(tcpManager_, &TcpManager::droneVideoTakeoverResponse, videoTakeover,
                &VideoTakeoverFacade::on290);
        connect(tcpManager_, &TcpManager::droneVideoImageReported, videoTakeover, &VideoTakeoverFacade::on291);
        connect(tcpManager_, &TcpManager::disconnected, videoTakeover, &VideoTakeoverFacade::onConnectionLost);
    }
    connect(tcpManager_, &TcpManager::deviceJammingModeSetResponse,
            homePage_, &HomePage::updateDeviceJammingSetResponse);
    connect(tcpManager_, &TcpManager::deviceJammingModeReported,
            homePage_, &HomePage::updateDeviceJammingReported);

    connect(settingsPage_, &SettingsPage::warningRemoveTimeChanged,
            homePage_, &HomePage::setWarningRemoveTimeSeconds);

    connect(homePage_, &HomePage::commJammingToggled, this,
            [this](bool checked)
            {
                tcpManager_->setDeviceJammingMode(0, checked ? 1 : 0);
            });
    connect(homePage_, &HomePage::navJammingToggled, this,
            [this](bool checked)
            {
                tcpManager_->setDeviceJammingMode(1, checked ? 1 : 0);
            });
    connect(homePage_, &HomePage::requestDroneDirectionFinding, this,
            [this](bool enabled, quint32 targetId)
            {
                tcpManager_->setDroneDirectionFinding(enabled, targetId);
            });
    connect(homePage_, &HomePage::requestDronePrecisionStrike, this,
            [this](bool enabled, quint32 timestamp, const QString &sn, int type, quint32 targetId)
            {
                tcpManager_->setDronePrecisionStrike(enabled, timestamp, sn, type, targetId);
            });
    connect(homePage_, &HomePage::requestDroneWideBandJamming, this,
            [this](bool enabled, quint32 frequencyKhz, const QString &sn, quint32 targetId)
            {
                tcpManager_->setDroneWideBandJamming(enabled, frequencyKhz, sn, targetId);
            });
    connect(homePage_, &HomePage::requestDroneVideoTakeover, this,
            [this](bool enabled, quint32 frequencyKhz, quint32 targetId)
            {
                tcpManager_->setDroneVideoTakeover(enabled, frequencyKhz, targetId);
            });

    if (statsRepository_)
    {
        connect(tcpManager_, &TcpManager::dronePrecisionStrikeResponse, this,
                [this](quint32 targetId, bool enabled, bool success, const QString &)
                {
                    if (enabled && success)
                    {
                        statsRepository_->insertCounterEvent(QStringLiteral("precision_strike"), targetId, true);
                    }
                });
        connect(tcpManager_, &TcpManager::droneWideBandJammingResponse, this,
                [this](quint32 targetId, bool enabled, bool success, const QString &)
                {
                    if (enabled && success)
                    {
                        statsRepository_->insertCounterEvent(QStringLiteral("wide_band_jamming"), targetId, true);
                    }
                });
    }
}

void HomeCoordinator::initializeState()
{
    if (!homePage_ || !settingsPage_)
    {
        return;
    }

    homePage_->setWarningRemoveTimeSeconds(settingsPage_->currentWarningRemoveTimeSeconds());
}

void HomeCoordinator::handleDeviceInfo(const QJsonObject &deviceInfo)
{
    if (!homePage_)
    {
        return;
    }

    if (!deviceInfo.contains(QStringLiteral("longitude")) || !deviceInfo.contains(QStringLiteral("latitude")))
    {
        return;
    }

    const double lng = deviceInfo.value(QStringLiteral("longitude")).toDouble();
    const double lat = deviceInfo.value(QStringLiteral("latitude")).toDouble();
    const double alt = deviceInfo.value(QStringLiteral("altitude")).toDouble();
    const double yaw = deviceInfo.value(QStringLiteral("azimuth")).toDouble();
    const double pitch = deviceInfo.value(QStringLiteral("pitch")).toDouble();

    homePage_->updateDeviceInfo(lng, lat, alt, yaw, pitch);
}

void HomeCoordinator::handleDroneTargetReported(const QJsonObject &targetInfo)
{
    if (!homePage_)
    {
        return;
    }

    homePage_->updateDroneTargetInfo(targetInfo);
}
