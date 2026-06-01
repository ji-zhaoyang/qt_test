#include "top_nav_bar.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QEvent>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPixmap>
#include <QStyle>
#include <QTimeZone>
#include <QVBoxLayout>

namespace
{
bool jsonArrayHasAlarm(const QJsonValue &value)
{
    if (!value.isArray())
    {
        return false;
    }

    const QJsonArray array = value.toArray();
    for (const QJsonValue &item : array)
    {
        if (item.toInt() != 0)
        {
            return true;
        }
    }
    return false;
}

} // namespace

TopNavBar::TopNavBar(QWidget *parent)
    : QWidget(parent), lblSysStatus(nullptr), timePopup(nullptr), statusBtn(nullptr), statusBadgeLabel(nullptr),
      statusPopup(nullptr), fanStatusValue(nullptr), clockStatusValue(nullptr), receiverPllStatusValue(nullptr),
      transmitterPllStatusValue(nullptr), eepromStatusValue(nullptr), temperatureChipStatusValue(nullptr),
      compassStatusValue(nullptr), adcStatusValue(nullptr), pa485StatusValue(nullptr), paUnderpowerStatusValue(nullptr),
      paOverpowerStatusValue(nullptr), hasDeviceStatusData(false), hasAnyAlarm(false), m_localTimer(nullptr)
{
    setFixedHeight(60);
    setStyleSheet("background-color: #2b2b2e; color: white; border-bottom: 1px solid #111;");
    setupUi();

    // 初始化并启动本地定时器，每秒刷新一次时间
    m_localTimer = new QTimer(this);
    connect(m_localTimer, &QTimer::timeout, this, &TopNavBar::updateLocalTime);
    m_localTimer->start(1000);

    // 立即执行一次以显示初始时间
    updateLocalTime();
}

void TopNavBar::setupUi()
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->setSpacing(5);

    // 导航按钮样式
    QString navBtnQss = R"(
        QPushButton {
            background-color: transparent;
            color: #b0b0b0;
            border: none;
            font-size: 16px;
            font-weight: bold;
            padding: 0 20px;
        }
        QPushButton:hover {
            color: #ffffff;
            background-color: #3a3a3d;
            border-radius: 4px;
        }
        QPushButton:checked {
            color: #333333;
            background-color: #ffffff;
            border-radius: 4px;
        }
    )";

    // 【终极重构】：彻底移除 QButtonGroup，纯手工管理状态
    // 因为 QButtonGroup 内部的焦点和状态机在与 QWebEngineView 混合使用时极易发生死锁。
    QList<QPushButton *> navBtns = {new QPushButton("🏠 首页", this), new QPushButton("⏱ 侦测历史", this),
                                    new QPushButton("📋 白名单", this), new QPushButton("📊 报表统计", this),
                                    new QPushButton("⚙ 系统设置", this)};

    for (int i = 0; i < navBtns.size(); ++i)
    {
        QPushButton *btn = navBtns[i];
        btn->setStyleSheet(navBtnQss);
        btn->setFixedHeight(40);
        layout->addWidget(btn);
    }

    // 模拟选中效果的辅助函数
    auto updateBtnStyle = [navBtns](int activeIndex)
    {
        for (int j = 0; j < navBtns.size(); ++j)
        {
            if (j == activeIndex)
            {
                // 激活状态：白底黑字
                navBtns[j]->setStyleSheet(
                    navBtns[j]->styleSheet() +
                    "QPushButton { color: #333333; background-color: #ffffff; border-radius: 4px; }");
            }
            else
            {
                // 默认状态：透明底灰字
                navBtns[j]->setStyleSheet(navBtns[j]->styleSheet() +
                                          "QPushButton { color: #cccccc; background-color: transparent; }");
            }
        }
    };

    // 初始化默认选中首页
    updateBtnStyle(0);

    // 绑定点击信号
    for (int i = 0; i < navBtns.size(); ++i)
    {
        connect(navBtns[i], &QPushButton::clicked, this,
                [this, updateBtnStyle, i]()
                {
                    updateBtnStyle(i);
                    emit pageSwitched(i);
                });
    }

    // 右侧系统状态区
    layout->addStretch();

    statusBtn = new QPushButton(this);
    statusBtn->setToolTip("设备状态");
    statusBtn->setCursor(Qt::PointingHandCursor);
    statusBtn->setFixedSize(36, 36);
    const QString statusIconPath = QCoreApplication::applicationDirPath() + "/assets/web/images/shebeizhuangtai.png";
    statusBtn->setIcon(QIcon(statusIconPath));
    statusBtn->setIconSize(QSize(24, 24));
    statusBtn->setStyleSheet("QPushButton { background: transparent; border: none; padding: 0; }"
                             "QPushButton:hover { background: rgba(88, 102, 122, 0.35); border-radius: 6px; }");

    statusBadgeLabel = new QLabel(statusBtn);
    statusBadgeLabel->setFixedSize(15, 15);
    statusBadgeLabel->move(statusBtn->width() - 13, statusBtn->height() - 14);
    statusBadgeLabel->setAlignment(Qt::AlignCenter);
    statusBadgeLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    refreshStatusButtonState();
    layout->addWidget(statusBtn);

    lblSysStatus = new QLabel("--:--:--", this);
    lblSysStatus->setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold; padding-right: 20px;");
    // 允许鼠标变成手型，并安装事件过滤器以拦截点击
    lblSysStatus->setCursor(Qt::PointingHandCursor);
    lblSysStatus->installEventFilter(this);
    layout->addWidget(lblSysStatus);

    // 初始化独立的时间弹窗（无边框，置顶，并且不抢夺焦点）
    timePopup = new QLabel(this, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    timePopup->setAttribute(Qt::WA_ShowWithoutActivating); // 关键：显示时不激活窗口，防止主窗口掉出全屏
    timePopup->setStyleSheet("background-color: #3b3b3b; color: #ffffff; padding: 6px 12px; border: 1px solid #111; "
                             "border-radius: 4px; font-size: 14px;");
    timePopup->hide();

    statusPopup = new QWidget(this, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    statusPopup->setAttribute(Qt::WA_ShowWithoutActivating);
    statusPopup->setStyleSheet("background-color: #1f1f22; border: 1px solid #2b2b2e; border-radius: 8px;");
    QVBoxLayout *statusLayout = new QVBoxLayout(statusPopup);
    statusLayout->setContentsMargins(14, 14, 14, 14);
    statusLayout->setSpacing(8);

    addStatusRow(statusLayout, "风扇状态", fanStatusValue);
    addStatusRow(statusLayout, "时钟状态", clockStatusValue);
    addStatusRow(statusLayout, "接收锁相环状态", receiverPllStatusValue);
    addStatusRow(statusLayout, "发送锁相环状态", transmitterPllStatusValue);
    addStatusRow(statusLayout, "EEPROM芯片状态", eepromStatusValue);
    addStatusRow(statusLayout, "温度芯片状态", temperatureChipStatusValue);
    addStatusRow(statusLayout, "电子罗盘芯片状态", compassStatusValue);
    addStatusRow(statusLayout, "Adc芯片状态", adcStatusValue);
    addStatusRow(statusLayout, "功放485通信状态", pa485StatusValue);
    addStatusRow(statusLayout, "功放欠功率告警", paUnderpowerStatusValue);
    addStatusRow(statusLayout, "功放过功率告警", paOverpowerStatusValue);
    statusPopup->hide();

    connect(statusBtn, &QPushButton::clicked, this, &TopNavBar::toggleStatusPopup);

    QPushButton *btnClose = new QPushButton("× ", this);
    btnClose->setFixedSize(40, 40);
    btnClose->setStyleSheet("QPushButton { background: transparent; color: white; border: none; font-size: 18px; } "
                            "QPushButton:hover { background: #e74c3c; border-radius: 4px; }");
    layout->addWidget(btnClose);
    // 绑定关闭事件
    connect(btnClose, &QPushButton::clicked, this, &TopNavBar::closeRequested);
}

QLabel *TopNavBar::createStatusValueLabel()
{
    QLabel *label = new QLabel("--", statusPopup);
    label->setStyleSheet("color: #8a8f98; font-size: 14px; font-weight: bold;");
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return label;
}

void TopNavBar::addStatusRow(QVBoxLayout *layout, const QString &name, QLabel *&valueLabel)
{
    QWidget *row = new QWidget(statusPopup);
    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(24);

    QLabel *nameLabel = new QLabel(name, row);
    nameLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold;");

    valueLabel = createStatusValueLabel();

    rowLayout->addWidget(nameLabel);
    rowLayout->addStretch();
    rowLayout->addWidget(valueLabel);
    layout->addWidget(row);
}

void TopNavBar::toggleStatusPopup()
{
    if (!statusPopup)
    {
        return;
    }

    if (statusPopup->isVisible())
    {
        statusPopup->hide();
        return;
    }

    if (timePopup && timePopup->isVisible())
    {
        timePopup->hide();
    }

    statusPopup->adjustSize();
    QPoint globalBottomLeft = statusBtn->mapToGlobal(QPoint(0, statusBtn->height() + 6));
    statusPopup->move(globalBottomLeft);
    statusPopup->show();
}

void TopNavBar::updateAlarmLabel(QLabel *label, bool isAlarm)
{
    if (!label)
    {
        return;
    }

    label->setText(isAlarm ? "告警" : "正常");
    label->setStyleSheet(QString("color: %1; font-size: 14px; font-weight: bold;")
                             .arg(isAlarm ? "#ff7a7a" : "#22d35e"));
}

void TopNavBar::refreshStatusButtonState()
{
    if (!statusBtn || !statusBadgeLabel)
    {
        return;
    }

    QString symbol = "?";
    QString color = "#8a8f98";
    QString bgColor = "#3f434a";
    if (hasDeviceStatusData)
    {
        symbol = hasAnyAlarm ? "✖" : "✔";
        color = hasAnyAlarm ? "#ff4d4f" : "#22d35e";
        bgColor = hasAnyAlarm ? "#351d22" : "#163323";
    }

    statusBadgeLabel->setText(symbol);
    statusBadgeLabel->setStyleSheet(QString("QLabel { color: %1; background-color: %2; border-radius: 7px; "
                                            "border: 1px solid #1f1f22; font-size: 10px; font-weight: bold; }")
                                        .arg(color, bgColor));
}

void TopNavBar::updateLocalTime()
{
    if (!lblSysStatus)
        return;

    QDateTime now = QDateTime::currentDateTime();

    // 获取当前系统的时区 ID
    QTimeZone systemZone = QTimeZone::systemTimeZone();
    QString timeZoneName = QString::fromUtf8(systemZone.id());

    // 更新显示的文本（只显示时分秒）
    lblSysStatus->setText(now.toString("HH:mm:ss"));

    // 缓存完整的日期时间和时区信息，供点击弹窗使用
    m_fullTimeStr = QString("%1 %2").arg(now.toString("yyyy-MM-dd HH:mm:ss")).arg(timeZoneName);
    if (timePopup)
    {
        timePopup->setText(m_fullTimeStr);
    }
}

// 注意：由于需求变更为显示本地时间，这个根据底层设备数据更新时间的函数暂时作废（空实现）
void TopNavBar::updateSystemTime(const QString &fullTimeStr, const QString &timeOnlyStr, double lat, double lng)
{
    Q_UNUSED(fullTimeStr);
    Q_UNUSED(timeOnlyStr);
    Q_UNUSED(lat);
    Q_UNUSED(lng);
    // 之前根据底层数据推算时区的逻辑已经被替换为 updateLocalTime
}

void TopNavBar::updateDeviceStatusInfo(const QJsonObject &deviceInfo)
{
    const bool fanAlarm = deviceInfo.value("fanAlarm").toInt() != 0;
    const bool clockAlarm = deviceInfo.value("clockAlarm").toInt() != 0;
    const bool receiverPllAlarm = deviceInfo.value("receiverPllAlarm").toInt() != 0;
    const bool transmitterPllAlarm = deviceInfo.value("transmitterPllAlarm").toInt() != 0;
    const bool eepromAlarm = deviceInfo.value("eepromAlarm").toInt() != 0;
    const bool temperatureChipAlarm = deviceInfo.value("temperatureChipAlarm").toInt() != 0;
    const bool compassAlarm = deviceInfo.value("compassAlarm").toInt() != 0;
    const bool adcAlarm = deviceInfo.value("adcAlarm").toInt() != 0;
    const bool pa485Alarm = jsonArrayHasAlarm(deviceInfo.value("pa485Alarm"));
    const bool paUnderpowerAlarm = jsonArrayHasAlarm(deviceInfo.value("paUnderpowerAlarm"));
    const bool paOverpowerAlarm = jsonArrayHasAlarm(deviceInfo.value("paOverpowerAlarm"));

    updateAlarmLabel(fanStatusValue, fanAlarm);
    updateAlarmLabel(clockStatusValue, clockAlarm);
    updateAlarmLabel(receiverPllStatusValue, receiverPllAlarm);
    updateAlarmLabel(transmitterPllStatusValue, transmitterPllAlarm);
    updateAlarmLabel(eepromStatusValue, eepromAlarm);
    updateAlarmLabel(temperatureChipStatusValue, temperatureChipAlarm);
    updateAlarmLabel(compassStatusValue, compassAlarm);
    updateAlarmLabel(adcStatusValue, adcAlarm);
    updateAlarmLabel(pa485StatusValue, pa485Alarm);
    updateAlarmLabel(paUnderpowerStatusValue, paUnderpowerAlarm);
    updateAlarmLabel(paOverpowerStatusValue, paOverpowerAlarm);

    hasDeviceStatusData = true;
    hasAnyAlarm = fanAlarm || clockAlarm || receiverPllAlarm || transmitterPllAlarm || eepromAlarm ||
                  temperatureChipAlarm || compassAlarm || adcAlarm || pa485Alarm || paUnderpowerAlarm ||
                  paOverpowerAlarm;
    refreshStatusButtonState();
}

bool TopNavBar::eventFilter(QObject *watched, QEvent *event)
{
    // 【调试输出】：打印所有在导航栏中发生的鼠标按下事件
    // if (event->type() == QEvent::MouseButtonPress) {
    //     QPushButton* btn = qobject_cast<QPushButton*>(watched);
    //     if (btn) {
    //         qDebug() << "[TopNavBar] 捕获到鼠标【按下】:" << btn->text();
    //     }
    // } else if (event->type() == QEvent::MouseButtonRelease) {
    //     QPushButton* btn = qobject_cast<QPushButton*>(watched);
    //     if (btn) {
    //         qDebug() << "[TopNavBar] 捕获到鼠标【释放】:" << btn->text();
    //     }
    // }

    if (watched == lblSysStatus && event->type() == QEvent::MouseButtonPress)
    {
        if (timePopup)
        {
            if (timePopup->isVisible())
            {
                // 如果已经显示，则再次点击时隐藏
                timePopup->hide();
            }
            else
            {
                if (statusPopup && statusPopup->isVisible())
                {
                    statusPopup->hide();
                }
                // 确保弹窗的大小已计算
                timePopup->adjustSize();

                // 将弹窗定位在文字正下方，并确保它不会超出屏幕右侧边界
                // lblSysStatus 的右边缘坐标
                QPoint globalBottomRight =
                    lblSysStatus->mapToGlobal(QPoint(lblSysStatus->width(), lblSysStatus->height() + 2));

                // 弹窗的左上角 x 坐标 = 按钮的右边缘 - 弹窗的宽度
                int popupX = globalBottomRight.x() - timePopup->width();
                int popupY = globalBottomRight.y();

                timePopup->move(QPoint(popupX, popupY));
                timePopup->show();
            }
        }
        return true; // 拦截事件，防止继续传播
    }
    return QWidget::eventFilter(watched, event);
}
