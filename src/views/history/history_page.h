#ifndef HISTORY_PAGE_H
#define HISTORY_PAGE_H

#include <QDateTime>
#include <QVector>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;
class QDateTimeEdit;
class QCalendarWidget;
class QListWidget;
class QTableWidget;
class QVBoxLayout;
class QResizeEvent;

class HistoryPage : public QWidget
{
    Q_OBJECT

public:
    struct QueryCriteria
    {
        QString serialKeyword;       // 序列号关键词
        QString detectType;          // 检测类型
        QDateTime startTime;         // 开始时间
        QDateTime endTime;           // 结束时间
        int page = 1;                // 当前页码
        int pageSize = 10;           // 每页记录数
    };

    struct HistoryRecord
    {
        QString recordKey;                     // 记录键值
        bool active = false;                   // 是否为活跃记录
        QString detectType;                    // 检测类型
        QString modelName;                     // 型号名称
        QString serialNumber;                  // 序列号
        bool inWhitelist = false;              // 是否在白名单中
        QDateTime foundTime;                   // 发现时间
        QDateTime lastSeenTime;                // 最后Seen时间
        double centerFrequencyKhz = 0.0;       // 中心频率（单位：kHz）
        double pilotLongitude = 0.0;           // 飞行器经度
        double pilotLatitude = 0.0;            // 飞行器纬度
        int azimuthDeg = 0;                    // 方位角（单位：度）
        int flightAltitudeMeters = 0;          // 飞行高度（单位：米）
        qint64 stayDurationSeconds = 0;        // 停留时长（单位：秒）
    };

    struct HistoryDetailEntry
    {
        QString recordKey;                     // 关联记录键值
        QString modelName;                     // 无人机型号
        QString serialNumber;                  // 无人机序列号
        double centerFrequencyKhz = 0.0;       // 中心频率（单位：kHz）
        double droneLongitude = 0.0;           // 无人机经度
        double droneLatitude = 0.0;            // 无人机纬度
        double pilotLongitude = 0.0;           // 飞手经度
        double pilotLatitude = 0.0;            // 飞手纬度
        int azimuthDeg = 0;                    // 角度（单位：度）
        int flightAltitudeMeters = 0;          // 飞行高度（单位：米）
        int distanceMeters = 0;                // 距离（单位：米）
        bool active = false;                   // 当次侦测状态
        QDateTime detectedAt;                  // 侦测时间
    };

    explicit HistoryPage(QWidget *parent = nullptr);

    void setRecords(const QVector<HistoryRecord> &records);
    void setPagination(int totalCount, int currentPage);
    void upsertRecord(const HistoryRecord &record);
    void removeRecord(const QString &recordKey);
    void clearRecords();
    QueryCriteria currentQueryCriteria() const;
    void showDetailDialog(const QString &recordKey, const QVector<HistoryDetailEntry> &details);
    void showReplayDialog(const QString &recordKey, const QVector<HistoryDetailEntry> &details);

signals:
    void clearRecordsRequested();
    void historyQueryRequested();
    void detailRequested(const QString &recordKey);
    void replayRequested(const QString &recordKey);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void applyFilters();
    void resetFilters();
    void clearAllRecords();
    void goToPreviousPage();
    void goToNextPage();

private:

    void setupUi();
    void setupToolbar(QVBoxLayout *pageLayout);
    void setupFilterBar(QVBoxLayout *pageLayout);
    void setupTable(QVBoxLayout *pageLayout);
    void setupPagination(QVBoxLayout *pageLayout);
    void loadMockData();
    void renderCurrentPage();
    void updatePageInfo();
    void showPositionDialog(const HistoryRecord &record);
    void ensureTimePickerPopup();
    void updateTimePickerPopupPosition();
    void updateTimePickerHeader();
    void syncTimePickerSelectionFromEdit();
    void applyTimePickerSelection();
    void showTimePickerPopup(QDateTimeEdit *targetEdit);
    void hideTimePickerPopup();

    QString formatFrequency(double frequencyKhz) const;
    QString formatDuration(qint64 seconds) const;
    const HistoryRecord *findRecordByKey(const QString &recordKey) const;

    QVector<HistoryRecord> allRecords_;
    int currentPage_ = 1;
    int pageSize_ = 10;
    int totalRecords_ = 0;

    QPushButton *exportButton_ = nullptr;
    QPushButton *batchReplayButton_ = nullptr;
    QLineEdit *serialEdit_ = nullptr;
    QComboBox *detectTypeCombo_ = nullptr;
    QDateTimeEdit *startTimeEdit_ = nullptr;
    QDateTimeEdit *endTimeEdit_ = nullptr;
    QPushButton *searchButton_ = nullptr;
    QPushButton *resetButton_ = nullptr;
    QPushButton *clearRecordsButton_ = nullptr;
    QTableWidget *table_ = nullptr;
    QLabel *pageInfoLabel_ = nullptr;
    QPushButton *prevPageButton_ = nullptr;
    QPushButton *nextPageButton_ = nullptr;
    QWidget *timePickerPopup_ = nullptr;
    QCalendarWidget *timePickerCalendar_ = nullptr;
    QLabel *timePickerHeaderLabel_ = nullptr;
    QPushButton *timePickerPrevYearButton_ = nullptr;
    QPushButton *timePickerPrevMonthButton_ = nullptr;
    QPushButton *timePickerNextMonthButton_ = nullptr;
    QPushButton *timePickerNextYearButton_ = nullptr;
    QListWidget *timePickerHourList_ = nullptr;
    QListWidget *timePickerMinuteList_ = nullptr;
    QListWidget *timePickerSecondList_ = nullptr;
    QPushButton *timePickerNowButton_ = nullptr;
    QPushButton *timePickerConfirmButton_ = nullptr;
    QDateTimeEdit *activeTimeEdit_ = nullptr;
};

#endif // HISTORY_PAGE_H
