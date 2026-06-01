#ifndef SPECTRUM_SWITCH_PAGE_H
#define SPECTRUM_SWITCH_PAGE_H

#include "network/core/protocol_types.h"
#include <QWidget>
#include <cstdint>
#include <QVector>

class QGraphicsOpacityEffect;
class QLabel;
class QPainterPath;
class QPropertyAnimation;
class QPushButton;
class QResizeEvent;
class QTimer;
class QVBoxLayout;
class QWidget;

class SpectrumSwitchPage : public QWidget
{
    Q_OBJECT

  public:
    explicit SpectrumSwitchPage(QWidget *parent = nullptr);

  signals:
    void requestOpenSpectrogram();
    void requestCloseSpectrogram();
    void requestOpenSpectrum();
    void requestCloseSpectrum();

  public slots:
    void showSwitchResult(uint16_t responseDataType, bool success, const QString &message);
    void updateSpectrumReport(const SpectrumReportData &reportData);
    void showFullSpectrumSwitchResult(bool enabled, bool success, const QString &message);
    void updateFullSpectrumReport(const FullSpectrumReportData &reportData);

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

  private:
    void setupUi();
    QPushButton *createActionButton(QWidget *parent, const QString &text) const;
    void ensureToastWidget();
    void updateToastPosition();
    void showToastResult(bool success, const QString &message);
    void setSpectrogramVisible(bool visible);
    void setFullSpectrumVisible(bool visible);
    void refreshSpectrumViews();
    QPixmap buildHeatmapPixmap(const SpectrumGroupData &groupData, int targetWidth, int targetHeight) const;
    void refreshFullSpectrumView();
    QPixmap buildFullSpectrumPixmap() const;
    QRect fullSpectrumPlotRect() const;
    QString extractDisplayMessage(bool success, const QString &message) const;
    QString titleStyle() const;
    QString buttonStyle() const;

    QPushButton *openSpectrogramButton;
    QPushButton *closeSpectrogramButton;
    QWidget *spectrogramContainer;
    QPushButton *openSpectrumButton;
    QPushButton *closeSpectrumButton;
    QWidget *fullSpectrumContainer;
    QLabel *reportInfoLabel;
    QLabel *axisHintLabel;
    QVector<QLabel *> groupTitleLabels;
    QVector<QLabel *> groupImageLabels;
    QLabel *fullSpectrumInfoLabel;
    QLabel *fullSpectrumImageLabel;
    QWidget *toastWidget;
    QLabel *toastIconLabel;
    QLabel *toastTextLabel;
    QTimer *toastHideTimer;
    QGraphicsOpacityEffect *toastOpacityEffect;
    QPropertyAnimation *toastFadeInAnimation;
    QPropertyAnimation *toastFadeOutAnimation;
    SpectrumReportData currentSpectrumReport;
    FullSpectrumReportData currentFullSpectrumReport;
    bool spectrogramVisible;
    bool fullSpectrumVisible;
};

#endif // SPECTRUM_SWITCH_PAGE_H
