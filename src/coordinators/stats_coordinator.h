#ifndef STATS_COORDINATOR_H
#define STATS_COORDINATOR_H

#include <QDate>
#include <QObject>

class StatsPage;
class StatsRepository;

class StatsCoordinator : public QObject
{
    Q_OBJECT

  public:
    explicit StatsCoordinator(StatsPage *statsPage, StatsRepository *statsRepository, QObject *parent = nullptr);

    void setupConnections();
    void refreshStatistics(const QDate &startDate, const QDate &endDate);

  public slots:
    void handleResetRequested();

  private:
    StatsPage *statsPage_ = nullptr;
    StatsRepository *statsRepository_ = nullptr;
};

#endif // STATS_COORDINATOR_H
