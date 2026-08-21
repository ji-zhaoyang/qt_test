#include "stats_web_bridge.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebEnginePage>
#include <QWebEngineView>

namespace
{
const char kDefaultPageTitle[] = "报表统计";
}

StatsWebBridge::StatsWebBridge(QWebEngineView *webView, QObject *parent) : QObject(parent), webView_(webView)
{
}

void StatsWebBridge::setWebView(QWebEngineView *webView)
{
    webView_ = webView;
}

bool StatsWebBridge::handleTitleCommand(const QString &title)
{
    if (title.startsWith(QStringLiteral("CMD:STATS_SEARCH:")))
    {
        const QStringList parts = title.split(QLatin1Char(':'));
        if (parts.size() >= 4)
        {
            const QDate startDate = QDate::fromString(parts.at(2), Qt::ISODate);
            const QDate endDate = QDate::fromString(parts.at(3), Qt::ISODate);
            if (startDate.isValid() && endDate.isValid())
            {
                emit searchRequested(startDate, endDate);
                resetPageTitle();
                return true;
            }
        }
    }

    if (title == QStringLiteral("CMD:STATS_RESET"))
    {
        emit resetRequested();
        resetPageTitle();
        return true;
    }

    return false;
}

void StatsWebBridge::sendEventToWeb(const QString &eventName, const QJsonObject &payload)
{
    if (!webView_ || !webView_->page())
    {
        return;
    }

    const QString json = QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    const QString jsCode = QStringLiteral("if(typeof %1 === 'function') %1(%2);").arg(eventName, json);
    webView_->page()->runJavaScript(jsCode);
}

void StatsWebBridge::sendModelStatistics(const QVector<DroneModelStat> &items)
{
    QJsonArray array;
    for (const DroneModelStat &item : items)
    {
        QJsonObject row;
        row.insert(QStringLiteral("model"), item.model);
        row.insert(QStringLiteral("count"), item.count);
        array.append(row);
    }
    QJsonObject payload;
    payload.insert(QStringLiteral("items"), array);
    sendEventToWeb(QStringLiteral("updateStatsModelFromQt"), payload);
}

void StatsWebBridge::sendTrackStatistics(const DroneTrackStatistics &stats)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("trackTotalCount"), stats.trackTotalCount);
    payload.insert(QStringLiteral("droneTotalCount"), stats.droneTotalCount);
    payload.insert(QStringLiteral("trackDailyMaxCount"), stats.trackDailyMaxCount);
    payload.insert(QStringLiteral("droneDailyMaxCount"), stats.droneDailyMaxCount);
    payload.insert(QStringLiteral("trackDailyAvgCount"), stats.trackDailyAvgCount);
    payload.insert(QStringLiteral("droneDailyAvgCount"), stats.droneDailyAvgCount);
    payload.insert(QStringLiteral("totalStayTime"), stats.totalStayTime);
    payload.insert(QStringLiteral("maxStayTime"), stats.maxStayTime);
    payload.insert(QStringLiteral("avgStayTime"), stats.avgStayTime);
    sendEventToWeb(QStringLiteral("updateStatsTrackFromQt"), payload);
}

void StatsWebBridge::sendTrackDailyStatistics(const QVector<DroneTrackDailyStat> &items, const QString &granularity)
{
    QJsonArray array;
    for (const DroneTrackDailyStat &item : items)
    {
        QJsonObject row;
        row.insert(QStringLiteral("date"), item.bucketStart.toString(Qt::ISODate));
        row.insert(QStringLiteral("stayTime"), item.stayTime);
        row.insert(QStringLiteral("trackCount"), item.trackCount);
        array.append(row);
    }
    QJsonObject payload;
    payload.insert(QStringLiteral("items"), array);
    payload.insert(QStringLiteral("granularity"), granularity);
    sendEventToWeb(QStringLiteral("updateStatsDetectionDailyFromQt"), payload);
}

void StatsWebBridge::sendCounterDailyStatistics(const QVector<CounterDailyStat> &items, const QString &granularity)
{
    QJsonArray array;
    for (const CounterDailyStat &item : items)
    {
        QJsonObject row;
        row.insert(QStringLiteral("date"), item.bucketStart.toString(Qt::ISODate));
        row.insert(QStringLiteral("count"), item.count);
        array.append(row);
    }
    QJsonObject payload;
    payload.insert(QStringLiteral("items"), array);
    payload.insert(QStringLiteral("granularity"), granularity);
    sendEventToWeb(QStringLiteral("updateStatsCounterDailyFromQt"), payload);
}

void StatsWebBridge::sendPlotStatistics(const QVector<DroneTrackPlotPoint> &items)
{
    QJsonArray array;
    for (const DroneTrackPlotPoint &item : items)
    {
        QJsonObject row;
        row.insert(QStringLiteral("lat"), item.lat);
        row.insert(QStringLiteral("lng"), item.lng);
        row.insert(QStringLiteral("droneModel"), item.droneModel);
        row.insert(QStringLiteral("targetId"), item.targetId);
        row.insert(QStringLiteral("createTime"), item.createTime.toString(Qt::ISODate));
        array.append(row);
    }
    QJsonObject payload;
    payload.insert(QStringLiteral("items"), array);
    sendEventToWeb(QStringLiteral("updateStatsPlotFromQt"), payload);
}

void StatsWebBridge::setLoading(bool loading)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("loading"), loading);
    sendEventToWeb(QStringLiteral("setStatsLoadingFromQt"), payload);
}

void StatsWebBridge::resetPageTitle() const
{
    if (webView_ && webView_->page())
    {
        webView_->page()->runJavaScript(QStringLiteral("document.title = \"报表统计\";"));
    }
}
