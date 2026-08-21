#ifndef STATS_WEB_BRIDGE_H
#define STATS_WEB_BRIDGE_H

#include <QDate>
#include <QJsonObject>
#include <QObject>
#include <QPointer>

#include "repositories/stats_repository.h"

class QWebEngineView;

class StatsWebBridge : public QObject
{
    Q_OBJECT

  public:
    explicit StatsWebBridge(QWebEngineView *webView = nullptr, QObject *parent = nullptr);

    void setWebView(QWebEngineView *webView);
    bool handleTitleCommand(const QString &title);

    void sendModelStatistics(const QVector<DroneModelStat> &items);
    void sendTrackStatistics(const DroneTrackStatistics &stats);
    void sendTrackDailyStatistics(const QVector<DroneTrackDailyStat> &items, const QString &granularity = QStringLiteral("day"));
    void sendCounterDailyStatistics(const QVector<CounterDailyStat> &items, const QString &granularity = QStringLiteral("day"));
    void sendPlotStatistics(const QVector<DroneTrackPlotPoint> &items);
    void setLoading(bool loading);

  signals:
    void searchRequested(const QDate &startDate, const QDate &endDate);
    void resetRequested();

  private:
    void sendEventToWeb(const QString &eventName, const QJsonObject &payload);
    void resetPageTitle() const;

    QPointer<QWebEngineView> webView_;
};

#endif // STATS_WEB_BRIDGE_H
