#ifndef SCREEN_FLASH_OVERLAY_H
#define SCREEN_FLASH_OVERLAY_H

#include <QWidget>

class QPaintEvent;
class QTimer;

class ScreenFlashOverlay : public QWidget
{
  public:
    explicit ScreenFlashOverlay(QWidget *parent = nullptr);

    void setFlashingEnabled(bool enabled);
    bool isFlashingEnabled() const;
    void syncToParentGeometry();

  protected:
    void paintEvent(QPaintEvent *event) override;

  private:
    QTimer *flashTimer;
    bool flashingEnabled;
    bool flashVisible;
};

#endif // SCREEN_FLASH_OVERLAY_H
