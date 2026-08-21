#include "stats_coordinator.h"

#include "repositories/stats_repository.h"
#include "views/statistics/stats_page.h"

StatsCoordinator::StatsCoordinator(StatsPage *statsPage, StatsRepository *statsRepository, QObject *parent)
    : QObject(parent), statsPage_(statsPage), statsRepository_(statsRepository)
{
}

void StatsCoordinator::setupConnections()
{
    if (!statsPage_)
    {
        return;
    }

    connect(statsPage_, &StatsPage::searchRequested, this, &StatsCoordinator::refreshStatistics);
    connect(statsPage_, &StatsPage::resetRequested, this, &StatsCoordinator::handleResetRequested);
    connect(statsPage_, &StatsPage::pageReady, this,
            [this]()
            {
                refreshStatistics(statsPage_->defaultStartDate(), statsPage_->defaultEndDate());
            });
}

void StatsCoordinator::refreshStatistics(const QDate &startDate, const QDate &endDate)
{
    if (!statsPage_ || !statsRepository_ || !startDate.isValid() || !endDate.isValid())
    {
        return;
    }

    const StatsDateRange range{startDate, endDate};
    const QString granularity =
        startDate == endDate ? QStringLiteral("hour") : QStringLiteral("day");
    statsPage_->setLoading(true);
    statsPage_->publishModelStatistics(statsRepository_->queryModelStatistics(range));
    statsPage_->publishTrackStatistics(statsRepository_->queryTrackStatistics(range));
    statsPage_->publishTrackDailyStatistics(statsRepository_->queryTrackDailyStatistics(range), granularity);
    statsPage_->publishCounterDailyStatistics(statsRepository_->queryCounterDailyStatistics(range), granularity);
    statsPage_->publishPlotStatistics(statsRepository_->queryPlotStatistics(range));
    statsPage_->setLoading(false);
}

void StatsCoordinator::handleResetRequested()
{
    if (!statsPage_)
    {
        return;
    }

    statsPage_->applyDefaultDateRange();
    refreshStatistics(statsPage_->defaultStartDate(), statsPage_->defaultEndDate());
}
