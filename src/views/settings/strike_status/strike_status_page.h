#ifndef STRIKE_STATUS_PAGE_H
#define STRIKE_STATUS_PAGE_H

#include <QWidget>
#include <QVector>

class QLabel;
class QString;

class StrikeStatusPage : public QWidget
{
    Q_OBJECT

  public:
    explicit StrikeStatusPage(QWidget *parent = nullptr);

  public slots:
    void updateStrikeStatus(const QVector<int> &switchStates);

  private:
    void setupUi();
    QWidget *createStatusPanel(QWidget *parent, int startIndex, int endIndex);
    QWidget *createStatusRow(QWidget *parent, int channelIndex);
    QString titleStyle() const;
    QString panelTitleStyle() const;
    QString channelTextStyle() const;
    QString stateBadgeStyle(bool enabled) const;

    QVector<QLabel *> stateLabels;
};

#endif // STRIKE_STATUS_PAGE_H
