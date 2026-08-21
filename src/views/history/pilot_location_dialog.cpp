#include "pilot_location_dialog.h"

#include <QCoreApplication>
#include <QDir>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QUrl>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QtMath>

namespace
{
QString secondaryButtonStyle()
{
    return QStringLiteral("QPushButton { background-color: #1b1d22; color: #d6d7da; border: 1px solid #3b3e46; "
                          "border-radius: 4px; padding: 0 16px; font-size: 13px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #252830; }");
}
} // namespace

PilotLocationDialog::PilotLocationDialog(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    hide();
    setStyleSheet(QStringLiteral("QWidget { background-color: rgba(0, 0, 0, 120); }"));
    setupUi();
}

bool PilotLocationDialog::hasValidCoordinate(double longitude, double latitude)
{
    if (qFuzzyIsNull(longitude) && qFuzzyIsNull(latitude))
    {
        return false;
    }

    return longitude >= -180.0 && longitude <= 180.0 && latitude >= -90.0 && latitude <= 90.0;
}

void PilotLocationDialog::setPilotLocation(double longitude, double latitude, const QString &label)
{
    longitude_ = longitude;
    latitude_ = latitude;
    label_ = label.trimmed().isEmpty() ? QStringLiteral("飞手位置") : label.trimmed();
    if (mapReady_)
    {
        pushLocationToWeb();
    }
}

void PilotLocationDialog::showOverlay()
{
    if (parentWidget())
    {
        setGeometry(parentWidget()->rect());
        raise();
    }
    updatePanelGeometry();
    show();
}

void PilotLocationDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addStretch();

    panelWidget_ = new QWidget(this);
    panelWidget_->setObjectName(QStringLiteral("pilotLocationPanel"));
    panelWidget_->setAttribute(Qt::WA_StyledBackground, true);
    panelWidget_->setStyleSheet(QStringLiteral(
        "QWidget#pilotLocationPanel { background-color: #17191d; border: 1px solid #2b2f36; border-radius: 8px; }"
        "QLabel { color: #ffffff; font-size: 13px; }"));
    mainLayout->addWidget(panelWidget_, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    QVBoxLayout *panelLayout = new QVBoxLayout(panelWidget_);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0);

    QWidget *titleBar = new QWidget(panelWidget_);
    titleBar->setFixedHeight(48);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(20, 0, 16, 0);
    titleLayout->setSpacing(8);

    QLabel *titleLabel = new QLabel(QStringLiteral("飞手位置"), titleBar);
    titleLabel->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 20px; font-weight: bold;"));
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    QPushButton *closeButton = new QPushButton(QStringLiteral("×"), titleBar);
    closeButton->setFixedSize(28, 28);
    closeButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: #c7cbd3; border: none; font-size: 18px; font-weight: bold; }"
        "QPushButton:hover { color: #ffffff; }"));
    connect(closeButton, &QPushButton::clicked, this, &QWidget::close);
    titleLayout->addWidget(closeButton);
    panelLayout->addWidget(titleBar);

    webView_ = new QWebEngineView(panelWidget_);
    webView_->setContextMenuPolicy(Qt::NoContextMenu);
    webView_->setMinimumSize(760, 420);
    panelLayout->addWidget(webView_, 1);

    QWidget *bottomBar = new QWidget(panelWidget_);
    bottomBar->setFixedHeight(58);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(20, 0, 20, 0);
    bottomLayout->setSpacing(12);

    QLabel *hintLabel =
        new QLabel(QStringLiteral("右侧二维码为高德地图导航链接，可用手机扫码导航"), bottomBar);
    hintLabel->setStyleSheet(QStringLiteral("color: #9ea4ae; font-size: 12px;"));
    bottomLayout->addWidget(hintLabel);
    bottomLayout->addStretch();

    QPushButton *closeBottomButton = new QPushButton(QStringLiteral("关闭"), bottomBar);
    closeBottomButton->setFixedSize(88, 34);
    closeBottomButton->setStyleSheet(secondaryButtonStyle());
    connect(closeBottomButton, &QPushButton::clicked, this, &QWidget::close);
    bottomLayout->addWidget(closeBottomButton);
    panelLayout->addWidget(bottomBar);

    connect(webView_, &QWebEngineView::loadFinished, this,
            [this](bool ok)
            {
                mapReady_ = ok;
                if (mapReady_)
                {
                    pushLocationToWeb();
                }
            });

    const QString webPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/assets/web/pilot_location.html");
    webView_->load(QUrl::fromLocalFile(webPath));
    updatePanelGeometry();
}

void PilotLocationDialog::pushLocationToWeb()
{
    if (!webView_ || !mapReady_)
    {
        return;
    }

    const QString js = QStringLiteral("if (typeof setPilotLocation === 'function') { setPilotLocation(%1, %2, '%3'); }")
                           .arg(latitude_, 0, 'f', 6)
                           .arg(longitude_, 0, 'f', 6)
                           .arg(label_.replace(QLatin1Char('\''), QStringLiteral("\\'")));
    webView_->page()->runJavaScript(js);
}

void PilotLocationDialog::updatePanelGeometry()
{
    if (!parentWidget() || !panelWidget_)
    {
        return;
    }

    const QSize parentSize = parentWidget()->size();
    const int panelWidth = qBound(820, parentSize.width() - 120, 1080);
    const int panelHeight = qBound(520, parentSize.height() - 120, 680);
    panelWidget_->setFixedSize(panelWidth, panelHeight);
}

void PilotLocationDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updatePanelGeometry();
}
