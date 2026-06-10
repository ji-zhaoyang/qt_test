#include "home_page.h"
#include "bottom_console.h"
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QResizeEvent>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <QWebEngineView>

namespace
{
const int kHomeRightPanelWidth = 336;
}

HomePage::HomePage(QWidget *parent)
    : QWidget(parent), mapWebView(nullptr), bottomConsole(nullptr), mapPageLoaded(false), hasPendingDeviceInfo(false),
      pendingLng(0.0), pendingLat(0.0), pendingAlt(0.0), pendingYaw(0.0), pendingPitch(0.0),
      pendingWarningRemoveTimeSeconds(20), commJammingEnabled(false), navJammingEnabled(false), toastWidget(nullptr),
      toastLabel(nullptr), toastTimer(new QTimer(this))
{
    setStyleSheet("background-color: #1e1e1e;");
    toastTimer->setSingleShot(true);
    connect(toastTimer, &QTimer::timeout, this,
            [this]()
            {
                if (toastWidget)
                {
                    toastWidget->hide();
                }
            });
    setupUi();
    setupConnections();
}

void HomePage::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    mapWebView = new QWebEngineView(this);

    const QString htmlPath = QCoreApplication::applicationDirPath() + "/assets/web/test_map.html";
    mapWebView->setUrl(QUrl::fromLocalFile(htmlPath));
    layout->addWidget(mapWebView, 1);

    QWidget *bottomBar = new QWidget(this);
    bottomBar->setFixedHeight(90);
    bottomBar->setStyleSheet("background-color: #212124;");

    QHBoxLayout *bottomBarLayout = new QHBoxLayout(bottomBar);
    bottomBarLayout->setContentsMargins(0, 0, 0, 0);
    bottomBarLayout->setSpacing(0);

    bottomConsole = new BottomConsole(bottomBar);
    bottomBarLayout->addWidget(bottomConsole, 1);

    QWidget *rightSpacer = new QWidget(bottomBar);
    rightSpacer->setFixedWidth(kHomeRightPanelWidth);
    rightSpacer->setStyleSheet("background-color: #262930; border-left: 1px solid #111;");
    bottomBarLayout->addWidget(rightSpacer);

    layout->addWidget(bottomBar);

    toastWidget = new QWidget(this);
    toastWidget->setStyleSheet("background-color: rgba(18, 20, 25, 244); "
                               "border: 1px solid rgba(255,255,255,0.06); "
                               "border-radius: 10px;");
    QHBoxLayout *toastLayout = new QHBoxLayout(toastWidget);
    toastLayout->setContentsMargins(22, 12, 22, 12);
    toastLayout->setSpacing(0);
    toastLabel = new QLabel(toastWidget);
    toastLabel->setStyleSheet("color: #f3f5f8; font-size: 14px; font-weight: 600;");
    toastLabel->setAlignment(Qt::AlignCenter);
    toastLayout->addWidget(toastLabel);
    toastWidget->setGraphicsEffect(nullptr);
    toastWidget->hide();
}

void HomePage::setupConnections()
{
    connect(mapWebView, &QWebEngineView::loadFinished, this,
            [this](bool ok)
            {
                mapPageLoaded = ok;
                if (ok && hasPendingDeviceInfo)
                {
                    dispatchDeviceInfoToMap();
                }
                if (ok)
                {
                    dispatchAllDroneTargetsToMap();
                    dispatchWarningRemoveTimeToMap();
                }
                if (ok && mapWebView)
                {
                    mapWebView->page()->runJavaScript(
                        QStringLiteral("if (typeof map !== 'undefined' && map.invalidateSize) { map.invalidateSize(); }"));
                }
            });

    connect(mapWebView, &QWebEngineView::titleChanged, this,
            [this](const QString &title)
            {
                if (title == "CMD:FULLSCREEN_ON")
                {
                    bottomConsole->hide();
                    emit fullscreenChanged(true);
                }
                else if (title == "CMD:FULLSCREEN_OFF")
                {
                    bottomConsole->show();
                    emit fullscreenChanged(false);
                }
                else if (title.startsWith(QStringLiteral("CMD:DIRECTION_FINDING:")))
                {
                    const QStringList parts = title.split(QLatin1Char(':'));
                    if (parts.size() >= 5)
                    {
                        const bool enabled = parts.at(2) == QStringLiteral("1");
                        bool ok = false;
                        const quint32 targetId = parts.at(3).toUInt(&ok);
                        if (ok)
                        {
                            emit requestDroneDirectionFinding(enabled, targetId);
                        }
                    }
                    mapWebView->page()->runJavaScript(QStringLiteral("document.title = 'Qt 离线地图';"));
                }
                else if (title.startsWith(QStringLiteral("CMD:PRECISION_STRIKE:")))
                {
                    const QStringList parts = title.split(QLatin1Char(':'));
                    if (parts.size() >= 8)
                    {
                        const bool enabled = parts.at(2) == QStringLiteral("1");
                        bool targetOk = false;
                        bool timestampOk = false;
                        bool typeOk = false;
                        const quint32 targetId = parts.at(3).toUInt(&targetOk);
                        const quint32 timestamp = parts.at(4).toUInt(&timestampOk);
                        const int type = parts.at(5).toInt(&typeOk);
                        const QString sn = QUrl::fromPercentEncoding(parts.at(6).toUtf8()).trimmed();
                        if (targetOk && timestampOk && typeOk && !sn.isEmpty())
                        {
                            emit requestDronePrecisionStrike(enabled, timestamp, sn, type, targetId);
                        }
                    }
                    mapWebView->page()->runJavaScript(QStringLiteral("document.title = 'Qt 离线地图';"));
                }
                else if (title.startsWith(QStringLiteral("CMD:WIDE_JAM:")))
                {
                    const QStringList parts = title.split(QLatin1Char(':'));
                    if (parts.size() >= 7)
                    {
                        const bool enabled = parts.at(2) == QStringLiteral("1");
                        bool targetOk = false;
                        bool frequencyOk = false;
                        const quint32 targetId = parts.at(3).toUInt(&targetOk);
                        const quint32 frequencyKhz = parts.at(4).toUInt(&frequencyOk);
                        const QString sn = QUrl::fromPercentEncoding(parts.at(5).toUtf8()).trimmed();
                        if (targetOk && frequencyOk && frequencyKhz > 0 && !sn.isEmpty())
                        {
                            emit requestDroneWideBandJamming(enabled, frequencyKhz, sn, targetId);
                        }
                    }
                    mapWebView->page()->runJavaScript(QStringLiteral("document.title = 'Qt 离线地图';"));
                }
            });

    connect(mapWebView->page(), &QWebEnginePage::renderProcessTerminated, this,
            [](QWebEnginePage::RenderProcessTerminationStatus, int)
            {
            });

    connect(bottomConsole, &BottomConsole::commJammingToggled, this, &HomePage::commJammingToggled);
    connect(bottomConsole, &BottomConsole::navJammingToggled, this, &HomePage::navJammingToggled);
}

void HomePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateHomeToastPosition();
}

void HomePage::showHomeToast(const QString &text)
{
    if (!toastWidget || !toastLabel)
    {
        return;
    }

    toastLabel->setText(text);
    toastWidget->adjustSize();
    const int minWidth = 220;
    if (toastWidget->width() < minWidth)
    {
        toastWidget->resize(minWidth, toastWidget->height());
    }
    updateHomeToastPosition();
    toastWidget->show();
    toastWidget->raise();
    toastTimer->start(2200);
}

void HomePage::updateHomeToastPosition()
{
    if (!toastWidget)
    {
        return;
    }

    const int x = qMax(12, (width() - toastWidget->width()) / 2);
    const int y = 28;
    toastWidget->move(x, y);
}

void HomePage::updateDeviceInfo(double lng, double lat, double alt, double yaw, double pitch)
{
    if (lat < -90.0 || lat > 90.0 || lng < -180.0 || lng > 180.0)
    {
        return;
    }

    pendingLng = lng;
    pendingLat = lat;
    pendingAlt = alt;
    pendingYaw = yaw;
    pendingPitch = pitch;
    hasPendingDeviceInfo = true;

    if (!mapPageLoaded)
    {
        return;
    }

    dispatchDeviceInfoToMap();
}

void HomePage::updateDroneTargetInfo(const QJsonObject &targetInfo)
{
    const QString targetKey = targetInfo.value(QStringLiteral("targetUniqueId")).toString().trimmed().isEmpty()
                                  ? QString::number(targetInfo.value(QStringLiteral("targetId")).toInt())
                                  : targetInfo.value(QStringLiteral("targetUniqueId")).toString().trimmed();
    if (targetKey.isEmpty())
    {
        return;
    }

    if (targetInfo.value(QStringLiteral("disappeared")).toBool())
    {
        pendingDroneTargets.remove(targetKey);
    }
    else
    {
        pendingDroneTargets.insert(targetKey, targetInfo);
    }

    if (!mapPageLoaded)
    {
        return;
    }

    dispatchDroneTargetToMap(targetInfo);
}

void HomePage::setWarningRemoveTimeSeconds(int seconds)
{
    pendingWarningRemoveTimeSeconds = qMax(0, seconds);
    if (!mapPageLoaded)
    {
        return;
    }

    dispatchWarningRemoveTimeToMap();
}

void HomePage::updateDroneDirectionFindingResponse(quint32 targetId, bool enabled, bool success, const QString &msg)
{
    if (!mapPageLoaded)
    {
        return;
    }
    dispatchDroneDirectionFindingResponseToMap(targetId, enabled, success, msg);
}

void HomePage::updateDroneDirectionPowerReport(const QJsonObject &reportData)
{
    if (!mapPageLoaded)
    {
        return;
    }
    dispatchDroneDirectionPowerReportToMap(reportData);
}

void HomePage::updateDronePrecisionStrikeResponse(quint32 targetId, bool enabled, bool success, const QString &msg)
{
    if (!mapPageLoaded)
    {
        return;
    }
    dispatchDronePrecisionStrikeResponseToMap(targetId, enabled, success, msg);
}

void HomePage::updateDroneWideBandJammingResponse(quint32 targetId, bool enabled, bool success, const QString &msg)
{
    if (!mapPageLoaded)
    {
        return;
    }
    dispatchDroneWideBandJammingResponseToMap(targetId, enabled, success, msg);
}

void HomePage::updateDeviceJammingSetResponse(int mode, int switchStatus, bool success, const QString &msg)
{
    Q_UNUSED(msg);
    const bool enabled = switchStatus == 1;
    if (success)
    {
        if (mode == 0)
        {
            commJammingEnabled = enabled;
            if (bottomConsole)
            {
                bottomConsole->setCommJammingChecked(enabled);
            }
        }
        else if (mode == 1)
        {
            navJammingEnabled = enabled;
            if (bottomConsole)
            {
                bottomConsole->setNavJammingChecked(enabled);
            }
        }
        return;
    }

    if (!bottomConsole)
    {
        return;
    }
    if (mode == 0)
    {
        bottomConsole->setCommJammingChecked(commJammingEnabled);
    }
    else if (mode == 1)
    {
        bottomConsole->setNavJammingChecked(navJammingEnabled);
    }
}

void HomePage::updateDeviceJammingReported(int mode, int switchStatus)
{
    const bool enabled = switchStatus == 1;
    QString text;
    if (mode == 0)
    {
        commJammingEnabled = enabled;
        if (bottomConsole)
        {
            bottomConsole->setCommJammingChecked(enabled);
        }
        text = enabled ? QStringLiteral("通信干扰打开") : QStringLiteral("通信干扰关闭");
    }
    else if (mode == 1)
    {
        navJammingEnabled = enabled;
        if (bottomConsole)
        {
            bottomConsole->setNavJammingChecked(enabled);
        }
        text = enabled ? QStringLiteral("导航干扰打开") : QStringLiteral("导航干扰关闭");
    }
    else
    {
        return;
    }

    showHomeToast(text);
}

void HomePage::dispatchDeviceInfoToMap()
{
    if (!hasPendingDeviceInfo || !mapWebView)
    {
        return;
    }

    const QString jsCode = QString("if(typeof updateMarker === 'function') updateMarker(%1, %2, %3);")
                               .arg(pendingLat, 0, 'f', 6)
                               .arg(pendingLng, 0, 'f', 6)
                               .arg(pendingAlt, 0, 'f', 2);
    mapWebView->page()->runJavaScript(jsCode);

    const QString jsDashboard = QString("if(typeof updateDashboard === 'function') updateDashboard(%1, %2);")
                                    .arg(pendingYaw, 0, 'f', 2)
                                    .arg(pendingPitch, 0, 'f', 2);
    mapWebView->page()->runJavaScript(jsDashboard);
}

void HomePage::dispatchDroneTargetToMap(const QJsonObject &targetInfo)
{
    if (!mapWebView)
    {
        return;
    }

    const QString payload = QString::fromUtf8(QJsonDocument(targetInfo).toJson(QJsonDocument::Compact));
    const QString jsCode =
        QStringLiteral("if(typeof updateDroneTargetFromQt === 'function') updateDroneTargetFromQt(%1);").arg(payload);
    mapWebView->page()->runJavaScript(jsCode);
}

void HomePage::dispatchAllDroneTargetsToMap()
{
    for (auto it = pendingDroneTargets.cbegin(); it != pendingDroneTargets.cend(); ++it)
    {
        dispatchDroneTargetToMap(it.value());
    }
}

void HomePage::dispatchWarningRemoveTimeToMap()
{
    if (!mapWebView)
    {
        return;
    }

    const QString jsCode =
        QStringLiteral("if(typeof setWarningClearDelayMs === 'function') setWarningClearDelayMs(%1);")
            .arg(pendingWarningRemoveTimeSeconds * 1000);
    mapWebView->page()->runJavaScript(jsCode);
}

void HomePage::dispatchDroneDirectionFindingResponseToMap(quint32 targetId, bool enabled, bool success, const QString &msg)
{
    if (!mapWebView)
    {
        return;
    }

    QJsonObject payload;
    payload[QStringLiteral("targetId")] = static_cast<qint64>(targetId);
    payload[QStringLiteral("enabled")] = enabled;
    payload[QStringLiteral("success")] = success;
    payload[QStringLiteral("message")] = msg;
    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const QString jsCode = QStringLiteral(
        "if(typeof updateDroneDirectionFindingResponseFromQt === 'function') updateDroneDirectionFindingResponseFromQt(%1);")
                               .arg(json);
    mapWebView->page()->runJavaScript(jsCode);
}

void HomePage::dispatchDroneDirectionPowerReportToMap(const QJsonObject &reportData)
{
    if (!mapWebView)
    {
        return;
    }

    const QString json = QString::fromUtf8(QJsonDocument(reportData).toJson(QJsonDocument::Compact));
    const QString jsCode = QStringLiteral(
        "if(typeof updateDroneDirectionPowerReportFromQt === 'function') updateDroneDirectionPowerReportFromQt(%1);")
                               .arg(json);
    mapWebView->page()->runJavaScript(jsCode);
}

void HomePage::dispatchDronePrecisionStrikeResponseToMap(quint32 targetId, bool enabled, bool success, const QString &msg)
{
    if (!mapWebView)
    {
        return;
    }

    QJsonObject payload;
    payload[QStringLiteral("targetId")] = static_cast<qint64>(targetId);
    payload[QStringLiteral("enabled")] = enabled;
    payload[QStringLiteral("success")] = success;
    payload[QStringLiteral("message")] = msg;
    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const QString jsCode = QStringLiteral(
        "if(typeof updateDronePrecisionStrikeResponseFromQt === 'function') updateDronePrecisionStrikeResponseFromQt(%1);")
                               .arg(json);
    mapWebView->page()->runJavaScript(jsCode);
}

void HomePage::dispatchDroneWideBandJammingResponseToMap(quint32 targetId, bool enabled, bool success, const QString &msg)
{
    if (!mapWebView)
    {
        return;
    }

    QJsonObject payload;
    payload[QStringLiteral("targetId")] = static_cast<qint64>(targetId);
    payload[QStringLiteral("enabled")] = enabled;
    payload[QStringLiteral("success")] = success;
    payload[QStringLiteral("message")] = msg;
    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const QString jsCode = QStringLiteral(
        "if(typeof updateDroneWideBandJammingResponseFromQt === 'function') updateDroneWideBandJammingResponseFromQt(%1);")
                               .arg(json);
    mapWebView->page()->runJavaScript(jsCode);
}
