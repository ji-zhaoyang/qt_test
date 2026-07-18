#include "history_page.h"

#include <algorithm>
#include <QAbstractItemView>
#include <QAbstractSpinBox>
#include <QCalendarWidget>
#include <QColor>
#include <QCoreApplication>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDir>
#include <QEvent>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QShowEvent>
#include <QSlider>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCharFormat>
#include <QTimer>
#include <QComboBox>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <QWebEngineView>

namespace
{
QString secondaryButtonStyle()
{
    return QStringLiteral("QPushButton { background-color: #1b1d22; color: #d6d7da; border: 1px solid #3b3e46; "
                          "border-radius: 4px; padding: 0 16px; font-size: 13px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #252830; }");
}

QString subtleOrangeButtonStyle()
{
    return QStringLiteral("QPushButton { background-color: #f2994a; color: #ffffff; border: none; border-radius: 2px; "
                          "padding: 0 14px; font-size: 13px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #f6a85f; }");
}

QString darkGhostButtonStyle()
{
    return QStringLiteral("QPushButton { background-color: #16181c; color: #ffffff; border: 1px solid #30333a; border-radius: 2px; "
                          "padding: 0 14px; font-size: 13px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #21242a; }");
}

QString flatInputStyle()
{
    return QStringLiteral("background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; border-radius: 2px; "
                          "padding: 0 10px; font-size: 13px;");
}

QString formatFrequencyValue(double frequencyKhz)
{
    if (frequencyKhz >= 1000000.0)
    {
        return QStringLiteral("%1GHz").arg(QString::number(frequencyKhz / 1000000.0, 'f', 3));
    }
    return QStringLiteral("%1MHz").arg(QString::number(frequencyKhz / 1000.0, 'f', 0));
}

QString formatDurationValue(qint64 seconds)
{
    const qint64 safeSeconds = qMax<qint64>(0, seconds);
    const qint64 hours = safeSeconds / 3600;
    const qint64 minutes = (safeSeconds % 3600) / 60;
    const qint64 remainSeconds = safeSeconds % 60;

    if (hours > 0)
    {
        return QStringLiteral("%1小时%2分%3秒").arg(hours).arg(minutes).arg(remainSeconds);
    }
    if (minutes > 0)
    {
        return QStringLiteral("%1分%2秒").arg(minutes).arg(remainSeconds);
    }
    return QStringLiteral("%1秒").arg(remainSeconds);
}

QVector<HistoryPage::HistoryDetailEntry> buildMockReplayDetails(const HistoryPage::HistoryRecord &record)
{
    const double basePilotLng = (qFuzzyIsNull(record.pilotLongitude) && qFuzzyIsNull(record.pilotLatitude)) ? 120.089000
                                                                                                              : record.pilotLongitude;
    const double basePilotLat = (qFuzzyIsNull(record.pilotLongitude) && qFuzzyIsNull(record.pilotLatitude)) ? 30.342000
                                                                                                              : record.pilotLatitude;

    const QVector<QPair<double, double>> droneOffsets = {
        qMakePair(-0.00120,  0.00020),
        qMakePair(-0.00085,  0.00042),
        qMakePair(-0.00045,  0.00068),
        qMakePair(-0.00005,  0.00095),
        qMakePair( 0.00036,  0.00108),
        qMakePair( 0.00078,  0.00096),
        qMakePair( 0.00108,  0.00062),
        qMakePair( 0.00126,  0.00018)
    };

    const int frameCount = droneOffsets.size();
    QDateTime startTime = record.foundTime.isValid() ? record.foundTime : QDateTime::currentDateTime().addSecs(-frameCount * 3);
    QDateTime endTime = record.lastSeenTime.isValid() ? record.lastSeenTime : startTime.addSecs((frameCount - 1) * 3);
    const qint64 totalSpanSeconds = qMax<qint64>(frameCount - 1, startTime.secsTo(endTime));
    const qint64 stepSeconds = qMax<qint64>(1, totalSpanSeconds / qMax(1, frameCount - 1));

    QVector<HistoryPage::HistoryDetailEntry> details;
    for (int i = 0; i < frameCount; ++i)
    {
        HistoryPage::HistoryDetailEntry detail;
        detail.recordKey = record.recordKey;
        detail.modelName = record.modelName;
        detail.serialNumber = record.serialNumber;
        detail.centerFrequencyKhz = record.centerFrequencyKhz;
        detail.pilotLongitude = basePilotLng;
        detail.pilotLatitude = basePilotLat;
        detail.droneLongitude = basePilotLng + droneOffsets.at(i).first;
        detail.droneLatitude = basePilotLat + droneOffsets.at(i).second;
        detail.azimuthDeg = (record.azimuthDeg + i * 6) % 360;
        detail.flightAltitudeMeters = qMax(10, record.flightAltitudeMeters + i * 8);
        detail.distanceMeters = 180 + i * 35;
        detail.active = (i != frameCount - 1) || record.active;
        detail.detectedAt = startTime.addSecs(stepSeconds * i);
        details.append(detail);
    }

    return details;
}

void applyCalendarTextColors(QCalendarWidget *calendar)
{
    if (!calendar)
    {
        return;
    }

    QPalette palette = calendar->palette();
    palette.setColor(QPalette::WindowText, QColor(QStringLiteral("#ffffff")));
    palette.setColor(QPalette::Text, QColor(QStringLiteral("#ffffff")));
    palette.setColor(QPalette::ButtonText, QColor(QStringLiteral("#ffffff")));
    palette.setColor(QPalette::HighlightedText, QColor(QStringLiteral("#ffffff")));
    calendar->setPalette(palette);

    QTextCharFormat weekdayFormat;
    weekdayFormat.setForeground(QColor(QStringLiteral("#ffffff")));
    for (Qt::DayOfWeek day : {Qt::Monday, Qt::Tuesday, Qt::Wednesday, Qt::Thursday, Qt::Friday})
    {
        calendar->setWeekdayTextFormat(day, weekdayFormat);
    }

    QTextCharFormat weekendFormat;
    weekendFormat.setForeground(QColor(QStringLiteral("#d98c84")));
    calendar->setWeekdayTextFormat(Qt::Saturday, weekendFormat);
    calendar->setWeekdayTextFormat(Qt::Sunday, weekendFormat);
}

QLabel *createChipLabel(const QString &text, const QString &bgColor, const QString &textColor, QWidget *parent)
{
    QLabel *label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(QStringLiteral("background-color: %1; color: %2; border-radius: 8px; padding: 2px 8px; "
                                        "font-size: 12px; font-weight: bold;")
                             .arg(bgColor, textColor));
    return label;
}

class HistoryDetailDialog : public QWidget
{
public:
    explicit HistoryDetailDialog(const HistoryPage::HistoryRecord &record,
                                 const QVector<HistoryPage::HistoryDetailEntry> &details,
                                 QWidget *parent = nullptr)
        : QWidget(parent), panelWidget_(new QWidget(this))
    {
        setAttribute(Qt::WA_StyledBackground, true);
        setAttribute(Qt::WA_DeleteOnClose, true);
        hide();
        setStyleSheet(QStringLiteral("QWidget { background-color: rgba(0, 0, 0, 120); }"));

        panelWidget_->setObjectName(QStringLiteral("historyDetailPanel"));
        panelWidget_->setAttribute(Qt::WA_StyledBackground, true);
        panelWidget_->setStyleSheet(QStringLiteral(
            "QWidget#historyDetailPanel { background-color: #17191d; color: #ffffff; border: 1px solid #2b2f36; border-radius: 8px; }"
            "QLabel { color: #ffffff; font-size: 13px; }"
            "QHeaderView::section { background-color: #17191d; color: #d8dbe2; border: none; border-bottom: 1px solid #2b2f36; "
            "padding: 10px 8px; font-size: 13px; font-weight: bold; }"
            "QTableWidget { background-color: #111214; color: #ffffff; border: 1px solid #2b2f36; gridline-color: #1f2329; }"
            "QTableWidget::item { padding: 8px 10px; border: none; }"));

        const QString statusText = record.active ? QStringLiteral("侦测中") : QStringLiteral("侦测结束");
        const QString foundTimeText = record.foundTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        const QString frequencyText = formatFrequencyValue(record.centerFrequencyKhz);

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        mainLayout->addStretch();
        mainLayout->addWidget(panelWidget_, 0, Qt::AlignCenter);
        mainLayout->addStretch();

        QVBoxLayout *layout = new QVBoxLayout(panelWidget_);
        layout->setContentsMargins(20, 18, 20, 18);
        layout->setSpacing(14);

        QWidget *titleBar = new QWidget(panelWidget_);
        QHBoxLayout *titleBarLayout = new QHBoxLayout(titleBar);
        titleBarLayout->setContentsMargins(0, 0, 0, 0);
        titleBarLayout->setSpacing(8);

        QLabel *titleLabel = new QLabel(QStringLiteral("侦测记录详情"), titleBar);
        titleLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 20px; font-weight: bold;"));
        titleBarLayout->addWidget(titleLabel);
        titleBarLayout->addStretch();

        QPushButton *closeButton = new QPushButton(QStringLiteral("×"), titleBar);
        closeButton->setFixedSize(28, 28);
        closeButton->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; color: #c7cbd3; border: none; font-size: 18px; font-weight: bold; }"
            "QPushButton:hover { color: #ffffff; }"));
        connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
        titleBarLayout->addWidget(closeButton);
        layout->addWidget(titleBar);

        QFrame *summaryCard = new QFrame(panelWidget_);
        summaryCard->setStyleSheet(
            QStringLiteral("QFrame { background-color: #111214; border: 1px solid #2b2f36; border-radius: 8px; }"));
        QGridLayout *summaryLayout = new QGridLayout(summaryCard);
        summaryLayout->setContentsMargins(18, 16, 18, 16);
        summaryLayout->setHorizontalSpacing(28);
        summaryLayout->setVerticalSpacing(12);

        auto addInfoBlock = [summaryCard, summaryLayout](int row, int column, const QString &name, const QString &value)
        {
            QWidget *block = new QWidget(summaryCard);
            QVBoxLayout *blockLayout = new QVBoxLayout(block);
            blockLayout->setContentsMargins(0, 0, 0, 0);
            blockLayout->setSpacing(6);

            QLabel *nameLabel = new QLabel(name, block);
            nameLabel->setStyleSheet(QStringLiteral("color: #8f96a3; font-size: 13px;"));
            QLabel *valueLabel = new QLabel(value, block);
            valueLabel->setWordWrap(true);
            valueLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 15px; font-weight: bold;"));

            blockLayout->addWidget(nameLabel);
            blockLayout->addWidget(valueLabel);
            summaryLayout->addWidget(block, row, column);
        };

        addInfoBlock(0, 0, QStringLiteral("无人机型号"), record.modelName);
        addInfoBlock(0, 1, QStringLiteral("无人机序列号"), record.serialNumber);
        addInfoBlock(0, 2, QStringLiteral("状态"), statusText);
        addInfoBlock(1, 0, QStringLiteral("发现时间"), foundTimeText);
        addInfoBlock(1, 1, QStringLiteral("中心频率"), frequencyText);
        addInfoBlock(1, 2, QStringLiteral("停留时长"), formatDurationValue(record.stayDurationSeconds));
        layout->addWidget(summaryCard);

        QLabel *detailTitleLabel = new QLabel(QStringLiteral("侦测记录明细"), panelWidget_);
        detailTitleLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 17px; font-weight: bold;"));
        layout->addWidget(detailTitleLabel);

        const int detailPageSize = 10;
        const int totalDetailPages = qMax(1, (details.size() + detailPageSize - 1) / detailPageSize);

        QTableWidget *detailTable = new QTableWidget(detailPageSize, 9, panelWidget_);
        detailTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        detailTable->setSelectionMode(QAbstractItemView::NoSelection);
        detailTable->setFocusPolicy(Qt::NoFocus);
        detailTable->verticalHeader()->setVisible(false);
        detailTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        detailTable->horizontalHeader()->setMinimumSectionSize(96);
        detailTable->verticalHeader()->setDefaultSectionSize(42);
        detailTable->setAlternatingRowColors(false);
        detailTable->setShowGrid(false);
        detailTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        detailTable->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        detailTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        detailTable->setWordWrap(false);
        detailTable->setHorizontalHeaderLabels({QStringLiteral("无人机型号"),
                                                QStringLiteral("无人机序列号"),
                                                QStringLiteral("中心频率"),
                                                QStringLiteral("无人机经纬度"),
                                                QStringLiteral("飞手经纬度"),
                                                QStringLiteral("角度(°)"),
                                                QStringLiteral("飞行高度(米)"),
                                                QStringLiteral("距离(米)"),
                                                QStringLiteral("侦测时间")});

        auto createItem = [](const QString &text)
        {
            QTableWidgetItem *item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            item->setForeground(QColor(QStringLiteral("#ffffff")));
            return item;
        };

        auto formatCoordinatePair = [](double lng, double lat)
        {
            return QStringLiteral("%1,%2").arg(lng, 0, 'f', 6).arg(lat, 0, 'f', 6);
        };

        detailTable->setProperty("detailPage", 1);

        auto renderDetailPage = [detailTable, details, detailPageSize, totalDetailPages, createItem, formatCoordinatePair]()
        {
            int currentPage = detailTable->property("detailPage").toInt();
            currentPage = qBound(1, currentPage, totalDetailPages);
            detailTable->setProperty("detailPage", currentPage);
            detailTable->clearContents();

            const int startIndex = (currentPage - 1) * detailPageSize;
            const int visibleCount = qMin(detailPageSize, details.size() - startIndex);
            for (int row = 0; row < detailPageSize; ++row)
            {
                const bool hasData = row < visibleCount;
                detailTable->setRowHidden(row, !hasData);
                if (!hasData)
                {
                    for (int column = 0; column < detailTable->columnCount(); ++column)
                    {
                        detailTable->setItem(row, column, createItem(QString()));
                    }
                    continue;
                }

                const HistoryPage::HistoryDetailEntry &detail = details.at(startIndex + row);
                detailTable->setItem(row, 0, createItem(detail.modelName));
                detailTable->setItem(row, 1, createItem(detail.serialNumber));
                detailTable->setItem(row, 2, createItem(formatFrequencyValue(detail.centerFrequencyKhz)));
                detailTable->setItem(row, 3, createItem(formatCoordinatePair(detail.droneLongitude, detail.droneLatitude)));
                detailTable->setItem(row, 4, createItem(formatCoordinatePair(detail.pilotLongitude, detail.pilotLatitude)));
                detailTable->setItem(row, 5, createItem(QString::number(detail.azimuthDeg)));
                detailTable->setItem(row, 6, createItem(QString::number(detail.flightAltitudeMeters)));
                detailTable->setItem(row, 7, createItem(QString::number(detail.distanceMeters)));
                detailTable->setItem(row, 8, createItem(detail.detectedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))));
            }
        };

        renderDetailPage();
        layout->addWidget(detailTable, 1);

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();

        QLabel *pageInfoLabel =
            new QLabel(QStringLiteral("共 %1 条，第 %2 / %3 页").arg(details.size()).arg(1).arg(totalDetailPages), panelWidget_);
        pageInfoLabel->setStyleSheet(QStringLiteral("color: #c1c5cc; font-size: 13px;"));
        buttonLayout->addWidget(pageInfoLabel);
        buttonLayout->addSpacing(12);

        QPushButton *prevPageButton = new QPushButton(QStringLiteral("上一页"), panelWidget_);
        prevPageButton->setFixedSize(78, 34);
        prevPageButton->setStyleSheet(darkGhostButtonStyle());
        prevPageButton->setEnabled(false);
        buttonLayout->addWidget(prevPageButton);

        QPushButton *nextPageButton = new QPushButton(QStringLiteral("下一页"), panelWidget_);
        nextPageButton->setFixedSize(78, 34);
        nextPageButton->setStyleSheet(darkGhostButtonStyle());
        nextPageButton->setEnabled(totalDetailPages > 1);
        buttonLayout->addWidget(nextPageButton);

        auto updateDetailPager = [detailTable, totalDetailPages, details, pageInfoLabel, prevPageButton, nextPageButton, renderDetailPage]()
        {
            const int currentPage = qBound(1, detailTable->property("detailPage").toInt(), totalDetailPages);
            detailTable->setProperty("detailPage", currentPage);
            renderDetailPage();
            pageInfoLabel->setText(QStringLiteral("共 %1 条，第 %2 / %3 页").arg(details.size()).arg(currentPage).arg(totalDetailPages));
            prevPageButton->setEnabled(currentPage > 1);
            nextPageButton->setEnabled(currentPage < totalDetailPages);
        };

        connect(prevPageButton, &QPushButton::clicked, this,
                [detailTable, updateDetailPager]()
                {
                    detailTable->setProperty("detailPage", detailTable->property("detailPage").toInt() - 1);
                    updateDetailPager();
                });
        connect(nextPageButton, &QPushButton::clicked, this,
                [detailTable, updateDetailPager]()
                {
                    detailTable->setProperty("detailPage", detailTable->property("detailPage").toInt() + 1);
                    updateDetailPager();
                });

        layout->addLayout(buttonLayout);
    }

    void showOverlay()
    {
        if (parentWidget())
        {
            setGeometry(parentWidget()->rect());
            raise();
        }
        updatePanelGeometry();
        show();
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        updatePanelGeometry();
    }

private:
    void updatePanelGeometry()
    {
        if (!parentWidget() || !panelWidget_)
        {
            return;
        }

        const QSize parentSize = parentWidget()->size();
        const int panelWidth = qBound(980, parentSize.width() - 80, 1480);
        const int panelHeight = qBound(620, parentSize.height() - 80, 820);
        panelWidget_->setFixedSize(panelWidth, panelHeight);
    }

    QWidget *panelWidget_ = nullptr;
};

class HistoryReplayOverlay : public QWidget
{
public:
    explicit HistoryReplayOverlay(const HistoryPage::HistoryRecord &record,
                                  const QVector<HistoryPage::HistoryDetailEntry> &details,
                                  QWidget *parent = nullptr)
        : QWidget(parent), record_(record), frames_(buildReplayFrames(record, details))
    {
        setAttribute(Qt::WA_StyledBackground, true);
        setAttribute(Qt::WA_DeleteOnClose, true);
        hide();
        setStyleSheet(QStringLiteral("QWidget { background-color: rgba(0, 0, 0, 120); }"));

        panelWidget_ = new QWidget(this);
        panelWidget_->setAttribute(Qt::WA_StyledBackground, true);
        panelWidget_->setObjectName(QStringLiteral("historyReplayPanel"));
        panelWidget_->setStyleSheet(QStringLiteral(
            "QWidget#historyReplayPanel { background-color: #17191d; border: 1px solid #2b2f36; border-radius: 8px; }"
            "QLabel { color: #ffffff; font-size: 13px; }"));

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
        mainLayout->addStretch();
        mainLayout->addWidget(panelWidget_, 0, Qt::AlignCenter);
        mainLayout->addStretch();

        QVBoxLayout *panelLayout = new QVBoxLayout(panelWidget_);
        panelLayout->setContentsMargins(20, 18, 20, 18);
        panelLayout->setSpacing(16);

        QWidget *titleBar = new QWidget(panelWidget_);
        QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
        titleLayout->setContentsMargins(0, 0, 0, 0);
        titleLayout->setSpacing(8);

        QLabel *titleLabel = new QLabel(QStringLiteral("轨迹回放"), titleBar);
        titleLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 20px; font-weight: bold;"));
        titleLayout->addWidget(titleLabel);
        titleLayout->addStretch();

        QPushButton *closeButton = new QPushButton(QStringLiteral("×"), titleBar);
        closeButton->setFixedSize(28, 28);
        closeButton->setStyleSheet(QStringLiteral(
            "QPushButton { background: transparent; color: #c7cbd3; border: none; font-size: 18px; font-weight: bold; }"
            "QPushButton:hover { color: #ffffff; }"));
        connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
        titleLayout->addWidget(closeButton);
        panelLayout->addWidget(titleBar);

        QWidget *contentWidget = new QWidget(panelWidget_);
        QHBoxLayout *contentLayout = new QHBoxLayout(contentWidget);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->setSpacing(16);

        QFrame *mapFrame = new QFrame(contentWidget);
        mapFrame->setStyleSheet(QStringLiteral("QFrame { background-color: #111214; border: 1px solid #2b2f36; border-radius: 8px; }"));
        QVBoxLayout *mapLayout = new QVBoxLayout(mapFrame);
        mapLayout->setContentsMargins(12, 12, 12, 12);
        mapLayout->setSpacing(0);

        webView_ = new QWebEngineView(mapFrame);
        webView_->setContextMenuPolicy(Qt::NoContextMenu);
        mapLayout->addWidget(webView_);
        contentLayout->addWidget(mapFrame, 3);

        QFrame *infoFrame = new QFrame(contentWidget);
        infoFrame->setStyleSheet(QStringLiteral("QFrame { background-color: #111214; border: 1px solid #2b2f36; border-radius: 8px; }"));
        QVBoxLayout *infoLayout = new QVBoxLayout(infoFrame);
        infoLayout->setContentsMargins(16, 16, 16, 16);
        infoLayout->setSpacing(12);

        QLabel *nameLabel = new QLabel(record_.modelName.isEmpty() ? QStringLiteral("未知型号") : record_.modelName, infoFrame);
        nameLabel->setWordWrap(true);
        nameLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 24px; font-weight: bold;"));
        infoLayout->addWidget(nameLabel);

        QWidget *chipRow = new QWidget(infoFrame);
        QHBoxLayout *chipLayout = new QHBoxLayout(chipRow);
        chipLayout->setContentsMargins(0, 0, 0, 0);
        chipLayout->setSpacing(8);
        chipLayout->addWidget(createChipLabel(record_.detectType.isEmpty() ? QStringLiteral("未知") : record_.detectType,
                                              QStringLiteral("#3a2412"),
                                              QStringLiteral("#f7b26f"),
                                              chipRow),
                              0, Qt::AlignLeft);
        chipLayout->addWidget(createChipLabel(record_.active ? QStringLiteral("侦测中") : QStringLiteral("侦测结束"),
                                              record_.active ? QStringLiteral("#163323") : QStringLiteral("#30333a"),
                                              record_.active ? QStringLiteral("#22d35e") : QStringLiteral("#cfd3da"),
                                              chipRow),
                              0, Qt::AlignLeft);
        chipLayout->addStretch();
        infoLayout->addWidget(chipRow);

        auto addSectionLabel = [infoFrame, infoLayout](const QString &title)
        {
            QLabel *sectionLabel = new QLabel(title, infoFrame);
            sectionLabel->setStyleSheet(QStringLiteral("color: #cfd3da; font-size: 13px; font-weight: bold;"));
            infoLayout->addWidget(sectionLabel);
        };

        auto addInfoRow = [](QWidget *parent, QVBoxLayout *layout, const QString &name, const QString &value) -> QLabel *
        {
            QWidget *rowWidget = new QWidget(parent);
            QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(10);

            QLabel *nameLabel = new QLabel(name, rowWidget);
            nameLabel->setStyleSheet(QStringLiteral("color: #8f96a3; font-size: 13px;"));
            QLabel *valueLabel = new QLabel(value, rowWidget);
            valueLabel->setWordWrap(true);
            valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            valueLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 14px; font-weight: bold;"));

            rowLayout->addWidget(nameLabel);
            rowLayout->addStretch();
            rowLayout->addWidget(valueLabel, 1);
            layout->addWidget(rowWidget);
            return valueLabel;
        };

        addSectionLabel(QStringLiteral("记录信息"));
        QFrame *recordCard = new QFrame(infoFrame);
        recordCard->setStyleSheet(QStringLiteral("QFrame { background-color: #17191d; border: 1px solid #242832; border-radius: 8px; }"));
        QVBoxLayout *recordLayout = new QVBoxLayout(recordCard);
        recordLayout->setContentsMargins(12, 12, 12, 12);
        recordLayout->setSpacing(8);
        addInfoRow(recordCard, recordLayout, QStringLiteral("序列号"),
                   record_.serialNumber.isEmpty() ? QStringLiteral("--") : record_.serialNumber);
        addInfoRow(recordCard, recordLayout, QStringLiteral("发现时间"),
                   record_.foundTime.isValid() ? record_.foundTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) : QStringLiteral("--"));
        addInfoRow(recordCard, recordLayout, QStringLiteral("最后时间"),
                   record_.lastSeenTime.isValid() ? record_.lastSeenTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")) : QStringLiteral("--"));
        addInfoRow(recordCard, recordLayout, QStringLiteral("中心频率"), formatFrequencyValue(record_.centerFrequencyKhz));
        addInfoRow(recordCard, recordLayout, QStringLiteral("停留时长"), formatDurationValue(record_.stayDurationSeconds));
        infoLayout->addWidget(recordCard);

        addSectionLabel(QStringLiteral("当前帧信息"));
        QFrame *currentCard = new QFrame(infoFrame);
        currentCard->setStyleSheet(QStringLiteral("QFrame { background-color: #17191d; border: 1px solid #2b2f36; border-radius: 8px; }"));
        QGridLayout *currentGrid = new QGridLayout(currentCard);
        currentGrid->setContentsMargins(12, 12, 12, 12);
        currentGrid->setHorizontalSpacing(10);
        currentGrid->setVerticalSpacing(10);

        auto addMetricCard =
            [currentCard, currentGrid](int row, int column, int rowSpan, int columnSpan, const QString &title, QLabel **outLabel)
        {
            QFrame *metricCard = new QFrame(currentCard);
            metricCard->setStyleSheet(QStringLiteral("QFrame { background-color: #111214; border: 1px solid #242832; border-radius: 6px; }"));
            QVBoxLayout *metricLayout = new QVBoxLayout(metricCard);
            metricLayout->setContentsMargins(10, 8, 10, 8);
            metricLayout->setSpacing(4);

            QLabel *titleLabel = new QLabel(title, metricCard);
            titleLabel->setStyleSheet(QStringLiteral("color: #8f96a3; font-size: 12px;"));
            QLabel *valueLabel = new QLabel(QStringLiteral("--"), metricCard);
            valueLabel->setWordWrap(true);
            valueLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 14px; font-weight: bold;"));

            metricLayout->addWidget(titleLabel);
            metricLayout->addWidget(valueLabel);
            currentGrid->addWidget(metricCard, row, column, rowSpan, columnSpan);
            *outLabel = valueLabel;
        };

        addMetricCard(0, 0, 1, 1, QStringLiteral("当前帧"), &currentFrameLabel_);
        addMetricCard(0, 1, 1, 1, QStringLiteral("当前时间"), &currentTimeLabel_);
        addMetricCard(1, 0, 1, 2, QStringLiteral("当前坐标"), &currentCoordinateLabel_);
        addMetricCard(2, 0, 1, 1, QStringLiteral("轨迹来源"), &currentSourceLabel_);
        addMetricCard(2, 1, 1, 1, QStringLiteral("当前距离"), &currentDistanceLabel_);
        addMetricCard(3, 0, 1, 1, QStringLiteral("当前高度"), &currentAltitudeLabel_);
        addMetricCard(3, 1, 1, 1, QStringLiteral("当前方位角"), &currentAzimuthLabel_);
        infoLayout->addWidget(currentCard);
        infoLayout->addStretch();

        contentLayout->addWidget(infoFrame, 1);
        panelLayout->addWidget(contentWidget, 1);

        QWidget *bottomBar = new QWidget(panelWidget_);
        QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
        bottomLayout->setContentsMargins(0, 0, 0, 0);
        bottomLayout->setSpacing(14);

        QWidget *leftControlWidget = new QWidget(bottomBar);
        QHBoxLayout *leftControlLayout = new QHBoxLayout(leftControlWidget);
        leftControlLayout->setContentsMargins(0, 0, 0, 0);
        leftControlLayout->setSpacing(8);

        playButton_ = new QPushButton(QStringLiteral("播放"), leftControlWidget);
        playButton_->setFixedSize(72, 34);
        playButton_->setStyleSheet(subtleOrangeButtonStyle());
        leftControlLayout->addWidget(playButton_);

        prevButton_ = new QPushButton(QStringLiteral("上一帧"), leftControlWidget);
        prevButton_->setFixedSize(78, 34);
        prevButton_->setStyleSheet(darkGhostButtonStyle());
        leftControlLayout->addWidget(prevButton_);

        nextButton_ = new QPushButton(QStringLiteral("下一帧"), leftControlWidget);
        nextButton_->setFixedSize(78, 34);
        nextButton_->setStyleSheet(darkGhostButtonStyle());
        leftControlLayout->addWidget(nextButton_);

        QLabel *speedLabel = new QLabel(QStringLiteral("倍速"), leftControlWidget);
        speedLabel->setStyleSheet(QStringLiteral("color: #8f96a3; font-size: 13px;"));
        leftControlLayout->addWidget(speedLabel);

        speedCombo_ = new QComboBox(leftControlWidget);
        speedCombo_->setFixedSize(86, 34);
        speedCombo_->setCursor(Qt::PointingHandCursor);
        speedCombo_->setStyleSheet(QStringLiteral("QComboBox { background-color: #111214; color: #ffffff; border: 1px solid #30333a; "
                                                  "border-radius: 4px; padding: 0 10px; font-size: 13px; font-weight: bold; }"
                                                  "QComboBox::drop-down { width: 24px; border-left: 1px solid #30333a; }"
                                                  "QComboBox::down-arrow { image: none; }"
                                                  "QComboBox QAbstractItemView { background-color: #17191d; color: #ffffff; "
                                                  "selection-background-color: #2c313a; border: 1px solid #30333a; }"));
        speedCombo_->addItem(QStringLiteral("1x"), 900);
        speedCombo_->addItem(QStringLiteral("2x"), 500);
        speedCombo_->addItem(QStringLiteral("4x"), 250);
        leftControlLayout->addWidget(speedCombo_);
        bottomLayout->addWidget(leftControlWidget);

        timelineSlider_ = new QSlider(Qt::Horizontal, bottomBar);
        timelineSlider_->setMinimum(0);
        timelineSlider_->setMaximum(qMax(0, frames_.size() - 1));
        timelineSlider_->setSingleStep(1);
        timelineSlider_->setPageStep(1);
        timelineSlider_->setStyleSheet(QStringLiteral(
            "QSlider::groove:horizontal { background: #23262d; height: 6px; border-radius: 3px; }"
            "QSlider::add-page:horizontal { background: #23262d; border-radius: 3px; }"
            "QSlider::sub-page:horizontal { background: #f2994a; border-radius: 3px; }"
            "QSlider::handle:horizontal { background: #ffffff; width: 16px; margin: -7px 0; border-radius: 8px; border: 2px solid #f2994a; }"));
        bottomLayout->addWidget(timelineSlider_, 1);

        frameSummaryLabel_ = new QLabel(QStringLiteral("0 / 0"), bottomBar);
        frameSummaryLabel_->setStyleSheet(QStringLiteral("color: #c1c5cc; font-size: 13px; min-width: 54px;"));
        bottomLayout->addWidget(frameSummaryLabel_);

        QPushButton *closeBottomButton = new QPushButton(QStringLiteral("关闭"), bottomBar);
        closeBottomButton->setFixedSize(88, 34);
        closeBottomButton->setStyleSheet(secondaryButtonStyle());
        connect(closeBottomButton, &QPushButton::clicked, this, &QWidget::close);
        bottomLayout->addWidget(closeBottomButton);
        panelLayout->addWidget(bottomBar);

        playbackTimer_ = new QTimer(this);
        playbackTimer_->setInterval(900);
        connect(playbackTimer_, &QTimer::timeout, this, [this]() { advanceFrame(); });
        connect(playButton_, &QPushButton::clicked, this, [this]() { togglePlayback(); });
        connect(prevButton_, &QPushButton::clicked, this, [this]() { stepFrame(-1); });
        connect(nextButton_, &QPushButton::clicked, this, [this]() { stepFrame(1); });
        connect(speedCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) { applyPlaybackSpeed(); });
        connect(timelineSlider_, &QSlider::valueChanged, this,
                [this](int value)
                {
                    pausePlayback();
                    setCurrentFrame(value);
                });

        connect(webView_, &QWebEngineView::loadFinished, this,
                [this](bool ok)
                {
                    mapReady_ = ok;
                    if (mapReady_)
                    {
                        pushReplayDataToMap();
                        setCurrentFrame(currentFrameIndex_);
                    }
                });

        const QString webPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/assets/web/history_replay.html");
        webView_->load(QUrl::fromLocalFile(webPath));
        applyPlaybackSpeed();
        refreshControls();
        setCurrentFrame(frames_.isEmpty() ? 0 : 0);
    }

    void showOverlay()
    {
        if (parentWidget())
        {
            setGeometry(parentWidget()->rect());
            raise();
        }
        updatePanelGeometry();
        show();
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        updatePanelGeometry();
    }

private:
    struct ReplayFrame
    {
        double latitude = 0.0;
        double longitude = 0.0;
        QString sourceType;
        QDateTime detectedAt;
        int distanceMeters = 0;
        int altitudeMeters = 0;
        int azimuthDeg = 0;
    };

    static bool hasValidCoordinate(double lng, double lat)
    {
        if (qFuzzyIsNull(lng) && qFuzzyIsNull(lat))
        {
            return false;
        }
        return lng >= -180.0 && lng <= 180.0 && lat >= -90.0 && lat <= 90.0;
    }

    static QString coordinateText(double lng, double lat)
    {
        return QStringLiteral("%1, %2").arg(lng, 0, 'f', 6).arg(lat, 0, 'f', 6);
    }

    static QVector<ReplayFrame> buildReplayFrames(const HistoryPage::HistoryRecord &record,
                                                  const QVector<HistoryPage::HistoryDetailEntry> &details)
    {
        QVector<HistoryPage::HistoryDetailEntry> orderedDetails = details;
        std::sort(orderedDetails.begin(), orderedDetails.end(),
                  [](const HistoryPage::HistoryDetailEntry &left, const HistoryPage::HistoryDetailEntry &right)
                  {
                      return left.detectedAt < right.detectedAt;
                  });

        QVector<ReplayFrame> frames;
        for (const HistoryPage::HistoryDetailEntry &detail : orderedDetails)
        {
            ReplayFrame frame;
            if (hasValidCoordinate(detail.droneLongitude, detail.droneLatitude))
            {
                frame.longitude = detail.droneLongitude;
                frame.latitude = detail.droneLatitude;
                frame.sourceType = QStringLiteral("无人机坐标");
            }
            else if (hasValidCoordinate(detail.pilotLongitude, detail.pilotLatitude))
            {
                frame.longitude = detail.pilotLongitude;
                frame.latitude = detail.pilotLatitude;
                frame.sourceType = QStringLiteral("飞手坐标");
            }
            else
            {
                continue;
            }

            frame.detectedAt = detail.detectedAt;
            frame.distanceMeters = detail.distanceMeters;
            frame.altitudeMeters = detail.flightAltitudeMeters;
            frame.azimuthDeg = detail.azimuthDeg;
            frames.append(frame);
        }

        if (!frames.isEmpty())
        {
            return frames;
        }

        if (hasValidCoordinate(record.pilotLongitude, record.pilotLatitude))
        {
            ReplayFrame fallbackFrame;
            fallbackFrame.longitude = record.pilotLongitude;
            fallbackFrame.latitude = record.pilotLatitude;
            fallbackFrame.sourceType = QStringLiteral("飞手坐标");
            fallbackFrame.detectedAt = record.lastSeenTime.isValid() ? record.lastSeenTime : record.foundTime;
            fallbackFrame.altitudeMeters = record.flightAltitudeMeters;
            fallbackFrame.azimuthDeg = record.azimuthDeg;
            frames.append(fallbackFrame);
        }

        return frames;
    }

    void updatePanelGeometry()
    {
        if (!parentWidget() || !panelWidget_)
        {
            return;
        }

        const QSize parentSize = parentWidget()->size();
        const int panelWidth = qBound(1000, parentSize.width() - 80, 1500);
        const int panelHeight = qBound(640, parentSize.height() - 80, 860);
        panelWidget_->setFixedSize(panelWidth, panelHeight);
    }

    void pushReplayDataToMap()
    {
        if (!webView_ || !mapReady_)
        {
            return;
        }

        QJsonArray frameArray;
        for (const ReplayFrame &frame : frames_)
        {
            QJsonObject item;
            item.insert(QStringLiteral("lat"), frame.latitude);
            item.insert(QStringLiteral("lng"), frame.longitude);
            item.insert(QStringLiteral("label"),
                        frame.detectedAt.isValid() ? frame.detectedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                                                   : QStringLiteral("--"));
            item.insert(QStringLiteral("subtitle"), frame.sourceType);
            frameArray.append(item);
        }

        const QString js = QStringLiteral("if (typeof setReplayData === 'function') { setReplayData(%1); }")
                               .arg(QString::fromUtf8(QJsonDocument(frameArray).toJson(QJsonDocument::Compact)));
        webView_->page()->runJavaScript(js);
    }

    void refreshControls()
    {
        const bool hasFrames = !frames_.isEmpty();
        if (playButton_)
        {
            playButton_->setEnabled(frames_.size() > 1);
            playButton_->setText(isPlaying_ ? QStringLiteral("暂停") : QStringLiteral("播放"));
        }
        if (prevButton_)
        {
            prevButton_->setEnabled(hasFrames && currentFrameIndex_ > 0);
        }
        if (nextButton_)
        {
            nextButton_->setEnabled(hasFrames && currentFrameIndex_ < frames_.size() - 1);
        }
        if (frameSummaryLabel_)
        {
            frameSummaryLabel_->setText(hasFrames
                                            ? QStringLiteral("%1 / %2").arg(currentFrameIndex_ + 1).arg(frames_.size())
                                            : QStringLiteral("0 / 0"));
        }
    }

    void applyPlaybackSpeed()
    {
        if (!playbackTimer_ || !speedCombo_)
        {
            return;
        }

        playbackTimer_->setInterval(speedCombo_->currentData().toInt());
    }

    void pausePlayback()
    {
        if (playbackTimer_)
        {
            playbackTimer_->stop();
        }
        isPlaying_ = false;
        refreshControls();
    }

    void togglePlayback()
    {
        if (frames_.size() <= 1 || !playbackTimer_)
        {
            return;
        }

        if (isPlaying_)
        {
            pausePlayback();
            return;
        }

        if (currentFrameIndex_ >= frames_.size() - 1)
        {
            setCurrentFrame(0);
        }

        playbackTimer_->start();
        isPlaying_ = true;
        refreshControls();
    }

    void advanceFrame()
    {
        if (frames_.isEmpty())
        {
            pausePlayback();
            return;
        }

        if (currentFrameIndex_ >= frames_.size() - 1)
        {
            pausePlayback();
            setCurrentFrame(0);
            return;
        }

        setCurrentFrame(currentFrameIndex_ + 1);
    }

    void stepFrame(int delta)
    {
        if (frames_.isEmpty())
        {
            return;
        }

        pausePlayback();
        setCurrentFrame(currentFrameIndex_ + delta);
    }

    void setCurrentFrame(int index)
    {
        if (frames_.isEmpty())
        {
            currentFrameIndex_ = 0;
            if (currentFrameLabel_)
            {
                currentFrameLabel_->setText(QStringLiteral("--"));
            }
            if (currentTimeLabel_)
            {
                currentTimeLabel_->setText(QStringLiteral("--"));
            }
            if (currentCoordinateLabel_)
            {
                currentCoordinateLabel_->setText(QStringLiteral("--"));
            }
            if (currentSourceLabel_)
            {
                currentSourceLabel_->setText(QStringLiteral("--"));
            }
            if (currentDistanceLabel_)
            {
                currentDistanceLabel_->setText(QStringLiteral("--"));
            }
            if (currentAltitudeLabel_)
            {
                currentAltitudeLabel_->setText(QStringLiteral("--"));
            }
            if (currentAzimuthLabel_)
            {
                currentAzimuthLabel_->setText(QStringLiteral("--"));
            }
            refreshControls();
            return;
        }

        currentFrameIndex_ = qBound(0, index, frames_.size() - 1);
        const ReplayFrame &frame = frames_.at(currentFrameIndex_);

        if (timelineSlider_ && timelineSlider_->value() != currentFrameIndex_)
        {
            QSignalBlocker blocker(timelineSlider_);
            timelineSlider_->setValue(currentFrameIndex_);
        }

        if (currentFrameLabel_)
        {
            currentFrameLabel_->setText(QStringLiteral("%1 / %2").arg(currentFrameIndex_ + 1).arg(frames_.size()));
        }
        if (currentTimeLabel_)
        {
            currentTimeLabel_->setText(frame.detectedAt.isValid()
                                           ? frame.detectedAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
                                           : QStringLiteral("--"));
        }
        if (currentCoordinateLabel_)
        {
            currentCoordinateLabel_->setText(coordinateText(frame.longitude, frame.latitude));
        }
        if (currentSourceLabel_)
        {
            currentSourceLabel_->setText(frame.sourceType);
        }
        if (currentDistanceLabel_)
        {
            currentDistanceLabel_->setText(frame.distanceMeters > 0 ? QStringLiteral("%1 米").arg(frame.distanceMeters)
                                                                    : QStringLiteral("--"));
        }
        if (currentAltitudeLabel_)
        {
            currentAltitudeLabel_->setText(frame.altitudeMeters != 0 ? QStringLiteral("%1 米").arg(frame.altitudeMeters)
                                                                     : QStringLiteral("--"));
        }
        if (currentAzimuthLabel_)
        {
            currentAzimuthLabel_->setText(frame.azimuthDeg != 0 ? QStringLiteral("%1°").arg(frame.azimuthDeg)
                                                                : QStringLiteral("--"));
        }

        if (mapReady_ && webView_)
        {
            const QString js = QStringLiteral("if (typeof setReplayIndex === 'function') { setReplayIndex(%1); }")
                                   .arg(currentFrameIndex_);
            webView_->page()->runJavaScript(js);
        }

        refreshControls();
    }

    HistoryPage::HistoryRecord record_;
    QVector<ReplayFrame> frames_;
    QWidget *panelWidget_ = nullptr;
    QWebEngineView *webView_ = nullptr;
    QSlider *timelineSlider_ = nullptr;
    QPushButton *playButton_ = nullptr;
    QPushButton *prevButton_ = nullptr;
    QPushButton *nextButton_ = nullptr;
    QComboBox *speedCombo_ = nullptr;
    QLabel *frameSummaryLabel_ = nullptr;
    QLabel *currentFrameLabel_ = nullptr;
    QLabel *currentTimeLabel_ = nullptr;
    QLabel *currentCoordinateLabel_ = nullptr;
    QLabel *currentSourceLabel_ = nullptr;
    QLabel *currentDistanceLabel_ = nullptr;
    QLabel *currentAltitudeLabel_ = nullptr;
    QLabel *currentAzimuthLabel_ = nullptr;
    QTimer *playbackTimer_ = nullptr;
    bool mapReady_ = false;
    bool isPlaying_ = false;
    int currentFrameIndex_ = 0;
};

class HistoryPositionDialog : public QDialog
{
public:
    explicit HistoryPositionDialog(double lng, double lat, QWidget *parent = nullptr) : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("飞手位置"));
        setModal(true);
        resize(380, 200);
        setStyleSheet(QStringLiteral("QDialog { background-color: #1d1f23; color: #ffffff; }"
                                     "QLabel { color: #ffffff; font-size: 13px; }"));

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(18, 18, 18, 18);
        layout->setSpacing(12);

        QLabel *titleLabel = new QLabel(QStringLiteral("飞手位置"), this);
        titleLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 18px; font-weight: bold;"));
        layout->addWidget(titleLabel);

        QFrame *card = new QFrame(this);
        card->setStyleSheet(QStringLiteral("QFrame { background-color: #111214; border: 1px solid #2d3138; border-radius: 8px; }"));
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 16, 16, 16);
        cardLayout->setSpacing(10);

        QLabel *lngLabel = new QLabel(QStringLiteral("经度：%1").arg(lng, 0, 'f', 6), card);
        QLabel *latLabel = new QLabel(QStringLiteral("纬度：%1").arg(lat, 0, 'f', 6), card);
        QLabel *tipLabel = new QLabel(QStringLiteral("下一步可继续接地图预览。"), card);
        tipLabel->setStyleSheet(QStringLiteral("color: #9ea4ae; font-size: 12px;"));
        cardLayout->addWidget(lngLabel);
        cardLayout->addWidget(latLabel);
        cardLayout->addWidget(tipLabel);

        layout->addWidget(card);
        layout->addStretch();

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        QPushButton *closeButton = new QPushButton(QStringLiteral("关闭"), this);
        closeButton->setFixedSize(88, 34);
        closeButton->setStyleSheet(secondaryButtonStyle());
        connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
        buttonLayout->addWidget(closeButton);
        layout->addLayout(buttonLayout);
    }
};
} // namespace

HistoryPage::HistoryPage(QWidget *parent) : QWidget(parent)
{
    setupUi();
    resetFilters();
    renderCurrentPage();
}

void HistoryPage::setRecords(const QVector<HistoryRecord> &records)
{
    allRecords_ = records;
    renderCurrentPage();
}

void HistoryPage::setPagination(int totalCount, int currentPage)
{
    totalRecords_ = qMax(0, totalCount);
    const int totalPages = qMax(1, (totalRecords_ + pageSize_ - 1) / pageSize_);
    currentPage_ = qBound(1, currentPage, totalPages);
    updatePageInfo();
}

void HistoryPage::upsertRecord(const HistoryRecord &record)
{
    if (record.recordKey.trimmed().isEmpty())
    {
        return;
    }

    for (HistoryRecord &existingRecord : allRecords_)
    {
        if (existingRecord.recordKey == record.recordKey)
        {
            existingRecord = record;
            renderCurrentPage();
            return;
        }
    }

    allRecords_.append(record);
    renderCurrentPage();
}

void HistoryPage::removeRecord(const QString &recordKey)
{
    if (recordKey.trimmed().isEmpty())
    {
        return;
    }

    for (int i = 0; i < allRecords_.size(); ++i)
    {
        if (allRecords_.at(i).recordKey == recordKey)
        {
            allRecords_.removeAt(i);
            break;
        }
    }

    const int totalPages = qMax(1, (qMax(0, totalRecords_) + pageSize_ - 1) / pageSize_);
    currentPage_ = qMin(currentPage_, totalPages);
    renderCurrentPage();
}

void HistoryPage::clearRecords()
{
    allRecords_.clear();
    totalRecords_ = 0;
    currentPage_ = 1;
    renderCurrentPage();
}

HistoryPage::QueryCriteria HistoryPage::currentQueryCriteria() const
{
    QueryCriteria criteria;
    criteria.serialKeyword = serialEdit_ ? serialEdit_->text().trimmed() : QString();
    criteria.detectType = detectTypeCombo_ ? detectTypeCombo_->currentText().trimmed() : QStringLiteral("请选择");
    criteria.startTime =
        (startTimeEdit_ && startTimeEdit_->dateTime() != startTimeEdit_->minimumDateTime()) ? startTimeEdit_->dateTime() : QDateTime();
    criteria.endTime =
        (endTimeEdit_ && endTimeEdit_->dateTime() != endTimeEdit_->minimumDateTime()) ? endTimeEdit_->dateTime() : QDateTime();
    criteria.page = currentPage_;
    criteria.pageSize = pageSize_;
    return criteria;
}

void HistoryPage::setupUi()
{
    setObjectName(QStringLiteral("historyPage"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral("#historyPage { background-color: #202020; color: #ffffff; }"));

    QVBoxLayout *hostLayout = new QVBoxLayout(this);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }"
                              "QScrollBar:vertical { background: #1e1e1e; width: 10px; margin: 0px; }"
                              "QScrollBar::handle:vertical { background: #555555; min-height: 30px; border-radius: 4px; }"
                              "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
                              "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");
    hostLayout->addWidget(scrollArea);

    QWidget *content = new QWidget(scrollArea);
    content->setStyleSheet(QStringLiteral("background-color: #202020;"));
    scrollArea->setWidget(content);

    QVBoxLayout *pageLayout = new QVBoxLayout(content);
    pageLayout->setContentsMargins(10, 10, 10, 14);
    pageLayout->setSpacing(10);

    setupToolbar(pageLayout);
    setupFilterBar(pageLayout);
    setupTable(pageLayout);
    setupPagination(pageLayout);
}

bool HistoryPage::eventFilter(QObject *watched, QEvent *event)
{
    QLineEdit *startLine = startTimeEdit_ ? startTimeEdit_->findChild<QLineEdit *>() : nullptr;
    QLineEdit *endLine = endTimeEdit_ ? endTimeEdit_->findChild<QLineEdit *>() : nullptr;

    if ((watched == startTimeEdit_ || watched == startLine) && event->type() == QEvent::MouseButtonPress)
    {
        showTimePickerPopup(startTimeEdit_);
        return true;
    }

    if ((watched == endTimeEdit_ || watched == endLine) && event->type() == QEvent::MouseButtonPress)
    {
        showTimePickerPopup(endTimeEdit_);
        return true;
    }

    return QWidget::eventFilter(watched, event);
}

void HistoryPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateTimePickerPopupPosition();
}

void HistoryPage::setupToolbar(QVBoxLayout *pageLayout)
{
    QFrame *toolbarFrame = new QFrame(this);
    toolbarFrame->setStyleSheet(QStringLiteral("QFrame { background-color: transparent; }"));

    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbarFrame);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(8);

    exportButton_ = new QPushButton(QStringLiteral("导出"), toolbarFrame);
    exportButton_->setFixedSize(72, 32);
    exportButton_->setStyleSheet(subtleOrangeButtonStyle());

    batchReplayButton_ = new QPushButton(QStringLiteral("回放"), toolbarFrame);
    batchReplayButton_->setFixedSize(72, 32);
    batchReplayButton_->setStyleSheet(subtleOrangeButtonStyle());

    toolbarLayout->addWidget(exportButton_);
    toolbarLayout->addWidget(batchReplayButton_);
    toolbarLayout->addStretch();

    pageLayout->addWidget(toolbarFrame);
}

void HistoryPage::setupFilterBar(QVBoxLayout *pageLayout)
{
    QFrame *filterFrame = new QFrame(this);
    filterFrame->setStyleSheet(QStringLiteral("QFrame { background-color: #26282d; border-radius: 0px; }"));

    QHBoxLayout *filterLayout = new QHBoxLayout(filterFrame);
    filterLayout->setContentsMargins(10, 8, 10, 8);
    filterLayout->setSpacing(8);

    QLabel *serialLabel = new QLabel(QStringLiteral("序列号:"), filterFrame);
    serialLabel->setStyleSheet(QStringLiteral("color: #d7d8dc; font-size: 13px;"));
    serialEdit_ = new QLineEdit(filterFrame);
    serialEdit_->setPlaceholderText(QStringLiteral("请输入"));
    serialEdit_->setFixedSize(150, 32);
    serialEdit_->setStyleSheet(flatInputStyle());

    QLabel *statusLabel = new QLabel(QStringLiteral("侦测类型:"), filterFrame);
    statusLabel->setStyleSheet(QStringLiteral("color: #d7d8dc; font-size: 13px;"));
    detectTypeCombo_ = new QComboBox(filterFrame);
    detectTypeCombo_->addItems(QStringList() << QStringLiteral("请选择")
                                             << QStringLiteral("测向")
                                             << QStringLiteral("精准")
                                             << QStringLiteral("TDOA")
                                             << QStringLiteral("RID")
                                             << QStringLiteral("WIFI")
                                             << QStringLiteral("未知"));
    detectTypeCombo_->setFixedSize(130, 32);
    detectTypeCombo_->setStyleSheet(QStringLiteral("QComboBox { %1 }"
                                                   "QComboBox::drop-down { width: 28px; border-left: 1px solid #2a2a2a; }"
                                                   "QComboBox::down-arrow { image: none; }"
                                                   "QComboBox QAbstractItemView { background-color: #202225; color: #ffffff; "
                                                   "selection-background-color: #3a3a3a; border: 1px solid #2e2e2e; }")
                                        .arg(flatInputStyle()));

    QLabel *timeLabel = new QLabel(QStringLiteral("时间区间:"), filterFrame);
    timeLabel->setStyleSheet(QStringLiteral("color: #d7d8dc; font-size: 13px;"));
    startTimeEdit_ = new QDateTimeEdit(filterFrame);
    startTimeEdit_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    startTimeEdit_->setCalendarPopup(true);
    startTimeEdit_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    startTimeEdit_->setKeyboardTracking(false);
    startTimeEdit_->setMinimumDateTime(QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0)));
    startTimeEdit_->setSpecialValueText(QStringLiteral("开始日期"));
    startTimeEdit_->setDateTime(startTimeEdit_->minimumDateTime());
    startTimeEdit_->setFixedSize(142, 32);
    startTimeEdit_->setStyleSheet(QStringLiteral("QDateTimeEdit { %1 }"
                                                 "QDateTimeEdit::drop-down { subcontrol-origin: padding; subcontrol-position: top right; "
                                                 "width: 28px; border-left: 1px solid #2a2a2a; }"
                                                 "QDateTimeEdit::down-arrow { image: none; }")
                                      .arg(flatInputStyle()));
    startTimeEdit_->setReadOnly(true);
    startTimeEdit_->installEventFilter(this);
    if (QLineEdit *lineEdit = startTimeEdit_->findChild<QLineEdit *>())
    {
        lineEdit->setReadOnly(true);
        lineEdit->installEventFilter(this);
    }

    QLabel *separatorLabel = new QLabel(QStringLiteral("~"), filterFrame);
    separatorLabel->setStyleSheet(QStringLiteral("color: #9da3ad; font-size: 13px;"));
    endTimeEdit_ = new QDateTimeEdit(filterFrame);
    endTimeEdit_->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    endTimeEdit_->setCalendarPopup(true);
    endTimeEdit_->setButtonSymbols(QAbstractSpinBox::NoButtons);
    endTimeEdit_->setKeyboardTracking(false);
    endTimeEdit_->setMinimumDateTime(QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0)));
    endTimeEdit_->setSpecialValueText(QStringLiteral("结束日期"));
    endTimeEdit_->setDateTime(endTimeEdit_->minimumDateTime());
    endTimeEdit_->setFixedSize(142, 32);
    endTimeEdit_->setStyleSheet(QStringLiteral("QDateTimeEdit { %1 }"
                                               "QDateTimeEdit::drop-down { subcontrol-origin: padding; subcontrol-position: top right; "
                                               "width: 28px; border-left: 1px solid #2a2a2a; }"
                                               "QDateTimeEdit::down-arrow { image: none; }")
                                    .arg(flatInputStyle()));
    endTimeEdit_->setReadOnly(true);
    endTimeEdit_->installEventFilter(this);
    if (QLineEdit *lineEdit = endTimeEdit_->findChild<QLineEdit *>())
    {
        lineEdit->setReadOnly(true);
        lineEdit->installEventFilter(this);
    }

    searchButton_ = new QPushButton(QStringLiteral("搜索"), filterFrame);
    searchButton_->setFixedSize(72, 32);
    searchButton_->setStyleSheet(subtleOrangeButtonStyle());

    resetButton_ = new QPushButton(QStringLiteral("重置"), filterFrame);
    resetButton_->setFixedSize(72, 32);
    resetButton_->setStyleSheet(darkGhostButtonStyle());

    filterLayout->addWidget(serialLabel);
    filterLayout->addWidget(serialEdit_);
    filterLayout->addSpacing(4);
    filterLayout->addWidget(statusLabel);
    filterLayout->addWidget(detectTypeCombo_);
    filterLayout->addSpacing(4);
    filterLayout->addWidget(timeLabel);
    filterLayout->addWidget(startTimeEdit_);
    filterLayout->addWidget(separatorLabel);
    filterLayout->addWidget(endTimeEdit_);
    filterLayout->addStretch();
    filterLayout->addWidget(searchButton_);
    filterLayout->addWidget(resetButton_);

    connect(searchButton_, &QPushButton::clicked, this, &HistoryPage::applyFilters);
    connect(resetButton_, &QPushButton::clicked, this, &HistoryPage::resetFilters);

    pageLayout->addWidget(filterFrame);
}

void HistoryPage::setupTable(QVBoxLayout *pageLayout)
{
    QFrame *tableFrame = new QFrame(this);
    tableFrame->setStyleSheet(QStringLiteral("QFrame { background-color: #111214; border-radius: 0px; }"));

    QVBoxLayout *tableLayout = new QVBoxLayout(tableFrame);
    tableLayout->setContentsMargins(8, 0, 8, 0);
    tableLayout->setSpacing(0);

    table_ = new QTableWidget(tableFrame);
    table_->setColumnCount(10);
    table_->setHorizontalHeaderLabels(QStringList() << QStringLiteral("状态")
                                                    << QStringLiteral("型号")
                                                    << QStringLiteral("序列号")
                                                    << QStringLiteral("发现时间")
                                                    << QStringLiteral("中心频率")
                                                    << QStringLiteral("飞手位置")
                                                    << QStringLiteral("角度(°)")
                                                    << QStringLiteral("飞行高度(米)")
                                                    << QStringLiteral("停留时长")
                                                    << QStringLiteral("操作"));
    table_->verticalHeader()->setVisible(false);
    table_->setShowGrid(false);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setSelectionMode(QAbstractItemView::NoSelection);
    table_->setFocusPolicy(Qt::NoFocus);
    table_->setAlternatingRowColors(false);
    table_->setRowCount(pageSize_);
    table_->setStyleSheet(QStringLiteral(
        "QTableWidget { background-color: #111214; color: #f0f0f0; border: none; font-size: 13px; }"
        "QHeaderView::section { background-color: #17191d; color: #ffffff; border: none; border-bottom: 1px solid #2a2d33; "
        "padding: 12px 16px; font-size: 13px; font-weight: bold; }"
        "QTableWidget::item { border-bottom: 1px solid #26292f; padding: 8px 16px; }"));
    table_->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table_->horizontalHeader()->setStretchLastSection(false);
    table_->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    table_->setColumnWidth(0, 136);
    table_->setColumnWidth(1, 152);
    table_->setColumnWidth(2, 198);
    table_->setColumnWidth(3, 178);
    table_->setColumnWidth(4, 116);
    table_->setColumnWidth(5, 250);
    table_->setColumnWidth(6, 92);
    table_->setColumnWidth(7, 114);
    table_->setColumnWidth(8, 116);
    table_->setColumnWidth(9, 188);

    tableLayout->addWidget(table_);
    pageLayout->addWidget(tableFrame, 1);
}

void HistoryPage::setupPagination(QVBoxLayout *pageLayout)
{
    QFrame *paginationFrame = new QFrame(this);
    paginationFrame->setStyleSheet(QStringLiteral("QFrame { background-color: transparent; }"));

    QHBoxLayout *paginationLayout = new QHBoxLayout(paginationFrame);
    paginationLayout->setContentsMargins(0, 0, 0, 0);
    paginationLayout->setSpacing(6);

    pageInfoLabel_ = new QLabel(QStringLiteral("共 0 条"), paginationFrame);
    pageInfoLabel_->setStyleSheet(QStringLiteral("color: #c1c5cc; font-size: 13px;"));

    clearRecordsButton_ = new QPushButton(QStringLiteral("清空记录"), paginationFrame);
    clearRecordsButton_->setFixedSize(88, 30);
    clearRecordsButton_->setStyleSheet(QStringLiteral("QPushButton { background-color: #d9534f; color: #ffffff; border: none; "
                                                      "border-radius: 2px; font-size: 13px; font-weight: bold; }"
                                                      "QPushButton:hover { background-color: #e46864; }"));

    prevPageButton_ = new QPushButton(QStringLiteral("上一页"), paginationFrame);
    prevPageButton_->setFixedSize(78, 30);
    prevPageButton_->setStyleSheet(darkGhostButtonStyle());

    nextPageButton_ = new QPushButton(QStringLiteral("下一页"), paginationFrame);
    nextPageButton_->setFixedSize(78, 30);
    nextPageButton_->setStyleSheet(darkGhostButtonStyle());

    connect(clearRecordsButton_, &QPushButton::clicked, this, &HistoryPage::clearAllRecords);
    connect(prevPageButton_, &QPushButton::clicked, this, &HistoryPage::goToPreviousPage);
    connect(nextPageButton_, &QPushButton::clicked, this, &HistoryPage::goToNextPage);

    paginationLayout->addWidget(clearRecordsButton_);
    paginationLayout->addSpacing(10);
    paginationLayout->addWidget(pageInfoLabel_);
    paginationLayout->addStretch();
    paginationLayout->addWidget(prevPageButton_);
    paginationLayout->addWidget(nextPageButton_);

    pageLayout->addWidget(paginationFrame);
}

void HistoryPage::loadMockData()
{
    const QDateTime now = QDateTime::currentDateTime();
    allRecords_ = {
        {QStringLiteral("mock-1"), false, QStringLiteral("RID"), QStringLiteral("Mavic 2 Zoom"), QStringLiteral("0M6CHTJR0A0874"), false, now.addSecs(-180), now.addSecs(-155), 2445000, 0.0, 0.0, 255, 0, 25},
        {QStringLiteral("mock-2"), false, QStringLiteral("RID"), QStringLiteral("Mavic 2 Zoom"), QStringLiteral("0M6CHTJR0A0874"), true, now.addSecs(-3911), now.addSecs(-3800), 2445000, 0.0, 0.0, 255, 0, 111},
        {QStringLiteral("mock-3"), false, QStringLiteral("未知"), QStringLiteral("Unknown DJI UAV"), QStringLiteral("**********90bca"), false, now.addDays(-1).addSecs(-4000), now.addDays(-1).addSecs(-2344), 2430000, 0.0, 0.0, 255, 0, 1656},
        {QStringLiteral("mock-4"), false, QStringLiteral("WIFI"), QStringLiteral("Unknown DJI UAV"), QStringLiteral("**********3b3c50"), false, now.addDays(-1).addSecs(-2600), now.addDays(-1).addSecs(-2063), 2445000, 0.0, 0.0, 255, 0, 537},
        {QStringLiteral("mock-5"), false, QStringLiteral("WIFI"), QStringLiteral("Unknown DJI UAV"), QStringLiteral("**********09138a"), false, now.addDays(-1).addSecs(-1500), now.addDays(-1).addSecs(-1450), 2445000, 0.0, 0.0, 255, 0, 50},
        {QStringLiteral("mock-6"), true, QStringLiteral("测向"), QStringLiteral("Mavic 2 Zoom"), QStringLiteral("0M6CHTJR0A0874"), true, now.addSecs(-174), now.addSecs(-5), 2445000, 120.089000, 30.342000, 255, 0, 169},
        {QStringLiteral("mock-7"), true, QStringLiteral("测向"), QStringLiteral("Mavic 2 Zoom"), QStringLiteral("0M6CHTJR0A0874"), true, now.addSecs(-162), now.addSecs(-120), 2445000, 120.089000, 30.342000, 255, 0, 42},
        {QStringLiteral("mock-8"), true, QStringLiteral("精准"), QStringLiteral("Mavic 2 Zoom"), QStringLiteral("0M6CHTJR0A0874"), true, now.addSecs(-150), now.addSecs(-120), 2445000, 120.089000, 30.342000, 255, 0, 30},
        {QStringLiteral("mock-9"), true, QStringLiteral("精准"), QStringLiteral("Mavic 2 Zoom"), QStringLiteral("0M6CHTJR0A0874"), true, now.addSecs(-128), now.addSecs(-10), 2445000, 120.089000, 30.342000, 255, 0, 161},
        {QStringLiteral("mock-10"), true, QStringLiteral("TDOA"), QStringLiteral("Mavic 2 Zoom"), QStringLiteral("0M6CHTJR0A0874"), true, now.addSecs(-102), now.addSecs(-60), 2445000, 120.089000, 30.342000, 255, 0, 42},
        {QStringLiteral("mock-11"), false, QStringLiteral("WIFI"), QStringLiteral("DJI FPV"), QStringLiteral("1A2B3C4D5E6F7"), false, now.addDays(-2), now.addDays(-2).addSecs(3720), 5745000, 119.998001, 30.100002, 123, 86, 3720},
        {QStringLiteral("mock-12"), true, QStringLiteral("RID"), QStringLiteral("Autel EVO II"), QStringLiteral("AUTEL-TEST-0001"), false, now.addSecs(-520), now.addSecs(-5), 915000, 118.223344, 29.998877, 96, 120, 520},
    };
}

void HistoryPage::applyFilters()
{
    currentPage_ = 1;
    emit historyQueryRequested();
}

void HistoryPage::resetFilters()
{
    if (serialEdit_)
    {
        serialEdit_->clear();
    }
    if (detectTypeCombo_)
    {
        detectTypeCombo_->setCurrentIndex(0);
    }
    if (startTimeEdit_)
    {
        startTimeEdit_->setDateTime(startTimeEdit_->minimumDateTime());
    }
    if (endTimeEdit_)
    {
        endTimeEdit_->setDateTime(endTimeEdit_->minimumDateTime());
    }

    currentPage_ = 1;
    emit historyQueryRequested();
}

void HistoryPage::goToPreviousPage()
{
    if (currentPage_ <= 1)
    {
        return;
    }

    --currentPage_;
    emit historyQueryRequested();
}

void HistoryPage::clearAllRecords()
{
    emit clearRecordsRequested();
}

void HistoryPage::goToNextPage()
{
    const int totalPages = qMax(1, (totalRecords_ + pageSize_ - 1) / pageSize_);
    if (currentPage_ >= totalPages)
    {
        return;
    }

    ++currentPage_;
    emit historyQueryRequested();
}

void HistoryPage::renderCurrentPage()
{
    if (!table_)
    {
        return;
    }

    table_->clearContents();
    table_->setRowCount(pageSize_);
    const int visibleRecordCount = qMin(pageSize_, allRecords_.size());

    for (int row = 0; row < pageSize_; ++row)
    {
        if (row >= visibleRecordCount)
        {
            table_->setCellWidget(row, 0, nullptr);
            table_->setCellWidget(row, 2, nullptr);
            table_->setCellWidget(row, 5, nullptr);
            table_->setCellWidget(row, 9, nullptr);
            for (int column = 0; column < table_->columnCount(); ++column)
            {
                if (column == 0 || column == 2 || column == 5 || column == 9)
                {
                    continue;
                }
                table_->setItem(row, column, new QTableWidgetItem(QString()));
            }
            continue;
        }

        const HistoryRecord &record = allRecords_.at(row);

        QWidget *statusWidget = new QWidget(table_);
        QHBoxLayout *statusLayout = new QHBoxLayout(statusWidget);
        statusLayout->setContentsMargins(0, 0, 0, 0);
        statusLayout->setSpacing(0);
        QLabel *statusChip = createChipLabel(record.active ? QStringLiteral("侦测中") : QStringLiteral("侦测结束"),
                                             record.active ? QStringLiteral("#163323") : QStringLiteral("#30333a"),
                                             record.active ? QStringLiteral("#22d35e") : QStringLiteral("#cfd3da"),
                                             statusWidget);
        statusChip->setMinimumWidth(74);
        statusChip->setStyleSheet(QStringLiteral("background-color: %1; color: %2; border-radius: 9px; padding: 4px 12px; "
                                                 "font-size: 12px; font-weight: bold;")
                                      .arg(record.active ? QStringLiteral("#163323") : QStringLiteral("#30333a"),
                                           record.active ? QStringLiteral("#22d35e") : QStringLiteral("#cfd3da")));
        statusLayout->addWidget(statusChip, 0, Qt::AlignCenter);
        table_->setCellWidget(row, 0, statusWidget);

        QTableWidgetItem *modelItem = new QTableWidgetItem(record.modelName);
        modelItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, 1, modelItem);

        QLabel *serialLabel = new QLabel(record.serialNumber, table_);
        serialLabel->setAlignment(Qt::AlignCenter);
        serialLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 13px;"));
        table_->setCellWidget(row, 2, serialLabel);

        QTableWidgetItem *timeItem = new QTableWidgetItem(record.foundTime.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        timeItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, 3, timeItem);

        QTableWidgetItem *freqItem = new QTableWidgetItem(formatFrequency(record.centerFrequencyKhz));
        freqItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, 4, freqItem);

        QWidget *positionWidget = new QWidget(table_);
        QHBoxLayout *positionLayout = new QHBoxLayout(positionWidget);
        positionLayout->setContentsMargins(0, 0, 0, 0);
        positionLayout->setSpacing(0);

        QPushButton *positionButton = new QPushButton(
            QStringLiteral("%1,%2").arg(record.pilotLongitude, 0, 'f', 6).arg(record.pilotLatitude, 0, 'f', 6), positionWidget);
        positionButton->setCursor(Qt::PointingHandCursor);
        positionButton->setMinimumWidth(220);
        positionButton->setStyleSheet(QStringLiteral("QPushButton { background: transparent; color: #2f80ed; border: none; "
                                                     "font-size: 13px; padding: 0; }"
                                                     "QPushButton:hover { color: #66a9ff; text-decoration: underline; }"));
        connect(positionButton, &QPushButton::clicked, this,
                [this, record]()
                {
                    showPositionDialog(record);
                });
        positionLayout->addWidget(positionButton, 0, Qt::AlignCenter);
        table_->setCellWidget(row, 5, positionWidget);

        QTableWidgetItem *azimuthItem = new QTableWidgetItem(QString::number(record.azimuthDeg));
        azimuthItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, 6, azimuthItem);

        QTableWidgetItem *altitudeItem = new QTableWidgetItem(QString::number(record.flightAltitudeMeters));
        altitudeItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, 7, altitudeItem);

        QTableWidgetItem *durationItem = new QTableWidgetItem(formatDuration(record.stayDurationSeconds));
        durationItem->setTextAlignment(Qt::AlignCenter);
        table_->setItem(row, 8, durationItem);

        QWidget *actionWidget = new QWidget(table_);
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(12, 0, 12, 0);
        actionLayout->setSpacing(14);
        actionLayout->setAlignment(Qt::AlignCenter);
        QPushButton *replayButton = new QPushButton(QStringLiteral("回放"), actionWidget);
        replayButton->setFixedSize(64, 30);
        replayButton->setCursor(Qt::PointingHandCursor);
        replayButton->setStyleSheet(QStringLiteral("QPushButton { background: transparent; color: #f2994a; border: none; "
                                                   "font-size: 15px; font-weight: bold; padding: 0px; }"
                                                   "QPushButton:hover { color: #f7b26f; }"));
        connect(replayButton, &QPushButton::clicked, this,
                [this, record]()
                {
                    emit replayRequested(record.recordKey);
                });

        QPushButton *detailButton = new QPushButton(QStringLiteral("详情"), actionWidget);
        detailButton->setFixedSize(64, 30);
        detailButton->setCursor(Qt::PointingHandCursor);
        detailButton->setStyleSheet(QStringLiteral("QPushButton { background: transparent; color: #f2994a; border: none; "
                                                   "font-size: 15px; font-weight: bold; padding: 0px; }"
                                                   "QPushButton:hover { color: #f7b26f; }"));
        connect(detailButton, &QPushButton::clicked, this,
                [this, record]()
                {
                    emit detailRequested(record.recordKey);
                });

        actionLayout->addWidget(replayButton);
        actionLayout->addWidget(detailButton);
        table_->setCellWidget(row, 9, actionWidget);
    }

    updatePageInfo();
}

void HistoryPage::updatePageInfo()
{
    const int totalPages = qMax(1, (totalRecords_ + pageSize_ - 1) / pageSize_);

    if (pageInfoLabel_)
    {
        pageInfoLabel_->setText(QStringLiteral("共 %1 条，第 %2 / %3 页").arg(totalRecords_).arg(currentPage_).arg(totalPages));
    }

    if (prevPageButton_)
    {
        prevPageButton_->setEnabled(currentPage_ > 1);
    }
    if (nextPageButton_)
    {
        nextPageButton_->setEnabled(currentPage_ < totalPages);
    }
}

QString HistoryPage::formatFrequency(double frequencyKhz) const
{
    if (frequencyKhz >= 1000000.0)
    {
        return QStringLiteral("%1GHz").arg(QString::number(frequencyKhz / 1000000.0, 'f', 3));
    }
    return QStringLiteral("%1MHz").arg(QString::number(frequencyKhz / 1000.0, 'f', 0));
}

QString HistoryPage::formatDuration(qint64 seconds) const
{
    const qint64 safeSeconds = qMax<qint64>(0, seconds);
    const qint64 hours = safeSeconds / 3600;
    const qint64 minutes = (safeSeconds % 3600) / 60;
    const qint64 remainSeconds = safeSeconds % 60;

    if (hours > 0)
    {
        return QStringLiteral("%1小时%2分%3秒").arg(hours).arg(minutes).arg(remainSeconds);
    }
    if (minutes > 0)
    {
        return QStringLiteral("%1分%2秒").arg(minutes).arg(remainSeconds);
    }
    return QStringLiteral("%1秒").arg(remainSeconds);
}

const HistoryPage::HistoryRecord *HistoryPage::findRecordByKey(const QString &recordKey) const
{
    if (recordKey.trimmed().isEmpty())
    {
        return nullptr;
    }

    for (const HistoryRecord &record : allRecords_)
    {
        if (record.recordKey == recordKey)
        {
            return &record;
        }
    }
    return nullptr;
}

void HistoryPage::showDetailDialog(const QString &recordKey, const QVector<HistoryDetailEntry> &details)
{
    const HistoryRecord *record = findRecordByKey(recordKey);
    if (!record)
    {
        return;
    }

    QVector<HistoryDetailEntry> dialogDetails = details;
    if (dialogDetails.isEmpty())
    {
        HistoryDetailEntry fallbackDetail;
        fallbackDetail.recordKey = record->recordKey;
        fallbackDetail.modelName = record->modelName;
        fallbackDetail.serialNumber = record->serialNumber;
        fallbackDetail.centerFrequencyKhz = record->centerFrequencyKhz;
        fallbackDetail.pilotLongitude = record->pilotLongitude;
        fallbackDetail.pilotLatitude = record->pilotLatitude;
        fallbackDetail.azimuthDeg = record->azimuthDeg;
        fallbackDetail.flightAltitudeMeters = record->flightAltitudeMeters;
        fallbackDetail.active = record->active;
        fallbackDetail.detectedAt = record->lastSeenTime.isValid() ? record->lastSeenTime : record->foundTime;
        dialogDetails.append(fallbackDetail);
    }

    QWidget *dialogParent = window();
    if (!dialogParent)
    {
        dialogParent = this;
    }

    HistoryDetailDialog *dialog = new HistoryDetailDialog(*record, dialogDetails, dialogParent);
    dialog->showOverlay();
}

void HistoryPage::showPositionDialog(const HistoryRecord &record)
{
    HistoryPositionDialog dialog(record.pilotLongitude, record.pilotLatitude, this);
    dialog.exec();
}

void HistoryPage::showReplayDialog(const QString &recordKey, const QVector<HistoryDetailEntry> &details)
{
    const HistoryRecord *record = findRecordByKey(recordKey);
    if (!record)
    {
        return;
    }

    QWidget *overlayParent = window();
    if (!overlayParent)
    {
        overlayParent = this;
    }

    QVector<HistoryDetailEntry> replayDetails = details;
    if (replayDetails.size() < 2)
    {
        replayDetails = buildMockReplayDetails(*record);
    }

    HistoryReplayOverlay *overlay = new HistoryReplayOverlay(*record, replayDetails, overlayParent);
    overlay->showOverlay();
}

void HistoryPage::ensureTimePickerPopup()
{
    if (timePickerPopup_)
    {
        return;
    }

    timePickerPopup_ = new QWidget(this);
    timePickerPopup_->setObjectName(QStringLiteral("historyTimePickerPopup"));
    timePickerPopup_->setAttribute(Qt::WA_StyledBackground, true);
    timePickerPopup_->setStyleSheet("#historyTimePickerPopup { background-color: #1f1f22; border: 1px solid #34343a; border-radius: 8px; }");
    timePickerPopup_->setFixedSize(560, 330);
    timePickerPopup_->hide();

    QVBoxLayout *popupLayout = new QVBoxLayout(timePickerPopup_);
    popupLayout->setContentsMargins(14, 14, 14, 14);
    popupLayout->setSpacing(12);

    const int calendarAreaWidth = 300;
    const int separatorWidth = 1;
    const int areaSpacing = 12;
    const int timeColumnWidth = 48;
    const int timeAreaLeftPadding = 8;
    const int rightTimeAreaWidth = timeAreaLeftPadding + timeColumnWidth * 3 + separatorWidth * 2;
    const int alignedRowWidth = calendarAreaWidth + separatorWidth + rightTimeAreaWidth + areaSpacing * 2;

    auto createHeaderButton = [this](const QString &text) -> QPushButton *
    {
        QPushButton *button = new QPushButton(text, timePickerPopup_);
        button->setFixedSize(30, 24);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet("QPushButton { background-color: transparent; color: #d7dbe1; border: none; "
                              "padding: 0px; font-size: 14px; font-weight: 600; }"
                              "QPushButton:hover { color: #ffffff; background-color: #2b2b2f; border-radius: 4px; }"
                              "QPushButton:pressed { background-color: #323238; }");
        return button;
    };

    timePickerPrevYearButton_ = createHeaderButton(QStringLiteral("<<"));
    timePickerPrevMonthButton_ = createHeaderButton(QStringLiteral("<"));
    timePickerNextMonthButton_ = createHeaderButton(QStringLiteral(">"));
    timePickerNextYearButton_ = createHeaderButton(QStringLiteral(">>"));

    timePickerHeaderLabel_ = new QLabel(timePickerPopup_);
    timePickerHeaderLabel_->setAlignment(Qt::AlignCenter);
    timePickerHeaderLabel_->setFixedWidth(120);
    timePickerHeaderLabel_->setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold;");

    QLabel *timePreviewLabel = new QLabel(timePickerPopup_);
    timePreviewLabel->setAlignment(Qt::AlignCenter);
    timePreviewLabel->setFixedWidth(rightTimeAreaWidth - timeAreaLeftPadding);
    timePreviewLabel->setStyleSheet("color: #ffffff; font-size: 16px; font-weight: bold;");

    QWidget *headerRowWidget = new QWidget(timePickerPopup_);
    headerRowWidget->setFixedWidth(alignedRowWidth);
    headerRowWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRowWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(areaSpacing);

    QWidget *leftHeaderWidget = new QWidget(headerRowWidget);
    leftHeaderWidget->setFixedWidth(calendarAreaWidth);
    leftHeaderWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *leftHeaderLayout = new QHBoxLayout(leftHeaderWidget);
    leftHeaderLayout->setContentsMargins(2, 0, 2, 0);
    leftHeaderLayout->setSpacing(4);

    QFrame *headerSeparator = new QFrame(timePickerPopup_);
    headerSeparator->setFixedWidth(separatorWidth);
    headerSeparator->setFixedHeight(24);
    headerSeparator->setStyleSheet("background-color: rgba(255, 255, 255, 120);");

    leftHeaderLayout->addWidget(timePickerPrevYearButton_);
    leftHeaderLayout->addWidget(timePickerPrevMonthButton_);
    leftHeaderLayout->addStretch();
    leftHeaderLayout->addWidget(timePickerHeaderLabel_, 0, Qt::AlignCenter);
    leftHeaderLayout->addStretch();
    leftHeaderLayout->addWidget(timePickerNextMonthButton_);
    leftHeaderLayout->addWidget(timePickerNextYearButton_);

    QWidget *rightHeaderWidget = new QWidget(headerRowWidget);
    rightHeaderWidget->setFixedWidth(rightTimeAreaWidth);
    rightHeaderWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *rightHeaderLayout = new QHBoxLayout(rightHeaderWidget);
    rightHeaderLayout->setContentsMargins(timeAreaLeftPadding, 0, 0, 0);
    rightHeaderLayout->setSpacing(0);
    rightHeaderLayout->addStretch();
    rightHeaderLayout->addWidget(timePreviewLabel, 0, Qt::AlignCenter);
    rightHeaderLayout->addStretch();

    headerLayout->addWidget(leftHeaderWidget, 0, Qt::AlignLeft);
    headerLayout->addWidget(headerSeparator, 0, Qt::AlignVCenter);
    headerLayout->addWidget(rightHeaderWidget, 0, Qt::AlignLeft);
    popupLayout->addWidget(headerRowWidget, 0, Qt::AlignHCenter);

    QWidget *contentRowWidget = new QWidget(timePickerPopup_);
    contentRowWidget->setFixedWidth(alignedRowWidth);
    contentRowWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *contentLayout = new QHBoxLayout(contentRowWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(areaSpacing);

    timePickerCalendar_ = new QCalendarWidget(timePickerPopup_);
    timePickerCalendar_->setGridVisible(false);
    timePickerCalendar_->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    timePickerCalendar_->setNavigationBarVisible(false);
    timePickerCalendar_->setFixedSize(calendarAreaWidth, 222);
    timePickerCalendar_->setStyleSheet("QCalendarWidget { background-color: #1f1f22; color: #ffffff; border: none; }"
                                       "QCalendarWidget QWidget#qt_calendar_navigationbar { background-color: #1f1f22; }"
                                       "QCalendarWidget QWidget#qt_calendar_calendarview { background-color: #1f1f22; "
                                       "alternate-background-color: #1f1f22; }"
                                       "QCalendarWidget QTableView { background-color: #1f1f22; alternate-background-color: #1f1f22; "
                                       "selection-background-color: #e58b3e; selection-color: #ffffff; color: #ffffff; }"
                                       "QCalendarWidget QAbstractItemView:enabled { color: #ffffff; selection-color: #ffffff; }"
                                       "QCalendarWidget QHeaderView::section { background-color: #1f1f22; color: #ffffff; "
                                       "border: none; padding: 6px 0; font-size: 14px; font-weight: bold; }");
    applyCalendarTextColors(timePickerCalendar_);
    contentLayout->addWidget(timePickerCalendar_);

    QFrame *calendarTimeSeparator = new QFrame(timePickerPopup_);
    calendarTimeSeparator->setFixedWidth(separatorWidth);
    calendarTimeSeparator->setFixedHeight(222);
    calendarTimeSeparator->setStyleSheet("background-color: rgba(255, 255, 255, 120);");
    contentLayout->addWidget(calendarTimeSeparator);

    auto createTimeList = [this, timeColumnWidth]() -> QListWidget *
    {
        QListWidget *list = new QListWidget(timePickerPopup_);
        list->setFixedSize(timeColumnWidth, 222);
        list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        list->setFocusPolicy(Qt::NoFocus);
        list->setStyleSheet("QListWidget { background-color: transparent; color: #ffffff; border: none; outline: none; "
                            "font-size: 14px; }"
                            "QListWidget::item { height: 36px; }"
                            "QListWidget::item:selected { background-color: #4a2c12; color: #ffffff; border-radius: 4px; }"
                            "QScrollBar:vertical { width: 8px; background: transparent; margin: 2px 0; }"
                            "QScrollBar::handle:vertical { background: #6b6f76; border-radius: 4px; min-height: 28px; }"
                            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical, "
                            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; height: 0px; }");
        return list;
    };

    timePickerHourList_ = createTimeList();
    timePickerMinuteList_ = createTimeList();
    timePickerSecondList_ = createTimeList();

    for (int hour = 0; hour < 24; ++hour)
    {
        timePickerHourList_->addItem(QStringLiteral("%1").arg(hour, 2, 10, QLatin1Char('0')));
    }
    for (int value = 0; value < 60; ++value)
    {
        const QString text = QStringLiteral("%1").arg(value, 2, 10, QLatin1Char('0'));
        timePickerMinuteList_->addItem(text);
        timePickerSecondList_->addItem(text);
    }

    QWidget *rightContentWidget = new QWidget(contentRowWidget);
    rightContentWidget->setFixedWidth(rightTimeAreaWidth);
    rightContentWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *timeListsLayout = new QHBoxLayout(rightContentWidget);
    timeListsLayout->setContentsMargins(timeAreaLeftPadding, 0, 0, 0);
    timeListsLayout->setSpacing(0);
    timeListsLayout->addWidget(timePickerHourList_);
    timeListsLayout->addWidget(timePickerMinuteList_);
    timeListsLayout->addWidget(timePickerSecondList_);
    contentLayout->addWidget(rightContentWidget);

    popupLayout->addWidget(contentRowWidget, 0, Qt::AlignHCenter);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(12);

    timePickerNowButton_ = new QPushButton(QStringLiteral("此刻"), timePickerPopup_);
    timePickerNowButton_->setFixedSize(76, 34);
    timePickerNowButton_->setStyleSheet(
        "QPushButton { background-color: transparent; color: #59a6ff; border: none; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { color: #7db8ff; }");

    timePickerConfirmButton_ = new QPushButton(QStringLiteral("确定"), timePickerPopup_);
    timePickerConfirmButton_->setFixedSize(76, 34);
    timePickerConfirmButton_->setStyleSheet(
        "QPushButton { background-color: #e58b3e; color: #ffffff; border: none; border-radius: 6px; "
        "font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #f09a4f; }");

    buttonLayout->addWidget(timePickerNowButton_, 0, Qt::AlignLeft);
    buttonLayout->addStretch();
    buttonLayout->addWidget(timePickerConfirmButton_, 0, Qt::AlignRight);
    popupLayout->addLayout(buttonLayout);

    connect(timePickerNowButton_, &QPushButton::clicked, this,
            [this]()
            {
                if (!activeTimeEdit_)
                {
                    return;
                }

                activeTimeEdit_->setDateTime(QDateTime::currentDateTime());
                syncTimePickerSelectionFromEdit();
            });
    connect(timePickerConfirmButton_, &QPushButton::clicked, this, &HistoryPage::applyTimePickerSelection);
    connect(timePickerPrevYearButton_, &QPushButton::clicked, timePickerCalendar_, &QCalendarWidget::showPreviousYear);
    connect(timePickerPrevMonthButton_, &QPushButton::clicked, timePickerCalendar_, &QCalendarWidget::showPreviousMonth);
    connect(timePickerNextMonthButton_, &QPushButton::clicked, timePickerCalendar_, &QCalendarWidget::showNextMonth);
    connect(timePickerNextYearButton_, &QPushButton::clicked, timePickerCalendar_, &QCalendarWidget::showNextYear);
    connect(timePickerCalendar_, &QCalendarWidget::currentPageChanged, this, &HistoryPage::updateTimePickerHeader);

    auto updatePreview = [this, timePreviewLabel]()
    {
        if (!timePickerHourList_ || !timePickerMinuteList_ || !timePickerSecondList_)
        {
            return;
        }

        timePreviewLabel->setText(QStringLiteral("%1:%2:%3")
                                      .arg(qMax(0, timePickerHourList_->currentRow()), 2, 10, QLatin1Char('0'))
                                      .arg(qMax(0, timePickerMinuteList_->currentRow()), 2, 10, QLatin1Char('0'))
                                      .arg(qMax(0, timePickerSecondList_->currentRow()), 2, 10, QLatin1Char('0')));
    };

    connect(timePickerHourList_, &QListWidget::currentRowChanged, this, [updatePreview](int) { updatePreview(); });
    connect(timePickerMinuteList_, &QListWidget::currentRowChanged, this, [updatePreview](int) { updatePreview(); });
    connect(timePickerSecondList_, &QListWidget::currentRowChanged, this, [updatePreview](int) { updatePreview(); });

    updateTimePickerHeader();
}

void HistoryPage::updateTimePickerPopupPosition()
{
    if (!timePickerPopup_ || !activeTimeEdit_)
    {
        return;
    }

    const QPoint anchorBottomLeft = activeTimeEdit_->mapTo(this, QPoint(0, activeTimeEdit_->height() + 8));
    int x = anchorBottomLeft.x();
    int y = anchorBottomLeft.y();

    if (x + timePickerPopup_->width() > width() - 12)
    {
        x = width() - timePickerPopup_->width() - 12;
    }
    if (x < 12)
    {
        x = 12;
    }
    if (y + timePickerPopup_->height() > height() - 12)
    {
        y = activeTimeEdit_->mapTo(this, QPoint(0, -timePickerPopup_->height() - 8)).y();
    }
    if (y < 12)
    {
        y = 12;
    }

    timePickerPopup_->move(x, y);
}

void HistoryPage::updateTimePickerHeader()
{
    if (!timePickerCalendar_ || !timePickerHeaderLabel_)
    {
        return;
    }

    timePickerHeaderLabel_->setText(
        QStringLiteral("%1年 %2月").arg(timePickerCalendar_->yearShown()).arg(timePickerCalendar_->monthShown()));
}

void HistoryPage::syncTimePickerSelectionFromEdit()
{
    if (!activeTimeEdit_)
    {
        return;
    }

    ensureTimePickerPopup();
    const QDateTime dateTime = activeTimeEdit_->dateTime();
    timePickerCalendar_->setSelectedDate(dateTime.date());
    timePickerCalendar_->showSelectedDate();

    auto selectRow = [](QListWidget *list, int row)
    {
        if (!list || row < 0 || row >= list->count())
        {
            return;
        }
        list->setCurrentRow(row);
        if (QListWidgetItem *item = list->item(row))
        {
            list->scrollToItem(item, QAbstractItemView::PositionAtTop);
        }
    };

    selectRow(timePickerHourList_, dateTime.time().hour());
    selectRow(timePickerMinuteList_, dateTime.time().minute());
    selectRow(timePickerSecondList_, dateTime.time().second());
}

void HistoryPage::applyTimePickerSelection()
{
    if (!activeTimeEdit_ || !timePickerCalendar_ || !timePickerHourList_ || !timePickerMinuteList_ || !timePickerSecondList_)
    {
        return;
    }

    const int hour = qMax(0, timePickerHourList_->currentRow());
    const int minute = qMax(0, timePickerMinuteList_->currentRow());
    const int second = qMax(0, timePickerSecondList_->currentRow());
    const QDate selectedDate = timePickerCalendar_->selectedDate();
    activeTimeEdit_->setDateTime(QDateTime(selectedDate, QTime(hour, minute, second)));
    hideTimePickerPopup();
}

void HistoryPage::showTimePickerPopup(QDateTimeEdit *targetEdit)
{
    if (!targetEdit)
    {
        return;
    }

    activeTimeEdit_ = targetEdit;
    ensureTimePickerPopup();
    syncTimePickerSelectionFromEdit();
    updateTimePickerPopupPosition();
    timePickerPopup_->show();
    timePickerPopup_->raise();
}

void HistoryPage::hideTimePickerPopup()
{
    if (!timePickerPopup_)
    {
        return;
    }

    timePickerPopup_->hide();
}
