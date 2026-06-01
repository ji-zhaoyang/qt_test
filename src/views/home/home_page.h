#ifndef HOME_PAGE_H
#define HOME_PAGE_H

#include <QWidget>

class BottomConsole;
class QWebEngineView;

class HomePage : public QWidget
{
    Q_OBJECT

  public:
    explicit HomePage(QWidget *parent = nullptr);

    void updateDeviceInfo(double lng, double lat, double alt, double yaw, double pitch);

  signals:
    void fullscreenChanged(bool isFullscreen);
    void commJammingToggled(bool checked);
    void navJammingToggled(bool checked);

  private:
    void setupUi();
    void setupConnections();
    void dispatchDeviceInfoToMap();

    QWebEngineView *mapWebView;
    BottomConsole *bottomConsole;
    bool mapPageLoaded;
    bool hasPendingDeviceInfo;
    double pendingLng;
    double pendingLat;
    double pendingAlt;
    double pendingYaw;
    double pendingPitch;
};

#endif // HOME_PAGE_H
