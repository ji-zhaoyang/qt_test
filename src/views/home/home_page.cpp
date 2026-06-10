#include "home_page.h"
#include "bottom_console.h"
#include "home_web_bridge.h"
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
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
    : QWidget(parent), mapWebView(nullptr), homeWebBridge(nullptr), bottomConsole(nullptr), mapPageLoaded(false),
      hasPendingDeviceInfo(false), pendingLng(0.0), pendingLat(0.0), pendingAlt(0.0), pendingYaw(0.0),
      pendingPitch(0.0), pendingWarningRemoveTimeSeconds(20), commJammingEnabled(false), navJammingEnabled(false),
      toastWidget(nullptr), toastLabel(nullptr), toastTimer(new QTimer(this))
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
    homeWebBridge = new HomeWebBridge(mapWebView, this);

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
                    if (homeWebBridge)
                    {
                        homeWebBridge->sendDeviceInfo(pendingLng, pendingLat, pendingAlt, pendingYaw, pendingPitch);
                    }
                }
                if (ok)
                {
                    dispatchAllDroneTargetsToMap();
                    if (homeWebBridge)
                    {
                        homeWebBridge->sendWarningRemoveTimeSeconds(pendingWarningRemoveTimeSeconds);
                    }
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
                if (homeWebBridge)
                {
                    homeWebBridge->handleTitleCommand(title);
                }
            });

    connect(mapWebView->page(), &QWebEnginePage::renderProcessTerminated, this,
            [](QWebEnginePage::RenderProcessTerminationStatus, int)
            {
            });

    connect(bottomConsole, &BottomConsole::commJammingToggled, this, &HomePage::commJammingToggled);
    connect(bottomConsole, &BottomConsole::navJammingToggled, this, &HomePage::navJammingToggled);

    connect(homeWebBridge, &HomeWebBridge::fullscreenRequested, this,
            [this](bool enabled)
            {
                if (!bottomConsole)
                {
                    return;
                }

                if (enabled)
                {
                    bottomConsole->hide();
                }
                else
                {
                    bottomConsole->show();
                }
                emit fullscreenChanged(enabled);
            });
    connect(homeWebBridge, &HomeWebBridge::directionFindingRequested, this, &HomePage::requestDroneDirectionFinding);
    connect(homeWebBridge, &HomeWebBridge::precisionStrikeRequested, this, &HomePage::requestDronePrecisionStrike);
    connect(homeWebBridge, &HomeWebBridge::wideBandJammingRequested, this, &HomePage::requestDroneWideBandJamming);
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

    if (!homeWebBridge)
    {
        return;
    }

    homeWebBridge->sendDeviceInfo(pendingLng, pendingLat, pendingAlt, pendingYaw, pendingPitch);
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

    if (!homeWebBridge)
    {
        return;
    }

    homeWebBridge->sendDroneTarget(targetInfo);
}

void HomePage::setWarningRemoveTimeSeconds(int seconds)
{
    pendingWarningRemoveTimeSeconds = qMax(0, seconds);
    if (!mapPageLoaded)
    {
        return;
    }

    if (!homeWebBridge)
    {
        return;
    }

    homeWebBridge->sendWarningRemoveTimeSeconds(pendingWarningRemoveTimeSeconds);
}

void HomePage::updateDroneDirectionFindingResponse(quint32 targetId, bool enabled, bool success, const QString &msg)
{
    if (!mapPageLoaded)
    {
        return;
    }
    if (!homeWebBridge)
    {
        return;
    }

    homeWebBridge->sendDirectionFindingResponse(targetId, enabled, success, msg);
}

void HomePage::updateDroneDirectionPowerReport(const QJsonObject &reportData)
{
    if (!mapPageLoaded)
    {
        return;
    }
    if (!homeWebBridge)
    {
        return;
    }

    homeWebBridge->sendDirectionPowerReport(reportData);
}

void HomePage::updateDronePrecisionStrikeResponse(quint32 targetId, bool enabled, bool success, const QString &msg)
{
    if (!mapPageLoaded)
    {
        return;
    }
    if (!homeWebBridge)
    {
        return;
    }

    homeWebBridge->sendPrecisionStrikeResponse(targetId, enabled, success, msg);
}

void HomePage::updateDroneWideBandJammingResponse(quint32 targetId, bool enabled, bool success, const QString &msg)
{
    if (!mapPageLoaded)
    {
        return;
    }
    if (!homeWebBridge)
    {
        return;
    }

    homeWebBridge->sendWideBandJammingResponse(targetId, enabled, success, msg);
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

void HomePage::dispatchAllDroneTargetsToMap()
{
    if (!homeWebBridge)
    {
        return;
    }

    for (auto it = pendingDroneTargets.cbegin(); it != pendingDroneTargets.cend(); ++it)
    {
        homeWebBridge->sendDroneTarget(it.value());
    }
}
