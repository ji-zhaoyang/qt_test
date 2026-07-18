#ifndef HISTORY_REPOSITORY_H
#define HISTORY_REPOSITORY_H

#include "views/history/history_page.h"

#include <QObject>

class QSqlDatabase;

class HistoryRepository : public QObject
{
    Q_OBJECT

public:
    explicit HistoryRepository(const QSqlDatabase &database, QObject *parent = nullptr);

    bool initialize();
    bool upsertRecord(const HistoryPage::HistoryRecord &record);
    bool appendDetailEntry(const HistoryPage::HistoryDetailEntry &detail);
    QVector<HistoryPage::HistoryRecord> queryAllRecords() const;
    QVector<HistoryPage::HistoryRecord> queryRecords(const HistoryPage::QueryCriteria &criteria) const;
    QVector<HistoryPage::HistoryDetailEntry> queryDetailEntries(const QString &recordKey) const;
    int countRecords(const HistoryPage::QueryCriteria &criteria) const;
    bool clearAllRecords();

private:
    QSqlDatabase database() const;

    QString connectionName_;
};

#endif // HISTORY_REPOSITORY_H
