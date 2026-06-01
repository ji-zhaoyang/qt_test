#ifndef MODE_SELECT_PAGE_H
#define MODE_SELECT_PAGE_H

#include <QByteArray>
#include <QWidget>
#include "../settings_role.h"
#include <cstdint>

class QCheckBox;
class QComboBox;
class QFrame;
class QGraphicsOpacityEffect;
class QLabel;
class QPropertyAnimation;
class QPushButton;
class QResizeEvent;
class QTimer;
class QVBoxLayout;

class ModeSelectPage : public QWidget
{
    Q_OBJECT

  public:
    explicit ModeSelectPage(QWidget *parent = nullptr);
    void setUserRole(SettingsUserRole role);

  signals:
    void requestSaveDroneReportMode(uint8_t mode);
    void requestSaveJamMode(uint8_t mode);
    void requestSaveNetworkMode(uint8_t mode);
    void requestSaveUavCategoryDisplayMode(uint8_t mode);
    void requestSaveDataEnable(uint8_t enabled);
    void requestSaveFeatureModes(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled);
    void requestQueryDroneReportMode();
    void requestQueryJamMode();
    void requestQueryNetworkMode();
    void requestQueryUavCategoryDisplayMode();
    void requestQueryDataEnable();
    void requestQueryFeatureModes();

  public slots:
    void updateDroneReportMode(uint8_t mode);
    void showDroneReportModeSaveResult(bool success, const QString &message);
    void updateJamMode(uint8_t mode);
    void showJamModeSaveResult(bool success, const QString &message);
    void updateNetworkMode(uint8_t mode);
    void showNetworkModeSaveResult(bool success, const QString &message);
    void updateUavCategoryDisplayMode(uint8_t mode);
    void showUavCategoryDisplayModeSaveResult(bool success, const QString &message);
    void updateDataEnable(uint8_t enabled);
    void showDataEnableSaveResult(bool success, const QString &message);
    void updateFeatureModes(uint8_t wifiRemoteIdEnabled, uint8_t fpvEnabled, uint8_t djiParseEnabled);
    void showFeatureModeSaveResult(bool success, const QString &message);
    void onConnectionLost();
    void onConnectionRestored();

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private slots:
    void handleSaveClicked();

  private:
    enum class SectionKey
    {
        DroneReportMode,
        JamMode,
        NetworkMode,
        UavCategoryDisplayMode,
        DataEnable,
        FeatureMode
    };

    struct PendingModeCommand
    {
        bool active = false;
        QByteArray targetPayload;
        bool waitingReconnect = false;
        bool awaitingRecoveryQuery = false;
        bool resendAttempted = false;
    };

    void setupUi();
    QWidget *createScrollableContent();
    QFrame *createSectionFrame(const QString &title, QVBoxLayout *pageLayout, QVBoxLayout *&sectionLayout);
    QComboBox *createStyledComboBox(QFrame *parent, const QStringList &items, const QString &currentText = QString()) const;
    void addSingleChoiceRow(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &labelText, QComboBox *comboBox,
                            const QString &noteText = QString());
    void addFeatureChoiceRow(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &labelText, QComboBox *comboBox);
    void addToggleChoiceRow(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &labelText, QCheckBox *checkBox);
    void addSectionSaveButton(QVBoxLayout *sectionLayout, QFrame *sectionFrame, const QString &sectionKey);
    QFrame *createSeparatorLine(QFrame *parent) const;
    QCheckBox *createStyledToggleSwitch(QFrame *parent, bool checked = false) const;
    QString comboBoxStyle() const;
    QString formLabelStyle() const;
    QString sectionTitleStyle() const;
    QString noteLabelStyle() const;
    QString saveButtonStyle() const;
    void showToastResult(bool success, const QString &message);
    void setSaveButtonPending(QPushButton *button, bool pending);
    void setupPendingTimer(QTimer *timer, SectionKey key, QPushButton *button);
    void clearPendingCommand(SectionKey key, bool resetButton = true);
    void startPendingCommand(SectionKey key, const QByteArray &targetPayload, QPushButton *button, QTimer *timer);
    void handleRecoveryQueryResult(SectionKey key, const QByteArray &currentPayload);
    void requestRecoveryQuery(SectionKey key);
    void resendPendingCommand(SectionKey key);
    PendingModeCommand &pendingCommand(SectionKey key);
    QTimer *pendingTimer(SectionKey key) const;
    QPushButton *saveButton(SectionKey key) const;
    void ensureToastWidget();
    void updateToastPosition();
    QString extractDisplayMessage(bool success, const QString &message) const;
    static QString sectionKeyToString(SectionKey key);
    static uint8_t comboIndexToDroneReportMode(QComboBox *comboBox);
    static int droneReportModeToComboIndex(uint8_t mode);
    static uint8_t comboIndexToJamMode(QComboBox *comboBox);
    static int jamModeToComboIndex(uint8_t mode);
    static uint8_t comboIndexToNetworkMode(QComboBox *comboBox);
    static int networkModeToComboIndex(uint8_t mode);
    static uint8_t comboIndexToUavCategoryDisplayMode(QComboBox *comboBox);
    static int uavCategoryDisplayModeToComboIndex(uint8_t mode);
    static uint8_t checkBoxToDataEnable(QCheckBox *checkBox);
    static uint8_t comboIndexToFeatureFlag(QComboBox *comboBox);
    static int featureFlagToComboIndex(uint8_t enabled);
    QByteArray currentFeatureModePayload() const;

    QComboBox *droneReportModeCombo;
    QComboBox *jamModeCombo;
    QComboBox *networkModeCombo;
    QComboBox *wifiRemoteIdFeatureCombo;
    QComboBox *fpvFeatureCombo;
    QComboBox *djiParseFeatureCombo;
    QComboBox *uavCategoryDisplayCombo;
    QCheckBox *dataEnableCheckBox;
    QPushButton *droneReportModeSaveButton;
    QPushButton *jamModeSaveButton;
    QPushButton *networkModeSaveButton;
    QPushButton *featureModeSaveButton;
    QPushButton *uavCategoryDisplaySaveButton;
    QPushButton *dataEnableSaveButton;
    QFrame *uavCategoryDisplayFrame;
    QFrame *dataEnableFrame;
    QWidget *toastWidget;
    QLabel *toastIconLabel;
    QLabel *toastTextLabel;
    QTimer *toastHideTimer;
    QGraphicsOpacityEffect *toastOpacityEffect;
    QPropertyAnimation *toastFadeInAnimation;
    QPropertyAnimation *toastFadeOutAnimation;
    QTimer *droneReportModePendingTimer;
    QTimer *jamModePendingTimer;
    QTimer *networkModePendingTimer;
    QTimer *uavCategoryDisplayModePendingTimer;
    QTimer *dataEnablePendingTimer;
    QTimer *featureModePendingTimer;
    PendingModeCommand droneReportModePendingCommand;
    PendingModeCommand jamModePendingCommand;
    PendingModeCommand networkModePendingCommand;
    PendingModeCommand uavCategoryDisplayModePendingCommand;
    PendingModeCommand dataEnablePendingCommand;
    PendingModeCommand featureModePendingCommand;
    SettingsUserRole currentUserRole;
};

#endif // MODE_SELECT_PAGE_H
