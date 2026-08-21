#ifndef PILOT_LOCATION_DIALOG_H
#define PILOT_LOCATION_DIALOG_H

#include <QWidget>

class QResizeEvent;
class QWebEngineView;

class PilotLocationDialog : public QWidget
{
    Q_OBJECT

  public:
    explicit PilotLocationDialog(QWidget *parent = nullptr);

    void setPilotLocation(double longitude, double latitude, const QString &label = QString());
    void showOverlay();

    static bool hasValidCoordinate(double longitude, double latitude);

  private:
    void setupUi();
    void updatePanelGeometry();
    void pushLocationToWeb();

  protected:
    void resizeEvent(QResizeEvent *event) override;

    QWidget *panelWidget_ = nullptr;
    QWebEngineView *webView_ = nullptr;
    bool mapReady_ = false;
    double longitude_ = 0.0;
    double latitude_ = 0.0;
    QString label_;
};

#endif // PILOT_LOCATION_DIALOG_H
