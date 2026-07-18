#include "database_manager.h"

#include <QDir>
#include <QSqlDatabase>
#include <QStandardPaths>
#include <QUuid>

namespace
{
QString resolveDatabaseDirectory()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (path.isEmpty())
    {
        path = QDir::currentPath();
    }
    return path;
}
} // namespace

DatabaseManager::DatabaseManager(QObject *parent) : QObject(parent)
{
    QString uuidText = QUuid::createUuid().toString();
    uuidText.remove(QLatin1Char('{'));
    uuidText.remove(QLatin1Char('}'));
    connectionName_ = QStringLiteral("history-db-%1").arg(uuidText);
}

DatabaseManager::~DatabaseManager()
{
    if (!connectionName_.isEmpty() && QSqlDatabase::contains(connectionName_))
    {
        QSqlDatabase db = QSqlDatabase::database(connectionName_);
        if (db.isOpen())
        {
            db.close();
        }
    }
}

bool DatabaseManager::initialize()
{
    const QString directoryPath = resolveDatabaseDirectory();
    QDir directory(directoryPath);
    if (!directory.exists() && !directory.mkpath(QStringLiteral(".")))
    {
        return false;
    }

    databasePath_ = directory.filePath(QStringLiteral("history.db"));
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    db.setDatabaseName(databasePath_);
    return db.open();
}

QSqlDatabase DatabaseManager::database() const
{
    return QSqlDatabase::database(connectionName_);
}

QString DatabaseManager::databasePath() const
{
    return databasePath_;
}

QString DatabaseManager::connectionName() const
{
    return connectionName_;
}
