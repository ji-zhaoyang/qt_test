#include "home_web_bridge.h"

#include <QJsonDocument>
#include <QUrl>
#include <QWebEnginePage>
#include <QWebEngineView>

namespace
{
const char kDefaultPageTitle[] = "Qt 离线地图";
}

HomeWebBridge::HomeWebBridge(QWebEngineView *webView, QObject *parent) : QObject(parent), webView_(webView)
{
}

void HomeWebBridge::setWebView(QWebEngineView *webView)
{
    webView_ = webView;
}

QWebEngineView *HomeWebBridge::webView() const
{
    return webView_.data();
}

bool HomeWebBridge::handleTitleCommand(const QString &title)
{
    if (title == QStringLiteral("CMD:FULLSCREEN_ON"))
    {
        emit fullscreenRequested(true);
        resetPageTitle();
        return true;
    }

    if (title == QStringLiteral("CMD:FULLSCREEN_OFF"))
    {
        emit fullscreenRequested(false);
        resetPageTitle();
        return true;
    }

    if (title.startsWith(QStringLiteral("CMD:DIRECTION_FINDING:")))
    {
        const bool handled = handleDirectionFindingCommand(title.split(QLatin1Char(':')));
        resetPageTitle();
        return handled;
    }

    if (title.startsWith(QStringLiteral("CMD:PRECISION_STRIKE:")))
    {
        const bool handled = handlePrecisionStrikeCommand(title.split(QLatin1Char(':')));
        resetPageTitle();
        return handled;
    }

    if (title.startsWith(QStringLiteral("CMD:WIDE_JAM:")))
    {
        const bool handled = handleWideBandJammingCommand(title.split(QLatin1Char(':')));
        resetPageTitle();
        return handled;
    }

    return false;
}

void HomeWebBridge::sendEventToWeb(const QString &eventName, const QJsonObject &payload)
{
    if (!webView_ || !webView_->page())
    {
        return;
    }

    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const QString jsCode = QStringLiteral(
        "if(typeof %1 === 'function') %1(%2);").arg(eventName, json);
    webView_->page()->runJavaScript(jsCode);
}

void HomeWebBridge::sendDeviceInfo(double lng, double lat, double alt, double yaw, double pitch)
{
    if (!webView_ || !webView_->page())
    {
        return;
    }

    const QString markerJs = QStringLiteral("if(typeof updateMarker === 'function') updateMarker(%1, %2, %3);")
                                 .arg(lat, 0, 'f', 6)
                                 .arg(lng, 0, 'f', 6)
                                 .arg(alt, 0, 'f', 2);
    webView_->page()->runJavaScript(markerJs);

    const QString dashboardJs = QStringLiteral("if(typeof updateDashboard === 'function') updateDashboard(%1, %2);")
                                    .arg(yaw, 0, 'f', 2)
                                    .arg(pitch, 0, 'f', 2);
    webView_->page()->runJavaScript(dashboardJs);
}

void HomeWebBridge::sendDroneTarget(const QJsonObject &targetInfo)
{
    sendEventToWeb(QStringLiteral("updateDroneTargetFromQt"), targetInfo);
}

void HomeWebBridge::sendWarningRemoveTimeSeconds(int seconds)
{
    if (!webView_ || !webView_->page())
    {
        return;
    }

    const QString jsCode =
        QStringLiteral("if(typeof setWarningClearDelayMs === 'function') setWarningClearDelayMs(%1);")
            .arg(qMax(0, seconds) * 1000);
    webView_->page()->runJavaScript(jsCode);
}

void HomeWebBridge::sendDirectionFindingResponse(quint32 targetId, bool enabled, bool success, const QString &message)
{
    QJsonObject payload;
    payload[QStringLiteral("targetId")] = static_cast<qint64>(targetId);
    payload[QStringLiteral("enabled")] = enabled;
    payload[QStringLiteral("success")] = success;
    payload[QStringLiteral("message")] = message;
    sendEventToWeb(QStringLiteral("updateDroneDirectionFindingResponseFromQt"), payload);
}

void HomeWebBridge::sendDirectionPowerReport(const QJsonObject &reportData)
{
    sendEventToWeb(QStringLiteral("updateDroneDirectionPowerReportFromQt"), reportData);
}

void HomeWebBridge::sendPrecisionStrikeResponse(quint32 targetId, bool enabled, bool success, const QString &message)
{
    QJsonObject payload;
    payload[QStringLiteral("targetId")] = static_cast<qint64>(targetId);
    payload[QStringLiteral("enabled")] = enabled;
    payload[QStringLiteral("success")] = success;
    payload[QStringLiteral("message")] = message;
    sendEventToWeb(QStringLiteral("updateDronePrecisionStrikeResponseFromQt"), payload);
}

void HomeWebBridge::sendWideBandJammingResponse(quint32 targetId, bool enabled, bool success, const QString &message)
{
    QJsonObject payload;
    payload[QStringLiteral("targetId")] = static_cast<qint64>(targetId);
    payload[QStringLiteral("enabled")] = enabled;
    payload[QStringLiteral("success")] = success;
    payload[QStringLiteral("message")] = message;
    sendEventToWeb(QStringLiteral("updateDroneWideBandJammingResponseFromQt"), payload);
}

bool HomeWebBridge::handleDirectionFindingCommand(const QStringList &parts)
{
    if (parts.size() < 5)
    {
        return false;
    }

    const bool enabled = parts.at(2) == QStringLiteral("1");
    bool ok = false;
    const quint32 targetId = parts.at(3).toUInt(&ok);
    if (!ok)
    {
        return false;
    }

    emit directionFindingRequested(enabled, targetId);
    return true;
}

bool HomeWebBridge::handlePrecisionStrikeCommand(const QStringList &parts)
{
    if (parts.size() < 8)
    {
        return false;
    }

    const bool enabled = parts.at(2) == QStringLiteral("1");
    bool targetOk = false;
    bool timestampOk = false;
    bool typeOk = false;
    const quint32 targetId = parts.at(3).toUInt(&targetOk);
    const quint32 timestamp = parts.at(4).toUInt(&timestampOk);
    const int type = parts.at(5).toInt(&typeOk);
    const QString sn = QUrl::fromPercentEncoding(parts.at(6).toUtf8()).trimmed();
    if (!targetOk || !timestampOk || !typeOk || sn.isEmpty())
    {
        return false;
    }

    emit precisionStrikeRequested(enabled, timestamp, sn, type, targetId);
    return true;
}

bool HomeWebBridge::handleWideBandJammingCommand(const QStringList &parts)
{
    if (parts.size() < 7)
    {
        return false;
    }

    const bool enabled = parts.at(2) == QStringLiteral("1");
    bool targetOk = false;
    bool frequencyOk = false;
    const quint32 targetId = parts.at(3).toUInt(&targetOk);
    const quint32 frequencyKhz = parts.at(4).toUInt(&frequencyOk);
    const QString sn = QUrl::fromPercentEncoding(parts.at(5).toUtf8()).trimmed();
    if (!targetOk || !frequencyOk || frequencyKhz == 0 || sn.isEmpty())
    {
        return false;
    }

    emit wideBandJammingRequested(enabled, frequencyKhz, sn, targetId);
    return true;
}

void HomeWebBridge::resetPageTitle() const
{
    if (!webView_ || !webView_->page())
    {
        return;
    }

    webView_->page()->runJavaScript(
        QStringLiteral("document.title = '%1';").arg(QString::fromLatin1(kDefaultPageTitle)));
}
