#include "whitelist_repository.h"

#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTime>
#include <QVariant>

#include <QtMath>

namespace
{
QString currentStorageTime()
{
    return QDateTime::currentDateTime().toString(Qt::ISODate);
}

bool tableHasColumn(QSqlDatabase db, const QString &tableName, const QString &columnName)
{
    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(tableName)))
    {
        return false;
    }

    while (query.next())
    {
        if (query.value(1).toString() == columnName)
        {
            return true;
        }
    }

    return false;
}

WhitelistPage::WhitelistRecord readRecord(const QSqlQuery &query)
{
    WhitelistPage::WhitelistRecord record;
    record.id = query.value(0).toInt();
    record.serialNumber = query.value(1).toString();
    record.recordKey = query.value(2).toString();
    record.modelName = query.value(3).toString();
    record.remarks = query.value(4).toString();
    record.effectiveTime = query.value(5).toString();
    record.effectiveArea = query.value(6).toString();
    return record;
}

QString sqlTextValue(const QString &value)
{
    return value.isNull() ? QStringLiteral("") : value.trimmed();
}

bool hasValidCoordinate(double longitude, double latitude)
{
    if (qFuzzyIsNull(longitude) && qFuzzyIsNull(latitude))
    {
        return false;
    }

    return longitude >= -180.0 && longitude <= 180.0 && latitude >= -90.0 && latitude <= 90.0;
}

double haversineMeters(double lat1, double lon1, double lat2, double lon2)
{
    const double earthRadius = 6371000.0;
    const double dLat = qDegreesToRadians(lat2 - lat1);
    const double dLon = qDegreesToRadians(lon2 - lon1);
    const double a = qSin(dLat / 2.0) * qSin(dLat / 2.0) +
                     qCos(qDegreesToRadians(lat1)) * qCos(qDegreesToRadians(lat2)) * qSin(dLon / 2.0) * qSin(dLon / 2.0);
    const double c = 2.0 * qAsin(qSqrt(a));
    return earthRadius * c;
}

QDateTime parseEffectiveDateTime(const QString &value, bool endOfDayFallback)
{
    QDateTime dateTime = QDateTime::fromString(value, Qt::ISODate);
    if (dateTime.isValid())
    {
        return dateTime;
    }

    const QDate date = QDate::fromString(value, Qt::ISODate);
    if (!date.isValid())
    {
        return QDateTime();
    }

    return QDateTime(date, endOfDayFallback ? QTime(23, 59, 59) : QTime(0, 0, 0));
}

bool isEffectiveTime(const QString &value, const QDateTime &now)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty() || trimmed == QStringLiteral("permanent"))
    {
        return true;
    }

    const QStringList parts = trimmed.split(QLatin1Char('|'));
    if (parts.size() < 3 || parts.at(0) != QStringLiteral("range"))
    {
        return false;
    }

    const QDateTime start = parseEffectiveDateTime(parts.at(1), false);
    const QDateTime end = parseEffectiveDateTime(parts.at(2), true);
    if (!start.isValid() || !end.isValid())
    {
        return false;
    }

    return now >= start && now <= end;
}

bool pointInPolygon(double latitude, double longitude, const QJsonArray &points)
{
    if (points.size() < 3)
    {
        return false;
    }

    bool inside = false;
    int previousIndex = points.size() - 1;
    for (int index = 0; index < points.size(); ++index)
    {
        const QJsonArray current = points.at(index).toArray();
        const QJsonArray previous = points.at(previousIndex).toArray();
        if (current.size() < 2 || previous.size() < 2)
        {
            previousIndex = index;
            continue;
        }

        const double currentLat = current.at(0).toDouble();
        const double currentLng = current.at(1).toDouble();
        const double previousLat = previous.at(0).toDouble();
        const double previousLng = previous.at(1).toDouble();
        const bool intersects = ((currentLat > latitude) != (previousLat > latitude)) &&
                                (longitude < (previousLng - currentLng) * (latitude - currentLat) /
                                                     (previousLat - currentLat + 1e-12) +
                                                 currentLng);
        if (intersects)
        {
            inside = !inside;
        }
        previousIndex = index;
    }

    return inside;
}

bool isEffectiveArea(const QString &value, double latitude, double longitude)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty() || trimmed == QStringLiteral("unlimited"))
    {
        return true;
    }

    if (!hasValidCoordinate(longitude, latitude))
    {
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(trimmed.toUtf8(), &parseError);
    if (!document.isObject())
    {
        return false;
    }

    const QJsonObject shape = document.object();
    const QString type = shape.value(QStringLiteral("type")).toString();

    if (type == QStringLiteral("circle"))
    {
        const QJsonArray center = shape.value(QStringLiteral("center")).toArray();
        if (center.size() < 2)
        {
            return false;
        }

        const double centerLat = center.at(0).toDouble();
        const double centerLng = center.at(1).toDouble();
        const double radius = shape.value(QStringLiteral("radius")).toDouble();
        if (radius <= 0.0)
        {
            return false;
        }

        return haversineMeters(latitude, longitude, centerLat, centerLng) <= radius;
    }

    if (type == QStringLiteral("rectangle"))
    {
        const QJsonArray bounds = shape.value(QStringLiteral("bounds")).toArray();
        if (bounds.size() < 2)
        {
            return false;
        }

        const QJsonArray southWest = bounds.at(0).toArray();
        const QJsonArray northEast = bounds.at(1).toArray();
        if (southWest.size() < 2 || northEast.size() < 2)
        {
            return false;
        }

        const double minLat = qMin(southWest.at(0).toDouble(), northEast.at(0).toDouble());
        const double maxLat = qMax(southWest.at(0).toDouble(), northEast.at(0).toDouble());
        const double minLng = qMin(southWest.at(1).toDouble(), northEast.at(1).toDouble());
        const double maxLng = qMax(southWest.at(1).toDouble(), northEast.at(1).toDouble());
        return latitude >= minLat && latitude <= maxLat && longitude >= minLng && longitude <= maxLng;
    }

    if (type == QStringLiteral("polygon"))
    {
        return pointInPolygon(latitude, longitude, shape.value(QStringLiteral("points")).toArray());
    }

    return false;
}

bool isRecordEffectiveForTarget(const WhitelistPage::WhitelistRecord &record, const QJsonObject &targetInfo,
                                const QDateTime &now)
{
    if (!isEffectiveTime(record.effectiveTime, now))
    {
        return false;
    }

    const double latitude = targetInfo.value(QStringLiteral("latitude")).toDouble();
    const double longitude = targetInfo.value(QStringLiteral("longitude")).toDouble();
    return isEffectiveArea(record.effectiveArea, latitude, longitude);
}

QStringList targetLookupKeys(const QJsonObject &targetInfo)
{
    QStringList keys;

    const QString uniqueId = targetInfo.value(QStringLiteral("targetUniqueId")).toString().trimmed();
    if (!uniqueId.isEmpty())
    {
        keys.append(uniqueId);
    }

    const QString targetId = QString::number(targetInfo.value(QStringLiteral("targetId")).toInt());
    if (!targetId.isEmpty() && targetId != QStringLiteral("0"))
    {
        keys.append(targetId);
    }

    const QString targetName = targetInfo.value(QStringLiteral("targetName")).toString().trimmed();
    if (!targetName.isEmpty())
    {
        keys.append(targetName);
    }

    return keys;
}

bool findRecordByKeyOrSerial(QSqlDatabase db, const QString &key, WhitelistPage::WhitelistRecord *record)
{
    if (!record)
    {
        return false;
    }

    const QString trimmedKey = key.trimmed();
    if (trimmedKey.isEmpty())
    {
        return false;
    }

    QSqlQuery query(db);
    query.prepare(QStringLiteral(
        "SELECT id, serial_number, record_key, model_name, remarks, effective_time, effective_area "
        "FROM whitelist WHERE serial_number = ? OR record_key = ? LIMIT 1"));
    query.addBindValue(trimmedKey);
    query.addBindValue(trimmedKey);
    if (!query.exec() || !query.next())
    {
        return false;
    }

    *record = readRecord(query);
    return true;
}

bool findRecordForTarget(QSqlDatabase db, const QJsonObject &targetInfo, WhitelistPage::WhitelistRecord *record)
{
    const QStringList keys = targetLookupKeys(targetInfo);
    for (const QString &key : keys)
    {
        if (findRecordByKeyOrSerial(db, key, record))
        {
            return true;
        }
    }

    return false;
}
} // namespace

void WhitelistRepository::setLastError(const QSqlQuery &query) const
{
    lastErrorText_ = query.lastError().text();
}

QString WhitelistRepository::lastError() const
{
    return lastErrorText_;
}

bool WhitelistRepository::hasRequiredColumns(QSqlDatabase db) const
{
    static const QStringList requiredColumns = {QStringLiteral("id"),         QStringLiteral("serial_number"),
                                                QStringLiteral("record_key"), QStringLiteral("model_name"),
                                                QStringLiteral("remarks"),    QStringLiteral("effective_time"),
                                                QStringLiteral("effective_area"), QStringLiteral("created_at"),
                                                QStringLiteral("updated_at")};
    for (const QString &column : requiredColumns)
    {
        if (!tableHasColumn(db, QStringLiteral("whitelist"), column))
        {
            lastErrorText_ = QStringLiteral("缺少字段: %1").arg(column);
            return false;
        }
    }
    return true;
}

WhitelistRepository::WhitelistRepository(const QSqlDatabase &database, QObject *parent)
    : QObject(parent), connectionName_(database.connectionName())
{
}

bool WhitelistRepository::initialize()
{
    return ensureSchema();
}

bool WhitelistRepository::ensureSchema()
{
    QSqlDatabase db = database();
    if (!db.isOpen())
    {
        lastErrorText_ = QStringLiteral("数据库未打开");
        return false;
    }

    QSqlQuery query(db);

    const bool tableExists = query.exec(QStringLiteral(
                              "SELECT name FROM sqlite_master WHERE type='table' AND name='whitelist'")) &&
                           query.next();

    if (!tableExists)
    {
        if (!query.exec(QStringLiteral(
                "CREATE TABLE whitelist ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "serial_number TEXT NOT NULL UNIQUE,"
                "record_key TEXT NOT NULL DEFAULT '',"
                "model_name TEXT NOT NULL DEFAULT '',"
                "remarks TEXT NOT NULL DEFAULT '',"
                "effective_time TEXT NOT NULL DEFAULT 'permanent',"
                "effective_area TEXT NOT NULL DEFAULT 'unlimited',"
                "created_at TEXT NOT NULL,"
                "updated_at TEXT NOT NULL"
                ")")))
        {
            setLastError(query);
            return false;
        }
        lastErrorText_.clear();
        return true;
    }

    if (!tableHasColumn(db, QStringLiteral("whitelist"), QStringLiteral("id")))
    {
        if (!query.exec(QStringLiteral("ALTER TABLE whitelist RENAME TO whitelist_legacy")))
        {
            setLastError(query);
            return false;
        }

        if (!query.exec(QStringLiteral(
                "CREATE TABLE whitelist ("
                "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                "serial_number TEXT NOT NULL UNIQUE,"
                "record_key TEXT NOT NULL DEFAULT '',"
                "model_name TEXT NOT NULL DEFAULT '',"
                "remarks TEXT NOT NULL DEFAULT '',"
                "effective_time TEXT NOT NULL DEFAULT 'permanent',"
                "effective_area TEXT NOT NULL DEFAULT 'unlimited',"
                "created_at TEXT NOT NULL,"
                "updated_at TEXT NOT NULL"
                ")")))
        {
            setLastError(query);
            return false;
        }

        if (!query.exec(QStringLiteral(
                "INSERT INTO whitelist (serial_number, record_key, model_name, remarks, effective_time, effective_area, "
                "created_at, updated_at) "
                "SELECT CASE WHEN trim(serial_number) != '' THEN trim(serial_number) ELSE trim(record_key) END, "
                "CASE WHEN trim(record_key) != '' THEN trim(record_key) "
                "WHEN trim(serial_number) != '' THEN trim(serial_number) ELSE trim(record_key) END, "
                "COALESCE(model_name, ''), '', 'permanent', 'unlimited', "
                "COALESCE(created_at, datetime('now')), COALESCE(created_at, datetime('now')) "
                "FROM whitelist_legacy")))
        {
            const QString now = currentStorageTime();
            QSqlQuery fallback(db);
            fallback.prepare(QStringLiteral(
                "INSERT INTO whitelist (serial_number, record_key, model_name, remarks, effective_time, effective_area, "
                "created_at, updated_at) "
                "SELECT CASE WHEN trim(serial_number) != '' THEN trim(serial_number) ELSE trim(record_key) END, "
                "CASE WHEN trim(record_key) != '' THEN trim(record_key) "
                "WHEN trim(serial_number) != '' THEN trim(serial_number) ELSE trim(record_key) END, "
                "'', '', 'permanent', 'unlimited', ?, ? FROM whitelist_legacy"));
            fallback.addBindValue(now);
            fallback.addBindValue(now);
            if (!fallback.exec())
            {
                setLastError(fallback);
                qWarning() << "白名单旧表迁移失败:" << lastErrorText_;
            }
        }
        query.exec(QStringLiteral("DROP TABLE IF EXISTS whitelist_legacy"));
    }

    const auto ensureColumn = [&db](const QString &column, const QString &ddl) -> bool
    {
        if (tableHasColumn(db, QStringLiteral("whitelist"), column))
        {
            return true;
        }
        QSqlQuery alter(db);
        if (!alter.exec(ddl))
        {
            return false;
        }
        return true;
    };

    if (!ensureColumn(QStringLiteral("serial_number"),
                      QStringLiteral("ALTER TABLE whitelist ADD COLUMN serial_number TEXT NOT NULL DEFAULT ''")) ||
        !ensureColumn(QStringLiteral("record_key"),
                      QStringLiteral("ALTER TABLE whitelist ADD COLUMN record_key TEXT NOT NULL DEFAULT ''")) ||
        !ensureColumn(QStringLiteral("model_name"),
                      QStringLiteral("ALTER TABLE whitelist ADD COLUMN model_name TEXT NOT NULL DEFAULT ''")) ||
        !ensureColumn(QStringLiteral("remarks"),
                      QStringLiteral("ALTER TABLE whitelist ADD COLUMN remarks TEXT NOT NULL DEFAULT ''")) ||
        !ensureColumn(QStringLiteral("effective_time"),
                      QStringLiteral("ALTER TABLE whitelist ADD COLUMN effective_time TEXT NOT NULL DEFAULT 'permanent'")) ||
        !ensureColumn(QStringLiteral("effective_area"),
                      QStringLiteral(
                          "ALTER TABLE whitelist ADD COLUMN effective_area TEXT NOT NULL DEFAULT 'unlimited'")) ||
        !ensureColumn(QStringLiteral("created_at"),
                      QStringLiteral("ALTER TABLE whitelist ADD COLUMN created_at TEXT NOT NULL DEFAULT ''")) ||
        !ensureColumn(QStringLiteral("updated_at"),
                      QStringLiteral("ALTER TABLE whitelist ADD COLUMN updated_at TEXT NOT NULL DEFAULT ''")))
    {
        lastErrorText_ = QStringLiteral("白名单表字段升级失败");
        qWarning() << lastErrorText_;
        return false;
    }

    const QString now = currentStorageTime();
    QSqlQuery backfill(db);
    backfill.prepare(QStringLiteral("UPDATE whitelist SET created_at = ? WHERE trim(created_at) = ''"));
    backfill.addBindValue(now);
    backfill.exec();
    backfill.prepare(QStringLiteral("UPDATE whitelist SET updated_at = ? WHERE trim(updated_at) = ''"));
    backfill.addBindValue(now);
    backfill.exec();
    query.exec(QStringLiteral(
        "UPDATE whitelist SET serial_number = record_key WHERE trim(serial_number) = '' AND trim(record_key) != ''"));
    query.exec(QStringLiteral(
        "UPDATE whitelist SET record_key = serial_number WHERE trim(record_key) = '' AND trim(serial_number) != ''"));
    query.exec(QStringLiteral("UPDATE whitelist SET model_name = '' WHERE model_name IS NULL"));
    query.exec(QStringLiteral("UPDATE whitelist SET remarks = '' WHERE remarks IS NULL"));
    query.exec(QStringLiteral("UPDATE whitelist SET record_key = '' WHERE record_key IS NULL"));
    query.exec(QStringLiteral("UPDATE whitelist SET serial_number = '' WHERE serial_number IS NULL"));

    if (!hasRequiredColumns(db))
    {
        qWarning() << "白名单表结构不完整:" << lastErrorText_;
        return false;
    }

    lastErrorText_.clear();
    return true;
}

bool WhitelistRepository::contains(const QString &recordKey) const
{
    const QString key = recordKey.trimmed();
    if (key.isEmpty())
    {
        return false;
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT 1 FROM whitelist WHERE record_key = ? OR serial_number = ? LIMIT 1"));
    query.addBindValue(key);
    query.addBindValue(key);
    if (!query.exec())
    {
        return false;
    }

    return query.next();
}

bool WhitelistRepository::containsSerialNumber(const QString &serialNumber) const
{
    const QString serial = serialNumber.trimmed();
    if (serial.isEmpty())
    {
        return false;
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral("SELECT 1 FROM whitelist WHERE serial_number = ? LIMIT 1"));
    query.addBindValue(serial);
    if (!query.exec())
    {
        return false;
    }

    return query.next();
}

bool WhitelistRepository::containsForTarget(const QJsonObject &targetInfo) const
{
    WhitelistPage::WhitelistRecord record;
    if (!findRecordForTarget(database(), targetInfo, &record))
    {
        return false;
    }

    return isRecordEffectiveForTarget(record, targetInfo, QDateTime::currentDateTime());
}

QVector<WhitelistPage::WhitelistRecord> WhitelistRepository::queryPage(int page, int pageSize) const
{
    QVector<WhitelistPage::WhitelistRecord> records;
    if (page < 1 || pageSize < 1)
    {
        return records;
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT id, serial_number, record_key, model_name, remarks, effective_time, effective_area "
        "FROM whitelist ORDER BY id DESC LIMIT ? OFFSET ?"));
    query.addBindValue(pageSize);
    query.addBindValue((page - 1) * pageSize);
    if (!query.exec())
    {
        return records;
    }

    while (query.next())
    {
        records.append(readRecord(query));
    }

    return records;
}

int WhitelistRepository::countAll() const
{
    QSqlQuery query(database());
    if (!query.exec(QStringLiteral("SELECT COUNT(*) FROM whitelist")) || !query.next())
    {
        return 0;
    }

    return query.value(0).toInt();
}

bool WhitelistRepository::insertRecord(const WhitelistPage::WhitelistRecord &record)
{
    lastErrorText_.clear();
    const QString serialNumber = record.serialNumber.trimmed();
    if (serialNumber.isEmpty())
    {
        lastErrorText_ = QStringLiteral("序列号不能为空");
        return false;
    }

    if (containsSerialNumber(serialNumber))
    {
        lastErrorText_ = QStringLiteral("该序列号已在白名单中");
        return false;
    }

    const QString now = currentStorageTime();
    const QString recordKey = record.recordKey.trimmed().isEmpty() ? serialNumber : record.recordKey.trimmed();

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "INSERT INTO whitelist (serial_number, record_key, model_name, remarks, effective_time, effective_area, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
    query.addBindValue(serialNumber);
    query.addBindValue(recordKey);
    query.addBindValue(sqlTextValue(record.modelName));
    query.addBindValue(sqlTextValue(record.remarks));
    query.addBindValue(record.effectiveTime.trimmed().isEmpty() ? QStringLiteral("permanent") : record.effectiveTime.trimmed());
    query.addBindValue(record.effectiveArea.trimmed().isEmpty() ? QStringLiteral("unlimited") : record.effectiveArea.trimmed());
    query.addBindValue(now);
    query.addBindValue(now);
    if (!query.exec())
    {
        setLastError(query);
        qWarning() << "白名单新增失败:" << lastErrorText_ << "serial=" << serialNumber;
        return false;
    }

    emit changed();
    return true;
}

bool WhitelistRepository::updateRecord(const WhitelistPage::WhitelistRecord &record)
{
    if (record.id <= 0)
    {
        return false;
    }

    const QString serialNumber = record.serialNumber.trimmed();
    if (serialNumber.isEmpty())
    {
        return false;
    }

    const QString recordKey = record.recordKey.trimmed().isEmpty() ? serialNumber : record.recordKey.trimmed();

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "UPDATE whitelist SET serial_number = ?, record_key = ?, model_name = ?, remarks = ?, "
        "effective_time = ?, effective_area = ?, updated_at = ? WHERE id = ?"));
    query.addBindValue(serialNumber);
    query.addBindValue(recordKey);
    query.addBindValue(sqlTextValue(record.modelName));
    query.addBindValue(sqlTextValue(record.remarks));
    query.addBindValue(record.effectiveTime.trimmed().isEmpty() ? QStringLiteral("permanent") : record.effectiveTime.trimmed());
    query.addBindValue(record.effectiveArea.trimmed().isEmpty() ? QStringLiteral("unlimited") : record.effectiveArea.trimmed());
    query.addBindValue(currentStorageTime());
    query.addBindValue(record.id);
    if (!query.exec())
    {
        return false;
    }

    emit changed();
    return true;
}

bool WhitelistRepository::removeById(int id)
{
    if (id <= 0)
    {
        return false;
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral("DELETE FROM whitelist WHERE id = ?"));
    query.addBindValue(id);
    if (!query.exec())
    {
        return false;
    }

    emit changed();
    return true;
}

bool WhitelistRepository::findBySerialNumber(const QString &serialNumber, WhitelistPage::WhitelistRecord *record) const
{
    if (!record)
    {
        return false;
    }

    const QString serial = serialNumber.trimmed();
    if (serial.isEmpty())
    {
        return false;
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral(
        "SELECT id, serial_number, record_key, model_name, remarks, effective_time, effective_area "
        "FROM whitelist WHERE serial_number = ? LIMIT 1"));
    query.addBindValue(serial);
    if (!query.exec() || !query.next())
    {
        return false;
    }

    *record = readRecord(query);
    return true;
}

QSqlDatabase WhitelistRepository::database() const
{
    return QSqlDatabase::database(connectionName_);
}
