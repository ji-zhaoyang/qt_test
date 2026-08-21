#include "stats_page.h"

#include "components/datetime_picker_popup.h"
#include "stats_web_bridge.h"

#include <QAbstractSpinBox>
#include <QCoreApplication>
#include <QDateTimeEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QTime>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineView>

namespace
{
QString primaryButtonStyle()
{
    return QStringLiteral("QPushButton { background-color: #f2994a; color: #ffffff; border: none; border-radius: 2px; "
                          "padding: 0 18px; font-size: 13px; font-weight: bold; min-height: 32px; }"
                          "QPushButton:hover { background-color: #f6a85f; }");
}

QString secondaryButtonStyle()
{
    return QStringLiteral("QPushButton { background-color: #1b1d22; color: #d6d7da; border: 1px solid #3b3e46; "
                          "border-radius: 2px; padding: 0 18px; font-size: 13px; font-weight: bold; min-height: 32px; }"
                          "QPushButton:hover { background-color: #252830; }");
}

QString flatInputStyle()
{
    return QStringLiteral("background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; border-radius: 2px; "
                          "padding: 0 10px; font-size: 13px;");
}

QString dateTimeEditStyle()
{
    return QStringLiteral("QDateTimeEdit { %1 }"
                          "QDateTimeEdit::drop-down { subcontrol-origin: padding; subcontrol-position: top right; "
                          "width: 28px; border-left: 1px solid #2a2a2a; }"
                          "QDateTimeEdit::down-arrow { image: none; }")
        .arg(flatInputStyle());
}
} // namespace

StatsPage::StatsPage(QWidget *parent) : QWidget(parent)
{
    setupUi();
    setupConnections();
    applyDefaultDateRange();
}

QDate StatsPage::defaultStartDate() const
{
    return QDate::currentDate().addDays(-3);
}

QDate StatsPage::defaultEndDate() const
{
    return QDate::currentDate();
}

void StatsPage::applyDefaultDateRange()
{
    if (!startDateEdit_ || !endDateEdit_)
    {
        return;
    }

    const QDate today = QDate::currentDate();
    startDateEdit_->setDateTime(QDateTime(defaultStartDate(), QTime(0, 0, 0)));
    endDateEdit_->setDateTime(QDateTime(today, QTime(23, 59, 59)));
}

QDateTimeEdit *StatsPage::createDateTimeEdit(QWidget *parent)
{
    QDateTimeEdit *edit = new QDateTimeEdit(parent);
    edit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    edit->setCalendarPopup(false);
    edit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    edit->setKeyboardTracking(false);
    edit->setMinimumDateTime(QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0)));
    edit->setFixedSize(142, 32);
    edit->setStyleSheet(dateTimeEditStyle());
    edit->setReadOnly(true);
    return edit;
}

void StatsPage::setupUi()
{
    setStyleSheet(QStringLiteral("background-color: #0e111f;"));

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->setSpacing(10);

    QWidget *toolbar = new QWidget(this);
    toolbar->setStyleSheet(QStringLiteral("background-color: #0e111f;"));
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(10);

    QLabel *rangeLabel = new QLabel(QStringLiteral("时间周期"), toolbar);
    rangeLabel->setStyleSheet(QStringLiteral("color: #d6dbe4; font-size: 13px;"));
    toolbarLayout->addWidget(rangeLabel);

    startDateEdit_ = createDateTimeEdit(toolbar);
    toolbarLayout->addWidget(startDateEdit_);

    QLabel *rangeSep = new QLabel(QStringLiteral("~"), toolbar);
    rangeSep->setStyleSheet(QStringLiteral("color: #9da3ad; font-size: 13px;"));
    toolbarLayout->addWidget(rangeSep);

    endDateEdit_ = createDateTimeEdit(toolbar);
    toolbarLayout->addWidget(endDateEdit_);

    timePicker_ = new DateTimePickerPopup(this, this);
    timePicker_->registerDateTimeEdit(startDateEdit_);
    timePicker_->registerDateTimeEdit(endDateEdit_);

    searchButton_ = new QPushButton(QStringLiteral("搜索"), toolbar);
    searchButton_->setStyleSheet(primaryButtonStyle());
    toolbarLayout->addWidget(searchButton_);

    resetButton_ = new QPushButton(QStringLiteral("重置"), toolbar);
    resetButton_->setStyleSheet(secondaryButtonStyle());
    toolbarLayout->addWidget(resetButton_);
    toolbarLayout->addStretch(1);

    layout->addWidget(toolbar);

    webView_ = new QWebEngineView(this);
    webBridge_ = new StatsWebBridge(webView_, this);
    layout->addWidget(webView_, 1);

    const QString htmlPath = QCoreApplication::applicationDirPath() + QStringLiteral("/assets/web/stats.html");
    webView_->setUrl(QUrl::fromLocalFile(htmlPath));
}

void StatsPage::setupConnections()
{
    connect(searchButton_, &QPushButton::clicked, this, &StatsPage::triggerSearch);
    connect(resetButton_, &QPushButton::clicked, this, &StatsPage::resetRequested);

    connect(webBridge_, &StatsWebBridge::searchRequested, this,
            [this](const QDate &startDate, const QDate &endDate)
            {
                if (startDateEdit_)
                {
                    startDateEdit_->setDateTime(QDateTime(startDate, QTime(0, 0, 0)));
                }
                if (endDateEdit_)
                {
                    endDateEdit_->setDateTime(QDateTime(endDate, QTime(23, 59, 59)));
                }
                emit searchRequested(startDate, endDate);
            });
    connect(webBridge_, &StatsWebBridge::resetRequested, this, &StatsPage::resetRequested);

    connect(webView_, &QWebEngineView::loadFinished, this,
            [this](bool ok)
            {
                pageLoaded_ = ok;
                if (ok)
                {
                    emit pageReady();
                }
            });

    connect(webView_, &QWebEngineView::titleChanged, this,
            [this](const QString &title)
            {
                if (webBridge_)
                {
                    webBridge_->handleTitleCommand(title);
                }
            });
}

void StatsPage::triggerSearch()
{
    if (!startDateEdit_ || !endDateEdit_)
    {
        return;
    }

    emit searchRequested(startDateEdit_->date(), endDateEdit_->date());
}

void StatsPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (timePicker_ && timePicker_->isVisible())
    {
        timePicker_->updatePosition();
    }
}

void StatsPage::setLoading(bool loading)
{
    if (webBridge_)
    {
        webBridge_->setLoading(loading);
    }
}

void StatsPage::publishModelStatistics(const QVector<DroneModelStat> &items)
{
    if (webBridge_)
    {
        webBridge_->sendModelStatistics(items);
    }
}

void StatsPage::publishTrackStatistics(const DroneTrackStatistics &stats)
{
    if (webBridge_)
    {
        webBridge_->sendTrackStatistics(stats);
    }
}

void StatsPage::publishTrackDailyStatistics(const QVector<DroneTrackDailyStat> &items, const QString &granularity)
{
    if (webBridge_)
    {
        webBridge_->sendTrackDailyStatistics(items, granularity);
    }
}

void StatsPage::publishCounterDailyStatistics(const QVector<CounterDailyStat> &items, const QString &granularity)
{
    if (webBridge_)
    {
        webBridge_->sendCounterDailyStatistics(items, granularity);
    }
}

void StatsPage::publishPlotStatistics(const QVector<DroneTrackPlotPoint> &items)
{
    if (webBridge_)
    {
        webBridge_->sendPlotStatistics(items);
    }
}
