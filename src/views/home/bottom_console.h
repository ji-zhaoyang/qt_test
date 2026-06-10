#ifndef BOTTOM_CONSOLE_H
#define BOTTOM_CONSOLE_H

#include <QLabel>
#include <QPushButton>
#include <QWidget>

class BottomConsole : public QWidget
{
    Q_OBJECT
  public:
    explicit BottomConsole(QWidget *parent = nullptr);
    void setCommJammingChecked(bool checked);
    void setNavJammingChecked(bool checked);

  signals:
    // 发出硬件控制指令信号
    void commJammingToggled(bool checked);
    void navJammingToggled(bool checked);

  private:
    void setupUi();
    void applyCommJammingState(bool checked);
    void applyNavJammingState(bool checked);

    QPushButton *btnCommJamming;
    QPushButton *btnNavJamming;
    QLabel *lblCommStatus;
    QLabel *lblNavBtnStatus;
};

#endif // BOTTOM_CONSOLE_H
