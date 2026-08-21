#include "home_page.h"
#include "bottom_console.h"
#include "home_web_bridge.h"
#include "repositories/whitelist_repository.h"
#include "video_takeover/video_takeover_facade.h"
#include "views/whitelist/whitelist_page.h"
#include <QCoreApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <QWebEngineView>

namespace
{
QString resolveTargetKey(const QJsonObject &targetInfo)
{
    const QString targetUniqueId = targetInfo.value(QStringLiteral("targetUniqueId")).toString().trimmed();
    if (!targetUniqueId.isEmpty())
    {
        return targetUniqueId;
    }

    return QString::number(targetInfo.value(QStringLiteral("targetId")).toInt());
}
} // namespace

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
    : QWidget(parent), mapWebView(nullptr), homeWebBridge(nullptr), videoTakeoverFacade_(nullptr),
      whitelistRepository(nullptr), bottomBar(nullptr),
      bottomConsole(nullptr), mapPageLoaded(false), hasPendingDeviceInfo(false), screenFlashEnabled(false),
      homePageVisible(true), mapAlarmFlashActive(false), pendingLng(0.0), pendingLat(0.0),
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

VideoTakeoverFacade *HomePage::videoTakeoverFacade() const
{
    return videoTakeoverFacade_;
}

HomePage::~HomePage() = default;

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
    bottomConsole = new BottomConsole(bottomBar);
    bottomBarLayout->addWidget(bottomConsole, 1);
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

    videoTakeoverFacade_ = new VideoTakeoverFacade(this, mapWebView, this);
    videoTakeoverFacade_->setWebBridge(homeWebBridge);
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
                    evaluateMapAlarmFlash(true);
                }
                if (videoTakeoverFacade_)
                {
                    videoTakeoverFacade_->setMapPageLoaded(ok);
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
    connect(homeWebBridge, &HomeWebBridge::videoTakeoverRequested, videoTakeoverFacade_,
            &VideoTakeoverFacade::onUserRequest);
    connect(homeWebBridge, &HomeWebBridge::whitelistAddRequested, this, &HomePage::addTargetToWhitelist);
    connect(videoTakeoverFacade_, &VideoTakeoverFacade::takeoverRequested, this, &HomePage::requestDroneVideoTakeover);
    connect(videoTakeoverFacade_, &VideoTakeoverFacade::toastRequested, this, &HomePage::showHomeToast);
}

void HomePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateHomeToastPosition();
    if (videoTakeoverFacade_)
    {
        videoTakeoverFacade_->updateMapGeometry();
    }
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

    evaluateMapAlarmFlash();
}

void HomePage::setWhitelistRepository(WhitelistRepository *repository)
{
    if (whitelistRepository == repository)
    {
        return;
    }

    if (whitelistRepository)
    {
        disconnect(whitelistRepository, nullptr, this, nullptr);
    }

    whitelistRepository = repository;
    if (whitelistRepository)
    {
        connect(whitelistRepository, &WhitelistRepository::changed, this,
                [this]()
                {
                    evaluateMapAlarmFlash();
                });
    }

    evaluateMapAlarmFlash();
}

void HomePage::setScreenFlashEnabled(bool enabled)
{
    screenFlashEnabled = enabled;
    evaluateMapAlarmFlash();
}

void HomePage::setHomePageVisible(bool visible)
{
    homePageVisible = visible;
    evaluateMapAlarmFlash();
}

void HomePage::addTargetToWhitelist(const QString &serialNumber, const QString &recordKey)
{
    if (!whitelistRepository)
    {
        showHomeToast(QStringLiteral("白名单未就绪"));
        return;
    }

    const QString serial = serialNumber.trimmed().isEmpty() ? recordKey.trimmed() : serialNumber.trimmed();
    if (serial.isEmpty())
    {
        showHomeToast(QStringLiteral("目标序列号为空"));
        return;
    }

    WhitelistPage::WhitelistRecord existing;
    if (whitelistRepository->findBySerialNumber(serial, &existing))
    {
        showHomeToast(QStringLiteral("该目标已在白名单中"));
        evaluateMapAlarmFlash(true);
        return;
    }

    WhitelistPage::WhitelistRecord record;
    record.serialNumber = serial;
    record.recordKey = recordKey.trimmed().isEmpty() ? serial : recordKey.trimmed();
    record.modelName = QStringLiteral("");
    record.effectiveTime = QStringLiteral("permanent");
    record.effectiveArea = QStringLiteral("unlimited");

    if (!whitelistRepository->insertRecord(record))
    {
        const QString message = whitelistRepository->lastError().trimmed().isEmpty()
                                    ? QStringLiteral("加入白名单失败")
                                    : whitelistRepository->lastError();
        showHomeToast(message);
        return;
    }

    showHomeToast(QStringLiteral("已加入白名单"));
    evaluateMapAlarmFlash(true);
}

void HomePage::evaluateMapAlarmFlash(bool forcePush)
{
    bool hasNonWhitelistDrone = false;

    for (auto it = pendingDroneTargets.cbegin(); it != pendingDroneTargets.cend(); ++it)
    {
        const QJsonObject &targetInfo = it.value();
        if (targetInfo.value(QStringLiteral("disappeared")).toBool())
        {
            continue;
        }

        if (whitelistRepository && whitelistRepository->containsForTarget(targetInfo))
        {
            continue;
        }

        hasNonWhitelistDrone = true;
        break;
    }

    const bool showFlash = screenFlashEnabled && homePageVisible && mapPageLoaded && hasNonWhitelistDrone;
    if (!forcePush && showFlash == mapAlarmFlashActive)
    {
        return;
    }

    mapAlarmFlashActive = showFlash;
    if (homeWebBridge)
    {
        homeWebBridge->setMapAlarmFlashActive(showFlash);
    }
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
    const QString targetKey = resolveTargetKey(targetInfo);
    if (targetKey.isEmpty())
    {
        return;
    }

    const bool disappeared = targetInfo.value(QStringLiteral("disappeared")).toBool();
    if (disappeared)
    {
        pendingDroneTargets.remove(targetKey);
        pendingDroneTargetLastSeenAt.remove(targetKey);
    }
    else
    {
        pendingDroneTargets.insert(targetKey, targetInfo);
        pendingDroneTargetLastSeenAt.insert(targetKey, QDateTime::currentDateTime());
    }

    evaluateMapAlarmFlash();

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
    evaluateMapAlarmFlash();
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
