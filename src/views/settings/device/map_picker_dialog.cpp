#include "map_picker_dialog.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWebEnginePage>

MapPickerDialog::MapPickerDialog(QWidget *parent) : QWidget(parent), panelWidget(nullptr), selectedLng(120.089), selectedLat(30.342)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, false);
    hide();
    setupOverlayStyle();
    setupUi();
}

MapPickerDialog::~MapPickerDialog() {}

void MapPickerDialog::setupUi()
{
    QVBoxLayout *mainLayout = createMainLayout();
    setupPanelWidget();

    QVBoxLayout *panelLayout = new QVBoxLayout(panelWidget);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(0);

    QPushButton *xBtn = setupTitleBar(panelLayout);
    setupMapArea(panelLayout);
    setupBottomBar(panelLayout);
    bindSignals(xBtn);

    mainLayout->addWidget(panelWidget, 0, Qt::AlignCenter);
    mainLayout->addStretch();
    updatePanelGeometry();
}

void MapPickerDialog::setupOverlayStyle()
{
    setStyleSheet("QWidget { background-color: rgba(0, 0, 0, 120); }");
}

QVBoxLayout *MapPickerDialog::createMainLayout()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addStretch();
    return mainLayout;
}

void MapPickerDialog::setupPanelWidget()
{
    panelWidget = new QWidget(this);
    panelWidget->setStyleSheet("QWidget { background-color: #2b2b2b; border: 1px solid #444; border-radius: 6px; }");
}

QPushButton *MapPickerDialog::setupTitleBar(QVBoxLayout *panelLayout)
{
    QWidget *titleBar = new QWidget(panelWidget);
    titleBar->setFixedHeight(40);
    titleBar->setStyleSheet("background-color: transparent; border-bottom: none;");

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(15, 0, 15, 0);

    QLabel *titleLabel = new QLabel("地图选点", titleBar);
    titleLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold;");

    QPushButton *xBtn = new QPushButton("✕", titleBar);
    xBtn->setFixedSize(30, 30);
    xBtn->setStyleSheet("QPushButton { color: #aaaaaa; font-size: 16px; border: none; background: transparent; }"
                        "QPushButton:hover { color: #ffffff; background-color: #ff4c4c; border-radius: 15px; }");

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(xBtn);

    panelLayout->addWidget(titleBar);
    return xBtn;
}

void MapPickerDialog::setupMapArea(QVBoxLayout *panelLayout)
{
    QWidget *mapContainer = new QWidget(panelWidget);
    mapContainer->setStyleSheet("background-color: #1e1e1e;");

    QVBoxLayout *mapLayout = new QVBoxLayout(mapContainer);
    mapLayout->setContentsMargins(15, 0, 15, 0);

    webView = new QWebEngineView(mapContainer);

    QString webPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/assets/web/map_picker.html");
    webView->load(QUrl::fromLocalFile(webPath));

    mapLayout->addWidget(webView);
    panelLayout->addWidget(mapContainer, 1);
}

void MapPickerDialog::setupBottomBar(QVBoxLayout *panelLayout)
{
    QWidget *bottomBar = new QWidget(panelWidget);
    bottomBar->setFixedHeight(60);
    bottomBar->setStyleSheet("background-color: transparent; border-top: none;");

    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(15, 0, 15, 0);

    coordLabel = new QLabel("经纬度: 120.089000, 30.342000", bottomBar);
    coordLabel->setStyleSheet("color: #cccccc; font-size: 13px;");

    closeBtn = new QPushButton("关闭", bottomBar);
    closeBtn->setFixedSize(80, 32);
    closeBtn->setStyleSheet("QPushButton { background-color: transparent; color: #cccccc; border: 1px solid #666; "
                            "border-radius: 4px; font-size: 13px; }"
                            "QPushButton:hover { background-color: #444; color: #fff; border-color: #888; }");

    confirmBtn = new QPushButton("确定", bottomBar);
    confirmBtn->setFixedSize(80, 32);
    confirmBtn->setStyleSheet("QPushButton { background-color: #e67e22; color: #ffffff; border: none; border-radius: "
                              "4px; font-size: 13px; font-weight: bold; }"
                              "QPushButton:hover { background-color: #d35400; }");

    bottomLayout->addWidget(coordLabel);
    bottomLayout->addStretch();
    bottomLayout->addWidget(closeBtn);
    bottomLayout->addSpacing(15);
    bottomLayout->addWidget(confirmBtn);

    panelLayout->addWidget(bottomBar);
}

void MapPickerDialog::bindSignals(QPushButton *xBtn)
{
    connect(xBtn, &QPushButton::clicked, this, &MapPickerDialog::onCloseClicked);
    connect(closeBtn, &QPushButton::clicked, this, &MapPickerDialog::onCloseClicked);
    connect(confirmBtn, &QPushButton::clicked, this, &MapPickerDialog::onConfirmClicked);
    connect(webView->page(), &QWebEnginePage::titleChanged, this, &MapPickerDialog::onWebTitleChanged);
}

void MapPickerDialog::updatePanelGeometry()
{
    if (!parentWidget() || !panelWidget)
    {
        return;
    }

    const QSize parentSize = parentWidget()->size();
    const QSize panelSize(qMax(800, parentSize.width() / 2), qMax(500, parentSize.height() / 2));
    panelWidget->setFixedSize(panelSize);
}

void MapPickerDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updatePanelGeometry();
}

void MapPickerDialog::showOverlay()
{
    if (parentWidget())
    {
        setGeometry(parentWidget()->rect());
        raise();
    }
    updatePanelGeometry();
    show();
}

void MapPickerDialog::hideOverlay()
{
    hide();
}

void MapPickerDialog::setInitialLocation(float lng, float lat)
{
    selectedLng = lng;
    selectedLat = lat;

    // 如果经纬度在合法范围内，调用 JS 设置初始点
    if (lat >= -90.0f && lat <= 90.0f && lng >= -180.0f && lng <= 180.0f)
    {
        QString js = QString("if (typeof setInitialPoint === 'function') { setInitialPoint(%1, %2); }").arg(lat).arg(lng);
        webView->page()->runJavaScript(js);
    }

    coordLabel->setText(QString("经纬度: %1, %2").arg(lng, 0, 'f', 6).arg(lat, 0, 'f', 6));
}

void MapPickerDialog::onWebTitleChanged(const QString &title)
{
    if (title.startsWith("COORD:"))
    {
        QString coordStr = title.mid(6);
        QStringList parts = coordStr.split(",");
        if (parts.size() == 2)
        {
            selectedLng = parts[0].toFloat();
            selectedLat = parts[1].toFloat();
            coordLabel->setText(QString("经纬度: %1, %2").arg(selectedLng, 0, 'f', 6).arg(selectedLat, 0, 'f', 6));
        }
    }
}

void MapPickerDialog::onConfirmClicked()
{
    emit locationConfirmed(selectedLng, selectedLat);
    hideOverlay();
}

void MapPickerDialog::onCloseClicked()
{
    emit canceled();
    hideOverlay();
}
