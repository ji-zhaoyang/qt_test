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

  signals:
    // 发出硬件控制指令信号
    void commJammingToggled(bool checked);
    void navJammingToggled(bool checked);

  private:
    void setupUi();

    QLabel *lblCommStatus;
    QLabel *lblNavBtnStatus;
};

#endif // BOTTOM_CONSOLE_H