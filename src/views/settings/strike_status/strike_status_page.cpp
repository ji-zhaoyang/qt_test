#include "strike_status_page.h"
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

StrikeStatusPage::StrikeStatusPage(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void StrikeStatusPage::updateStrikeStatus(const QVector<int> &switchStates)
{
    for (int i = 0; i < stateLabels.size(); ++i)
    {
        QLabel *stateLabel = stateLabels.at(i);
        if (!stateLabel)
        {
            continue;
        }

        const bool enabled = i < switchStates.size() && switchStates.at(i) == 1;
        stateLabel->setText(enabled ? QStringLiteral("开启") : QStringLiteral("关闭"));
        stateLabel->setStyleSheet(stateBadgeStyle(enabled));
    }
}

void StrikeStatusPage::setupUi()
{
    setObjectName("strikeStatusPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#strikeStatusPage { background-color: #202020; color: #ffffff; }");

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
    pageLayout->setContentsMargins(24, 10, 24, 18);
    pageLayout->setSpacing(12);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel *pageTitle = new QLabel(QStringLiteral("打击状态"), content);
    pageTitle->setStyleSheet(titleStyle());
    pageLayout->addWidget(pageTitle);

    QWidget *panelsContainer = new QWidget(content);
    panelsContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout *panelsLayout = new QHBoxLayout(panelsContainer);
    panelsLayout->setContentsMargins(0, 0, 0, 0);
    panelsLayout->setSpacing(14);
    panelsLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    panelsLayout->addWidget(createStatusPanel(panelsContainer, 1, 6), 0, Qt::AlignTop);
    panelsLayout->addWidget(createStatusPanel(panelsContainer, 7, 12), 0, Qt::AlignTop);
    pageLayout->addWidget(panelsContainer);
    pageLayout->addStretch();
}

QWidget *StrikeStatusPage::createStatusPanel(QWidget *parent, int startIndex, int endIndex)
{
    QFrame *panel = new QFrame(parent);
    panel->setStyleSheet("QFrame { background-color: #2b2b2b; border-radius: 6px; }");
    panel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(18, 16, 18, 16);
    panelLayout->setSpacing(0);

    QLabel *panelTitle = new QLabel(QStringLiteral("打击通道状态"), panel);
    panelTitle->setStyleSheet(panelTitleStyle());
    panelLayout->addWidget(panelTitle);

    QFrame *divider = new QFrame(panel);
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("background-color: #404040; max-height: 1px;");
    panelLayout->addWidget(divider);

    for (int channelIndex = startIndex; channelIndex <= endIndex; ++channelIndex)
    {
        panelLayout->addWidget(createStatusRow(panel, channelIndex));
    }

    return panel;
}

QWidget *StrikeStatusPage::createStatusRow(QWidget *parent, int channelIndex)
{
    QWidget *row = new QWidget(parent);
    row->setStyleSheet("background-color: transparent; border-bottom: 1px solid #3a3a3a;");

    QGridLayout *rowLayout = new QGridLayout(row);
    rowLayout->setContentsMargins(0, 10, 0, 10);
    rowLayout->setHorizontalSpacing(8);
    rowLayout->setVerticalSpacing(0);

    QLabel *channelLabel = new QLabel(QStringLiteral("%1路").arg(channelIndex), row);
    channelLabel->setStyleSheet(channelTextStyle());
    rowLayout->addWidget(channelLabel, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);

    QLabel *stateLabel = new QLabel(QStringLiteral("关闭"), row);
    stateLabel->setAlignment(Qt::AlignCenter);
    stateLabel->setMinimumSize(66, 28);
    stateLabel->setStyleSheet(stateBadgeStyle(false));
    rowLayout->addWidget(stateLabel, 0, 1, Qt::AlignRight | Qt::AlignVCenter);
    stateLabels.append(stateLabel);

    return row;
}

QString StrikeStatusPage::titleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold; padding-left: 2px;");
}

QString StrikeStatusPage::panelTitleStyle() const
{
    return QStringLiteral("color: #eaeaea; font-size: 14px; font-weight: bold;");
}

QString StrikeStatusPage::channelTextStyle() const
{
    return QStringLiteral("color: #f2f2f2; font-size: 14px;");
}

QString StrikeStatusPage::stateBadgeStyle(bool enabled) const
{
    return enabled ? QStringLiteral("QLabel { background-color: #2f9e44; color: #ffffff; border-radius: 2px; "
                                    "font-size: 14px; font-weight: bold; padding: 4px 14px; }")
                   : QStringLiteral("QLabel { background-color: #b43d36; color: #ffffff; border-radius: 2px; "
                                    "font-size: 14px; font-weight: bold; padding: 4px 14px; }");
}
