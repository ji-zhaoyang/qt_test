#ifndef DIRECTION_CALIBRATION_VALUE_PAGE_H
#define DIRECTION_CALIBRATION_VALUE_PAGE_H

#include "network/core/protocol_types.h"
#include <QWidget>

class QGraphicsOpacityEffect;
class QLabel;
class QLineEdit;
class QPropertyAnimation;
class QPushButton;
class QResizeEvent;
class QTimer;

class DirectionCalibrationValuePage : public QWidget
{
    Q_OBJECT

  public:
    explicit DirectionCalibrationValuePage(QWidget *parent = nullptr);

  signals:
    void requestQueryDirectionCalibrationValues();
    void requestSaveDirectionCalibrationValues(const DirectionCalibrationValueList &values);

  public slots:
    void updateDirectionCalibrationValues(const DirectionCalibrationValueList &values);
    void showSaveResult(bool success, const QString &message);

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private:
    void setupUi();
    QWidget *createHeaderRow(QWidget *parent) const;
    QWidget *createDataRow(QWidget *parent, int index);
    QLineEdit *createValueEdit(QWidget *parent) const;
    QLabel *createValueDisplayLabel(QWidget *parent) const;
    bool buildSavePayload(DirectionCalibrationValueList &values, QString &errorMessage) const;
    void handleQueryClicked();
    void handleApplyClicked();
    void handleCancelClicked();
    void handleConfirmClicked();
    void setEditMode(bool editing);
    void syncEditsFromCurrentValues();
    void syncDisplayFromValues(const DirectionCalibrationValueList &values);
    void ensureToastWidget();
    void updateToastPosition();
    void showToastResult(bool success, const QString &message);
    QString extractDisplayMessage(bool success, const QString &message) const;
    QString titleStyle() const;
    QString headerTextStyle() const;
    QString cellTextStyle() const;
    QString valueTextStyle() const;
    QString actionButtonStyle() const;

    DirectionCalibrationValueList currentValues;
    QVector<QLabel *> valueLabels;
    QVector<QLineEdit *> valueEdits;
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

#endif // DIRECTION_CALIBRATION_VALUE_PAGE_H
