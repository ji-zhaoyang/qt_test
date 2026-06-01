#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMouseEvent>
#include <QStackedWidget>

class AppController;
class HomePage;
class HistoryPage;
class ScreenFlashOverlay;
class SettingsPage;
class StatsPage;
class TopNavBar;
class WhitelistPage;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    MainWindow(QWidget *parent = nullptr);

  protected:
    // 因为去掉了原生标题栏，需要重写鼠标事件来实现窗口拖动
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

  private:
    void setupWindow();
    void addPages(HomePage *pageHome, HistoryPage *pageHistory, WhitelistPage *pageWhitelist, StatsPage *pageStats,
                  SettingsPage *pageSettings);
    void setupMainLayout(TopNavBar *topNavBar);
    void connectNavigation(TopNavBar *topNavBar);
    void connectHomePage(HomePage *pageHome, TopNavBar *topNavBar);
    void connectController(TopNavBar *topNavBar);

    QStackedWidget *stackedWidget; // 页面堆栈（类似网页路由）
    QPoint dragPosition;           // 记录鼠标拖动位置

    AppController *appController;    // 页面与网络层协调器
    ScreenFlashOverlay *screenFlashOverlay;
};
#endif
