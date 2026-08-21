#ifndef STATS_PAGE_H
#define STATS_PAGE_H

#include "repositories/stats_repository.h"

#include <QDate>
#include <QWidget>

class QDateTimeEdit;
class QPushButton;
class QResizeEvent;
class QWebEngineView;
class DateTimePickerPopup;
class StatsWebBridge;

class StatsPage : public QWidget
{
    Q_OBJECT

  public:
    explicit StatsPage(QWidget *parent = nullptr);

    QDate defaultStartDate() const;
    QDate defaultEndDate() const;
    void applyDefaultDateRange();

    void setLoading(bool loading);
    void publishModelStatistics(const QVector<DroneModelStat> &items);
    void publishTrackStatistics(const DroneTrackStatistics &stats);
    void publishTrackDailyStatistics(const QVector<DroneTrackDailyStat> &items, const QString &granularity = QStringLiteral("day"));
    void publishCounterDailyStatistics(const QVector<CounterDailyStat> &items, const QString &granularity = QStringLiteral("day"));
    void publishPlotStatistics(const QVector<DroneTrackPlotPoint> &items);

  signals:
    void searchRequested(const QDate &startDate, const QDate &endDate);
    void resetRequested();
    void pageReady();

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private:
    QDateTimeEdit *createDateTimeEdit(QWidget *parent);
    void setupUi();
    void setupConnections();
    void triggerSearch();

    QDateTimeEdit *startDateEdit_ = nullptr;
    QDateTimeEdit *endDateEdit_ = nullptr;
    QPushButton *searchButton_ = nullptr;
    QPushButton *resetButton_ = nullptr;
    QWebEngineView *webView_ = nullptr;
    StatsWebBridge *webBridge_ = nullptr;
    DateTimePickerPopup *timePicker_ = nullptr;
    bool pageLoaded_ = false;
};

#endif // STATS_PAGE_H
