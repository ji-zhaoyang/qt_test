#include "model_library_page.h"
#include "model_library_edit_dialog.h"

#include <QComboBox>
#include <QDebug>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStackedLayout>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
constexpr uint8_t kModelLibraryModeNormal = 0;
constexpr uint8_t kModelLibraryModeAutoBuild = 1;
constexpr uint8_t kModelLibraryModeManualStart = 2;
}

ModelLibraryPage::ModelLibraryPage(QWidget *parent)
    : QWidget(parent),
      libraryTypeComboBox(nullptr),
      setButton(nullptr),
      refreshButton(nullptr),
      previousPageButton(nullptr),
      nextPageButton(nullptr),
      recordCountLabel(nullptr),
      pageInfoLabel(nullptr),
      tableBodyFrame(nullptr),
      tableContentStack(nullptr),
      recordListContainer(nullptr),
      recordListLayout(nullptr),
      emptyIconLabel(nullptr),
      emptyTextLabel(nullptr),
      emptyContainer(nullptr),
      toastWidget(nullptr),
      toastIconLabel(nullptr),
      toastTextLabel(nullptr),
      toastHideTimer(nullptr),
      toastOpacityEffect(nullptr),
      toastFadeInAnimation(nullptr),
      toastFadeOutAnimation(nullptr),
      editDialog(nullptr),
      currentPage(1),
      pageSize(10),
      totalRecords(0),
      lastRecordOperationWasDelete(false)
{
    setupUi();
}

void ModelLibraryPage::setupUi()
{
    setObjectName("modelLibraryPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#modelLibraryPage { background-color: #202020; color: #ffffff; }");

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
    pageLayout->setContentsMargins(20, 10, 20, 18);
    pageLayout->setSpacing(10);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel *pageTitle = new QLabel(QStringLiteral("机型库"), content);
    pageTitle->setStyleSheet(titleStyle());
    pageLayout->addWidget(pageTitle);

    QFrame *panel = new QFrame(content);
    panel->setStyleSheet("QFrame { background-color: #2a2d33; border-radius: 0px; }");
    panel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    pageLayout->addWidget(panel);

    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(12, 14, 12, 16);
    panelLayout->setSpacing(12);

    QWidget *toolbarRow = new QWidget(panel);
    toolbarRow->setStyleSheet("background-color: transparent;");
    QHBoxLayout *toolbarLayout = new QHBoxLayout(toolbarRow);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(8);

    libraryTypeComboBox = new QComboBox(toolbarRow);
    libraryTypeComboBox->addItem(QStringLiteral("正常侦测"), kModelLibraryModeNormal);
    libraryTypeComboBox->addItem(QStringLiteral("自动建库"), kModelLibraryModeAutoBuild);
    libraryTypeComboBox->addItem(QStringLiteral("手动开始建库"), kModelLibraryModeManualStart);
    libraryTypeComboBox->setFixedSize(180, 32);
    libraryTypeComboBox->setStyleSheet(comboBoxStyle());
    toolbarLayout->addWidget(libraryTypeComboBox, 0, Qt::AlignLeft | Qt::AlignVCenter);

    setButton = new QPushButton(QStringLiteral("设置"), toolbarRow);
    setButton->setFixedSize(150, 32);
    setButton->setStyleSheet(primaryButtonStyle());
    connect(setButton, &QPushButton::clicked, this,
            [this]()
            {
                const uint8_t mode = modeFromIndex(libraryTypeComboBox->currentIndex());
                emit requestSaveModelLibraryMode(mode);
            });
    toolbarLayout->addWidget(setButton, 0, Qt::AlignLeft | Qt::AlignVCenter);

    toolbarLayout->addStretch();

    refreshButton = new QPushButton(QStringLiteral("刷新"), toolbarRow);
    refreshButton->setFixedSize(150, 32);
    refreshButton->setStyleSheet(primaryButtonStyle());
    connect(refreshButton, &QPushButton::clicked, this,
            [this]()
            {
                emit requestQueryModelLibraryMode();
                queryCurrentPage();
            });
    toolbarLayout->addWidget(refreshButton, 0, Qt::AlignRight | Qt::AlignVCenter);

    panelLayout->addWidget(toolbarRow);
    panelLayout->addWidget(createHeaderRow(panel));

    tableBodyFrame = new QFrame(panel);
    tableBodyFrame->setStyleSheet("QFrame { background-color: #08090b; border-radius: 0px; }");
    tableBodyFrame->setMinimumHeight(120);
    panelLayout->addWidget(tableBodyFrame);

    tableContentStack = new QStackedLayout(tableBodyFrame);
    tableContentStack->setContentsMargins(0, 0, 0, 0);
    tableContentStack->setStackingMode(QStackedLayout::StackOne);

    recordListContainer = new QWidget(tableBodyFrame);
    recordListContainer->setStyleSheet("background-color: transparent;");
    recordListLayout = new QVBoxLayout(recordListContainer);
    recordListLayout->setContentsMargins(0, 0, 0, 0);
    recordListLayout->setSpacing(1);
    recordListLayout->setAlignment(Qt::AlignTop);
    tableContentStack->addWidget(recordListContainer);

    emptyContainer = new QWidget(tableBodyFrame);
    emptyContainer->setStyleSheet("background-color: transparent;");
    QVBoxLayout *emptyLayout = new QVBoxLayout(emptyContainer);
    emptyLayout->setContentsMargins(0, 16, 0, 16);
    emptyLayout->setSpacing(6);
    emptyLayout->setAlignment(Qt::AlignCenter);

    emptyIconLabel = new QLabel(QStringLiteral("▱"), emptyContainer);
    emptyIconLabel->setAlignment(Qt::AlignCenter);
    emptyIconLabel->setStyleSheet(emptyIconStyle());
    emptyLayout->addWidget(emptyIconLabel);

    emptyTextLabel = new QLabel(QStringLiteral("暂无数据"), emptyContainer);
    emptyTextLabel->setAlignment(Qt::AlignCenter);
    emptyTextLabel->setStyleSheet(emptyTextStyle());
    emptyLayout->addWidget(emptyTextLabel);

    tableContentStack->addWidget(emptyContainer);
    tableContentStack->setCurrentWidget(emptyContainer);

    QWidget *paginationRow = new QWidget(panel);
    paginationRow->setStyleSheet("background-color: transparent;");
    QHBoxLayout *paginationLayout = new QHBoxLayout(paginationRow);
    paginationLayout->setContentsMargins(0, 0, 0, 0);
    paginationLayout->setSpacing(10);

    recordCountLabel = new QLabel(QStringLiteral("共 0 条"), paginationRow);
    recordCountLabel->setStyleSheet("color: #a1a1aa; font-size: 13px;");

    previousPageButton = new QPushButton(QStringLiteral("上一页"), paginationRow);
    previousPageButton->setFixedSize(84, 30);
    previousPageButton->setStyleSheet(primaryButtonStyle());

    pageInfoLabel = new QLabel(QStringLiteral("第 1 / 1 页"), paginationRow);
    pageInfoLabel->setAlignment(Qt::AlignCenter);
    pageInfoLabel->setStyleSheet("color: #e5e7eb; font-size: 13px;");

    nextPageButton = new QPushButton(QStringLiteral("下一页"), paginationRow);
    nextPageButton->setFixedSize(84, 30);
    nextPageButton->setStyleSheet(primaryButtonStyle());

    paginationLayout->addWidget(recordCountLabel);
    paginationLayout->addStretch();
    paginationLayout->addWidget(previousPageButton);
    paginationLayout->addWidget(pageInfoLabel);
    paginationLayout->addWidget(nextPageButton);
    panelLayout->addWidget(paginationRow);

    editDialog = new ModelLibraryEditDialog(this);
    connect(editDialog, &ModelLibraryEditDialog::saveRequested, this,
            [this](const ModelLibraryUpdateRequest &request)
            {
                lastRecordOperationWasDelete = request.deleteFlag != 0;
                emit requestUpdateModelLibraryRecord(request);
            });
    connect(previousPageButton, &QPushButton::clicked, this, [this]() { queryPage(currentPage - 1); });
    connect(nextPageButton, &QPushButton::clicked, this, [this]() { queryPage(currentPage + 1); });

    renderRecordList();
    updatePaginationState();
}

void ModelLibraryPage::updateModelLibraryMode(uint8_t mode)
{
    const int index = indexFromMode(mode);
    if (index >= 0)
    {
        libraryTypeComboBox->setCurrentIndex(index);
    }
    else
    {
        qDebug() << "[ModelLibraryPage] 收到未展示的机型库模式:" << mode;
    }
}

void ModelLibraryPage::updateModelLibraryRecords(const ModelLibraryPageResult &result)
{
    totalRecords = qMax(0, result.total);
    currentPage = qMax(1, result.current);
    pageSize = qMax(1, result.size);
    recordCache = result.records;

    renderRecordList();
    updatePaginationState();
}

void ModelLibraryPage::showSaveResult(bool success, const QString &message)
{
    showToastResult(success, message);
    if (success)
    {
        emit requestQueryModelLibraryMode();
        queryCurrentPage();
    }
}

void ModelLibraryPage::showRecordSaveResult(bool success, const QString &message)
{
    showToastResult(success, message);
    if (!success)
    {
        lastRecordOperationWasDelete = false;
        return;
    }

    int targetPage = currentPage;
    if (lastRecordOperationWasDelete && recordCache.size() == 1 && currentPage > 1)
    {
        targetPage = currentPage - 1;
    }
    lastRecordOperationWasDelete = false;
    queryPage(targetPage);
}

void ModelLibraryPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateToastPosition();
}

void ModelLibraryPage::ensureToastWidget()
{
    if (toastWidget)
    {
        return;
    }

    toastWidget = new QWidget(this);
    toastWidget->setObjectName("modelLibraryToast");
    toastWidget->setStyleSheet("#modelLibraryToast { background-color: rgba(32, 32, 32, 230); border: 1px solid #4a4a4a; "
                               "border-radius: 4px; }");
    toastWidget->hide();

    QHBoxLayout *toastLayout = new QHBoxLayout(toastWidget);
    toastLayout->setContentsMargins(14, 10, 14, 10);
    toastLayout->setSpacing(8);

    toastIconLabel = new QLabel(QStringLiteral("!"), toastWidget);
    toastIconLabel->setStyleSheet("color: #ffffff; font-size: 14px; font-weight: bold;");
    toastLayout->addWidget(toastIconLabel);

    toastTextLabel = new QLabel(toastWidget);
    toastTextLabel->setStyleSheet("color: #ffffff; font-size: 14px;");
    toastLayout->addWidget(toastTextLabel);

    toastOpacityEffect = new QGraphicsOpacityEffect(toastWidget);
    toastOpacityEffect->setOpacity(0.0);
    toastWidget->setGraphicsEffect(toastOpacityEffect);

    toastFadeInAnimation = new QPropertyAnimation(toastOpacityEffect, "opacity", this);
    toastFadeInAnimation->setDuration(160);
    toastFadeInAnimation->setStartValue(0.0);
    toastFadeInAnimation->setEndValue(1.0);

    toastFadeOutAnimation = new QPropertyAnimation(toastOpacityEffect, "opacity", this);
    toastFadeOutAnimation->setDuration(220);
    toastFadeOutAnimation->setStartValue(1.0);
    toastFadeOutAnimation->setEndValue(0.0);
    connect(toastFadeOutAnimation, &QPropertyAnimation::finished, toastWidget, &QWidget::hide);

    toastHideTimer = new QTimer(this);
    toastHideTimer->setSingleShot(true);
    connect(toastHideTimer, &QTimer::timeout, this,
            [this]()
            {
                if (!toastWidget || !toastWidget->isVisible())
                {
                    return;
                }
                toastFadeOutAnimation->stop();
                toastFadeOutAnimation->start();
            });
}

void ModelLibraryPage::updateToastPosition()
{
    if (!toastWidget)
    {
        return;
    }

    toastWidget->adjustSize();
    const int x = (width() - toastWidget->width()) / 2;
    toastWidget->move(qMax(0, x), 16);
}

void ModelLibraryPage::showToastResult(bool success, const QString &message)
{
    ensureToastWidget();
    toastHideTimer->stop();
    toastFadeOutAnimation->stop();

    toastIconLabel->setText(success ? QStringLiteral("OK") : QStringLiteral("!"));
    toastTextLabel->setText(extractDisplayMessage(success, message));
    updateToastPosition();
    toastWidget->show();
    toastWidget->raise();
    toastFadeInAnimation->stop();
    toastFadeInAnimation->start();
    toastHideTimer->start(2200);
}

void ModelLibraryPage::renderRecordList()
{
    if (!recordListLayout || !tableContentStack)
    {
        return;
    }

    while (QLayoutItem *item = recordListLayout->takeAt(0))
    {
        if (QWidget *widget = item->widget())
        {
            widget->deleteLater();
        }
        delete item;
    }

    if (recordCache.isEmpty())
    {
        tableContentStack->setCurrentWidget(emptyContainer);
        return;
    }

    for (int i = 0; i < recordCache.size(); ++i)
    {
        recordListLayout->addWidget(createRecordRow(recordCache.at(i), i));
    }
    recordListLayout->addStretch();
    tableContentStack->setCurrentWidget(recordListContainer);
}

void ModelLibraryPage::updatePaginationState()
{
    const int pageCount = totalPages();
    if (recordCountLabel)
    {
        recordCountLabel->setText(QStringLiteral("共 %1 条").arg(totalRecords));
    }
    if (pageInfoLabel)
    {
        pageInfoLabel->setText(QStringLiteral("第 %1 / %2 页").arg(qMax(1, currentPage)).arg(pageCount));
    }
    if (previousPageButton)
    {
        previousPageButton->setEnabled(currentPage > 1);
    }
    if (nextPageButton)
    {
        nextPageButton->setEnabled(totalRecords > 0 && currentPage < pageCount);
    }
}

void ModelLibraryPage::queryPage(int page)
{
    int targetPage = qMax(1, page);
    if (totalRecords > 0)
    {
        targetPage = qMin(targetPage, totalPages());
    }

    emit requestQueryModelLibraryRecords(targetPage, pageSize);
}

void ModelLibraryPage::queryCurrentPage()
{
    queryPage(currentPage);
}

void ModelLibraryPage::openEditDialog(int rowIndex)
{
    if (!editDialog || rowIndex < 0 || rowIndex >= recordCache.size())
    {
        return;
    }

    editDialog->setRecord(recordCache.at(rowIndex));
    editDialog->exec();
}

void ModelLibraryPage::submitDeleteRecord(int rowIndex)
{
    if (rowIndex < 0 || rowIndex >= recordCache.size())
    {
        return;
    }

    const ModelLibraryRecord record = recordCache.at(rowIndex);
    const QString title = record.name.trimmed().isEmpty() ? QStringLiteral("该记录") : record.name.trimmed();
    const auto reply = QMessageBox::question(this, QStringLiteral("删除机型"),
                                             QStringLiteral("确认删除“%1”吗？").arg(title),
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes)
    {
        return;
    }

    lastRecordOperationWasDelete = true;
    emit requestUpdateModelLibraryRecord(ModelLibraryUpdateRequest{1, record});
}

QString ModelLibraryPage::formatFreqBands(const QVector<ModelLibraryFreqBand> &freqBands) const
{
    if (freqBands.isEmpty())
    {
        return QStringLiteral("-");
    }

    QStringList bandTexts;
    bandTexts.reserve(freqBands.size());
    for (const ModelLibraryFreqBand &band : freqBands)
    {
        bandTexts.append(QStringLiteral("%1-%2").arg(band.start).arg(band.end));
    }
    return bandTexts.join(QStringLiteral(", "));
}

QString ModelLibraryPage::enableText(int enable) const
{
    return enable == 0 ? QStringLiteral("禁用") : QStringLiteral("启用");
}

QString ModelLibraryPage::extractDisplayMessage(bool success, const QString &message) const
{
    if (success)
    {
        return QStringLiteral("设置成功");
    }

    const QString trimmed = message.trimmed();
    const int infoPos = trimmed.indexOf(QStringLiteral("Info:"));
    if (infoPos >= 0)
    {
        const QString infoText = trimmed.mid(infoPos + 5).trimmed();
        if (!infoText.isEmpty())
        {
            return infoText;
        }
    }
    return trimmed.isEmpty() ? QStringLiteral("设置失败") : trimmed;
}

int ModelLibraryPage::indexFromMode(uint8_t mode) const
{
    for (int i = 0; i < libraryTypeComboBox->count(); ++i)
    {
        if (libraryTypeComboBox->itemData(i).toUInt() == mode)
        {
            return i;
        }
    }
    return -1;
}

uint8_t ModelLibraryPage::modeFromIndex(int index) const
{
    return static_cast<uint8_t>(libraryTypeComboBox->itemData(index).toUInt());
}

int ModelLibraryPage::totalPages() const
{
    return qMax(1, (totalRecords + pageSize - 1) / pageSize);
}

QWidget *ModelLibraryPage::createHeaderRow(QWidget *parent) const
{
    QWidget *headerRow = new QWidget(parent);
    headerRow->setStyleSheet("background-color: #121315;");

    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(12, 12, 12, 12);
    headerLayout->setSpacing(0);

    auto createHeaderLabel = [this, headerRow](const QString &text, int minWidth, bool withDivider)
    {
        QLabel *label = new QLabel(text, headerRow);
        label->setMinimumWidth(minWidth);
        label->setStyleSheet(headerTextStyle());
        if (withDivider)
        {
            label->setStyleSheet(headerTextStyle() + QStringLiteral("border-right: 1px solid #282828;"));
        }
        return label;
    };

    headerLayout->addWidget(createHeaderLabel(QStringLiteral("名称"), 120, true), 2);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("频段(MHz)"), 220, true), 4);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("识别门限"), 150, true), 3);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("启用状态"), 120, true), 2);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("操作"), 90, false), 1);

    return headerRow;
}

QWidget *ModelLibraryPage::createRecordRow(const ModelLibraryRecord &record, int rowIndex)
{
    QFrame *rowFrame = new QFrame(recordListContainer);
    rowFrame->setStyleSheet("QFrame { background-color: #101113; border: none; }");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowFrame);
    rowLayout->setContentsMargins(12, 10, 12, 10);
    rowLayout->setSpacing(0);

    auto createCellLabel = [this, rowFrame](const QString &text, int minimumWidth, bool withDivider)
    {
        QLabel *label = new QLabel(text, rowFrame);
        label->setMinimumWidth(minimumWidth);
        label->setWordWrap(true);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        label->setStyleSheet(QStringLiteral("color: #e5e7eb; font-size: 13px; padding-left: 12px; padding-right: 8px;") +
                             (withDivider ? QStringLiteral("border-right: 1px solid #1f2937;") : QString()));
        return label;
    };

    rowLayout->addWidget(createCellLabel(record.name.trimmed().isEmpty() ? QStringLiteral("-") : record.name, 120, true), 2);
    rowLayout->addWidget(createCellLabel(formatFreqBands(record.freqbands), 220, true), 4);
    rowLayout->addWidget(createCellLabel(QString::number(record.sensitivity), 150, true), 3);
    rowLayout->addWidget(createCellLabel(enableText(record.enable), 120, true), 2);

    QWidget *actionWidget = new QWidget(rowFrame);
    actionWidget->setStyleSheet("background-color: transparent;");
    QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
    actionLayout->setContentsMargins(12, 0, 0, 0);
    actionLayout->setSpacing(8);

    QPushButton *editButton = new QPushButton(QStringLiteral("编辑"), actionWidget);
    editButton->setFixedSize(62, 30);
    editButton->setStyleSheet("QPushButton { background-color: transparent; color: #60a5fa; border: 1px solid #1d4ed8; "
                              "border-radius: 2px; font-size: 13px; font-weight: 500; }"
                              "QPushButton:hover { background-color: rgba(96, 165, 250, 36); }");

    QPushButton *deleteButton = new QPushButton(QStringLiteral("删除"), actionWidget);
    deleteButton->setFixedSize(62, 30);
    deleteButton->setStyleSheet("QPushButton { background-color: transparent; color: #f87171; border: 1px solid #991b1b; "
                                "border-radius: 2px; font-size: 13px; font-weight: 500; }"
                                "QPushButton:hover { background-color: rgba(248, 113, 113, 36); }");

    actionLayout->addWidget(editButton);
    actionLayout->addWidget(deleteButton);
    actionLayout->addStretch();
    rowLayout->addWidget(actionWidget, 1);

    connect(editButton, &QPushButton::clicked, this, [this, rowIndex]() { openEditDialog(rowIndex); });
    connect(deleteButton, &QPushButton::clicked, this, [this, rowIndex]() { submitDeleteRecord(rowIndex); });

    return rowFrame;
}

QString ModelLibraryPage::titleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold;");
}

QString ModelLibraryPage::comboBoxStyle() const
{
    return QStringLiteral("QComboBox { background-color: #101113; color: #ffffff; border: 1px solid #4a2f1e; "
                          "border-radius: 2px; padding-left: 10px; padding-right: 28px; font-size: 14px; }"
                          "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 28px; "
                          "border-left: 1px solid #2a2a2a; }"
                          "QComboBox::down-arrow { image: none; }"
                          "QComboBox QAbstractItemView { background-color: #202225; color: #ffffff; "
                          "selection-background-color: #3a2418; border: 1px solid #2e2e2e; }");
}

QString ModelLibraryPage::primaryButtonStyle() const
{
    return QStringLiteral("QPushButton { background-color: #f0f0f0; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #dfdfdf; }");
}

QString ModelLibraryPage::headerTextStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 14px; font-weight: bold; padding-left: 12px;");
}

QString ModelLibraryPage::emptyIconStyle() const
{
    return QStringLiteral("color: #23262b; font-size: 36px; font-weight: bold;");
}

QString ModelLibraryPage::emptyTextStyle() const
{
    return QStringLiteral("color: #2d3138; font-size: 13px;");
}
