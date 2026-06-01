#include "home_page.h"
#include "bottom_console.h"
#include <QCoreApplication>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <QWebEngineView>

HomePage::HomePage(QWidget *parent)
    : QWidget(parent), mapWebView(nullptr), bottomConsole(nullptr), mapPageLoaded(false), hasPendingDeviceInfo(false),
      pendingLng(0.0), pendingLat(0.0), pendingAlt(0.0), pendingYaw(0.0), pendingPitch(0.0)
{
    setStyleSheet("background-color: #1e1e1e;");
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

    bottomConsole = new BottomConsole(this);
    layout->addWidget(bottomConsole);
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
            });

    connect(mapWebView->page(), &QWebEnginePage::renderProcessTerminated, this,
            [](QWebEnginePage::RenderProcessTerminationStatus, int)
            {
            });

    connect(bottomConsole, &BottomConsole::commJammingToggled, this, &HomePage::commJammingToggled);
    connect(bottomConsole, &BottomConsole::navJammingToggled, this, &HomePage::navJammingToggled);
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
