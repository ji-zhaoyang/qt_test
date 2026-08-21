#ifndef WHITELIST_REPOSITORY_H
#define WHITELIST_REPOSITORY_H

#include "views/whitelist/whitelist_page.h"

#include <QJsonObject>
#include <QObject>
#include <QString>

class QSqlDatabase;
class QSqlQuery;

class WhitelistRepository : public QObject
{
    Q_OBJECT

public:
    explicit WhitelistRepository(const QSqlDatabase &database, QObject *parent = nullptr);

    bool initialize();
    bool contains(const QString &recordKey) const;
    // 序列号命中 + 有效时间 + 有效区域（无人机坐标）均满足时返回 true
    bool containsForTarget(const QJsonObject &targetInfo) const;

    QVector<WhitelistPage::WhitelistRecord> queryPage(int page, int pageSize) const;
    int countAll() const;
    bool insertRecord(const WhitelistPage::WhitelistRecord &record);
    bool updateRecord(const WhitelistPage::WhitelistRecord &record);
    bool removeById(int id);
    bool findBySerialNumber(const QString &serialNumber, WhitelistPage::WhitelistRecord *record) const;
    QString lastError() const;

signals:
    void changed();

private:
    QSqlDatabase database() const;
    bool ensureSchema();
    bool containsSerialNumber(const QString &serialNumber) const;
    bool hasRequiredColumns(QSqlDatabase db) const;
    void setLastError(const QSqlQuery &query) const;

    QString connectionName_;
    mutable QString lastErrorText_;
};

#endif // WHITELIST_REPOSITORY_H
