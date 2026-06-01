#ifndef ANGLE_CALIBRATION_PAGE_H
#define ANGLE_CALIBRATION_PAGE_H

#include <QWidget>
#include <cstdint>

class QGraphicsOpacityEffect;
class QLineEdit;
class QPushButton;
class QPropertyAnimation;
class QResizeEvent;
class QTimer;
class QLabel;

class AngleCalibrationPage : public QWidget
{
    Q_OBJECT

  public:
    explicit AngleCalibrationPage(QWidget *parent = nullptr);

  signals:
    void requestStartCalibration();
    void requestFinishCalibration();
    void requestConfirmCalibration(uint16_t angle);
    void requestCancelCalibration();

  public slots:
    void showCalibrationResult(uint16_t responseDataType, bool success, const QString &message);

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private:
    void setupUi();
    QWidget *createStepLabel(QWidget *parent, const QString &text) const;
    QPushButton *createActionButton(QWidget *parent, const QString &text) const;
    void ensureToastWidget();
    void updateToastPosition();
    QString extractDisplayMessage(bool success, const QString &message) const;
    void showOperationMessage(const QString &title, bool success, const QString &message);
    QString titleStyle() const;
    QString sectionTitleStyle() const;
    QString stepTextStyle() const;
    QString inputStyle() const;
    QString buttonStyle() const;

    QPushButton *startCheckButton;
    QPushButton *finishRotateButton;
    QPushButton *confirmCalibrationButton;
    QPushButton *cancelCalibrationButton;
    QLineEdit *angleInput;
    QWidget *toastWidget;
    QLabel *toastIconLabel;
    QLabel *toastTextLabel;
    QTimer *toastHideTimer;
    QGraphicsOpacityEffect *toastOpacityEffect;
    QPropertyAnimation *toastFadeInAnimation;
    QPropertyAnimation *toastFadeOutAnimation;
};

#endif // ANGLE_CALIBRATION_PAGE_H
