#ifndef DATETIME_PICKER_POPUP_H
#define DATETIME_PICKER_POPUP_H

#include <QObject>

#include <QList>

class QCalendarWidget;
class QDateTimeEdit;
class QLabel;
class QListWidget;
class QPushButton;
class QWidget;

class DateTimePickerPopup : public QObject
{
    Q_OBJECT

  public:
    explicit DateTimePickerPopup(QWidget *positionParent, QObject *parent = nullptr);

    void registerDateTimeEdit(QDateTimeEdit *edit);
    void updatePosition();
    void hidePopup();
    bool isVisible() const;

    ~DateTimePickerPopup() override;

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

  private:
    void ensurePopup();
    void showForEdit(QDateTimeEdit *targetEdit);
    void syncSelectionFromEdit();
    void applySelection();
    void updateHeader();
    bool isRegisteredEditWidget(QWidget *widget) const;
    bool isClickInsidePopup(const QPoint &globalPos) const;
    void installAppEventFilter();
    void removeAppEventFilter();

    QWidget *positionParent_;
    bool appFilterInstalled_;
    QList<QDateTimeEdit *> registeredEdits_;
    QDateTimeEdit *activeEdit_;
    QWidget *popup_;
    QCalendarWidget *calendar_;
    QLabel *headerLabel_;
    QPushButton *prevYearButton_;
    QPushButton *prevMonthButton_;
    QPushButton *nextMonthButton_;
    QPushButton *nextYearButton_;
    QListWidget *hourList_;
    QListWidget *minuteList_;
    QListWidget *secondList_;
    QPushButton *nowButton_;
    QPushButton *confirmButton_;
    QLabel *timePreviewLabel_;
};

#endif // DATETIME_PICKER_POPUP_H
