#ifndef STRIKE_FREQUENCY_PAGE_H
#define STRIKE_FREQUENCY_PAGE_H

#include "network/core/protocol_types.h"
#include <QWidget>
#include <QVector>

class QCheckBox;
class QComboBox;
class QGraphicsOpacityEffect;
class QLabel;
class QLineEdit;
class QPropertyAnimation;
class QPushButton;
class QResizeEvent;
class QTimer;

class StrikeFrequencyPage : public QWidget
{
    Q_OBJECT

  public:
    explicit StrikeFrequencyPage(QWidget *parent = nullptr);

  signals:
    void requestQueryStrikeFrequencyBands();
    void requestSaveStrikeFrequencyBands(const StrikeFrequencyBandList &bands);

  public slots:
    void updateStrikeFrequencyBands(const StrikeFrequencyBandList &bands);
    void showStrikeFrequencySaveResult(bool success, const QString &message);

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private:
    struct RowWidgets
    {
        QCheckBox *enabledCheckBox = nullptr;
        QLabel *indexLabel = nullptr;
        QLineEdit *startEdit = nullptr;
        QLabel *startHintLabel = nullptr;
        QLineEdit *endEdit = nullptr;
        QLabel *endHintLabel = nullptr;
        QComboBox *attComboBox = nullptr;
        StrikeFrequencyBandConfig metadata;
    };

    void setupUi();
    void applyBandToRow(int rowIndex, const StrikeFrequencyBandConfig &band);
    bool buildSavePayload(StrikeFrequencyBandList &bands, QString &errorMessage) const;
    void handleSaveClicked();
    void ensureToastWidget();
    void updateToastPosition();
    void showToastResult(bool success, const QString &message);
    QString extractDisplayMessage(bool success, const QString &message) const;
    QString formatFrequencyValue(double value) const;
    QString titleStyle() const;
    QString headerTextStyle() const;
    QString rowIndexStyle() const;
    QString rangeTextStyle() const;
    QString inputStyle() const;
    QString comboBoxStyle() const;
    QString toggleStyle() const;
    QString footerTextStyle() const;
    QString buttonStyle() const;

    QPushButton *saveButton;
    QVector<RowWidgets> rowWidgets;
    QWidget *toastWidget;
    QLabel *toastIconLabel;
    QLabel *toastTextLabel;
    QTimer *toastHideTimer;
    QGraphicsOpacityEffect *toastOpacityEffect;
    QPropertyAnimation *toastFadeInAnimation;
    QPropertyAnimation *toastFadeOutAnimation;
};

#endif // STRIKE_FREQUENCY_PAGE_H
