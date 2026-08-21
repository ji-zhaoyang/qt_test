#ifndef STATS_REPOSITORY_H
#define STATS_REPOSITORY_H

#include <QDate>
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVector>

class QSqlDatabase;

struct StatsDateRange
{
    QDate startDate;
    QDate endDate;
};

struct DroneModelStat
{
    QString model;
    int count = 0;
};

struct DroneTrackStatistics
{
    int trackTotalCount = 0;
    int droneTotalCount = 0;
    int trackDailyMaxCount = 0;
    int droneDailyMaxCount = 0;
    double trackDailyAvgCount = 0.0;
    double droneDailyAvgCount = 0.0;
    double totalStayTime = 0.0;
    double maxStayTime = 0.0;
    double avgStayTime = 0.0;
};

struct DroneTrackDailyStat
{
    QDateTime bucketStart;
    double stayTime = 0.0;
    int trackCount = 0;
};

struct CounterDailyStat
{
    QDateTime bucketStart;
    int count = 0;
};

struct DroneTrackPlotPoint
{
    double lat = 0.0;
    double lng = 0.0;
    QString droneModel;
    QString targetId;
    QDateTime createTime;
};

class StatsRepository : public QObject
{
    Q_OBJECT

  public:
    explicit StatsRepository(const QSqlDatabase &database, QObject *parent = nullptr);

    bool initialize();
    bool insertCounterEvent(const QString &eventType, quint32 targetId, bool success, const QDateTime &createdAt = QDateTime());

    QVector<DroneModelStat> queryModelStatistics(const StatsDateRange &range) const;
    DroneTrackStatistics queryTrackStatistics(const StatsDateRange &range) const;
    QVector<DroneTrackDailyStat> queryTrackDailyStatistics(const StatsDateRange &range) const;
    QVector<CounterDailyStat> queryCounterDailyStatistics(const StatsDateRange &range) const;
    QVector<DroneTrackPlotPoint> queryPlotStatistics(const StatsDateRange &range) const;

  private:
    QSqlDatabase database() const;
    int inclusiveDayCount(const StatsDateRange &range) const;
    bool isSingleDayRange(const StatsDateRange &range) const;
    QVector<DroneTrackDailyStat> queryTrackHourlyStatistics(const StatsDateRange &range) const;
    QVector<CounterDailyStat> queryCounterHourlyStatistics(const StatsDateRange &range) const;

    QString connectionName_;
};

#endif // STATS_REPOSITORY_H
