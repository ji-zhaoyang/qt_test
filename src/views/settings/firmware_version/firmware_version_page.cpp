#include "firmware_version_page.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

FirmwareVersionPage::FirmwareVersionPage(QWidget *parent) : QWidget(parent)
{
    setupUi();
}

void FirmwareVersionPage::updateFirmwareVersions(const QString &appVersion, const QString &fpgaVersion,
                                                 const QString &gpuVersion)
{
    if (valueLabels.contains(QStringLiteral("App")))
    {
        valueLabels.value(QStringLiteral("App"))->setText(appVersion.trimmed().isEmpty() ? QStringLiteral("--")
                                                                                         : appVersion.trimmed());
    }

    if (valueLabels.contains(QStringLiteral("Fpga")))
    {
        valueLabels.value(QStringLiteral("Fpga"))->setText(fpgaVersion.trimmed().isEmpty() ? QStringLiteral("--")
                                                                                           : fpgaVersion.trimmed());
    }

    if (valueLabels.contains(QStringLiteral("Gpu")))
    {
        valueLabels.value(QStringLiteral("Gpu"))->setText(gpuVersion.trimmed().isEmpty() ? QStringLiteral("--")
                                                                                         : gpuVersion.trimmed());
    }
}

void FirmwareVersionPage::updateDeviceSerial(const QString &serialText)
{
    if (valueLabels.contains(QStringLiteral("设备序列号")))
    {
        valueLabels.value(QStringLiteral("设备序列号"))
            ->setText(serialText.trimmed().isEmpty() ? QStringLiteral("--") : serialText.trimmed());
    }
}

void FirmwareVersionPage::setupUi()
{
    setObjectName("firmwareVersionPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("background-color: #202020; color: #ffffff;");

    QVBoxLayout *hostLayout = new QVBoxLayout(this);
    hostLayout->setContentsMargins(0, 0, 0, 0);
    hostLayout->setSpacing(0);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }"
                              "QScrollBar:vertical { background: #1e1e1e; width: 10px; margin: 0px; }"
                              "QScrollBar::handle:vertical { background: #555555; min-height: 30px; border-radius: 4px; }"
                              "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
                              "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }");
    hostLayout->addWidget(scrollArea);

    QWidget *content = new QWidget(scrollArea);
    content->setStyleSheet("background-color: #202020;");
    scrollArea->setWidget(content);

    QVBoxLayout *pageLayout = new QVBoxLayout(content);
    pageLayout->setContentsMargins(40, 30, 20, 30);
    pageLayout->setSpacing(20);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel *pageTitle = new QLabel(QStringLiteral("固件版本号"), content);
    pageTitle->setStyleSheet(titleStyle());
    pageLayout->addWidget(pageTitle);

    QFrame *card = new QFrame(content);
    card->setStyleSheet("QFrame { background-color: #2b2b2b; border-radius: 6px; }");
    pageLayout->addWidget(card);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(20, 16, 20, 18);
    cardLayout->setSpacing(0);

    addInfoRow(cardLayout, card, QStringLiteral("设备序列号"), QStringLiteral("--"));
    addInfoRow(cardLayout, card, QStringLiteral("App"), QStringLiteral("V3.1.0  V260519"));
    addInfoRow(cardLayout, card, QStringLiteral("Fpga"), QStringLiteral("V1.7.0  V260107"));
    addInfoRow(cardLayout, card, QStringLiteral("Gpu"), QStringLiteral("V2.4.3  V260519"));
    addInfoRow(cardLayout, card, QStringLiteral("QT"), QStringLiteral("V1.0.0  202605211134"));

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 18, 0, 0);
    buttonLayout->setSpacing(12);

    QPushButton *firmwareUpgradeButton = new QPushButton(QStringLiteral("固件升级"), card);
    firmwareUpgradeButton->setFixedSize(120, 36);
    firmwareUpgradeButton->setStyleSheet(buttonStyle());

    QPushButton *clientUpgradeButton = new QPushButton(QStringLiteral("客户端升级"), card);
    clientUpgradeButton->setFixedSize(120, 36);
    clientUpgradeButton->setStyleSheet(buttonStyle());

    buttonLayout->addWidget(firmwareUpgradeButton);
    buttonLayout->addWidget(clientUpgradeButton);
    buttonLayout->addStretch();
    cardLayout->addLayout(buttonLayout);

    pageLayout->addStretch();
}

void FirmwareVersionPage::addInfoRow(QVBoxLayout *cardLayout, QWidget *cardWidget, const QString &labelText,
                                     const QString &valueText)
{
    QWidget *rowWidget = new QWidget(cardWidget);
    rowWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 14, 0, 14);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel(labelText, rowWidget);
    label->setStyleSheet(rowLabelStyle());
    label->setFixedWidth(180);
    rowLayout->addWidget(label);

    rowLayout->addStretch();

    QLabel *valueLabel = new QLabel(valueText, rowWidget);
    valueLabel->setStyleSheet(rowValueStyle());
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    rowLayout->addWidget(valueLabel);

    valueLabels.insert(labelText, valueLabel);
    cardLayout->addWidget(rowWidget);
    cardLayout->addWidget(createSeparatorLine(cardWidget));
}

QWidget *FirmwareVersionPage::createSeparatorLine(QWidget *parent) const
{
    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #444; max-height: 1px;");
    return line;
}

QString FirmwareVersionPage::titleStyle() const
{
    return "color: #ffffff; font-size: 15px; font-weight: bold;";
}

QString FirmwareVersionPage::rowLabelStyle() const
{
    return "color: #cccccc; font-size: 13px;";
}

QString FirmwareVersionPage::rowValueStyle() const
{
    return "color: #e6e6e6; font-size: 13px;";
}

QString FirmwareVersionPage::buttonStyle() const
{
    return "QPushButton { background-color: #ffffff; color: #000000; border: none; border-radius: 2px; font-size: 13px; "
           "font-weight: bold; }"
           "QPushButton:hover { background-color: #e0e0e0; }";
}
