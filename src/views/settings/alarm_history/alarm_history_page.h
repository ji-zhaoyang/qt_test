#ifndef ALARM_HISTORY_PAGE_H
#define ALARM_HISTORY_PAGE_H

#include "network/core/protocol_types.h"
#include <QWidget>

class QLabel;

class AlarmHistoryPage : public QWidget
{
    Q_OBJECT

  public:
    explicit AlarmHistoryPage(QWidget *parent = nullptr);

  public slots:
    void updateAlarmHistory(const AlarmHistoryInfo &info);

  private:
    void setupUi();
    QWidget *createHeaderRow(QWidget *parent) const;
    QWidget *createDataRow(QWidget *parent, const QString &leftName, const QString &rightName);
    QString alarmStateText(int state) const;
    QString titleStyle() const;
    QString headerTextStyle() const;
    QString nameTextStyle() const;
    QString valueTextStyle() const;

    QVector<QLabel *> valueLabels;
};

#endif // ALARM_HISTORY_PAGE_H
