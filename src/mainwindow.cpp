#include "mainwindow.h"
#include "app_config.h"
#include "app_controller.h"
#include "components/screen_flash_overlay.h"
#include "components/top_nav_bar.h"
#include "views/history/history_page.h"
#include "views/home/home_page.h"
#include "views/settings/settings_page.h"
#include "views/statistics/stats_page.h"
#include "views/whitelist/whitelist_page.h"
#include <QResizeEvent>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), appController(nullptr), screenFlashOverlay(nullptr)
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
    appController = new AppController(pageHome, pageHistory, pageSettings, AppConfig::defaultConnectionConfig(), this);
    appController->connectToDevice();

    setupMainLayout(topNavBar);
    screenFlashOverlay = new ScreenFlashOverlay(this);
    screenFlashOverlay->syncToParentGeometry();
    connectHomePage(pageHome, topNavBar);
    connectNavigation(topNavBar);
    connectController(topNavBar);
    connect(pageSettings, &SettingsPage::requestSetScreenFlashEnabled, this,
            [this, pageSettings](bool enabled)
            {
                if (screenFlashOverlay)
                {
                    screenFlashOverlay->setFlashingEnabled(enabled);
                    pageSettings->updateScreenFlashEnabled(screenFlashOverlay->isFlashingEnabled());
                }
            });
    connect(pageSettings, &SettingsPage::requestQueryScreenFlashEnabled, this,
            [this, pageSettings]()
            {
                pageSettings->updateScreenFlashEnabled(screenFlashOverlay && screenFlashOverlay->isFlashingEnabled());
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

void MainWindow::connectNavigation(TopNavBar *topNavBar)
{
    connect(topNavBar, &TopNavBar::pageSwitched, this,
            [this](int index)
            {
                if (index >= 0 && index < stackedWidget->count())
                {
                    stackedWidget->setCurrentIndex(index);
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

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (screenFlashOverlay)
    {
        screenFlashOverlay->syncToParentGeometry();
    }
}
