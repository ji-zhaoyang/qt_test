#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <QObject>
#include <QString>

class QSqlDatabase;

class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseManager(QObject *parent = nullptr);
    ~DatabaseManager() override;

    bool initialize();
    QSqlDatabase database() const;
    QString databasePath() const;
    QString connectionName() const;

private:
    QString connectionName_;
    QString databasePath_;
};

#endif // DATABASE_MANAGER_H
