#include "history_coordinator.h"

#include "network/core/tcp_manager.h"
#include "repositories/history_repository.h"
#include "views/settings/settings_page.h"
#include "views/history/history_page.h"

#include <algorithm>
#include <QDebug>
#include <QJsonObject>
#include <QTimer>

namespace
{
constexpr int kHistoryExpiryCheckIntervalMs = 2000;
constexpr int kDefaultHistoryInactiveTimeoutSeconds = 20;
const QChar kHistorySessionSeparator = QLatin1Char('|');
} // namespace

HistoryCoordinator::HistoryCoordinator(HistoryPage *historyPage, SettingsPage *settingsPage, TcpManager *tcpManager,
                                       HistoryRepository *historyRepository, QObject *parent)
    : QObject(parent), historyPage_(historyPage), settingsPage_(settingsPage), tcpManager_(tcpManager),
      historyRepository_(historyRepository), inactiveTimeoutSeconds_(kDefaultHistoryInactiveTimeoutSeconds)
{
    expiryCheckTimer_ = new QTimer(this);
    expiryCheckTimer_->setInterval(kHistoryExpiryCheckIntervalMs);
    connect(expiryCheckTimer_, &QTimer::timeout,
            this, &HistoryCoordinator::markExpiredRecordsFinished);
}

void HistoryCoordinator::setupConnections()
{
    if (!historyPage_ || !tcpManager_)
    {
        return;
    }

    connect(tcpManager_, &TcpManager::droneTargetReported,
            this, &HistoryCoordinator::handleDroneTargetReported);
    connect(historyPage_, &HistoryPage::historyQueryRequested,
            this, &HistoryCoordinator::queryHistoryPage);
    connect(historyPage_, &HistoryPage::clearRecordsRequested,
            this, &HistoryCoordinator::clearHistoryRecords);
    connect(historyPage_, &HistoryPage::detailRequested,
            this, &HistoryCoordinator::showRecordDetails);
    connect(historyPage_, &HistoryPage::replayRequested,
            this, &HistoryCoordinator::showRecordReplay);
    if (settingsPage_)
    {
        connect(settingsPage_, &SettingsPage::warningRemoveTimeChanged,
                this, &HistoryCoordinator::setInactiveTimeoutSeconds);
    }

    if (expiryCheckTimer_)
    {
        expiryCheckTimer_->start();
    }
}

void HistoryCoordinator::initializeState()
{
    if (!historyPage_)
    {
        return;
    }

    if (historyRepository_)
    {
        rebuildTrackedRecords(historyRepository_->queryAllRecords());
    }
    else
    {
        trackedRecords_.clear();
        activeSessionKeysByBase_.clear();
    }
    if (settingsPage_)
    {
        setInactiveTimeoutSeconds(settingsPage_->currentWarningRemoveTimeSeconds());
    }
    reloadHistoryPage();
    markExpiredRecordsFinished();
}

void HistoryCoordinator::handleDroneTargetReported(const QJsonObject &targetInfo)
{
    if (!historyPage_)
    {
        return;
    }

    const QString baseRecordKey = buildBaseRecordKey(targetInfo);
    if (baseRecordKey.isEmpty())
    {
        return;
    }

    // 从协议中解析时间戳
    const QDateTime eventTime = QDateTime::currentDateTime();

    if (targetInfo.value(QStringLiteral("disappeared")).toBool())
    {
        const QString sessionRecordKey = activeSessionKeysByBase_.value(baseRecordKey).trimmed();
        auto it = trackedRecords_.find(sessionRecordKey);
        if (sessionRecordKey.isEmpty() || it == trackedRecords_.end())
        {
            return;
        }

        if (eventTime.isValid())
        {
            it->lastSeenAt = eventTime;
            it->record.lastSeenTime = it->lastSeenAt;
            it->record.stayDurationSeconds = calculateStayDurationSeconds(it->record.foundTime, it->lastSeenAt, true);
        }
        it->record.active = false;
        activeSessionKeysByBase_.remove(baseRecordKey);
        persistAndPublishRecord(it->record);
        return;
    }

    upsertTrackedRecord(baseRecordKey, targetInfo, eventTime);
}

void HistoryCoordinator::markExpiredRecordsFinished()
{
    if (!historyPage_ || trackedRecords_.isEmpty())
    {
        return;
    }

    const QDateTime now = QDateTime::currentDateTime();
    for (auto it = trackedRecords_.begin(); it != trackedRecords_.end(); ++it)
    {
        if (!it->record.active || !it->lastSeenAt.isValid())
        {
            continue;
        }

        if (it->lastSeenAt.secsTo(now) < inactiveTimeoutSeconds_)
        {
            continue;
        }

        it->record.active = false;
        it->record.stayDurationSeconds = calculateStayDurationSeconds(it->record.foundTime, it->lastSeenAt, true);
        activeSessionKeysByBase_.remove(it->baseRecordKey);
        persistAndPublishRecord(it->record);
    }
}

void HistoryCoordinator::setInactiveTimeoutSeconds(int seconds)
{
    inactiveTimeoutSeconds_ = qMax(0, seconds);
    qDebug().noquote() << QStringLiteral("[WarningRemoveTime][History] seconds=%1").arg(inactiveTimeoutSeconds_);
}

qint64 HistoryCoordinator::calculateStayDurationSeconds(const QDateTime &foundTime, const QDateTime &lastSeenTime,
                                                        bool includeWarningRemoveTime) const
{
    const qint64 detectSeconds = foundTime.isValid() && lastSeenTime.isValid()
                                     ? qMax<qint64>(0, foundTime.secsTo(lastSeenTime))
                                     : 0;
    return includeWarningRemoveTime ? detectSeconds + qMax(0, inactiveTimeoutSeconds_) : detectSeconds;
}

void HistoryCoordinator::clearHistoryRecords()
{
    trackedRecords_.clear();
    activeSessionKeysByBase_.clear();
    if (historyRepository_)
    {
        historyRepository_->clearAllRecords();
    }
    if (historyPage_)
    {
        historyPage_->clearRecords();
    }
}

void HistoryCoordinator::showRecordDetails(const QString &recordKey)
{
    if (!historyPage_ || recordKey.trimmed().isEmpty())
    {
        return;
    }

    QVector<HistoryPage::HistoryDetailEntry> details;
    if (historyRepository_)
    {
        details = historyRepository_->queryDetailEntries(recordKey);
    }

    historyPage_->showDetailDialog(recordKey, details);
}

void HistoryCoordinator::showRecordReplay(const QString &recordKey)
{
    if (!historyPage_ || recordKey.trimmed().isEmpty())
    {
        return;
    }

    QVector<HistoryPage::HistoryDetailEntry> details;
    if (historyRepository_)
    {
        details = historyRepository_->queryDetailEntries(recordKey);
    }

    historyPage_->showReplayDialog(recordKey, details);
}

void HistoryCoordinator::queryHistoryPage()
{
    reloadHistoryPage();
}

QString HistoryCoordinator::buildBaseRecordKey(const QJsonObject &targetInfo) const
{
    const QString uniqueId = targetInfo.value(QStringLiteral("targetUniqueId")).toString().trimmed();
    if (!uniqueId.isEmpty())
    {
        return uniqueId;
    }

    const qint64 targetId = targetInfo.value(QStringLiteral("targetId")).toVariant().toLongLong();
    if (targetId > 0)
    {
        return QStringLiteral("target-%1").arg(targetId);
    }

    const QString serialNumber = resolveSerialNumber(targetInfo);
    if (!serialNumber.isEmpty())
    {
        return QStringLiteral("sn-%1").arg(serialNumber);
    }

    return QString();
}

QString HistoryCoordinator::buildSessionRecordKey(const QString &baseRecordKey, const QDateTime &eventTime) const
{
    const QDateTime safeEventTime = eventTime.isValid() ? eventTime : QDateTime::currentDateTime();
    return QStringLiteral("%1%2%3")
        .arg(baseRecordKey,
             QString(kHistorySessionSeparator),
             safeEventTime.toString(QStringLiteral("yyyyMMddHHmmsszzz")));
}

QString HistoryCoordinator::extractBaseRecordKey(const QString &recordKey) const
{
    const int separatorIndex = recordKey.indexOf(kHistorySessionSeparator);
    if (separatorIndex <= 0)
    {
        return recordKey.trimmed();
    }

    return recordKey.left(separatorIndex).trimmed();
}

QString HistoryCoordinator::resolveDetectType(const QJsonObject &targetInfo) const
{
    if (!targetInfo.value(QStringLiteral("targetUniqueId")).toString().trimmed().isEmpty())
    {
        return QStringLiteral("RID");
    }

    return QStringLiteral("未知");
}

QString HistoryCoordinator::resolveSerialNumber(const QJsonObject &targetInfo) const
{
    const QString uniqueId = targetInfo.value(QStringLiteral("targetUniqueId")).toString().trimmed();
    if (!uniqueId.isEmpty())
    {
        return uniqueId;
    }

    const qint64 targetId = targetInfo.value(QStringLiteral("targetId")).toVariant().toLongLong();
    if (targetId > 0)
    {
        return QStringLiteral("目标-%1").arg(targetId);
    }

    return QStringLiteral("--");
}

QString HistoryCoordinator::resolveModelName(const QJsonObject &targetInfo) const
{
    const QString modelName = targetInfo.value(QStringLiteral("targetName")).toString().trimmed();
    return modelName.isEmpty() ? QStringLiteral("未知型号") : modelName;
}

HistoryPage::HistoryDetailEntry HistoryCoordinator::buildDetailEntry(const QString &recordKey, const QJsonObject &targetInfo,
                                                                    const QDateTime &eventTime, bool active) const
{
    HistoryPage::HistoryDetailEntry detail;
    detail.recordKey = recordKey;
    detail.modelName = resolveModelName(targetInfo);
    detail.serialNumber = resolveSerialNumber(targetInfo);
    detail.centerFrequencyKhz = targetInfo.value(QStringLiteral("frequencyKhz")).toDouble();
    detail.droneLongitude = targetInfo.value(QStringLiteral("longitude")).toDouble();
    detail.droneLatitude = targetInfo.value(QStringLiteral("latitude")).toDouble();
    detail.pilotLongitude = targetInfo.value(QStringLiteral("controllerLongitude")).toDouble();
    detail.pilotLatitude = targetInfo.value(QStringLiteral("controllerLatitude")).toDouble();
    detail.azimuthDeg = targetInfo.value(QStringLiteral("azimuth")).toInt();
    detail.flightAltitudeMeters = targetInfo.value(QStringLiteral("altitudeFromTakeoff")).toInt();
    detail.distanceMeters = targetInfo.value(QStringLiteral("distance")).toInt();
    detail.active = active;
    detail.detectedAt = eventTime.isValid() ? eventTime : QDateTime::currentDateTime();
    return detail;
}

void HistoryCoordinator::upsertTrackedRecord(const QString &baseRecordKey, const QJsonObject &targetInfo, const QDateTime &eventTime)
{
    const QString activeSessionRecordKey = activeSessionKeysByBase_.value(baseRecordKey).trimmed();
    const bool hasReusableSession = !activeSessionRecordKey.isEmpty() && trackedRecords_.contains(activeSessionRecordKey) &&
                                    trackedRecords_.value(activeSessionRecordKey).record.active;
    const QString sessionRecordKey =
        hasReusableSession ? activeSessionRecordKey : buildSessionRecordKey(baseRecordKey, eventTime);

    TrackedRecord trackedRecord;
    if (trackedRecords_.contains(sessionRecordKey))
    {
        trackedRecord = trackedRecords_.value(sessionRecordKey);
    }
    else
    {
        trackedRecord.record.recordKey = sessionRecordKey;
        trackedRecord.record.foundTime = eventTime;
        trackedRecord.record.lastSeenTime = eventTime;
        trackedRecord.record.detectType = resolveDetectType(targetInfo);
        trackedRecord.record.serialNumber = resolveSerialNumber(targetInfo);
        trackedRecord.record.modelName = resolveModelName(targetInfo);
    }

    trackedRecord.baseRecordKey = baseRecordKey;
    trackedRecord.record.recordKey = sessionRecordKey;
    trackedRecord.record.active = true;
    trackedRecord.record.detectType = resolveDetectType(targetInfo);
    trackedRecord.record.modelName = resolveModelName(targetInfo);
    trackedRecord.record.serialNumber = resolveSerialNumber(targetInfo);
    trackedRecord.record.centerFrequencyKhz = targetInfo.value(QStringLiteral("frequencyKhz")).toDouble();
    trackedRecord.record.pilotLongitude = targetInfo.value(QStringLiteral("controllerLongitude")).toDouble();
    trackedRecord.record.pilotLatitude = targetInfo.value(QStringLiteral("controllerLatitude")).toDouble();
    trackedRecord.record.azimuthDeg = targetInfo.value(QStringLiteral("azimuth")).toInt();
    trackedRecord.record.flightAltitudeMeters = targetInfo.value(QStringLiteral("altitudeFromTakeoff")).toInt();

    trackedRecord.lastSeenAt = eventTime.isValid() ? eventTime : QDateTime::currentDateTime();
    if (!trackedRecord.record.foundTime.isValid())
    {
        trackedRecord.record.foundTime = trackedRecord.lastSeenAt;
    }
    trackedRecord.record.lastSeenTime = trackedRecord.lastSeenAt;
    trackedRecord.record.stayDurationSeconds =
        calculateStayDurationSeconds(trackedRecord.record.foundTime, trackedRecord.lastSeenAt, false);

    trackedRecords_.insert(sessionRecordKey, trackedRecord);
    activeSessionKeysByBase_.insert(baseRecordKey, sessionRecordKey);
    if (historyRepository_)
    {
        historyRepository_->appendDetailEntry(buildDetailEntry(sessionRecordKey, targetInfo, trackedRecord.lastSeenAt, true));
    }
    persistAndPublishRecord(trackedRecord.record);
}

void HistoryCoordinator::persistAndPublishRecord(const HistoryPage::HistoryRecord &record)
{
    if (historyRepository_)
    {
        historyRepository_->upsertRecord(record);
    }
    reloadHistoryPage();
}

void HistoryCoordinator::rebuildTrackedRecords(const QVector<HistoryPage::HistoryRecord> &records)
{
    trackedRecords_.clear();
    activeSessionKeysByBase_.clear();
    for (const HistoryPage::HistoryRecord &record : records)
    {
        if (record.recordKey.trimmed().isEmpty())
        {
            continue;
        }

        TrackedRecord trackedRecord;
        trackedRecord.record = record;
        trackedRecord.baseRecordKey = extractBaseRecordKey(record.recordKey);
        trackedRecord.lastSeenAt = record.lastSeenTime.isValid() ? record.lastSeenTime : record.foundTime;
        trackedRecords_.insert(record.recordKey, trackedRecord);
        if (record.active && !trackedRecord.baseRecordKey.isEmpty())
        {
            activeSessionKeysByBase_.insert(trackedRecord.baseRecordKey, record.recordKey);
        }
    }
}

void HistoryCoordinator::reloadHistoryPage()
{
    if (!historyPage_)
    {
        return;
    }

    if (!historyRepository_)
    {
        QVector<HistoryPage::HistoryRecord> records;
        for (auto it = trackedRecords_.cbegin(); it != trackedRecords_.cend(); ++it)
        {
            records.append(it.value().record);
        }
        std::sort(records.begin(), records.end(),
                  [](const HistoryPage::HistoryRecord &left, const HistoryPage::HistoryRecord &right)
                  {
                      return left.foundTime > right.foundTime;
                  });
        historyPage_->setPagination(records.size(), 1);
        historyPage_->setRecords(records.mid(0, historyPage_->currentQueryCriteria().pageSize));
        return;
    }

    HistoryPage::QueryCriteria criteria = historyPage_->currentQueryCriteria();
    const int totalCount = historyRepository_->countRecords(criteria);
    const int totalPages = qMax(1, (totalCount + criteria.pageSize - 1) / criteria.pageSize);
    criteria.page = qBound(1, criteria.page, totalPages);
    historyPage_->setPagination(totalCount, criteria.page);
    historyPage_->setRecords(historyRepository_->queryRecords(criteria));
}
