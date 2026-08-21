#include "mainwindow.h"
#include "app_config.h"
#include "app_controller.h"
#include "components/top_nav_bar.h"
#include "preferences/alarm_preferences.h"
#include "views/history/history_page.h"
#include "views/home/home_page.h"
#include "views/settings/settings_page.h"
#include "views/statistics/stats_page.h"
#include "views/whitelist/whitelist_page.h"
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), appController(nullptr)
{
    TopNavBar *topNavBar = new TopNavBar(this);
    //QStackedWidget 是一个卡片堆叠容器——同一时刻只显示一个子页面，其他页面隐藏但保持存活。
    stackedWidget = new QStackedWidget(this);

    HomePage *pageHome = new HomePage(this);
    HistoryPage *pageHistory = new HistoryPage(this);
    WhitelistPage *pageWhitelist = new WhitelistPage(this);
    StatsPage *pageStats = new StatsPage(this);
    SettingsPage *pageSettings = new SettingsPage(this);

    addPages(pageHome, pageHistory, pageWhitelist, pageStats, pageSettings);
    appController = new AppController(pageHome, pageHistory, pageStats, pageSettings, AppConfig::defaultConnectionConfig(), this);
    appController->connectToDevice();

    pageHome->setScreenFlashEnabled(AlarmPreferences::screenFlashEnabled());
    if (appController->whitelistRepository())
    {
        pageHome->setWhitelistRepository(appController->whitelistRepository());
        pageWhitelist->setRepository(appController->whitelistRepository());
    }

    setupMainLayout(topNavBar);
    connectHomePage(pageHome, topNavBar);
    connectNavigation(topNavBar, pageHome);
    connectController(topNavBar);
    connect(pageSettings, &SettingsPage::requestSetScreenFlashEnabled, this,
            [pageHome, pageSettings](bool enabled)
            {
                AlarmPreferences::setScreenFlashEnabled(enabled);
                pageHome->setScreenFlashEnabled(enabled);
                pageSettings->updateScreenFlashEnabled(enabled);
            });
    connect(pageSettings, &SettingsPage::requestQueryScreenFlashEnabled, this,
            [pageSettings]()
            {
                pageSettings->updateScreenFlashEnabled(AlarmPreferences::screenFlashEnabled());
            });
    showFullScreen();
}

void MainWindow::addPages(HomePage *pageHome, HistoryPage *pageHistory, WhitelistPage *pageWhitelist,
                          StatsPage *pageStats,
                          SettingsPage *pageSettings)
{
    // 依次加入堆栈 (必须和 TopNavBar 的按钮顺序严格对应)
    stackedWidget->addWidget(pageHome);      // 0
    stackedWidget->addWidget(pageHistory);   // 1
    stackedWidget->addWidget(pageWhitelist); // 2
    stackedWidget->addWidget(pageStats);     // 3
    stackedWidget->addWidget(pageSettings);  // 4
}

void MainWindow::setupMainLayout(TopNavBar *topNavBar)
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(topNavBar);
    mainLayout->addWidget(stackedWidget);

    setCentralWidget(centralWidget);
}

void MainWindow::connectHomePage(HomePage *pageHome, TopNavBar *topNavBar)
{
    connect(pageHome, &HomePage::fullscreenChanged, this,
            [topNavBar](bool isFullscreen)
            {
                topNavBar->setVisible(!isFullscreen);
            });
}

void MainWindow::connectNavigation(TopNavBar *topNavBar, HomePage *pageHome)
{
    connect(topNavBar, &TopNavBar::pageSwitched, this,
            [this, pageHome](int index)
            {
                if (index >= 0 && index < stackedWidget->count())
                {
                    stackedWidget->setCurrentIndex(index);
                    pageHome->setHomePageVisible(index == 0);
                }
            });
    connect(topNavBar, &TopNavBar::closeRequested, this, &QMainWindow::close);
}

void MainWindow::connectController(TopNavBar *topNavBar)
{
    connect(appController, &AppController::deviceStatusInfoUpdated, topNavBar, &TopNavBar::updateDeviceStatusInfo);
}

// 拖动窗口的实现（简单版）
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    dragPosition = event->globalPos() - frameGeometry().topLeft();
}
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
    {
        move(event->globalPos() - dragPosition);
    }
}
