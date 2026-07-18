#ifndef HISTORY_COORDINATOR_H
#define HISTORY_COORDINATOR_H

#include "views/history/history_page.h"

#include <QObject>
#include <QDateTime>
#include <QHash>

class SettingsPage;
class TcpManager;
class QJsonObject;
class QTimer;
class HistoryRepository;

class HistoryCoordinator : public QObject
{
    Q_OBJECT

public:
    explicit HistoryCoordinator(HistoryPage *historyPage, SettingsPage *settingsPage, TcpManager *tcpManager,
                                HistoryRepository *historyRepository,
                                QObject *parent = nullptr);

    void setupConnections();
    void initializeState();

public slots:
    void handleDroneTargetReported(const QJsonObject &targetInfo);
    void markExpiredRecordsFinished();
    void setInactiveTimeoutSeconds(int seconds);
    void clearHistoryRecords();
    void showRecordDetails(const QString &recordKey);
    void showRecordReplay(const QString &recordKey);

private:
    struct TrackedRecord
    {
        HistoryPage::HistoryRecord record;
        QString baseRecordKey;
        QDateTime lastSeenAt;
    };

    QString buildBaseRecordKey(const QJsonObject &targetInfo) const;
    QString buildSessionRecordKey(const QString &baseRecordKey, const QDateTime &eventTime) const;
    QString extractBaseRecordKey(const QString &recordKey) const;
    QString resolveDetectType(const QJsonObject &targetInfo) const;
    QString resolveSerialNumber(const QJsonObject &targetInfo) const;
    QString resolveModelName(const QJsonObject &targetInfo) const;
    HistoryPage::HistoryDetailEntry buildDetailEntry(const QString &recordKey, const QJsonObject &targetInfo,
                                                     const QDateTime &eventTime, bool active) const;
    void upsertTrackedRecord(const QString &baseRecordKey, const QJsonObject &targetInfo, const QDateTime &eventTime);
    qint64 calculateStayDurationSeconds(const QDateTime &foundTime, const QDateTime &lastSeenTime,
                                        bool includeWarningRemoveTime) const;
    void queryHistoryPage();
    void reloadHistoryPage();
    void persistAndPublishRecord(const HistoryPage::HistoryRecord &record);
    void rebuildTrackedRecords(const QVector<HistoryPage::HistoryRecord> &records);

    HistoryPage *historyPage_;
    SettingsPage *settingsPage_;
    TcpManager *tcpManager_;
    HistoryRepository *historyRepository_;
    QTimer *expiryCheckTimer_ = nullptr;
    int inactiveTimeoutSeconds_ = 20;
    QHash<QString, TrackedRecord> trackedRecords_;
    QHash<QString, QString> activeSessionKeysByBase_;
};

#endif // HISTORY_COORDINATOR_H
