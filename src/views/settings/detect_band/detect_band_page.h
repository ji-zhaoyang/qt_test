#ifndef DETECT_BAND_PAGE_H
#define DETECT_BAND_PAGE_H

#include "network/core/protocol_types.h"
#include <QVector>
#include <QWidget>

class QLabel;
class QGraphicsOpacityEffect;
class QPropertyAnimation;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QDoubleSpinBox;
class QResizeEvent;
class QTimer;
class QVBoxLayout;
class QWidget;

class DetectBandPage : public QWidget
{
    Q_OBJECT

  public:
    explicit DetectBandPage(QWidget *parent = nullptr);

  signals:
    void requestSaveDetectBands(const QVector<DetectBandParam> &bands);

  public slots:
    void updateDetectBands(const QVector<DetectBandParam> &bands);
    void showSaveResult(bool success, const QString &message);

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private slots:
    void addDefaultRow();
    void handleSaveClicked();

  private:
    struct BandRowWidgets
    {
        QWidget *container;
        QDoubleSpinBox *freqInput;
        QSpinBox *measureCountInput;
        QSpinBox *gainInput;
        QPushButton *deleteButton;
        QLabel *freqHintLabel;
        QLabel *measureHintLabel;
        QLabel *gainHintLabel;
    };

    void setupUi();
    QWidget *createScrollContent(QScrollArea *scrollArea);
    QWidget *createHeaderRow(QWidget *parent);
    QWidget *createBandRow(QWidget *parent, double freqMhz, int measureCount, int gain);
    QWidget *createColumnCell(QWidget *parent, QWidget *fieldWidget, QLabel **hintLabel, const QString &hintText);
    QWidget *createNumericContainer(QWidget *parent, QWidget *fieldWidget);
    QDoubleSpinBox *createFreqInput(QWidget *parent, double value) const;
    QSpinBox *createMeasureCountInput(QWidget *parent, int value) const;
    QSpinBox *createGainInput(QWidget *parent, int value) const;
    QVector<DetectBandParam> collectBands() const;
    void appendBandRow(double freqMhz, int measureCount, int gain);
    void clearBandRows();
    void removeBandRow(QWidget *rowWidget);
    void refreshDeleteButtonState();
    QString inputContainerStyle() const;
    QString inputFieldStyle() const;
    QString hintLabelStyle() const;
    QString actionButtonStyle() const;
    QString deleteButtonStyle() const;
    QString addRowButtonStyle() const;
    QString rowBackgroundStyle() const;
    void applyStyledBackground(QWidget *widget, const QString &styleSheet) const;
    void ensureToastWidget();
    void updateToastPosition();
    QString extractDisplayMessage(bool success, const QString &message) const;

    QVBoxLayout *rowsLayout;
    QPushButton *addRowButton;
    QPushButton *saveButton;
    QVector<BandRowWidgets> bandRows;
    QWidget *toastWidget;
    QLabel *toastIconLabel;
    QLabel *toastTextLabel;
    QTimer *toastHideTimer;
    QGraphicsOpacityEffect *toastOpacityEffect;
    QPropertyAnimation *toastFadeInAnimation;
    QPropertyAnimation *toastFadeOutAnimation;
};

#endif // DETECT_BAND_PAGE_H
