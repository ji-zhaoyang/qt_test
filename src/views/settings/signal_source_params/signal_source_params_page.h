#ifndef SIGNAL_SOURCE_PARAMS_PAGE_H
#define SIGNAL_SOURCE_PARAMS_PAGE_H

#include "network/core/protocol_types.h"
#include <QWidget>
#include <QVector>

class QComboBox;
class QFrame;
class QGraphicsOpacityEffect;
class QLabel;
class QLineEdit;
class QPropertyAnimation;
class QPushButton;
class QResizeEvent;
class QTimer;
class QVBoxLayout;

class SignalSourceParamsPage : public QWidget
{
    Q_OBJECT

  public:
    explicit SignalSourceParamsPage(QWidget *parent = nullptr);

  signals:
    void requestQuerySignalSourceParams();
    void requestSaveSignalSourceParams(int serialScan, const QVector<int> &scanModes, int signalMode,
                                       const QVector<int> &vcoScans);

  public slots:
    void updateSignalSourceParams(const SignalSourceParamsConfig &config);
    void showSaveResult(bool success, const QString &message);

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private:
    void setupUi();
    QFrame *createSectionFrame(const QString &title, QVBoxLayout *pageLayout, QVBoxLayout *&sectionLayout);
    QWidget *createFormRow(QFrame *parent, const QString &labelText, QWidget *fieldWidget, bool required = false);
    QWidget *createNoteRow(QFrame *parent, const QString &noteText) const;
    QFrame *createSeparatorLine(QFrame *parent) const;
    QComboBox *createStyledComboBox(QFrame *parent, const QStringList &items, int width, const QString &currentText) const;
    QLineEdit *createStyledLineEdit(QFrame *parent, const QString &text, int width) const;
    QPushButton *createPrimaryButton(QFrame *parent, const QString &text, int width = 120) const;
    bool buildSavePayload(int &serialScan, QVector<int> &scanModes, int &signalMode, QVector<int> &vcoScans,
                          QString &errorMessage) const;
    void handleSaveClicked();
    void ensureToastWidget();
    void updateToastPosition();
    void showToastResult(bool success, const QString &message);
    QString extractDisplayMessage(bool success, const QString &message) const;
    QString sectionTitleStyle() const;
    QString formLabelStyle() const;
    QString noteLabelStyle() const;

    QLineEdit *serialScanPeriodEdit;
    QComboBox *channel1ModeComboBox;
    QComboBox *channel2ModeComboBox;
    QComboBox *channel3ModeComboBox;
    QComboBox *channel4ModeComboBox;
    QComboBox *channel5ModeComboBox;
    QComboBox *channel6ModeComboBox;
    QComboBox *signalModeComboBox;
    QLineEdit *channel1PeriodEdit;
    QLineEdit *channel2PeriodEdit;
    QLineEdit *channel3PeriodEdit;
    QLineEdit *channel4PeriodEdit;
    QLineEdit *channel5PeriodEdit;
    QLineEdit *channel6PeriodEdit;
    QPushButton *saveButton;
    QWidget *toastWidget;
    QLabel *toastIconLabel;
    QLabel *toastTextLabel;
    QTimer *toastHideTimer;
    QGraphicsOpacityEffect *toastOpacityEffect;
    QPropertyAnimation *toastFadeInAnimation;
    QPropertyAnimation *toastFadeOutAnimation;
};

#endif // SIGNAL_SOURCE_PARAMS_PAGE_H
