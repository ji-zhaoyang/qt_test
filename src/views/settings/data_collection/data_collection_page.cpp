#include "data_collection_page.h"
#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

DataCollectionPage::DataCollectionPage(QWidget *parent)
    : QWidget(parent), fileNameEdit(nullptr), collectRadio(nullptr), realTimeRadio(nullptr), modeGroup(nullptr),
      collectionTimeSpinBox(nullptr), channelEdit(nullptr), startButton(nullptr), clearButton(nullptr),
      fileListWidget(nullptr), emptyStateIconLabel(nullptr), emptyStateTextLabel(nullptr)
{
    setupUi();
}

void DataCollectionPage::setupUi()
{
    setObjectName("dataCollectionPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#dataCollectionPage { background-color: #202020; color: #ffffff; }");

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
    pageLayout->setContentsMargins(40, 28, 20, 30);
    pageLayout->setSpacing(22);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel *pageTitle = new QLabel(QStringLiteral("数据采集"), content);
    pageTitle->setStyleSheet(titleStyle());
    pageLayout->addWidget(pageTitle);

    QFrame *card = new QFrame(content);
    card->setStyleSheet("QFrame { background-color: #2b2b2b; border-radius: 6px; }");
    pageLayout->addWidget(card);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(22, 12, 22, 24);
    cardLayout->setSpacing(0);

    fileNameEdit = new QLineEdit(card);
    fileNameEdit->setPlaceholderText(QStringLiteral("请输入"));
    fileNameEdit->setFixedSize(150, 34);
    fileNameEdit->setStyleSheet(inputStyle());
    cardLayout->addWidget(createFormRow(card, QStringLiteral("文件名"), fileNameEdit));
    cardLayout->addWidget(createSeparatorLine(card));

    cardLayout->addWidget(createModeRow(card));
    cardLayout->addWidget(createSeparatorLine(card));

    cardLayout->addWidget(createTimeRow(card));
    cardLayout->addWidget(createSeparatorLine(card));

    channelEdit = new QLineEdit(card);
    channelEdit->setText(QStringLiteral("1"));
    channelEdit->setFixedSize(150, 34);
    channelEdit->setStyleSheet(inputStyle());
    cardLayout->addWidget(createFormRow(card, QStringLiteral("采集通道"), channelEdit));
    cardLayout->addWidget(createSeparatorLine(card));

    QWidget *actionRow = new QWidget(card);
    actionRow->setStyleSheet("background-color: transparent;");
    QHBoxLayout *actionLayout = new QHBoxLayout(actionRow);
    actionLayout->setContentsMargins(0, 18, 0, 18);
    actionLayout->setSpacing(0);

    startButton = new QPushButton(QStringLiteral("开始采集"), actionRow);
    startButton->setFixedSize(128, 38);
    startButton->setStyleSheet(actionButtonStyle());
    actionLayout->addWidget(startButton, 0, Qt::AlignLeft | Qt::AlignVCenter);
    actionLayout->addStretch();
    cardLayout->addWidget(actionRow);

    QWidget *filesSection = createFilesSection(card);
    cardLayout->addWidget(filesSection);

    pageLayout->addStretch();
}

QWidget *DataCollectionPage::createSeparatorLine(QWidget *parent) const
{
    QFrame *line = new QFrame(parent);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #444444; max-height: 1px;");
    return line;
}

QWidget *DataCollectionPage::createFormRow(QWidget *parent, const QString &labelText, QWidget *fieldWidget) const
{
    QWidget *rowWidget = new QWidget(parent);
    rowWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 15, 0, 15);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel(labelText, rowWidget);
    label->setStyleSheet(rowLabelStyle());
    label->setFixedWidth(180);
    rowLayout->addWidget(label);
    rowLayout->addStretch();
    rowLayout->addWidget(fieldWidget, 0, Qt::AlignRight | Qt::AlignVCenter);
    return rowWidget;
}

QWidget *DataCollectionPage::createModeRow(QWidget *parent)
{
    QWidget *rowWidget = new QWidget(parent);
    rowWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 14, 0, 14);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel(QStringLiteral("模式"), rowWidget);
    label->setStyleSheet(rowLabelStyle());
    label->setFixedWidth(180);
    rowLayout->addWidget(label);
    rowLayout->addStretch();

    QWidget *modeWidget = new QWidget(rowWidget);
    modeWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *modeLayout = new QHBoxLayout(modeWidget);
    modeLayout->setContentsMargins(0, 0, 0, 0);
    modeLayout->setSpacing(20);

    collectRadio = new QRadioButton(QStringLiteral("采集"), modeWidget);
    realTimeRadio = new QRadioButton(QStringLiteral("实时"), modeWidget);
    collectRadio->setChecked(true);
    collectRadio->setStyleSheet("QRadioButton { color: #e6e6e6; font-size: 14px; }");
    realTimeRadio->setStyleSheet("QRadioButton { color: #e6e6e6; font-size: 14px; }");

    modeGroup = new QButtonGroup(this);
    modeGroup->addButton(collectRadio, 0);
    modeGroup->addButton(realTimeRadio, 1);

    modeLayout->addWidget(collectRadio);
    modeLayout->addWidget(realTimeRadio);
    rowLayout->addWidget(modeWidget, 0, Qt::AlignRight | Qt::AlignVCenter);
    return rowWidget;
}

QWidget *DataCollectionPage::createTimeRow(QWidget *parent)
{
    QWidget *rowWidget = new QWidget(parent);
    rowWidget->setStyleSheet("background-color: transparent;");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 15, 0, 15);
    rowLayout->setSpacing(10);

    QLabel *label = new QLabel(QStringLiteral("采集时间"), rowWidget);
    label->setStyleSheet(rowLabelStyle());
    label->setFixedWidth(180);
    rowLayout->addWidget(label);
    rowLayout->addStretch();

    QWidget *timeWidget = new QWidget(rowWidget);
    timeWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *timeLayout = new QHBoxLayout(timeWidget);
    timeLayout->setContentsMargins(0, 0, 0, 0);
    timeLayout->setSpacing(6);

    collectionTimeSpinBox = new QSpinBox(timeWidget);
    collectionTimeSpinBox->setRange(1, 9999);
    collectionTimeSpinBox->setValue(4);
    collectionTimeSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    collectionTimeSpinBox->setFixedSize(76, 34);
    collectionTimeSpinBox->setStyleSheet(inputStyle());

    QLabel *unitLabel = new QLabel(QStringLiteral("秒"), timeWidget);
    unitLabel->setStyleSheet("color: #bdbdbd; font-size: 14px;");

    timeLayout->addWidget(collectionTimeSpinBox);
    timeLayout->addWidget(unitLabel);
    rowLayout->addWidget(timeWidget, 0, Qt::AlignRight | Qt::AlignVCenter);
    return rowWidget;
}

QWidget *DataCollectionPage::createFilesSection(QWidget *parent)
{
    QWidget *section = new QWidget(parent);
    section->setStyleSheet("background-color: transparent;");

    QVBoxLayout *layout = new QVBoxLayout(section);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    QWidget *titleRow = new QWidget(section);
    titleRow->setStyleSheet("background-color: transparent;");
    QHBoxLayout *titleLayout = new QHBoxLayout(titleRow);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(0);

    QLabel *title = new QLabel(QStringLiteral("采集模式文件列表"), titleRow);
    title->setStyleSheet(listTitleStyle());
    titleLayout->addWidget(title);
    titleLayout->addStretch();

    clearButton = new QPushButton(QStringLiteral("清空文件"), titleRow);
    clearButton->setFixedSize(128, 34);
    clearButton->setStyleSheet(secondaryButtonStyle());
    titleLayout->addWidget(clearButton);
    layout->addWidget(titleRow);

    QWidget *listFrame = new QWidget(section);
    listFrame->setMinimumHeight(220);
    listFrame->setStyleSheet("background-color: #2b2b2b; border: 1px solid #3a3a3a; border-radius: 4px;");

    QVBoxLayout *listLayout = new QVBoxLayout(listFrame);
    listLayout->setContentsMargins(0, 0, 0, 0);
    listLayout->setSpacing(0);

    fileListWidget = new QListWidget(listFrame);
    fileListWidget->setFrameShape(QFrame::NoFrame);
    fileListWidget->setStyleSheet("QListWidget { background: transparent; color: #e6e6e6; border: none; }"
                                  "QListWidget::item { padding: 10px 14px; border-bottom: 1px solid #3b3b3b; }"
                                  "QListWidget::item:selected { background-color: #3f3f3f; }");
    fileListWidget->setVisible(false);
    listLayout->addWidget(fileListWidget);

    QWidget *emptyState = new QWidget(listFrame);
    emptyState->setStyleSheet("background-color: transparent;");
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyState);
    emptyLayout->setContentsMargins(0, 30, 0, 20);
    emptyLayout->setSpacing(8);
    emptyLayout->setAlignment(Qt::AlignCenter);

    emptyStateIconLabel = new QLabel(QStringLiteral("📄"), emptyState);
    emptyStateIconLabel->setAlignment(Qt::AlignCenter);
    emptyStateIconLabel->setStyleSheet("font-size: 42px; color: #9a9a9a;");

    emptyStateTextLabel = new QLabel(QStringLiteral("暂无文件"), emptyState);
    emptyStateTextLabel->setAlignment(Qt::AlignCenter);
    emptyStateTextLabel->setStyleSheet(emptyStateTextStyle());

    emptyLayout->addWidget(emptyStateIconLabel);
    emptyLayout->addWidget(emptyStateTextLabel);
    listLayout->addWidget(emptyState, 1);

    layout->addWidget(listFrame);
    return section;
}

QString DataCollectionPage::titleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold;");
}

QString DataCollectionPage::rowLabelStyle() const
{
    return QStringLiteral("color: #cccccc; font-size: 14px;");
}

QString DataCollectionPage::inputStyle() const
{
    return QStringLiteral("QLineEdit, QSpinBox { background-color: #111111; color: #ffffff; border: 1px solid #3d3d3d; "
                          "border-radius: 2px; padding: 0 10px; font-size: 14px; }");
}

QString DataCollectionPage::actionButtonStyle() const
{
    return QStringLiteral("QPushButton { background-color: #ffffff; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #e0e0e0; }");
}

QString DataCollectionPage::secondaryButtonStyle() const
{
    return QStringLiteral("QPushButton { background-color: #ffffff; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #e0e0e0; }");
}

QString DataCollectionPage::listTitleStyle() const
{
    return QStringLiteral("color: #e6e6e6; font-size: 14px;");
}

QString DataCollectionPage::emptyStateTextStyle() const
{
    return QStringLiteral("color: #a8a8a8; font-size: 13px;");
}
