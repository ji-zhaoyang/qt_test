#ifndef MAP_PICKER_DIALOG_H
#define MAP_PICKER_DIALOG_H

#include <QLabel>
#include <QPushButton>
#include <QWebEngineView>
#include <QWidget>

class QVBoxLayout;
class QHBoxLayout;

class MapPickerDialog : public QWidget
{
    Q_OBJECT
  public:
    explicit MapPickerDialog(QWidget *parent = nullptr);
    ~MapPickerDialog();

    // 设置初始经纬度
    void setInitialLocation(float lng, float lat);
    void showOverlay();
    void hideOverlay();

    // 获取选择的经纬度
    float getSelectedLng() const
    {
        return selectedLng;
    }
    float getSelectedLat() const
    {
        return selectedLat;
    }

  signals:
    void locationConfirmed(float lng, float lat);
    void canceled();

  private slots:
    void onWebTitleChanged(const QString &title);
    void onConfirmClicked();
    void onCloseClicked();

  private:
    void setupUi();
    void setupOverlayStyle();
    QVBoxLayout *createMainLayout();
    void setupPanelWidget();
    QPushButton *setupTitleBar(QVBoxLayout *panelLayout);
    void setupMapArea(QVBoxLayout *panelLayout);
    void setupBottomBar(QVBoxLayout *panelLayout);
    void bindSignals(QPushButton *xBtn);
    void updatePanelGeometry();

  protected:
    void resizeEvent(QResizeEvent *event) override;

    QWidget *panelWidget;
    QWebEngineView *webView;
    QLabel *coordLabel;
    QPushButton *confirmBtn;
    QPushButton *closeBtn;

    float selectedLng;
    float selectedLat;
};

#endif // MAP_PICKER_DIALOG_H
