#include "alarm_history_page.h"
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QScrollArea>
#include <QVBoxLayout>

namespace
{
struct AlarmHistoryRow
{
    QString leftName;
    QString rightName;
};
} // namespace

AlarmHistoryPage::AlarmHistoryPage(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void AlarmHistoryPage::setupUi()
{
    setObjectName("alarmHistoryPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#alarmHistoryPage { background-color: #202020; color: #ffffff; }");

    QVBoxLayout *hostLayout = new QVBoxLayout(this);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }"
                              "QScrollBar:vertical { background: #1e1e1e; width: 10px; margin: 0px; }"
                              "QScrollBar::handle:vertical { background: #555555; min-height: 30px; border-radius: 4px; }"
                              "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
                              "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");
    hostLayout->addWidget(scrollArea);

    QWidget *content = new QWidget(scrollArea);
    content->setStyleSheet("background-color: #202020;");
    scrollArea->setWidget(content);

    QVBoxLayout *pageLayout = new QVBoxLayout(content);
    pageLayout->setContentsMargins(0, 10, 0, 18);
    pageLayout->setSpacing(10);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel *pageTitle = new QLabel(QStringLiteral("告警历史查询"), content);
    pageTitle->setStyleSheet(titleStyle());
    pageLayout->addWidget(pageTitle);

    QFrame *tableCard = new QFrame(content);
    tableCard->setStyleSheet("QFrame { background-color: #0f0f0f; border-radius: 0px; }");
    pageLayout->addWidget(tableCard);

    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);

    tableLayout->addWidget(createHeaderRow(tableCard));

    const QList<AlarmHistoryRow> rows = {
        {QStringLiteral("服务器连接状态"), QStringLiteral("设备温度")},
        {QStringLiteral("设备风扇告警状态"), QStringLiteral("设备时钟告警状态")},
        {QStringLiteral("设备接收锁相环告警状态"), QStringLiteral("设备发送锁相环告警状态")},
        {QStringLiteral("设备ADC芯片告警状态"), QStringLiteral("设备eeprom芯片告警状态")},
        {QStringLiteral("设备温度芯片告警状态"), QStringLiteral("设备电子罗盘芯片告警状态")},
        {QStringLiteral("PA1 串口告警状态"), QStringLiteral("PA2 串口告警状态")},
        {QStringLiteral("PA3 串口告警状态"), QStringLiteral("PA4 串口告警状态")},
        {QStringLiteral("PA5 串口告警状态"), QStringLiteral("PA6 串口告警状态")},
        {QStringLiteral("PA1 过功率告警状态"), QStringLiteral("PA1 过功率告警历史次数")},
        {QStringLiteral("PA2 过功率告警状态"), QStringLiteral("PA2 过功率告警历史次数")},
        {QStringLiteral("PA3 过功率告警状态"), QStringLiteral("PA3 过功率告警历史次数")},
        {QStringLiteral("PA4 过功率告警状态"), QStringLiteral("PA4 过功率告警历史次数")},
        {QStringLiteral("PA5 过功率告警状态"), QStringLiteral("PA5 过功率告警历史次数")},
        {QStringLiteral("PA6 过功率告警状态"), QStringLiteral("PA6 过功率告警历史次数")},
        {QStringLiteral("PA1 欠功率告警状态"), QStringLiteral("PA1 欠功率告警历史次数")},
        {QStringLiteral("PA2 欠功率告警状态"), QStringLiteral("PA2 欠功率告警历史次数")},
        {QStringLiteral("PA3 欠功率告警状态"), QStringLiteral("PA3 欠功率告警历史次数")},
        {QStringLiteral("PA4 欠功率告警状态"), QStringLiteral("PA4 欠功率告警历史次数")},
        {QStringLiteral("PA5 欠功率告警状态"), QStringLiteral("PA5 欠功率告警历史次数")},
        {QStringLiteral("PA6 欠功率告警状态"), QStringLiteral("PA6 欠功率告警历史次数")},
    };

    for (const AlarmHistoryRow &row : rows)
    {
        tableLayout->addWidget(createDataRow(tableCard, row.leftName, row.rightName));
    }

    pageLayout->addStretch();
}

void AlarmHistoryPage::updateAlarmHistory(const AlarmHistoryInfo &info)
{
    QStringList values;
    values << alarmStateText(info.serverConnectionStatus)
           << QString::number(info.deviceTemperature)
           << alarmStateText(info.fanAlarm)
           << alarmStateText(info.clockAlarm)
           << alarmStateText(info.receiverPllAlarm)
           << alarmStateText(info.transmitterPllAlarm)
           << alarmStateText(info.adcAlarm)
           << alarmStateText(info.eepromAlarm)
           << alarmStateText(info.temperatureChipAlarm)
           << alarmStateText(info.compassAlarm);

    for (int value : info.paSerialAlarms)
    {
        values << alarmStateText(value);
    }
    for (int i = 0; i < info.paOverpowerStatus.size(); ++i)
    {
        values << alarmStateText(info.paOverpowerStatus.at(i))
               << QString::number(i < info.paOverpowerCounts.size() ? info.paOverpowerCounts.at(i) : 0);
    }
    for (int i = 0; i < info.paUnderpowerStatus.size(); ++i)
    {
        values << alarmStateText(info.paUnderpowerStatus.at(i))
               << QString::number(i < info.paUnderpowerCounts.size() ? info.paUnderpowerCounts.at(i) : 0);
    }

    for (int i = 0; i < valueLabels.size() && i < values.size(); ++i)
    {
        valueLabels.at(i)->setText(values.at(i));
    }
}

QWidget *AlarmHistoryPage::createHeaderRow(QWidget *parent) const
{
    QWidget *headerRow = new QWidget(parent);
    headerRow->setStyleSheet("background-color: #151515;");

    QGridLayout *headerLayout = new QGridLayout(headerRow);
    headerLayout->setContentsMargins(18, 14, 18, 14);
    headerLayout->setHorizontalSpacing(14);
    headerLayout->setVerticalSpacing(0);
    headerLayout->setColumnStretch(0, 5);
    headerLayout->setColumnStretch(1, 1);
    headerLayout->setColumnStretch(2, 0);
    headerLayout->setColumnMinimumWidth(2, 1);
    headerLayout->setColumnStretch(3, 5);
    headerLayout->setColumnStretch(4, 1);

    auto createHeaderLabel = [this, headerRow](const QString &text)
    {
        QLabel *label = new QLabel(text, headerRow);
        label->setStyleSheet(headerTextStyle());
        return label;
    };

    QFrame *middleDivider = new QFrame(headerRow);
    middleDivider->setFrameShape(QFrame::VLine);
    middleDivider->setStyleSheet("background-color: #2f2f2f; max-width: 1px;");

    headerLayout->addWidget(createHeaderLabel(QStringLiteral("名称")), 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("数值")), 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
    headerLayout->addWidget(middleDivider, 0, 2, Qt::AlignCenter);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("名称")), 0, 3, Qt::AlignLeft | Qt::AlignVCenter);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("数值")), 0, 4, Qt::AlignLeft | Qt::AlignVCenter);

    return headerRow;
}

QWidget *AlarmHistoryPage::createDataRow(QWidget *parent, const QString &leftName, const QString &rightName)
{
    QWidget *row = new QWidget(parent);
    row->setStyleSheet("background-color: #0f0f0f; border-top: 1px solid #272727;");

    QGridLayout *rowLayout = new QGridLayout(row);
    rowLayout->setContentsMargins(18, 16, 18, 16);
    rowLayout->setHorizontalSpacing(14);
    rowLayout->setVerticalSpacing(0);
    rowLayout->setColumnStretch(0, 5);
    rowLayout->setColumnStretch(1, 1);
    rowLayout->setColumnStretch(2, 0);
    rowLayout->setColumnMinimumWidth(2, 1);
    rowLayout->setColumnStretch(3, 5);
    rowLayout->setColumnStretch(4, 1);

    auto createTextLabel = [row](const QString &text, const QString &style)
    {
        QLabel *label = new QLabel(text, row);
        label->setWordWrap(true);
        label->setStyleSheet(style);
        return label;
    };

    QFrame *middleDivider = new QFrame(row);
    middleDivider->setFrameShape(QFrame::VLine);
    middleDivider->setStyleSheet("background-color: #232323; max-width: 1px;");

    QLabel *leftValueLabel = createTextLabel(QStringLiteral("--"), valueTextStyle());
    QLabel *rightValueLabel = createTextLabel(QStringLiteral("--"), valueTextStyle());
    valueLabels.append(leftValueLabel);
    valueLabels.append(rightValueLabel);

    rowLayout->addWidget(createTextLabel(leftName, nameTextStyle()), 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(leftValueLabel, 0, 1, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(middleDivider, 0, 2, Qt::AlignCenter);
    rowLayout->addWidget(createTextLabel(rightName, nameTextStyle()), 0, 3, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(rightValueLabel, 0, 4, Qt::AlignLeft | Qt::AlignVCenter);

    return row;
}

QString AlarmHistoryPage::alarmStateText(int state) const
{
    return state == 0 ? QStringLiteral("正常") : QStringLiteral("告警");
}

QString AlarmHistoryPage::titleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold; padding-left: 24px;");
}

QString AlarmHistoryPage::headerTextStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 14px; font-weight: bold;");
}

QString AlarmHistoryPage::nameTextStyle() const
{
    return QStringLiteral("color: #f0f0f0; font-size: 14px;");
}

QString AlarmHistoryPage::valueTextStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 14px; font-weight: bold;");
}
