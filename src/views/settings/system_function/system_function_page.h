#ifndef SYSTEM_FUNCTION_PAGE_H
#define SYSTEM_FUNCTION_PAGE_H

#include <QDateTime>
#include <QWidget>
#include "../settings_role.h"
#include <cstdint>

class QCheckBox;
class QCalendarWidget;
class QComboBox;
class QDateTimeEdit;
class QFrame;
class QGraphicsOpacityEffect;
class QLabel;
class QLineEdit;
class QListWidget;
class QPropertyAnimation;
class QPushButton;
class QRadioButton;
class QResizeEvent;
class QTimer;
class QVBoxLayout;

class SystemFunctionPage : public QWidget
{
    Q_OBJECT

  public:
    explicit SystemFunctionPage(QWidget *parent = nullptr);
    void setUserRole(SettingsUserRole role);

  signals:
    void requestSaveBuzzerEnabled(uint8_t enabled);
    void requestSaveSystemTime(const QDateTime &dateTime);
    void requestQueryBuzzerEnabled();
    void requestSetScreenFlashEnabled(bool enabled);
    void requestRebootDevice();

  public slots:
    void updateBuzzerEnabled(uint8_t enabled);
    void updateDeviceReportedTime(const QString &timestamp);
    void showSystemTimeSaveResult(bool success, const QString &message);
    void showAlarmSaveResult(bool success, const QString &message);
    void showRebootResult(bool success, const QString &message);

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

  private:
    void setupUi();
    QFrame *createSectionFrame(const QString &title, QVBoxLayout *pageLayout, QVBoxLayout *&sectionLayout);
    QWidget *createFormRow(QFrame *parent, const QString &labelText, QWidget *fieldWidget, bool required = false);
    QWidget *createReadOnlyRow(QFrame *parent, const QString &labelText, const QString &valueText);
    QWidget *createReadOnlyRow(QFrame *parent, const QString &labelText, QLabel *valueLabel);
    QWidget *createNoteRow(QFrame *parent, const QString &noteText) const;
    QFrame *createSeparatorLine(QFrame *parent) const;
    QComboBox *createStyledComboBox(QFrame *parent, const QStringList &items, int width, const QString &currentText) const;
    QLineEdit *createStyledLineEdit(QFrame *parent, const QString &text, int width) const;
    QLineEdit *createStyledPasswordEdit(QFrame *parent, const QString &placeholderText, int width) const;
    QDateTimeEdit *createStyledDateTimeEdit(QFrame *parent) const;
    QCheckBox *createStyledToggleSwitch(QFrame *parent, bool checked = false) const;
    QPushButton *createPrimaryButton(QFrame *parent, const QString &text, int width = 120) const;
    void ensureToastWidget();
    void ensureRebootConfirmOverlay();
    void updateRebootConfirmGeometry();
    void showRebootConfirmOverlay();
    void hideRebootConfirmOverlay();
    void ensureTimePickerPopup();
    void updateTimePickerPopupPosition();
    void updateTimePickerHeader();
    void syncTimePickerSelectionFromEdit();
    void applyTimePickerSelection();
    void showTimePickerPopup();
    void hideTimePickerPopup();
    void updateToastPosition();
    void showToastResult(bool success, const QString &message);
    QString extractDisplayMessage(bool success, const QString &message) const;
    QString sectionTitleStyle() const;
    QString formLabelStyle(bool required) const;
    QString readOnlyValueStyle() const;
    QString noteLabelStyle() const;

    QCheckBox *alarmVoiceToggle;
    QCheckBox *screenFlashToggle;
    QPushButton *alarmSaveButton;
    QPushButton *rebootButton;
    QLabel *currentTimeValueLabel;
    QComboBox *timezoneComboBox;
    QDateTimeEdit *setTimeEdit;
    QCheckBox *syncTimeToggle;
    QPushButton *timeSaveButton;
    QLineEdit *warningRemoveTimeEdit;
    QComboBox *mapTypeComboBox;
    QPushButton *paramSaveButton;
    QLabel *diskSpaceValueLabel;
    QPushButton *uploadButton;
    QPushButton *clearButton;
    QFrame *changePasswordFrame;
    QRadioButton *normalAdminPasswordRadio;
    QRadioButton *advancedAdminPasswordRadio;
    QLineEdit *oldPasswordEdit;
    QLineEdit *newPasswordEdit;
    QLineEdit *confirmPasswordEdit;
    QPushButton *changePasswordSaveButton;
    QWidget *toastWidget;
    QLabel *toastIconLabel;
    QLabel *toastTextLabel;
    QTimer *toastHideTimer;
    QGraphicsOpacityEffect *toastOpacityEffect;
    QPropertyAnimation *toastFadeInAnimation;
    QPropertyAnimation *toastFadeOutAnimation;
    QWidget *rebootConfirmOverlay;
    QWidget *rebootConfirmPanel;
    QLabel *rebootConfirmIconLabel;
    QLabel *rebootConfirmTitleLabel;
    QLabel *rebootConfirmMessageLabel;
    QPushButton *rebootConfirmCancelButton;
    QPushButton *rebootConfirmOkButton;
    QWidget *timePickerPopup;
    QCalendarWidget *timePickerCalendar;
    QLabel *timePickerHeaderLabel;
    QPushButton *timePickerPrevYearButton;
    QPushButton *timePickerPrevMonthButton;
    QPushButton *timePickerNextMonthButton;
    QPushButton *timePickerNextYearButton;
    QListWidget *timePickerHourList;
    QListWidget *timePickerMinuteList;
    QListWidget *timePickerSecondList;
    QPushButton *timePickerNowButton;
    QPushButton *timePickerConfirmButton;
    SettingsUserRole currentUserRole;
};

#endif // SYSTEM_FUNCTION_PAGE_H
