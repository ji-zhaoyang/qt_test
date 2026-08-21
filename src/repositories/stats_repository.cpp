#include "stats_repository.h"

#include <QDateTime>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTime>
#include <QVariant>
#include <QtMath>

namespace
{
QString toRangeStartText(const QDate &date)
{
    return QDateTime(date, QTime(0, 0, 0)).toString(Qt::ISODate);
}

QString toRangeEndText(const QDate &date)
{
    return QDateTime(date, QTime(23, 59, 59)).toString(Qt::ISODate);
}

struct HourlyQueryContext
{
    QDateTime rangeStart;
    QDateTime rangeEnd;
    QVector<QDateTime> buckets;
};

QString hourBucketKey(const QDateTime &dateTime)
{
    return dateTime.toString(QStringLiteral("yyyyMMddHH"));
}

HourlyQueryContext buildHourlyContext(const StatsDateRange &range)
{
    HourlyQueryContext context;
    const QDate day = range.startDate;
    const bool isToday = day == QDate::currentDate();
    QDateTime bucketStart;

    if (isToday)
    {
        const QDateTime now = QDateTime::currentDateTime();
        bucketStart = QDateTime(now.addDays(-1).date(), QTime(now.time().hour(), 0, 0));
        context.rangeStart = QDateTime(day.addDays(-1), QTime(0, 0, 0));
        context.rangeEnd = QDateTime(day, QTime(23, 59, 59));
    }
    else
    {
        bucketStart = QDateTime(day, QTime(0, 0, 0));
        context.rangeStart = bucketStart;
        context.rangeEnd = QDateTime(day.addDays(1), QTime(0, 59, 59));
    }

    context.buckets.reserve(25);
    for (int index = 0; index < 25; ++index)
    {
        context.buckets.append(bucketStart.addSecs(index * 3600));
    }
    return context;
}
} // namespace

StatsRepository::StatsRepository(const QSqlDatabase &database, QObject *parent)
    : QObject(parent), connectionName_(database.connectionName())
{
}

bool StatsRepository::initialize()
{
    QSqlQuery query(database());
    return query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS counter_events ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "event_type TEXT NOT NULL,"
        "target_id INTEGER NOT NULL DEFAULT 0,"
        "success INTEGER NOT NULL DEFAULT 1,"
        "created_at TEXT NOT NULL"
        ")"));
}

bool StatsRepository::insertCounterEvent(const QString &eventType, quint32 targetId, bool success,
                                         const QDateTime &createdAt)
{
    QDateTime eventTime = createdAt.isValid() ? createdAt : QDateTime::currentDateTime();
    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "INSERT INTO counter_events (event_type, target_id, success, created_at) "
        "VALUES (:event_type, :target_id, :success, :created_at)"));
    query.bindValue(QStringLiteral(":event_type"), eventType);
    query.bindValue(QStringLiteral(":target_id"), targetId);
    query.bindValue(QStringLiteral(":success"), success ? 1 : 0);
    query.bindValue(QStringLiteral(":created_at"), eventTime.toString(Qt::ISODate));
    return query.exec();
}

QSqlDatabase StatsRepository::database() const
{
    return QSqlDatabase::database(connectionName_);
}

int StatsRepository::inclusiveDayCount(const StatsDateRange &range) const
{
    if (!range.startDate.isValid() || !range.endDate.isValid() || range.endDate < range.startDate)
    {
        return 0;
    }
    return range.startDate.daysTo(range.endDate) + 1;
}

bool StatsRepository::isSingleDayRange(const StatsDateRange &range) const
{
    return range.startDate.isValid() && range.endDate.isValid() && range.startDate == range.endDate;
}

QVector<DroneTrackDailyStat> StatsRepository::queryTrackHourlyStatistics(const StatsDateRange &range) const
{
    QVector<DroneTrackDailyStat> results;
    const QDate day = range.startDate;
    if (!day.isValid())
    {
        return results;
    }

    const HourlyQueryContext context = buildHourlyContext(range);
    QHash<QString, DroneTrackDailyStat> byKey;
    for (const QDateTime &bucket : context.buckets)
    {
        DroneTrackDailyStat item;
        item.bucketStart = bucket;
        byKey.insert(hourBucketKey(bucket), item);
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT strftime('%Y-%m-%dT%H:00:00', found_time) AS hour_key, "
        "COALESCE(SUM(stay_duration_seconds), 0) / 60.0 AS stay_minutes, "
        "COUNT(*) AS track_count "
        "FROM detection_history "
        "WHERE found_time >= :start_time AND found_time <= :end_time "
        "GROUP BY strftime('%Y-%m-%d %H', found_time) "
        "ORDER BY hour_key ASC"));
    query.bindValue(QStringLiteral(":start_time"), context.rangeStart.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":end_time"), context.rangeEnd.toString(Qt::ISODate));
    if (query.exec())
    {
        while (query.next())
        {
            const QDateTime bucket = QDateTime::fromString(query.value(0).toString(), Qt::ISODate);
            if (!bucket.isValid())
            {
                continue;
            }
            const QString key = hourBucketKey(bucket);
            if (!byKey.contains(key))
            {
                continue;
            }
            DroneTrackDailyStat item;
            item.bucketStart = bucket;
            item.stayTime = query.value(1).toDouble();
            item.trackCount = query.value(2).toInt();
            byKey.insert(key, item);
        }
    }

    for (const QDateTime &bucket : context.buckets)
    {
        results.append(byKey.value(hourBucketKey(bucket)));
    }
    return results;
}

QVector<CounterDailyStat> StatsRepository::queryCounterHourlyStatistics(const StatsDateRange &range) const
{
    QVector<CounterDailyStat> results;
    const QDate day = range.startDate;
    if (!day.isValid())
    {
        return results;
    }

    const HourlyQueryContext context = buildHourlyContext(range);
    QHash<QString, CounterDailyStat> byKey;
    for (const QDateTime &bucket : context.buckets)
    {
        CounterDailyStat item;
        item.bucketStart = bucket;
        byKey.insert(hourBucketKey(bucket), item);
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT strftime('%Y-%m-%dT%H:00:00', created_at) AS hour_key, COUNT(*) AS counter_count "
        "FROM counter_events "
        "WHERE success = 1 AND created_at >= :start_time AND created_at <= :end_time "
        "GROUP BY strftime('%Y-%m-%d %H', created_at) "
        "ORDER BY hour_key ASC"));
    query.bindValue(QStringLiteral(":start_time"), context.rangeStart.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":end_time"), context.rangeEnd.toString(Qt::ISODate));
    if (query.exec())
    {
        while (query.next())
        {
            const QDateTime bucket = QDateTime::fromString(query.value(0).toString(), Qt::ISODate);
            if (!bucket.isValid())
            {
                continue;
            }
            const QString key = hourBucketKey(bucket);
            if (!byKey.contains(key))
            {
                continue;
            }
            CounterDailyStat item;
            item.bucketStart = bucket;
            item.count = query.value(1).toInt();
            byKey.insert(key, item);
        }
    }

    for (const QDateTime &bucket : context.buckets)
    {
        results.append(byKey.value(hourBucketKey(bucket)));
    }
    return results;
}

QVector<DroneModelStat> StatsRepository::queryModelStatistics(const StatsDateRange &range) const
{
    QVector<DroneModelStat> results;
    if (!range.startDate.isValid() || !range.endDate.isValid())
    {
        return results;
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT model_name, COUNT(*) AS model_count "
        "FROM detection_history "
        "WHERE found_time >= :start_time AND found_time <= :end_time "
        "AND TRIM(model_name) != '' "
        "GROUP BY model_name "
        "ORDER BY model_count DESC"));
    query.bindValue(QStringLiteral(":start_time"), toRangeStartText(range.startDate));
    query.bindValue(QStringLiteral(":end_time"), toRangeEndText(range.endDate));
    if (!query.exec())
    {
        return results;
    }

    while (query.next())
    {
        DroneModelStat item;
        item.model = query.value(0).toString();
        item.count = query.value(1).toInt();
        results.append(item);
    }
    return results;
}

DroneTrackStatistics StatsRepository::queryTrackStatistics(const StatsDateRange &range) const
{
    DroneTrackStatistics stats;
    if (!range.startDate.isValid() || !range.endDate.isValid())
    {
        return stats;
    }

    const QString startTime = toRangeStartText(range.startDate);
    const QString endTime = toRangeEndText(range.endDate);
    const int dayCount = inclusiveDayCount(range);

    QSqlQuery totalQuery(database());
    totalQuery.prepare(QStringLiteral(
        "SELECT COUNT(*), COUNT(DISTINCT serial_number), "
        "COALESCE(SUM(stay_duration_seconds), 0), "
        "COALESCE(MAX(stay_duration_seconds), 0), "
        "COALESCE(AVG(stay_duration_seconds), 0) "
        "FROM detection_history "
        "WHERE found_time >= :start_time AND found_time <= :end_time"));
    totalQuery.bindValue(QStringLiteral(":start_time"), startTime);
    totalQuery.bindValue(QStringLiteral(":end_time"), endTime);
    if (totalQuery.exec() && totalQuery.next())
    {
        stats.trackTotalCount = totalQuery.value(0).toInt();
        stats.droneTotalCount = totalQuery.value(1).toInt();
        stats.totalStayTime = totalQuery.value(2).toLongLong() / 60.0;
        stats.maxStayTime = totalQuery.value(3).toLongLong() / 60.0;
        stats.avgStayTime = totalQuery.value(4).toDouble() / 60.0;
    }

    QSqlQuery trackDailyMaxQuery(database());
    trackDailyMaxQuery.prepare(QStringLiteral(
        "SELECT MAX(daily_count) FROM ("
        "  SELECT COUNT(*) AS daily_count "
        "  FROM detection_history "
        "  WHERE found_time >= :start_time AND found_time <= :end_time "
        "  GROUP BY date(found_time)"
        ")"));
    trackDailyMaxQuery.bindValue(QStringLiteral(":start_time"), startTime);
    trackDailyMaxQuery.bindValue(QStringLiteral(":end_time"), endTime);
    if (trackDailyMaxQuery.exec() && trackDailyMaxQuery.next())
    {
        stats.trackDailyMaxCount = trackDailyMaxQuery.value(0).toInt();
    }

    QSqlQuery droneDailyMaxQuery(database());
    droneDailyMaxQuery.prepare(QStringLiteral(
        "SELECT MAX(daily_count) FROM ("
        "  SELECT COUNT(DISTINCT serial_number) AS daily_count "
        "  FROM detection_history "
        "  WHERE found_time >= :start_time AND found_time <= :end_time "
        "  GROUP BY date(found_time)"
        ")"));
    droneDailyMaxQuery.bindValue(QStringLiteral(":start_time"), startTime);
    droneDailyMaxQuery.bindValue(QStringLiteral(":end_time"), endTime);
    if (droneDailyMaxQuery.exec() && droneDailyMaxQuery.next())
    {
        stats.droneDailyMaxCount = droneDailyMaxQuery.value(0).toInt();
    }

    if (dayCount > 0)
    {
        stats.trackDailyAvgCount = static_cast<double>(stats.trackTotalCount) / dayCount;
        stats.droneDailyAvgCount = static_cast<double>(stats.droneTotalCount) / dayCount;
    }

    return stats;
}

QVector<DroneTrackDailyStat> StatsRepository::queryTrackDailyStatistics(const StatsDateRange &range) const
{
    if (isSingleDayRange(range))
    {
        return queryTrackHourlyStatistics(range);
    }

    QVector<DroneTrackDailyStat> results;
    if (!range.startDate.isValid() || !range.endDate.isValid())
    {
        return results;
    }

    QHash<QDate, DroneTrackDailyStat> byDate;
    for (QDate day = range.startDate; day <= range.endDate; day = day.addDays(1))
    {
        DroneTrackDailyStat item;
        item.bucketStart = QDateTime(day, QTime(0, 0, 0));
        byDate.insert(day, item);
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT date(found_time) AS day_key, "
        "COALESCE(SUM(stay_duration_seconds), 0) / 60.0 AS stay_minutes, "
        "COUNT(*) AS track_count "
        "FROM detection_history "
        "WHERE found_time >= :start_time AND found_time <= :end_time "
        "GROUP BY date(found_time) "
        "ORDER BY day_key ASC"));
    query.bindValue(QStringLiteral(":start_time"), toRangeStartText(range.startDate));
    query.bindValue(QStringLiteral(":end_time"), toRangeEndText(range.endDate));
    if (query.exec())
    {
        while (query.next())
        {
            const QDate day = QDate::fromString(query.value(0).toString(), QStringLiteral("yyyy-MM-dd"));
            if (!day.isValid())
            {
                continue;
            }
            DroneTrackDailyStat item;
            item.bucketStart = QDateTime(day, QTime(0, 0, 0));
            item.stayTime = query.value(1).toDouble();
            item.trackCount = query.value(2).toInt();
            byDate.insert(day, item);
        }
    }

    for (QDate day = range.startDate; day <= range.endDate; day = day.addDays(1))
    {
        results.append(byDate.value(day));
    }
    return results;
}

QVector<DroneTrackPlotPoint> StatsRepository::queryPlotStatistics(const StatsDateRange &range) const
{
    QVector<DroneTrackPlotPoint> results;
    if (!range.startDate.isValid() || !range.endDate.isValid())
    {
        return results;
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT d.drone_latitude, d.drone_longitude, h.model_name, h.record_key, d.detected_at "
        "FROM detection_history_detail d "
        "INNER JOIN detection_history h ON h.record_key = d.record_key "
        "WHERE d.detected_at >= :start_time AND d.detected_at <= :end_time "
        "AND ABS(d.drone_latitude) > 0.000001 AND ABS(d.drone_longitude) > 0.000001 "
        "ORDER BY d.detected_at ASC "
        "LIMIT 5000"));
    query.bindValue(QStringLiteral(":start_time"), toRangeStartText(range.startDate));
    query.bindValue(QStringLiteral(":end_time"), toRangeEndText(range.endDate));
    if (!query.exec())
    {
        return results;
    }

    while (query.next())
    {
        DroneTrackPlotPoint point;
        point.lat = query.value(0).toDouble();
        point.lng = query.value(1).toDouble();
        point.droneModel = query.value(2).toString();
        point.targetId = query.value(3).toString();
        point.createTime = QDateTime::fromString(query.value(4).toString(), Qt::ISODate);
        results.append(point);
    }
    return results;
}

QVector<CounterDailyStat> StatsRepository::queryCounterDailyStatistics(const StatsDateRange &range) const
{
    if (isSingleDayRange(range))
    {
        return queryCounterHourlyStatistics(range);
    }

    QVector<CounterDailyStat> results;
    if (!range.startDate.isValid() || !range.endDate.isValid())
    {
        return results;
    }

    QHash<QDate, CounterDailyStat> byDate;
    for (QDate day = range.startDate; day <= range.endDate; day = day.addDays(1))
    {
        CounterDailyStat item;
        item.bucketStart = QDateTime(day, QTime(0, 0, 0));
        byDate.insert(day, item);
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT date(created_at) AS day_key, COUNT(*) AS counter_count "
        "FROM counter_events "
        "WHERE success = 1 AND created_at >= :start_time AND created_at <= :end_time "
        "GROUP BY date(created_at) "
        "ORDER BY day_key ASC"));
    query.bindValue(QStringLiteral(":start_time"), toRangeStartText(range.startDate));
    query.bindValue(QStringLiteral(":end_time"), toRangeEndText(range.endDate));
    if (query.exec())
    {
        while (query.next())
        {
            const QDate day = QDate::fromString(query.value(0).toString(), QStringLiteral("yyyy-MM-dd"));
            if (!day.isValid())
            {
                continue;
            }
            CounterDailyStat item;
            item.bucketStart = QDateTime(day, QTime(0, 0, 0));
            item.count = query.value(1).toInt();
            byDate.insert(day, item);
        }
    }

    for (QDate day = range.startDate; day <= range.endDate; day = day.addDays(1))
    {
        results.append(byDate.value(day));
    }
    return results;
}
