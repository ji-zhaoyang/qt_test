#include "bottom_console.h"
#include <QHBoxLayout>
#include <QVBoxLayout>

BottomConsole::BottomConsole(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(90);
    setStyleSheet("background-color: #212124; color: white; border-top: 1px solid #111;");
    setupUi();
}

void BottomConsole::setupUi()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(20, 10, 20, 10);
    mainLayout->setSpacing(20);

    // --- 干扰大按钮区域（铺满底部） ---
    QPushButton *btnCommJamming = new QPushButton(this);
    QPushButton *btnNavJamming = new QPushButton(this);

    // 设置按钮水平方向自动拉伸铺满
    btnCommJamming->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnCommJamming->setFixedHeight(60);
    btnNavJamming->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnNavJamming->setFixedHeight(60);

    QVBoxLayout *commLayout = new QVBoxLayout(btnCommJamming);
    commLayout->setContentsMargins(20, 10, 20, 10);
    commLayout->setSpacing(2);
    QLabel *lblCommTitle = new QLabel("通信干扰", btnCommJamming);
    lblCommTitle->setStyleSheet(
        "font-size: 16px; font-weight: bold; color: white; background: transparent; border: none;");
    lblCommStatus = new QLabel("已关闭", btnCommJamming);
    lblCommStatus->setStyleSheet("font-size: 12px; color: #aaa; background: transparent; border: none;");
    commLayout->addWidget(lblCommTitle);
    commLayout->addWidget(lblCommStatus);

    QVBoxLayout *navBtnLayout = new QVBoxLayout(btnNavJamming);
    navBtnLayout->setContentsMargins(20, 10, 20, 10);
    navBtnLayout->setSpacing(2);
    QLabel *lblNavTitle = new QLabel("导航干扰", btnNavJamming);
    lblNavTitle->setStyleSheet(
        "font-size: 16px; font-weight: bold; color: white; background: transparent; border: none;");
    lblNavBtnStatus = new QLabel("已关闭", btnNavJamming);
    lblNavBtnStatus->setStyleSheet("font-size: 12px; color: #aaa; background: transparent; border: none;");
    navBtnLayout->addWidget(lblNavTitle);
    navBtnLayout->addWidget(lblNavBtnStatus);

    QString btnStyle = R"(
        QPushButton {
            background-color: #2b2b2e;
            border-radius: 4px;
            border: 1px solid #111;
            min-width: 150px;
        }
        QPushButton:checked {
            background-color: #e74c3c; /* 开启状态变红 */
            border: 1px solid #c0392b;
        }
    )";
    btnCommJamming->setStyleSheet(btnStyle);
    btnNavJamming->setStyleSheet(btnStyle);
    btnCommJamming->setCheckable(true);
    btnNavJamming->setCheckable(true);

    mainLayout->addWidget(btnCommJamming);
    mainLayout->addWidget(btnNavJamming);

    // 绑定并转发信号
    connect(btnCommJamming, &QPushButton::toggled, this,
            [this](bool checked)
            {
                lblCommStatus->setText(checked ? "已开启" : "已关闭");
                lblCommStatus->setStyleSheet(
                    checked ? "font-size: 12px; color: #ffcccc; background: transparent; border: none;"
                            : "font-size: 12px; color: #aaa; background: transparent; border: none;");
                emit commJammingToggled(checked);
            });

    connect(btnNavJamming, &QPushButton::toggled, this,
            [this](bool checked)
            {
                lblNavBtnStatus->setText(checked ? "已开启" : "已关闭");
                lblNavBtnStatus->setStyleSheet(
                    checked ? "font-size: 12px; color: #ffcccc; background: transparent; border: none;"
                            : "font-size: 12px; color: #aaa; background: transparent; border: none;");
                emit navJammingToggled(checked);
            });
}