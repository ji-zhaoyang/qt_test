#include "data_collection_page.h"
#include <QAbstractSpinBox>
#include <QButtonGroup>
#include <QDateTime>
#include <QDebug>
#include <QDir>
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

namespace
{
const char *const kDefaultFtpIp = "10.0.76.153";
const int kDefaultFtpPort = 21;
const char *const kDefaultFtpUser = "finsung";
const char *const kDefaultFtpPassword = "finsung";
const char *const kDefaultCollectionFtpPath = "/home/finsung/collection";
const char *const kDefaultRealtimeFtpPath = "/home/finsung/timely";
const double kDefaultFreq = 1.0;
}

DataCollectionPage::DataCollectionPage(QWidget *parent)
    : QWidget(parent), collectRadio(nullptr), realTimeRadio(nullptr), modeGroup(nullptr), fileNameEdit(nullptr),
      collectionTimeRow(nullptr), collectionTimeSeparator(nullptr), collectionTimeSpinBox(nullptr), channelRow(nullptr),
      channelSeparator(nullptr), channelSpinBox(nullptr), startButton(nullptr), clearButton(nullptr),
      filesTitleLabel(nullptr), fileListWidget(nullptr), emptyStateIconLabel(nullptr), emptyStateTextLabel(nullptr),
      statusLabel(nullptr)
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
    fileNameEdit->setPlaceholderText(QStringLiteral("可留空，设备自动生成"));
    fileNameEdit->setFixedSize(150, 34);
    fileNameEdit->setStyleSheet(inputStyle());
    cardLayout->addWidget(createFormRow(card, QStringLiteral("文件名"), fileNameEdit));
    cardLayout->addWidget(createSeparatorLine(card));

    cardLayout->addWidget(createModeRow(card));
    cardLayout->addWidget(createSeparatorLine(card));

    collectionTimeSpinBox = new QSpinBox(card);
    collectionTimeSpinBox->setRange(1, 8);
    collectionTimeSpinBox->setValue(1);
    collectionTimeSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    collectionTimeSpinBox->setFixedSize(150, 34);
    collectionTimeSpinBox->setStyleSheet(inputStyle());
    collectionTimeRow = createFormRow(card, QStringLiteral("采集时间"), collectionTimeSpinBox);
    collectionTimeSeparator = createSeparatorLine(card);
    cardLayout->addWidget(collectionTimeRow);
    cardLayout->addWidget(collectionTimeSeparator);

    channelSpinBox = new QSpinBox(card);
    channelSpinBox->setRange(1, 4);
    channelSpinBox->setValue(1);
    channelSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    channelSpinBox->setFixedSize(150, 34);
    channelSpinBox->setStyleSheet(inputStyle());
    channelRow = createFormRow(card, QStringLiteral("采集通道"), channelSpinBox);
    channelSeparator = createSeparatorLine(card);
    cardLayout->addWidget(channelRow);
    cardLayout->addWidget(channelSeparator);

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

    statusLabel = new QLabel(card);
    statusLabel->setVisible(false);
    statusLabel->setWordWrap(true);
    statusLabel->setStyleSheet("font-size: 13px; padding: 0 0 14px 0;");
    cardLayout->addWidget(statusLabel);

    QWidget *filesSection = createFilesSection(card);
    cardLayout->addWidget(filesSection);

    pageLayout->addStretch();

    connect(startButton, &QPushButton::clicked, this,
            [this]()
            {
                if (!fileNameEdit || !modeGroup || !collectionTimeSpinBox || !channelSpinBox)
                {
                    return;
                }

                PatternUploadRequest request;
                request.ip = QString::fromLatin1(kDefaultFtpIp);
                request.port = kDefaultFtpPort;
                request.user = QString::fromLatin1(kDefaultFtpUser);
                request.password = QString::fromLatin1(kDefaultFtpPassword);
                request.path = request.type == 2 ? QString::fromLatin1(kDefaultRealtimeFtpPath)
                                                 : QString::fromLatin1(kDefaultCollectionFtpPath);
                request.filename = fileNameEdit->text().trimmed();
                request.time = collectionTimeSpinBox->value();
                request.type = modeGroup->checkedId();
                request.channel = channelSpinBox->value();
                request.freq = kDefaultFreq;

                // #region debug-point A:data-collection-click
                qDebug().noquote()
                    << QStringLiteral("[DEBUG-A] data_collection_page.cpp:startButton | 点击开始采集 | "
                                      "filename=\"%1\" type=%2 time=%3 channel=%4 freq=%5 ftp=%6:%7 user=%8 path=%9")
                           .arg(request.filename,
                                QString::number(request.type),
                                QString::number(request.time),
                                QString::number(request.channel),
                                QString::number(request.freq, 'f', 1),
                                request.ip,
                                QString::number(request.port),
                                request.user,
                                request.path);
                // #endregion

                if (request.type != 1 && request.type != 2)
                {
                    showStatusMessage(false, QStringLiteral("请选择采集类型"));
                    return;
                }

                showStatusMessage(true, QStringLiteral("任务已下发，等待设备应答..."));
                emit requestUploadPatternFile(request);
            });

    connect(clearButton, &QPushButton::clicked, this,
            [this]()
            {
                if (!fileListWidget)
                {
                    return;
                }

                const QDir dir(currentModeDirectoryPath());
                if (!dir.exists())
                {
                    refreshCurrentFileList();
                    return;
                }

                const QFileInfoList fileInfos =
                    dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
                for (const QFileInfo &fileInfo : fileInfos)
                {
                    QFile::remove(fileInfo.absoluteFilePath());
                }
                refreshCurrentFileList();
            });

    connect(modeGroup, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this,
            [this](int)
            {
                updateModeFieldVisibility();
                updateFilesSectionForMode();
            });

    updateModeFieldVisibility();
    updateFilesSectionForMode();
}

void DataCollectionPage::updateModeFieldVisibility()
{
    const bool isRealtimeMode = modeGroup && modeGroup->checkedId() == 2;

    if (collectionTimeRow)
    {
        collectionTimeRow->setVisible(!isRealtimeMode);
    }
    if (collectionTimeSeparator)
    {
        collectionTimeSeparator->setVisible(!isRealtimeMode);
    }
    if (channelRow)
    {
        channelRow->setVisible(!isRealtimeMode);
    }
    if (channelSeparator)
    {
        channelSeparator->setVisible(!isRealtimeMode);
    }
}

void DataCollectionPage::refreshCurrentFileList()
{
    if (!fileListWidget)
    {
        return;
    }

    fileListWidget->clear();

    const QDir dir(currentModeDirectoryPath());
    if (dir.exists())
    {
        const QFileInfoList fileInfos =
            dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Time | QDir::Reversed);
        for (const QFileInfo &fileInfo : fileInfos)
        {
            const QString itemText =
                QStringLiteral("%1  (%2)")
                    .arg(fileInfo.fileName(),
                         fileInfo.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
            fileListWidget->addItem(itemText);
        }
    }
    updateFileListVisibility();
}

void DataCollectionPage::updateFilesSectionForMode()
{
    const bool isRealtimeMode = currentModeType() == 2;

    if (filesTitleLabel)
    {
        filesTitleLabel->setText(isRealtimeMode ? QStringLiteral("实时数据文件列表") : QStringLiteral("长数据文件列表"));
    }
    if (emptyStateTextLabel)
    {
        emptyStateTextLabel->setText(isRealtimeMode ? QStringLiteral("暂无实时数据文件记录")
                                                    : QStringLiteral("暂无长数据文件记录"));
    }

    refreshCurrentFileList();
}

int DataCollectionPage::currentModeType() const
{
    return modeGroup ? modeGroup->checkedId() : 1;
}

QString DataCollectionPage::currentModeDirectoryPath() const
{
    return currentModeType() == 2 ? QString::fromLatin1(kDefaultRealtimeFtpPath)
                                  : QString::fromLatin1(kDefaultCollectionFtpPath);
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

    QLabel *label = new QLabel(QStringLiteral("采集类型"), rowWidget);
    label->setStyleSheet(rowLabelStyle());
    label->setFixedWidth(180);
    rowLayout->addWidget(label);
    rowLayout->addStretch();

    QWidget *modeWidget = new QWidget(rowWidget);
    modeWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *modeLayout = new QHBoxLayout(modeWidget);
    modeLayout->setContentsMargins(0, 0, 0, 0);
    modeLayout->setSpacing(20);

    collectRadio = new QRadioButton(QStringLiteral("采集长数据"), modeWidget);
    realTimeRadio = new QRadioButton(QStringLiteral("采集实时数据"), modeWidget);
    collectRadio->setChecked(true);
    collectRadio->setStyleSheet("QRadioButton { color: #e6e6e6; font-size: 14px; }");
    realTimeRadio->setStyleSheet("QRadioButton { color: #e6e6e6; font-size: 14px; }");

    modeGroup = new QButtonGroup(this);
    modeGroup->addButton(collectRadio, 1);
    modeGroup->addButton(realTimeRadio, 2);

    modeLayout->addWidget(collectRadio);
    modeLayout->addWidget(realTimeRadio);
    rowLayout->addWidget(modeWidget, 0, Qt::AlignRight | Qt::AlignVCenter);
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

    filesTitleLabel = new QLabel(QStringLiteral("长数据文件列表"), titleRow);
    filesTitleLabel->setStyleSheet(listTitleStyle());
    titleLayout->addWidget(filesTitleLabel);
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

    emptyStateTextLabel = new QLabel(QStringLiteral("暂无 pattern 文件记录"), emptyState);
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

void DataCollectionPage::showPatternUploadResult(bool success, const QString &message)
{
    showStatusMessage(success, message);
    if (success)
    {
        refreshCurrentFileList();
    }
}

void DataCollectionPage::updateFileListVisibility()
{
    const bool hasItems = fileListWidget && fileListWidget->count() > 0;
    if (fileListWidget)
    {
        fileListWidget->setVisible(hasItems);
    }
    if (emptyStateIconLabel)
    {
        emptyStateIconLabel->setVisible(!hasItems);
    }
    if (emptyStateTextLabel)
    {
        emptyStateTextLabel->setVisible(!hasItems);
    }
}

void DataCollectionPage::showStatusMessage(bool success, const QString &message)
{
    if (!statusLabel)
    {
        return;
    }

    statusLabel->setVisible(true);
    statusLabel->setStyleSheet(success ? QStringLiteral("color: #67c23a; font-size: 13px; padding: 0 0 14px 0;")
                                       : QStringLiteral("color: #ff6a55; font-size: 13px; padding: 0 0 14px 0;"));
    statusLabel->setText(extractDisplayMessage(message));
}

QString DataCollectionPage::extractDisplayMessage(const QString &message) const
{
    const QString trimmed = message.trimmed();
    if (trimmed.isEmpty())
    {
        return QStringLiteral("设备未返回结果");
    }

    const int infoPos = trimmed.indexOf(QStringLiteral("Info:"));
    if (infoPos >= 0)
    {
        const QString infoText = trimmed.mid(infoPos + 5).trimmed();
        if (!infoText.isEmpty())
        {
            return infoText;
        }
    }

    return trimmed;
}
