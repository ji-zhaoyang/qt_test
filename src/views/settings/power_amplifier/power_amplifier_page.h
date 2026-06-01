#ifndef POWER_AMPLIFIER_PAGE_H
#define POWER_AMPLIFIER_PAGE_H

#include "network/core/protocol_types.h"
#include <QWidget>

class QFrame;
class QGraphicsOpacityEffect;
class QLabel;
class QLineEdit;
class QPropertyAnimation;
class QPushButton;
class QResizeEvent;
class QTimer;

class PowerAmplifierPage : public QWidget
{
    Q_OBJECT

  public:
    explicit PowerAmplifierPage(QWidget *parent = nullptr);

  signals:
    void requestQueryPowerAmplifierParams();
    void requestSavePowerAmplifierParams(const PowerAmplifierParamList &params);

  public slots:
    void updatePowerAmplifierParams(const PowerAmplifierParamList &params);
    void showSaveResult(bool success, const QString &message);

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private:
    void setupUi();
    QWidget *createHeaderRow(QWidget *parent) const;
    QWidget *createDataRow(QWidget *parent, int index);
    QLineEdit *createValueEdit(QWidget *parent) const;
    QLabel *createValueDisplayLabel(QWidget *parent) const;
    QLabel *createOutpowerLabel(QWidget *parent) const;
    bool buildSavePayload(PowerAmplifierParamList &params, QString &errorMessage) const;
    void handleQueryClicked();
    void handleApplyClicked();
    void handleCancelClicked();
    void handleConfirmClicked();
    void setEditMode(bool editing);
    void syncEditsFromCurrentParams();
    void syncDisplayFromParams(const PowerAmplifierParamList &params);
    void ensureToastWidget();
    void updateToastPosition();
    void showToastResult(bool success, const QString &message);
    QString extractDisplayMessage(bool success, const QString &message) const;
    QString titleStyle() const;
    QString headerTextStyle() const;
    QString cellTextStyle() const;
    QString valueTextStyle() const;
    QString actionButtonStyle() const;

    PowerAmplifierParamList currentParams;
    QVector<QLabel *> kValueLabels;
    QVector<QLineEdit *> kEdits;
    QVector<QLabel *> bValueLabels;
    QVector<QLineEdit *> bEdits;
    QVector<QLabel *> outpowerLabels;
    QVector<QLabel *> attValueLabels;
    QVector<QLineEdit *> attEdits;
    QPushButton *queryButton;
    QPushButton *applyButton;
    QPushButton *cancelButton;
    QPushButton *confirmButton;
    bool editMode;
    QWidget *toastWidget;
    QLabel *toastIconLabel;
    QLabel *toastTextLabel;
    QTimer *toastHideTimer;
    QGraphicsOpacityEffect *toastOpacityEffect;
    QPropertyAnimation *toastFadeInAnimation;
    QPropertyAnimation *toastFadeOutAnimation;
};

#endif // POWER_AMPLIFIER_PAGE_H
