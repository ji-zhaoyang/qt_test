#ifndef TOP_NAV_BAR_H
#define TOP_NAV_BAR_H

#include <QLabel>
#include <QList>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

class QJsonObject;
class QVBoxLayout;

class TopNavBar : public QWidget
{
    Q_OBJECT
  public:
    explicit TopNavBar(QWidget *parent = nullptr);

    // 新增：供外部调用以更新右上角的时间显示
    void updateSystemTime(const QString &fullTimeStr, const QString &timeOnlyStr, double lat, double lng);
    void updateDeviceStatusInfo(const QJsonObject &deviceInfo);

  signals:
    void pageSwitched(int index);
    void closeRequested();

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    void setupUi();
    QLabel *createStatusValueLabel();
    void addStatusRow(QVBoxLayout *layout, const QString &name, QLabel *&valueLabel);
    void toggleStatusPopup();
    void updateAlarmLabel(QLabel *label, bool isAlarm);
    void refreshStatusButtonState();

    QLabel *lblSysStatus;  // 提升为成员变量以便更新
    QLabel *timePopup;     // 新增：用于点击显示完整时间的独立弹窗
    QString m_fullTimeStr; // 保存完整的时间字符串
    QPushButton *statusBtn;
    QLabel *statusBadgeLabel;
    QWidget *statusPopup;
    QLabel *fanStatusValue;
    QLabel *clockStatusValue;
    QLabel *receiverPllStatusValue;
    QLabel *transmitterPllStatusValue;
    QLabel *eepromStatusValue;
    QLabel *temperatureChipStatusValue;
    QLabel *compassStatusValue;
    QLabel *adcStatusValue;
    QLabel *pa485StatusValue;
    QLabel *paUnderpowerStatusValue;
    QLabel *paOverpowerStatusValue;
    bool hasDeviceStatusData;
    bool hasAnyAlarm;

    QTimer *m_localTimer; // 新增：用于每秒更新本地时间的定时器

  private slots:
    void updateLocalTime(); // 新增：更新本地时间的槽函数
};

#endif // TOP_NAV_BAR_H
