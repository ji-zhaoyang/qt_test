#include "system_log_page.h"
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

namespace
{
struct SystemLogEntry
{
    QString content;
    QString createdTime;
};
} // namespace

SystemLogPage::SystemLogPage(QWidget *parent) : QWidget(parent), clearLogButton(nullptr), exportLogButton(nullptr), pageSizeComboBox(nullptr)
{
    setupUi();
}

void SystemLogPage::setupUi()
{
    setObjectName("systemLogPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#systemLogPage { background-color: #202020; color: #ffffff; }");

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
    pageLayout->setContentsMargins(12, 10, 12, 18);
    pageLayout->setSpacing(12);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel *pageTitle = new QLabel(QStringLiteral("系统日志"), content);
    pageTitle->setStyleSheet(titleStyle());
    pageLayout->addWidget(pageTitle);

    QFrame *tableCard = new QFrame(content);
    tableCard->setStyleSheet("QFrame { background-color: #0d0d0d; border-radius: 6px; }");
    pageLayout->addWidget(tableCard);

    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);

    tableLayout->addWidget(createHeaderRow(tableCard));

    const QList<SystemLogEntry> rows = {
        {QStringLiteral("发现无人机Unknown Signal，距离核心点 0m，方位角 0°，中心频率 2527.9MHz"),
         QStringLiteral("05-22 09:44:06")},
        {QStringLiteral("发现无人机Unknown Signal，距离核心点 0m，方位角 0°，中心频率 2015.5MHz"),
         QStringLiteral("05-22 09:44:02")},
        {QStringLiteral("发现无人机Unknown Signal，距离核心点 0m，方位角 0°，中心频率 1739.8MHz"),
         QStringLiteral("05-22 09:43:56")},
        {QStringLiteral("发现无人机Unknown Signal，距离核心点 0m，方位角 0°，中心频率 1745.8MHz"),
         QStringLiteral("05-22 09:43:41")},
        {QStringLiteral("发现无人机Unknown Signal，距离核心点 0m，方位角 0°，中心频率 2580MHz"),
         QStringLiteral("05-22 09:43:35")},
        {QStringLiteral("发现无人机Unknown Signal，距离核心点 0m，方位角 0°，中心频率 1947.1MHz"),
         QStringLiteral("05-22 09:43:27")},
        {QStringLiteral("发现无人机Unknown Signal，距离核心点 0m，方位角 0°，中心频率 1943.3MHz"),
         QStringLiteral("05-22 09:43:15")},
        {QStringLiteral("发现无人机Unknown Signal，距离核心点 0m，方位角 0°，中心频率 2540.6MHz"),
         QStringLiteral("05-22 09:43:10")},
        {QStringLiteral("发现无人机Unknown Signal，距离核心点 0m，方位角 0°，中心频率 2609MHz"),
         QStringLiteral("05-22 09:42:48")},
        {QStringLiteral("发现无人机Unknown Signal，距离核心点 0m，方位角 0°，中心频率 1949.8MHz"),
         QStringLiteral("05-22 09:42:47")},
    };

    for (const SystemLogEntry &row : rows)
    {
        tableLayout->addWidget(createLogRow(tableCard, row.content, row.createdTime));
    }

    tableLayout->addWidget(createFooterRow(tableCard));
}

QWidget *SystemLogPage::createHeaderRow(QWidget *parent) const
{
    QWidget *headerRow = new QWidget(parent);
    headerRow->setStyleSheet("background-color: #151515; border-top-left-radius: 6px; border-top-right-radius: 6px;");

    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(14, 14, 14, 14);
    headerLayout->setSpacing(0);

    QLabel *contentLabel = new QLabel(QStringLiteral("内容"), headerRow);
    contentLabel->setStyleSheet(headerTextStyle() + QStringLiteral("padding-right: 14px;"));
    headerLayout->addWidget(contentLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);

    QLabel *timeLabel = new QLabel(QStringLiteral("创建时间"), headerRow);
    timeLabel->setFixedWidth(150);
    timeLabel->setStyleSheet(headerTextStyle() + QStringLiteral("padding-left: 14px; border-left: 1px solid #2c2c2c;"));
    headerLayout->addWidget(timeLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);

    return headerRow;
}

QWidget *SystemLogPage::createLogRow(QWidget *parent, const QString &contentText, const QString &timeText) const
{
    QWidget *row = new QWidget(parent);
    row->setStyleSheet("background-color: #0d0d0d; border-top: 1px solid #272727;");

    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(14, 16, 14, 16);
    rowLayout->setSpacing(0);

    QLabel *contentLabel = new QLabel(contentText, row);
    contentLabel->setWordWrap(true);
    contentLabel->setStyleSheet(contentTextStyle());
    rowLayout->addWidget(contentLabel, 1, Qt::AlignLeft | Qt::AlignVCenter);

    QLabel *timeLabel = new QLabel(timeText, row);
    timeLabel->setFixedWidth(150);
    timeLabel->setStyleSheet(timeTextStyle());
    rowLayout->addWidget(timeLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);

    return row;
}

QWidget *SystemLogPage::createFooterRow(QWidget *parent)
{
    QWidget *footerRow = new QWidget(parent);
    footerRow->setStyleSheet("background-color: #2a2d33; border-bottom-left-radius: 6px; border-bottom-right-radius: 6px;");

    QHBoxLayout *footerLayout = new QHBoxLayout(footerRow);
    footerLayout->setContentsMargins(12, 14, 12, 14);
    footerLayout->setSpacing(12);

    clearLogButton = createActionButton(footerRow, QStringLiteral("清空日志"));
    exportLogButton = createActionButton(footerRow, QStringLiteral("导出日志"));
    footerLayout->addWidget(clearLogButton, 0, Qt::AlignLeft | Qt::AlignVCenter);
    footerLayout->addWidget(exportLogButton, 0, Qt::AlignLeft | Qt::AlignVCenter);
    footerLayout->addStretch();

    QLabel *totalLabel = new QLabel(QStringLiteral("共 8576 条"), footerRow);
    totalLabel->setStyleSheet(paginationTextStyle());
    footerLayout->addWidget(totalLabel, 0, Qt::AlignVCenter);

    QLabel *leftArrow = new QLabel(QStringLiteral("‹"), footerRow);
    leftArrow->setStyleSheet(QStringLiteral("color: #7f8084; font-size: 20px; padding: 0 6px;"));
    footerLayout->addWidget(leftArrow, 0, Qt::AlignVCenter);

    footerLayout->addWidget(createPageButton(footerRow, QStringLiteral("1"), true));
    footerLayout->addWidget(createPageButton(footerRow, QStringLiteral("2")));
    footerLayout->addWidget(createPageButton(footerRow, QStringLiteral("3")));
    footerLayout->addWidget(createPageButton(footerRow, QStringLiteral("4")));
    footerLayout->addWidget(createPageButton(footerRow, QStringLiteral("5")));

    QLabel *ellipsisLabel = new QLabel(QStringLiteral("..."), footerRow);
    ellipsisLabel->setStyleSheet(paginationTextStyle());
    footerLayout->addWidget(ellipsisLabel, 0, Qt::AlignVCenter);

    footerLayout->addWidget(createPageButton(footerRow, QStringLiteral("858")));

    QLabel *rightArrow = new QLabel(QStringLiteral("›"), footerRow);
    rightArrow->setStyleSheet(QStringLiteral("color: #d6d7da; font-size: 20px; padding: 0 6px;"));
    footerLayout->addWidget(rightArrow, 0, Qt::AlignVCenter);

    pageSizeComboBox = new QComboBox(footerRow);
    pageSizeComboBox->addItems(QStringList() << QStringLiteral("10 条/页") << QStringLiteral("20 条/页")
                                             << QStringLiteral("50 条/页"));
    pageSizeComboBox->setFixedSize(106, 36);
    pageSizeComboBox->setStyleSheet("QComboBox { background-color: #0e0f12; color: #ffffff; border: 1px solid #323232; "
                                    "border-radius: 2px; padding-left: 10px; padding-right: 24px; font-size: 14px; }"
                                    "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 26px; "
                                    "border-left: 1px solid #2a2a2a; }"
                                    "QComboBox::down-arrow { image: none; }"
                                    "QComboBox QAbstractItemView { background-color: #202225; color: #ffffff; "
                                    "selection-background-color: #3a3a3a; border: 1px solid #2e2e2e; }");
    footerLayout->addWidget(pageSizeComboBox, 0, Qt::AlignVCenter);

    return footerRow;
}

QPushButton *SystemLogPage::createPageButton(QWidget *parent, const QString &text, bool active) const
{
    QPushButton *button = new QPushButton(text, parent);
    button->setFixedSize(34, 34);
    button->setStyleSheet(active ? QStringLiteral("QPushButton { background-color: #161616; color: #ffffff; border: 1px solid #d1771a; "
                                                  "border-radius: 6px; font-size: 14px; }")
                                 : QStringLiteral("QPushButton { background-color: transparent; color: #ffffff; border: none; "
                                                  "font-size: 14px; }"));
    return button;
}

QPushButton *SystemLogPage::createActionButton(QWidget *parent, const QString &text) const
{
    QPushButton *button = new QPushButton(text, parent);
    button->setFixedSize(222, 32);
    button->setStyleSheet("QPushButton { background-color: #f0f0f0; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #dfdfdf; }");
    return button;
}

QString SystemLogPage::titleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold;");
}

QString SystemLogPage::headerTextStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 14px; font-weight: bold;");
}

QString SystemLogPage::contentTextStyle() const
{
    return QStringLiteral("color: #f0f0f0; font-size: 14px;");
}

QString SystemLogPage::timeTextStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 14px; padding-left: 14px; border-left: 1px solid #232323;");
}

QString SystemLogPage::paginationTextStyle() const
{
    return QStringLiteral("color: #f2f2f2; font-size: 14px;");
}
