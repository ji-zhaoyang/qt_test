#include "datetime_picker_popup.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCalendarWidget>
#include <QCoreApplication>
#include <QColor>
#include <QDateTimeEdit>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPalette>
#include <QPushButton>
#include <QTextCharFormat>
#include <QVBoxLayout>
#include <QtGlobal>

namespace
{
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
} // namespace

DateTimePickerPopup::DateTimePickerPopup(QWidget *positionParent, QObject *parent)
    : QObject(parent), positionParent_(positionParent), appFilterInstalled_(false), activeEdit_(nullptr), popup_(nullptr),
      calendar_(nullptr), headerLabel_(nullptr), prevYearButton_(nullptr), prevMonthButton_(nullptr),
      nextMonthButton_(nullptr), nextYearButton_(nullptr), hourList_(nullptr), minuteList_(nullptr),
      secondList_(nullptr), nowButton_(nullptr), confirmButton_(nullptr), timePreviewLabel_(nullptr)
{
    if (positionParent_)
    {
        positionParent_->installEventFilter(this);
    }
}

void DateTimePickerPopup::registerDateTimeEdit(QDateTimeEdit *edit)
{
    if (!edit || registeredEdits_.contains(edit))
    {
        return;
    }

    registeredEdits_.append(edit);
    edit->installEventFilter(this);
    if (QLineEdit *lineEdit = edit->findChild<QLineEdit *>())
    {
        lineEdit->installEventFilter(this);
    }
}

DateTimePickerPopup::~DateTimePickerPopup()
{
    removeAppEventFilter();
}

bool DateTimePickerPopup::isVisible() const
{
    return popup_ && popup_->isVisible();
}

bool DateTimePickerPopup::isRegisteredEditWidget(QWidget *widget) const
{
    while (widget)
    {
        if (registeredEdits_.contains(qobject_cast<QDateTimeEdit *>(widget)))
        {
            return true;
        }
        widget = widget->parentWidget();
    }
    return false;
}

bool DateTimePickerPopup::isClickInsidePopup(const QPoint &globalPos) const
{
    if (!popup_ || !popup_->isVisible())
    {
        return false;
    }

    return popup_->rect().contains(popup_->mapFromGlobal(globalPos));
}

void DateTimePickerPopup::installAppEventFilter()
{
    if (appFilterInstalled_)
    {
        return;
    }

    if (QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance()))
    {
        app->installEventFilter(this);
        appFilterInstalled_ = true;
    }
}

void DateTimePickerPopup::removeAppEventFilter()
{
    if (!appFilterInstalled_)
    {
        return;
    }

    if (QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance()))
    {
        app->removeEventFilter(this);
    }
    appFilterInstalled_ = false;
}

void DateTimePickerPopup::ensurePopup()
{
    if (popup_)
    {
        return;
    }

    popup_ = new QWidget(positionParent_);
    popup_->setObjectName(QStringLiteral("dateTimePickerPopup"));
    popup_->setAttribute(Qt::WA_StyledBackground, true);
    popup_->setStyleSheet(QStringLiteral("#dateTimePickerPopup { background-color: #1f1f22; border: 1px solid #34343a; "
                                         "border-radius: 8px; }"));
    popup_->setFixedSize(560, 330);
    popup_->hide();
    popup_->setAttribute(Qt::WA_ShowWithoutActivating, true);

    QVBoxLayout *popupLayout = new QVBoxLayout(popup_);
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
        QPushButton *button = new QPushButton(text, popup_);
        button->setFixedSize(30, 24);
        button->setCursor(Qt::PointingHandCursor);
        button->setStyleSheet(QStringLiteral("QPushButton { background-color: transparent; color: #d7dbe1; border: none; "
                                             "padding: 0px; font-size: 14px; font-weight: 600; }"
                                             "QPushButton:hover { color: #ffffff; background-color: #2b2b2f; border-radius: 4px; }"
                                             "QPushButton:pressed { background-color: #323238; }"));
        return button;
    };

    prevYearButton_ = createHeaderButton(QStringLiteral("<<"));
    prevMonthButton_ = createHeaderButton(QStringLiteral("<"));
    nextMonthButton_ = createHeaderButton(QStringLiteral(">"));
    nextYearButton_ = createHeaderButton(QStringLiteral(">>"));

    headerLabel_ = new QLabel(popup_);
    headerLabel_->setAlignment(Qt::AlignCenter);
    headerLabel_->setFixedWidth(120);
    headerLabel_->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold;"));

    timePreviewLabel_ = new QLabel(popup_);
    timePreviewLabel_->setAlignment(Qt::AlignCenter);
    timePreviewLabel_->setFixedWidth(rightTimeAreaWidth - timeAreaLeftPadding);
    timePreviewLabel_->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold;"));

    QWidget *headerRowWidget = new QWidget(popup_);
    headerRowWidget->setFixedWidth(alignedRowWidth);
    QHBoxLayout *headerLayout = new QHBoxLayout(headerRowWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(areaSpacing);

    QWidget *leftHeaderWidget = new QWidget(headerRowWidget);
    leftHeaderWidget->setFixedWidth(calendarAreaWidth);
    QHBoxLayout *leftHeaderLayout = new QHBoxLayout(leftHeaderWidget);
    leftHeaderLayout->setContentsMargins(2, 0, 2, 0);
    leftHeaderLayout->setSpacing(4);

    QFrame *headerSeparator = new QFrame(popup_);
    headerSeparator->setFixedWidth(separatorWidth);
    headerSeparator->setFixedHeight(24);
    headerSeparator->setStyleSheet(QStringLiteral("background-color: rgba(255, 255, 255, 120);"));

    leftHeaderLayout->addWidget(prevYearButton_);
    leftHeaderLayout->addWidget(prevMonthButton_);
    leftHeaderLayout->addStretch();
    leftHeaderLayout->addWidget(headerLabel_, 0, Qt::AlignCenter);
    leftHeaderLayout->addStretch();
    leftHeaderLayout->addWidget(nextMonthButton_);
    leftHeaderLayout->addWidget(nextYearButton_);

    QWidget *rightHeaderWidget = new QWidget(headerRowWidget);
    rightHeaderWidget->setFixedWidth(rightTimeAreaWidth);
    QHBoxLayout *rightHeaderLayout = new QHBoxLayout(rightHeaderWidget);
    rightHeaderLayout->setContentsMargins(timeAreaLeftPadding, 0, 0, 0);
    rightHeaderLayout->addStretch();
    rightHeaderLayout->addWidget(timePreviewLabel_, 0, Qt::AlignCenter);
    rightHeaderLayout->addStretch();

    headerLayout->addWidget(leftHeaderWidget, 0, Qt::AlignLeft);
    headerLayout->addWidget(headerSeparator, 0, Qt::AlignVCenter);
    headerLayout->addWidget(rightHeaderWidget, 0, Qt::AlignLeft);
    popupLayout->addWidget(headerRowWidget, 0, Qt::AlignHCenter);

    QWidget *contentRowWidget = new QWidget(popup_);
    contentRowWidget->setFixedWidth(alignedRowWidth);
    QHBoxLayout *contentLayout = new QHBoxLayout(contentRowWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(areaSpacing);

    calendar_ = new QCalendarWidget(popup_);
    calendar_->setGridVisible(false);
    calendar_->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    calendar_->setNavigationBarVisible(false);
    calendar_->setFixedSize(calendarAreaWidth, 222);
    calendar_->setStyleSheet(QStringLiteral(
        "QCalendarWidget { background-color: #1f1f22; color: #ffffff; border: none; }"
        "QCalendarWidget QWidget#qt_calendar_navigationbar { background-color: #1f1f22; }"
        "QCalendarWidget QWidget#qt_calendar_calendarview { background-color: #1f1f22; alternate-background-color: #1f1f22; }"
        "QCalendarWidget QTableView { background-color: #1f1f22; alternate-background-color: #1f1f22; "
        "selection-background-color: #e58b3e; selection-color: #ffffff; color: #ffffff; }"
        "QCalendarWidget QAbstractItemView:enabled { color: #ffffff; selection-color: #ffffff; }"
        "QCalendarWidget QHeaderView::section { background-color: #1f1f22; color: #ffffff; border: none; "
        "padding: 6px 0; font-size: 14px; font-weight: bold; }"));
    applyCalendarTextColors(calendar_);
    contentLayout->addWidget(calendar_);

    QFrame *calendarTimeSeparator = new QFrame(popup_);
    calendarTimeSeparator->setFixedWidth(separatorWidth);
    calendarTimeSeparator->setFixedHeight(222);
    calendarTimeSeparator->setStyleSheet(QStringLiteral("background-color: rgba(255, 255, 255, 120);"));
    contentLayout->addWidget(calendarTimeSeparator);

    auto createTimeList = [this, timeColumnWidth]() -> QListWidget *
    {
        QListWidget *list = new QListWidget(popup_);
        list->setFixedSize(timeColumnWidth, 222);
        list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
        list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        list->setFocusPolicy(Qt::NoFocus);
        list->setStyleSheet(QStringLiteral("QListWidget { background-color: transparent; color: #ffffff; border: none; "
                                           "outline: none; font-size: 14px; }"
                                           "QListWidget::item { height: 36px; }"
                                           "QListWidget::item:selected { background-color: #4a2c12; color: #ffffff; "
                                           "border-radius: 4px; }"
                                           "QScrollBar:vertical { width: 8px; background: transparent; margin: 2px 0; }"
                                           "QScrollBar::handle:vertical { background: #6b6f76; border-radius: 4px; "
                                           "min-height: 28px; }"
                                           "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical, "
                                           "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { "
                                           "background: transparent; height: 0px; }"));
        return list;
    };

    hourList_ = createTimeList();
    minuteList_ = createTimeList();
    secondList_ = createTimeList();

    for (int hour = 0; hour < 24; ++hour)
    {
        hourList_->addItem(QStringLiteral("%1").arg(hour, 2, 10, QLatin1Char('0')));
    }
    for (int value = 0; value < 60; ++value)
    {
        const QString text = QStringLiteral("%1").arg(value, 2, 10, QLatin1Char('0'));
        minuteList_->addItem(text);
        secondList_->addItem(text);
    }

    QWidget *rightContentWidget = new QWidget(contentRowWidget);
    rightContentWidget->setFixedWidth(rightTimeAreaWidth);
    QHBoxLayout *timeListsLayout = new QHBoxLayout(rightContentWidget);
    timeListsLayout->setContentsMargins(timeAreaLeftPadding, 0, 0, 0);
    timeListsLayout->setSpacing(0);
    timeListsLayout->addWidget(hourList_);
    timeListsLayout->addWidget(minuteList_);
    timeListsLayout->addWidget(secondList_);
    contentLayout->addWidget(rightContentWidget);
    popupLayout->addWidget(contentRowWidget, 0, Qt::AlignHCenter);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);

    nowButton_ = new QPushButton(QStringLiteral("此刻"), popup_);
    nowButton_->setFixedSize(76, 34);
    nowButton_->setStyleSheet(QStringLiteral("QPushButton { background-color: transparent; color: #59a6ff; border: none; "
                                             "font-size: 14px; font-weight: bold; }"
                                             "QPushButton:hover { color: #7db8ff; }"));

    confirmButton_ = new QPushButton(QStringLiteral("确定"), popup_);
    confirmButton_->setFixedSize(76, 34);
    confirmButton_->setStyleSheet(QStringLiteral("QPushButton { background-color: #e58b3e; color: #ffffff; border: none; "
                                                 "border-radius: 6px; font-size: 14px; font-weight: bold; }"
                                                 "QPushButton:hover { background-color: #f09a4f; }"));

    buttonLayout->addWidget(nowButton_, 0, Qt::AlignLeft);
    buttonLayout->addStretch();
    buttonLayout->addWidget(confirmButton_, 0, Qt::AlignRight);
    popupLayout->addLayout(buttonLayout);

    connect(nowButton_, &QPushButton::clicked, this,
            [this]()
            {
                if (!activeEdit_)
                {
                    return;
                }

                activeEdit_->setDateTime(QDateTime::currentDateTime());
                syncSelectionFromEdit();
            });
    connect(confirmButton_, &QPushButton::clicked, this, &DateTimePickerPopup::applySelection);
    connect(prevYearButton_, &QPushButton::clicked, calendar_, &QCalendarWidget::showPreviousYear);
    connect(prevMonthButton_, &QPushButton::clicked, calendar_, &QCalendarWidget::showPreviousMonth);
    connect(nextMonthButton_, &QPushButton::clicked, calendar_, &QCalendarWidget::showNextMonth);
    connect(nextYearButton_, &QPushButton::clicked, calendar_, &QCalendarWidget::showNextYear);
    connect(calendar_, &QCalendarWidget::currentPageChanged, this, &DateTimePickerPopup::updateHeader);

    auto updatePreview = [this]()
    {
        if (!hourList_ || !minuteList_ || !secondList_ || !timePreviewLabel_)
        {
            return;
        }

        timePreviewLabel_->setText(QStringLiteral("%1:%2:%3")
                                       .arg(qMax(0, hourList_->currentRow()), 2, 10, QLatin1Char('0'))
                                       .arg(qMax(0, minuteList_->currentRow()), 2, 10, QLatin1Char('0'))
                                       .arg(qMax(0, secondList_->currentRow()), 2, 10, QLatin1Char('0')));
    };

    connect(hourList_, &QListWidget::currentRowChanged, this, [updatePreview](int) { updatePreview(); });
    connect(minuteList_, &QListWidget::currentRowChanged, this, [updatePreview](int) { updatePreview(); });
    connect(secondList_, &QListWidget::currentRowChanged, this, [updatePreview](int) { updatePreview(); });

    updateHeader();
}

bool DateTimePickerPopup::eventFilter(QObject *watched, QEvent *event)
{
    if (popup_ && popup_->isVisible())
    {
        if (event->type() == QEvent::KeyPress)
        {
            if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape)
            {
                hidePopup();
                return true;
            }
        }

        if (event->type() == QEvent::MouseButtonPress)
        {
            auto *mouseEvent = static_cast<QMouseEvent *>(event);
            QWidget *widget = qobject_cast<QWidget *>(watched);
            const QPoint globalPos = mouseEvent->globalPos();

            if (isClickInsidePopup(globalPos))
            {
                return QObject::eventFilter(watched, event);
            }

            QDateTimeEdit *clickedEdit = nullptr;
            if (widget)
            {
                clickedEdit = qobject_cast<QDateTimeEdit *>(widget);
                if (!clickedEdit)
                {
                    if (auto *lineEdit = qobject_cast<QLineEdit *>(widget))
                    {
                        clickedEdit = qobject_cast<QDateTimeEdit *>(lineEdit->parent());
                    }
                }
            }

            if (clickedEdit && registeredEdits_.contains(clickedEdit))
            {
                showForEdit(clickedEdit);
                return true;
            }

            hidePopup();
            return false;
        }
    }

    if (event->type() != QEvent::MouseButtonPress)
    {
        return QObject::eventFilter(watched, event);
    }

    auto *edit = qobject_cast<QDateTimeEdit *>(watched);
    if (!edit)
    {
        if (auto *lineEdit = qobject_cast<QLineEdit *>(watched))
        {
            edit = qobject_cast<QDateTimeEdit *>(lineEdit->parent());
        }
    }

    if (edit && registeredEdits_.contains(edit))
    {
        showForEdit(edit);
        return true;
    }

    return QObject::eventFilter(watched, event);
}

void DateTimePickerPopup::showForEdit(QDateTimeEdit *targetEdit)
{
    if (!targetEdit)
    {
        return;
    }

    activeEdit_ = targetEdit;
    ensurePopup();
    syncSelectionFromEdit();
    updatePosition();
    popup_->show();
    popup_->raise();
    installAppEventFilter();
}

void DateTimePickerPopup::syncSelectionFromEdit()
{
    if (!activeEdit_)
    {
        return;
    }

    ensurePopup();
    const QDateTime dateTime = activeEdit_->dateTime();
    calendar_->setSelectedDate(dateTime.date());
    calendar_->showSelectedDate();

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

    selectRow(hourList_, dateTime.time().hour());
    selectRow(minuteList_, dateTime.time().minute());
    selectRow(secondList_, dateTime.time().second());

    if (timePreviewLabel_)
    {
        timePreviewLabel_->setText(dateTime.toString(QStringLiteral("HH:mm:ss")));
    }
}

void DateTimePickerPopup::applySelection()
{
    if (!activeEdit_ || !calendar_ || !hourList_ || !minuteList_ || !secondList_)
    {
        return;
    }

    const int hour = qMax(0, hourList_->currentRow());
    const int minute = qMax(0, minuteList_->currentRow());
    const int second = qMax(0, secondList_->currentRow());
    activeEdit_->setDateTime(QDateTime(calendar_->selectedDate(), QTime(hour, minute, second)));
    hidePopup();
}

void DateTimePickerPopup::hidePopup()
{
    removeAppEventFilter();
    if (popup_)
    {
        popup_->hide();
    }
    activeEdit_ = nullptr;
}

void DateTimePickerPopup::updatePosition()
{
    if (!popup_ || !activeEdit_ || !positionParent_)
    {
        return;
    }

    const QPoint anchorBottomLeft = activeEdit_->mapTo(positionParent_, QPoint(0, activeEdit_->height() + 8));
    int x = anchorBottomLeft.x();
    int y = anchorBottomLeft.y();

    if (x + popup_->width() > positionParent_->width() - 12)
    {
        x = positionParent_->width() - popup_->width() - 12;
    }
    if (x < 12)
    {
        x = 12;
    }
    if (y + popup_->height() > positionParent_->height() - 12)
    {
        y = activeEdit_->mapTo(positionParent_, QPoint(0, -popup_->height() - 8)).y();
    }
    if (y < 12)
    {
        y = 12;
    }

    popup_->move(x, y);
}

void DateTimePickerPopup::updateHeader()
{
    if (!calendar_ || !headerLabel_)
    {
        return;
    }

    headerLabel_->setText(QStringLiteral("%1年 %2月").arg(calendar_->yearShown()).arg(calendar_->monthShown()));
}
