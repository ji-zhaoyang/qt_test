#ifndef DEVICE_SETTINGS_PAGE_H
#define DEVICE_SETTINGS_PAGE_H

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include "../settings_role.h"
#include <cstdint>

class QFrame;
class QGraphicsOpacityEffect;
class QHBoxLayout;
class QLabel;
class QPropertyAnimation;
class QResizeEvent;
class QTimer;
class QVBoxLayout;
class MapPickerDialog;

class DeviceSettingsPage : public QWidget
{
    Q_OBJECT

  public:
    explicit DeviceSettingsPage(SettingsUserRole role, QWidget *parent = nullptr);

    uint8_t currentLocationMode() const;
    void setUserRole(SettingsUserRole role);

  signals:
    void requestSaveGps(uint8_t mode, float lng, float lat, float alt);
    void requestSaveFullScan(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin, double att);
    void requestSaveDeviceIp(const QString &ip, int port, const QString &mask, const QString &route,
                             const QString &dns);
    void requestSaveTcpServerIp(const QString &ip, int port);

  public slots:
    void updateGpsInfo(uint8_t mode, float lng, float lat, float alt);
    void updateFullScanSettings(double ssth, double ssJgMax, double ssJgMin, double ssMax, double ssMin, double att);
    void updateDeviceIpSettings(const QString &ip, int port, const QString &mask, const QString &route,
                                const QString &dns);
    void updateTcpServerIpSettings(const QString &ip, int port);
    void showSaveResult(bool success, const QString &message);

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private slots:
    void onPickMapClicked();

  private:
    void setupUi();
    void setupScrollablePage(QVBoxLayout *&pageLayout);
    void setupPageTitle(QVBoxLayout *pageLayout);
    void setupCommonSettingsSection(QVBoxLayout *pageLayout);
    void setupModuleModeDisplayRow(QFrame *formFrame, QVBoxLayout *formLayout);
    QFrame *createSeparatorLine(QFrame *parent);
    QHBoxLayout *createFormRow(QFrame *parent, const QString &labelText, QWidget *inputWidget,
                               const QString &suffix = QString());
    void addRootInputRow(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &labelText, QWidget *inputWidget,
                         const QString &suffix = QString(), const QString &noteText = QString());
    QDoubleSpinBox *createNumericSpinBox(double defaultValue, int decimals, QFrame *parent);
    QWidget *createSpinBoxContainer(QDoubleSpinBox *spinBox, QWidget *parent);
    QPushButton *createActionButton(const QString &text, QFrame *parent);
    void setupLocationModeRow(QFrame *formFrame, QVBoxLayout *formLayout);
    void setupCoordinateRows(QFrame *formFrame, QVBoxLayout *formLayout);
    void setupDeviceActionButtons(QFrame *formFrame, QVBoxLayout *formLayout);
    void setupRootAdvancedSection(QVBoxLayout *pageLayout);
    QVBoxLayout *createRootSectionFrame(const QString &title, QVBoxLayout *advancedLayout);
    QWidget *createRootInput(const QString &defaultValue, QFrame *parent);
    void addRootRow(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &labelText, const QString &value,
                    const QString &suffix = QString(), const QString &noteText = QString());
    void addSectionSaveButton(QVBoxLayout *sectionLayout, QFrame *sectionFrame);
    void setupScanSettingsSection(QVBoxLayout *advancedLayout);
    void setupDeviceIpSection(QVBoxLayout *advancedLayout);
    void setupRemotePlatformSection(QVBoxLayout *advancedLayout);
    void bindSignals();
    void applyLocationModeState(bool isManual);
    void applyRolePermissions();
    void ensureToastWidget();
    void updateToastPosition();
    QString extractDisplayMessage(bool success, const QString &message) const;

    QComboBox *locationModeCombo;
    QDoubleSpinBox *lngInput;
    QDoubleSpinBox *latInput;
    QDoubleSpinBox *altInput;
    QWidget *lngInputContainer;
    QWidget *latInputContainer;
    QWidget *altInputContainer;
    QPushButton *saveBtn;
    QPushButton *pickMapBtn;
    QPushButton *lockPosBtn;
    QPushButton *scanSaveBtn;
    QPushButton *deviceIpSaveBtn;
    QPushButton *remotePlatformSaveBtn;
    QFrame *rootAdvancedFrame;
    MapPickerDialog *mapPickerDialog;
    QString btnStyle;
    SettingsUserRole userRole;
    QDoubleSpinBox *scanSsthInput;
    QDoubleSpinBox *scanSsJgMaxInput;
    QDoubleSpinBox *scanSsJgMinInput;
    QDoubleSpinBox *scanSsMaxInput;
    QDoubleSpinBox *scanSsMinInput;
    QDoubleSpinBox *scanAttInput;
    QLineEdit *deviceIpInput;
    QDoubleSpinBox *devicePortInput;
    QWidget *devicePortInputContainer;
    QLineEdit *deviceMaskInput;
    QLineEdit *deviceRouteInput;
    QLineEdit *deviceDnsInput;
    QLineEdit *remoteIpInput;
    QDoubleSpinBox *remotePortInput;
    QWidget *remotePortInputContainer;
    QWidget *toastWidget;
    QLabel *toastIconLabel;
    QLabel *toastTextLabel;
    QTimer *toastHideTimer;
    QGraphicsOpacityEffect *toastOpacityEffect;
    QPropertyAnimation *toastFadeInAnimation;
    QPropertyAnimation *toastFadeOutAnimation;
};

#endif // DEVICE_SETTINGS_PAGE_H
