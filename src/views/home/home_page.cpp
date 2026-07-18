#include "home_page.h"
#include "bottom_console.h"
#include "home_web_bridge.h"
#include <QCoreApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
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

/**
 * @brief 首页页面构造函数，初始化地图WebView、Web桥接器、底部控制台等核心组件。
 *
 * 初始化所有成员变量为默认状态：地图未加载、无待发送设备信息、
 * 预警消除时间默认20秒、通信/导航干扰默认关闭。创建单次触发的
 * Toast提示定时器，超时后自动隐藏提示_widget。随后依次调用
 * setupUi()构建界面布局和setupConnections()建立信号槽连接。
 *
 * @param parent 父QWidget，用于Qt对象树内存管理，默认为nullptr
 */
HomePage::HomePage(QWidget *parent)
    : QWidget(parent), mapWebView(nullptr), homeWebBridge(nullptr), bottomBar(nullptr), rightPanel(nullptr),
      rightPanelTitleLabel(nullptr), rightPanelCountValueLabel(nullptr), rightPanelModelValueLabel(nullptr),
      rightPanelSerialValueLabel(nullptr), rightPanelFrequencyValueLabel(nullptr), rightPanelDistanceValueLabel(nullptr),
      bottomConsole(nullptr), mapPageLoaded(false), hasPendingDeviceInfo(false), pendingLng(0.0), pendingLat(0.0),
      pendingAlt(0.0), pendingYaw(0.0), pendingPitch(0.0), pendingWarningRemoveTimeSeconds(20),
      commJammingEnabled(false), navJammingEnabled(false), toastWidget(nullptr), toastLabel(nullptr),
      toastTimer(new QTimer(this)), droneTargetCleanupTimer(new QTimer(this))
{
    setStyleSheet("background-color: #1e1e1e;");
    // 创建单次触发的Toast提示定时器，超时后自动隐藏提示_widget
    toastTimer->setSingleShot(true);
    connect(toastTimer, &QTimer::timeout, this,
            [this]()
            {
                if (toastWidget)
                {
                    toastWidget->hide();
                }
            });
    droneTargetCleanupTimer->setInterval(1000);
    connect(droneTargetCleanupTimer, &QTimer::timeout, this, &HomePage::cleanupExpiredDroneTargets);
    droneTargetCleanupTimer->start();
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

    // 一个深色背景容器，高度固定 90px，只承载底部控制台
    bottomBar = new QWidget(this);
    bottomBar->setFixedHeight(90);
    bottomBar->setStyleSheet("background-color: #212124;");

    QHBoxLayout *bottomBarLayout = new QHBoxLayout(bottomBar);
    bottomBarLayout->setContentsMargins(0, 0, 0, 0);
    bottomBarLayout->setSpacing(0);

    // 干扰控制按钮区
    bottomConsole = new BottomConsole(bottomBar);   // 干扰控制按钮
    bottomBarLayout->addWidget(bottomConsole, 1);   // 占据剩余空间，自动拉伸
    rightPanel = nullptr;
    rightPanelTitleLabel = nullptr;
    rightPanelCountValueLabel = nullptr;
    rightPanelModelValueLabel = nullptr;
    rightPanelSerialValueLabel = nullptr;
    rightPanelFrequencyValueLabel = nullptr;
    rightPanelDistanceValueLabel = nullptr;
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
    toastTimer->start(1500);
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

void HomePage::updateRightPanelVisibility()
{
    if (!rightPanel)
    {
        return;
    }

    const bool hasTargets = !pendingDroneTargets.isEmpty();
    rightPanel->setVisible(hasTargets);

    if (!hasTargets)
    {
        if (rightPanelCountValueLabel)
        {
            rightPanelCountValueLabel->setText(QStringLiteral("0"));
        }
        if (rightPanelModelValueLabel)
        {
            rightPanelModelValueLabel->setText(QStringLiteral("--"));
        }
        if (rightPanelSerialValueLabel)
        {
            rightPanelSerialValueLabel->setText(QStringLiteral("--"));
        }
        if (rightPanelFrequencyValueLabel)
        {
            rightPanelFrequencyValueLabel->setText(QStringLiteral("--"));
        }
        if (rightPanelDistanceValueLabel)
        {
            rightPanelDistanceValueLabel->setText(QStringLiteral("--"));
        }
    }
}

void HomePage::refreshRightPanelContentFromPendingTargets()
{
    if (pendingDroneTargets.isEmpty())
    {
        updateRightPanelVisibility();
        return;
    }

    QString latestTargetKey;
    QDateTime latestSeenAt;
    for (auto it = pendingDroneTargetLastSeenAt.cbegin(); it != pendingDroneTargetLastSeenAt.cend(); ++it)
    {
        if (!pendingDroneTargets.contains(it.key()))
        {
            continue;
        }
        if (latestTargetKey.isEmpty() || it.value() > latestSeenAt)
        {
            latestTargetKey = it.key();
            latestSeenAt = it.value();
        }
    }

    if (latestTargetKey.isEmpty())
    {
        refreshRightPanelContent(pendingDroneTargets.cbegin().value());
        return;
    }

    refreshRightPanelContent(pendingDroneTargets.value(latestTargetKey));
}

void HomePage::refreshRightPanelContent(const QJsonObject &targetInfo)
{
    if (rightPanelCountValueLabel)
    {
        rightPanelCountValueLabel->setText(QString::number(pendingDroneTargets.size()));
    }
    if (rightPanelModelValueLabel)
    {
        rightPanelModelValueLabel->setText(resolvePanelModelName(targetInfo));
    }
    if (rightPanelSerialValueLabel)
    {
        rightPanelSerialValueLabel->setText(resolvePanelSerialNumber(targetInfo));
    }
    if (rightPanelFrequencyValueLabel)
    {
        rightPanelFrequencyValueLabel->setText(formatPanelFrequency(targetInfo.value(QStringLiteral("frequencyKhz")).toDouble()));
    }
    if (rightPanelDistanceValueLabel)
    {
        rightPanelDistanceValueLabel->setText(QStringLiteral("%1米").arg(targetInfo.value(QStringLiteral("distance")).toInt()));
    }
}

void HomePage::cleanupExpiredDroneTargets()
{
    if (pendingDroneTargets.isEmpty())
    {
        return;
    }

    const QDateTime now = QDateTime::currentDateTime();
    const int timeoutSeconds = qMax(0, pendingWarningRemoveTimeSeconds);
    QStringList expiredKeys;

    for (auto it = pendingDroneTargetLastSeenAt.cbegin(); it != pendingDroneTargetLastSeenAt.cend(); ++it)
    {
        if (!pendingDroneTargets.contains(it.key()))
        {
            expiredKeys.append(it.key());
            continue;
        }
        if (it.value().isValid() && it.value().secsTo(now) >= timeoutSeconds)
        {
            expiredKeys.append(it.key());
        }
    }

    if (expiredKeys.isEmpty())
    {
        return;
    }

    for (const QString &key : expiredKeys)
    {
        pendingDroneTargets.remove(key);
        pendingDroneTargetLastSeenAt.remove(key);
    }

    updateRightPanelVisibility();
    refreshRightPanelContentFromPendingTargets();
}

QString HomePage::formatPanelFrequency(double frequencyKhz) const
{
    if (frequencyKhz >= 1000000.0)
    {
        return QStringLiteral("%1GHz").arg(QString::number(frequencyKhz / 1000000.0, 'f', 3));
    }
    return QStringLiteral("%1MHz").arg(QString::number(frequencyKhz / 1000.0, 'f', 0));
}

QString HomePage::resolvePanelSerialNumber(const QJsonObject &targetInfo) const
{
    const QString uniqueId = targetInfo.value(QStringLiteral("targetUniqueId")).toString().trimmed();
    if (!uniqueId.isEmpty())
    {
        return uniqueId;
    }

    const qint64 targetId = targetInfo.value(QStringLiteral("targetId")).toVariant().toLongLong();
    if (targetId > 0)
    {
        return QStringLiteral("目标-%1").arg(targetId);
    }

    return QStringLiteral("--");
}

QString HomePage::resolvePanelModelName(const QJsonObject &targetInfo) const
{
    const QString modelName = targetInfo.value(QStringLiteral("targetName")).toString().trimmed();
    return modelName.isEmpty() ? QStringLiteral("未知型号") : modelName;
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
        pendingDroneTargetLastSeenAt.remove(targetKey);
    }
    else
    {
        pendingDroneTargets.insert(targetKey, targetInfo);
        pendingDroneTargetLastSeenAt.insert(targetKey, QDateTime::currentDateTime());
    }

    updateRightPanelVisibility();
    if (!targetInfo.value(QStringLiteral("disappeared")).toBool())
    {
        refreshRightPanelContent(targetInfo);
    }
    else
    {
        refreshRightPanelContentFromPendingTargets();
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
    qDebug().noquote() << QStringLiteral("[WarningRemoveTime][Home] seconds=%1").arg(pendingWarningRemoveTimeSeconds);
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
