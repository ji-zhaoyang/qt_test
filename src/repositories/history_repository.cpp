#include "history_repository.h"

#include <climits>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace
{
QString toStorageText(const QDateTime &dateTime)
{
    return dateTime.isValid() ? dateTime.toString(Qt::ISODate) : QString();
}

QDateTime fromStorageText(const QVariant &value)
{
    const QString text = value.toString().trimmed();
    if (text.isEmpty())
    {
        return QDateTime();
    }

    QDateTime dateTime = QDateTime::fromString(text, Qt::ISODate);
    if (!dateTime.isValid())
    {
        dateTime = QDateTime::fromString(text, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }
    return dateTime;
}

QString buildWhereClause(const HistoryPage::QueryCriteria &criteria, QVariantList *bindValues)
{
    QStringList clauses;

    if (!criteria.serialKeyword.trimmed().isEmpty())
    {
        clauses << QStringLiteral("serial_number LIKE ?");
        bindValues->append(QStringLiteral("%%1%").arg(criteria.serialKeyword.trimmed()));
    }

    if (!criteria.detectType.trimmed().isEmpty() && criteria.detectType != QStringLiteral("请选择"))
    {
        clauses << QStringLiteral("detect_type = ?");
        bindValues->append(criteria.detectType.trimmed());
    }

    if (criteria.startTime.isValid())
    {
        clauses << QStringLiteral("found_time >= ?");
        bindValues->append(toStorageText(criteria.startTime));
    }

    if (criteria.endTime.isValid())
    {
        clauses << QStringLiteral("found_time <= ?");
        bindValues->append(toStorageText(criteria.endTime));
    }

    return clauses.isEmpty() ? QString() : QStringLiteral(" WHERE %1").arg(clauses.join(QStringLiteral(" AND ")));
}
} // namespace

HistoryRepository::HistoryRepository(const QSqlDatabase &database, QObject *parent)
    : QObject(parent), connectionName_(database.connectionName())
{
}

bool HistoryRepository::initialize()
{
    QSqlQuery query(database());
    const bool summaryTableReady = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS detection_history ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "record_key TEXT NOT NULL UNIQUE,"
        "active INTEGER NOT NULL DEFAULT 1,"
        "detect_type TEXT NOT NULL DEFAULT '',"
        "model_name TEXT NOT NULL DEFAULT '',"
        "serial_number TEXT NOT NULL DEFAULT '',"
        "in_whitelist INTEGER NOT NULL DEFAULT 0,"
        "found_time TEXT NOT NULL,"
        "last_seen_time TEXT NOT NULL,"
        "center_frequency_khz REAL NOT NULL DEFAULT 0,"
        "pilot_longitude REAL NOT NULL DEFAULT 0,"
        "pilot_latitude REAL NOT NULL DEFAULT 0,"
        "azimuth_deg INTEGER NOT NULL DEFAULT 0,"
        "flight_altitude_meters INTEGER NOT NULL DEFAULT 0,"
        "stay_duration_seconds INTEGER NOT NULL DEFAULT 0"
        ")"));
    if (!summaryTableReady)
    {
        return false;
    }

    QSqlQuery detailQuery(database());
    return detailQuery.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS detection_history_detail ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "record_key TEXT NOT NULL,"
        "model_name TEXT NOT NULL DEFAULT '',"
        "serial_number TEXT NOT NULL DEFAULT '',"
        "center_frequency_khz REAL NOT NULL DEFAULT 0,"
        "drone_longitude REAL NOT NULL DEFAULT 0,"
        "drone_latitude REAL NOT NULL DEFAULT 0,"
        "pilot_longitude REAL NOT NULL DEFAULT 0,"
        "pilot_latitude REAL NOT NULL DEFAULT 0,"
        "azimuth_deg INTEGER NOT NULL DEFAULT 0,"
        "flight_altitude_meters INTEGER NOT NULL DEFAULT 0,"
        "distance_meters INTEGER NOT NULL DEFAULT 0,"
        "active INTEGER NOT NULL DEFAULT 1,"
        "detected_at TEXT NOT NULL"
        ")"));
}

bool HistoryRepository::upsertRecord(const HistoryPage::HistoryRecord &record)
{
    if (record.recordKey.trimmed().isEmpty())
    {
        return false;
    }

    auto bindRecordValues = [&record](QSqlQuery &query)
    {
        query.bindValue(QStringLiteral(":record_key"), record.recordKey);
        query.bindValue(QStringLiteral(":active"), record.active ? 1 : 0);
        query.bindValue(QStringLiteral(":detect_type"), record.detectType);
        query.bindValue(QStringLiteral(":model_name"), record.modelName);
        query.bindValue(QStringLiteral(":serial_number"), record.serialNumber);
        query.bindValue(QStringLiteral(":in_whitelist"), record.inWhitelist ? 1 : 0);
        query.bindValue(QStringLiteral(":found_time"), toStorageText(record.foundTime));
        query.bindValue(QStringLiteral(":last_seen_time"), toStorageText(record.lastSeenTime));
        query.bindValue(QStringLiteral(":center_frequency_khz"), record.centerFrequencyKhz);
        query.bindValue(QStringLiteral(":pilot_longitude"), record.pilotLongitude);
        query.bindValue(QStringLiteral(":pilot_latitude"), record.pilotLatitude);
        query.bindValue(QStringLiteral(":azimuth_deg"), record.azimuthDeg);
        query.bindValue(QStringLiteral(":flight_altitude_meters"), record.flightAltitudeMeters);
        query.bindValue(QStringLiteral(":stay_duration_seconds"), QVariant::fromValue<qlonglong>(record.stayDurationSeconds));
    };

    QSqlQuery updateQuery(database());
    updateQuery.prepare(QStringLiteral(
        "UPDATE detection_history SET "
        "active = :active, "
        "detect_type = :detect_type, "
        "model_name = :model_name, "
        "serial_number = :serial_number, "
        "in_whitelist = :in_whitelist, "
        "found_time = :found_time, "
        "last_seen_time = :last_seen_time, "
        "center_frequency_khz = :center_frequency_khz, "
        "pilot_longitude = :pilot_longitude, "
        "pilot_latitude = :pilot_latitude, "
        "azimuth_deg = :azimuth_deg, "
        "flight_altitude_meters = :flight_altitude_meters, "
        "stay_duration_seconds = :stay_duration_seconds "
        "WHERE record_key = :record_key"));
    bindRecordValues(updateQuery);
    if (!updateQuery.exec())
    {
        return false;
    }

    if (updateQuery.numRowsAffected() > 0)
    {
        return true;
    }

    QSqlQuery insertQuery(database());
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO detection_history ("
        "record_key, active, detect_type, model_name, serial_number, in_whitelist, "
        "found_time, last_seen_time, center_frequency_khz, pilot_longitude, pilot_latitude, "
        "azimuth_deg, flight_altitude_meters, stay_duration_seconds"
        ") VALUES ("
        ":record_key, :active, :detect_type, :model_name, :serial_number, :in_whitelist, "
        ":found_time, :last_seen_time, :center_frequency_khz, :pilot_longitude, :pilot_latitude, "
        ":azimuth_deg, :flight_altitude_meters, :stay_duration_seconds"
        ")"));
    bindRecordValues(insertQuery);
    if (!insertQuery.exec())
    {
        return false;
    }

    return true;
}

bool HistoryRepository::appendDetailEntry(const HistoryPage::HistoryDetailEntry &detail)
{
    if (detail.recordKey.trimmed().isEmpty() || !detail.detectedAt.isValid())
    {
        return false;
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "INSERT INTO detection_history_detail ("
        "record_key, model_name, serial_number, center_frequency_khz, drone_longitude, drone_latitude, "
        "pilot_longitude, pilot_latitude, azimuth_deg, flight_altitude_meters, distance_meters, active, detected_at"
        ") VALUES ("
        ":record_key, :model_name, :serial_number, :center_frequency_khz, :drone_longitude, :drone_latitude, "
        ":pilot_longitude, :pilot_latitude, :azimuth_deg, :flight_altitude_meters, :distance_meters, :active, :detected_at"
        ")"));
    query.bindValue(QStringLiteral(":record_key"), detail.recordKey);
    query.bindValue(QStringLiteral(":model_name"), detail.modelName);
    query.bindValue(QStringLiteral(":serial_number"), detail.serialNumber);
    query.bindValue(QStringLiteral(":center_frequency_khz"), detail.centerFrequencyKhz);
    query.bindValue(QStringLiteral(":drone_longitude"), detail.droneLongitude);
    query.bindValue(QStringLiteral(":drone_latitude"), detail.droneLatitude);
    query.bindValue(QStringLiteral(":pilot_longitude"), detail.pilotLongitude);
    query.bindValue(QStringLiteral(":pilot_latitude"), detail.pilotLatitude);
    query.bindValue(QStringLiteral(":azimuth_deg"), detail.azimuthDeg);
    query.bindValue(QStringLiteral(":flight_altitude_meters"), detail.flightAltitudeMeters);
    query.bindValue(QStringLiteral(":distance_meters"), detail.distanceMeters);
    query.bindValue(QStringLiteral(":active"), detail.active ? 1 : 0);
    query.bindValue(QStringLiteral(":detected_at"), toStorageText(detail.detectedAt));
    return query.exec();
}

QVector<HistoryPage::HistoryRecord> HistoryRepository::queryAllRecords() const
{
    HistoryPage::QueryCriteria criteria;
    criteria.page = 1;
    criteria.pageSize = INT_MAX;
    return queryRecords(criteria);
}

QVector<HistoryPage::HistoryRecord> HistoryRepository::queryRecords(const HistoryPage::QueryCriteria &criteria) const
{
    QVector<HistoryPage::HistoryRecord> records;

    QVariantList bindValues;
    const QString whereClause = buildWhereClause(criteria, &bindValues);
    QSqlQuery query(database());
    query.prepare(QStringLiteral(
                      "SELECT record_key, active, detect_type, model_name, serial_number, in_whitelist, "
                      "found_time, last_seen_time, center_frequency_khz, pilot_longitude, pilot_latitude, "
                      "azimuth_deg, flight_altitude_meters, stay_duration_seconds "
                      "FROM detection_history") +
                  whereClause +
                  QStringLiteral(" ORDER BY found_time DESC LIMIT ? OFFSET ?"));
    for (const QVariant &bindValue : bindValues)
    {
        query.addBindValue(bindValue);
    }

    const int safePageSize = qMax(1, criteria.pageSize);
    const int safePage = qMax(1, criteria.page);
    query.addBindValue(safePageSize);
    query.addBindValue((safePage - 1) * safePageSize);

    if (!query.exec())
    {
        return records;
    }

    while (query.next())
    {
        HistoryPage::HistoryRecord record;
        record.recordKey = query.value(0).toString();
        record.active = query.value(1).toInt() != 0;
        record.detectType = query.value(2).toString();
        record.modelName = query.value(3).toString();
        record.serialNumber = query.value(4).toString();
        record.inWhitelist = query.value(5).toInt() != 0;
        record.foundTime = fromStorageText(query.value(6));
        record.lastSeenTime = fromStorageText(query.value(7));
        record.centerFrequencyKhz = query.value(8).toDouble();
        record.pilotLongitude = query.value(9).toDouble();
        record.pilotLatitude = query.value(10).toDouble();
        record.azimuthDeg = query.value(11).toInt();
        record.flightAltitudeMeters = query.value(12).toInt();
        record.stayDurationSeconds = query.value(13).toLongLong();
        records.append(record);
    }

    return records;
}

QVector<HistoryPage::HistoryDetailEntry> HistoryRepository::queryDetailEntries(const QString &recordKey) const
{
    QVector<HistoryPage::HistoryDetailEntry> details;
    if (recordKey.trimmed().isEmpty())
    {
        return details;
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT record_key, model_name, serial_number, center_frequency_khz, drone_longitude, drone_latitude, "
        "pilot_longitude, pilot_latitude, azimuth_deg, flight_altitude_meters, distance_meters, active, detected_at "
        "FROM detection_history_detail WHERE record_key = ? ORDER BY detected_at DESC"));
    query.addBindValue(recordKey.trimmed());
    if (!query.exec())
    {
        return details;
    }

    while (query.next())
    {
        HistoryPage::HistoryDetailEntry detail;
        detail.recordKey = query.value(0).toString();
        detail.modelName = query.value(1).toString();
        detail.serialNumber = query.value(2).toString();
        detail.centerFrequencyKhz = query.value(3).toDouble();
        detail.droneLongitude = query.value(4).toDouble();
        detail.droneLatitude = query.value(5).toDouble();
        detail.pilotLongitude = query.value(6).toDouble();
        detail.pilotLatitude = query.value(7).toDouble();
        detail.azimuthDeg = query.value(8).toInt();
        detail.flightAltitudeMeters = query.value(9).toInt();
        detail.distanceMeters = query.value(10).toInt();
        detail.active = query.value(11).toInt() != 0;
        detail.detectedAt = fromStorageText(query.value(12));
        details.append(detail);
    }

    return details;
}

int HistoryRepository::countRecords(const HistoryPage::QueryCriteria &criteria) const
{
    QVariantList bindValues;
    const QString whereClause = buildWhereClause(criteria, &bindValues);

    QSqlQuery query(database());
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM detection_history") + whereClause);
    for (const QVariant &bindValue : bindValues)
    {
        query.addBindValue(bindValue);
    }

    if (!query.exec() || !query.next())
    {
        return 0;
    }

    return query.value(0).toInt();
}

bool HistoryRepository::clearAllRecords()
{
    QSqlDatabase db = database();
    if (!db.transaction())
    {
        return false;
    }

    QSqlQuery summaryQuery(db);
    if (!summaryQuery.exec(QStringLiteral("DELETE FROM detection_history")))
    {
        db.rollback();
        return false;
    }

    QSqlQuery detailQuery(db);
    if (!detailQuery.exec(QStringLiteral("DELETE FROM detection_history_detail")))
    {
        db.rollback();
        return false;
    }

    return db.commit();
}

QSqlDatabase HistoryRepository::database() const
{
    return QSqlDatabase::database(connectionName_);
}
