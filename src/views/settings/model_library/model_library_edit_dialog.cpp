#include "model_library_edit_dialog.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

ModelLibraryEditDialog::ModelLibraryEditDialog(QWidget *parent)
    : QDialog(parent),
      currentType(0),
      nameEdit(nullptr),
      sensitivitySpinBox(nullptr),
      enableComboBox(nullptr),
      freqBandScrollArea(nullptr),
      freqBandListWidget(nullptr),
      freqBandListLayout(nullptr),
      addFreqBandButton(nullptr),
      cancelButton(nullptr),
      confirmButton(nullptr)
{
    setModal(true);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground, true);
    resize(720, 560);
    setupUi();
    clearForCreate();
}

void ModelLibraryEditDialog::clearForCreate()
{
    currentType = 0;
    nameEdit->clear();
    sensitivitySpinBox->setValue(1);
    enableComboBox->setCurrentIndex(0);

    clearFreqBandRows();
    addFreqBandRow();
}

void ModelLibraryEditDialog::setRecord(const ModelLibraryRecord &record)
{
    currentType = record.type;
    nameEdit->setText(record.name);
    sensitivitySpinBox->setValue(record.sensitivity);

    const int enableIndex = qMax(0, enableComboBox->findData(record.enable));
    enableComboBox->setCurrentIndex(enableIndex);

    clearFreqBandRows();
    if (record.freqbands.isEmpty())
    {
        addFreqBandRow();
    }
    else
    {
        for (const ModelLibraryFreqBand &band : record.freqbands)
        {
            addFreqBandRow(band);
        }
    }
}

ModelLibraryRecord ModelLibraryEditDialog::record() const
{
    ModelLibraryRecord result;
    result.type = currentType;
    result.name = nameEdit->text().trimmed();
    result.sensitivity = sensitivitySpinBox->value();
    result.enable = enableComboBox->currentData().toInt();

    for (const FreqBandRow &row : freqBandRows)
    {
        ModelLibraryFreqBand band;
        band.start = row.startEdit->text().toInt();
        band.end = row.endEdit->text().toInt();
        result.freqbands.append(band);
    }

    return result;
}

ModelLibraryUpdateRequest ModelLibraryEditDialog::buildUpdateRequest(int deleteFlag) const
{
    ModelLibraryUpdateRequest request;
    request.deleteFlag = deleteFlag;
    request.record = record();
    return request;
}

void ModelLibraryEditDialog::setupUi()
{
    setStyleSheet(
        "QDialog { background-color: #2b2b2b; border: 1px solid #444444; border-radius: 8px; }"
        "QLabel { color: #ffffff; }"
        "QLineEdit, QSpinBox, QComboBox {"
        "  min-height: 36px;"
        "  color: #ffffff;"
        "  background-color: #1f1f1f;"
        "  border: 1px solid #4b5563;"
        "  border-radius: 4px;"
        "  padding: 0 10px;"
        "}"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: #f59e0b; }"
        "QPushButton { border-radius: 4px; min-height: 34px; }");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);

    QWidget *titleBar = new QWidget(this);
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *titleLabel = new QLabel(QStringLiteral("编辑机型库"), titleBar);
    titleLabel->setStyleSheet("font-size: 18px; font-weight: 600; color: #ffffff;");

    QPushButton *closeButton = new QPushButton(QStringLiteral("关闭"), titleBar);
    closeButton->setFixedSize(72, 34);
    closeButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: #cbd5e1; border: 1px solid #4b5563; }"
        "QPushButton:hover { background-color: #374151; color: #ffffff; }");

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(closeButton);
    mainLayout->addWidget(titleBar);

    nameEdit = new QLineEdit(this);
    nameEdit->setPlaceholderText(QStringLiteral("请输入机型名称"));
    mainLayout->addWidget(createFieldRow(QStringLiteral("名称"), nameEdit));

    sensitivitySpinBox = new QSpinBox(this);
    sensitivitySpinBox->setRange(0, 255);
    sensitivitySpinBox->setValue(1);
    mainLayout->addWidget(createFieldRow(QStringLiteral("灵敏度"), sensitivitySpinBox));

    enableComboBox = new QComboBox(this);
    enableComboBox->addItem(QStringLiteral("启用"), 1);
    enableComboBox->addItem(QStringLiteral("禁用"), 0);
    mainLayout->addWidget(createFieldRow(QStringLiteral("状态"), enableComboBox));

    QWidget *freqBandHeader = new QWidget(this);
    QHBoxLayout *freqBandHeaderLayout = new QHBoxLayout(freqBandHeader);
    freqBandHeaderLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *freqBandTitle = new QLabel(QStringLiteral("频段配置"), freqBandHeader);
    freqBandTitle->setStyleSheet("font-size: 15px; font-weight: 600; color: #ffffff;");

    addFreqBandButton = new QPushButton(QStringLiteral("新增频段"), freqBandHeader);
    addFreqBandButton->setFixedSize(96, 34);
    addFreqBandButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: #fbbf24; border: 1px solid #f59e0b; }"
        "QPushButton:hover { background-color: rgba(245, 158, 11, 32); }");

    freqBandHeaderLayout->addWidget(freqBandTitle);
    freqBandHeaderLayout->addStretch();
    freqBandHeaderLayout->addWidget(addFreqBandButton);
    mainLayout->addWidget(freqBandHeader);

    freqBandScrollArea = new QScrollArea(this);
    freqBandScrollArea->setWidgetResizable(true);
    freqBandScrollArea->setFrameShape(QFrame::NoFrame);
    freqBandScrollArea->setStyleSheet("QScrollArea { background: transparent; }");

    freqBandListWidget = new QWidget(freqBandScrollArea);
    freqBandListWidget->setStyleSheet("background-color: transparent;");
    freqBandListLayout = new QVBoxLayout(freqBandListWidget);
    freqBandListLayout->setContentsMargins(0, 0, 0, 0);
    freqBandListLayout->setSpacing(12);
    freqBandListLayout->addStretch();
    freqBandScrollArea->setWidget(freqBandListWidget);
    mainLayout->addWidget(freqBandScrollArea, 1);

    QWidget *bottomBar = new QWidget(this);
    QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->setSpacing(12);

    cancelButton = new QPushButton(QStringLiteral("取消"), bottomBar);
    cancelButton->setFixedSize(88, 38);
    cancelButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: #cbd5e1; border: 1px solid #4b5563; }"
        "QPushButton:hover { background-color: #374151; color: #ffffff; }");

    confirmButton = new QPushButton(QStringLiteral("确定"), bottomBar);
    confirmButton->setFixedSize(88, 38);
    confirmButton->setStyleSheet(
        "QPushButton { background-color: #f59e0b; color: #ffffff; border: none; font-weight: 600; }"
        "QPushButton:hover { background-color: #d97706; }");

    bottomLayout->addStretch();
    bottomLayout->addWidget(cancelButton);
    bottomLayout->addWidget(confirmButton);
    mainLayout->addWidget(bottomBar);

    connect(closeButton, &QPushButton::clicked, this, &ModelLibraryEditDialog::reject);
    connect(cancelButton, &QPushButton::clicked, this, &ModelLibraryEditDialog::reject);
    connect(confirmButton, &QPushButton::clicked, this, &ModelLibraryEditDialog::handleConfirmClicked);
    connect(addFreqBandButton, &QPushButton::clicked, this, [this]() { addFreqBandRow(); });
}

QWidget *ModelLibraryEditDialog::createFieldRow(const QString &labelText, QWidget *editor) const
{
    QWidget *rowWidget = new QWidget();
    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(12);

    QLabel *label = new QLabel(labelText, rowWidget);
    label->setFixedWidth(72);
    label->setStyleSheet("font-size: 14px; color: #d1d5db;");

    rowLayout->addWidget(label);
    rowLayout->addWidget(editor, 1);
    return rowWidget;
}

void ModelLibraryEditDialog::addFreqBandRow(const ModelLibraryFreqBand &band)
{
    QWidget *rowWidget = new QWidget(freqBandListWidget);
    rowWidget->setStyleSheet("background-color: #232323; border: 1px solid #3f3f46; border-radius: 6px;");

    QHBoxLayout *rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(14, 12, 14, 12);
    rowLayout->setSpacing(10);

    QLabel *startLabel = new QLabel(QStringLiteral("起始"), rowWidget);
    startLabel->setStyleSheet("color: #d1d5db;");

    QLineEdit *startEdit = new QLineEdit(rowWidget);
    startEdit->setValidator(new QIntValidator(0, 999999, startEdit));
    startEdit->setPlaceholderText(QStringLiteral("MHz"));
    startEdit->setText(band.start > 0 ? QString::number(band.start) : QString());

    QLabel *endLabel = new QLabel(QStringLiteral("结束"), rowWidget);
    endLabel->setStyleSheet("color: #d1d5db;");

    QLineEdit *endEdit = new QLineEdit(rowWidget);
    endEdit->setValidator(new QIntValidator(0, 999999, endEdit));
    endEdit->setPlaceholderText(QStringLiteral("MHz"));
    endEdit->setText(band.end > 0 ? QString::number(band.end) : QString());

    QPushButton *removeButton = new QPushButton(QStringLiteral("删除"), rowWidget);
    removeButton->setFixedSize(72, 34);
    removeButton->setStyleSheet(
        "QPushButton { background-color: transparent; color: #f87171; border: 1px solid #7f1d1d; }"
        "QPushButton:hover { background-color: rgba(248, 113, 113, 36); }"
        "QPushButton:disabled { color: #6b7280; border-color: #374151; }");

    rowLayout->addWidget(startLabel);
    rowLayout->addWidget(startEdit, 1);
    rowLayout->addWidget(endLabel);
    rowLayout->addWidget(endEdit, 1);
    rowLayout->addWidget(removeButton);

    const int insertIndex = qMax(0, freqBandListLayout->count() - 1);
    freqBandListLayout->insertWidget(insertIndex, rowWidget);

    FreqBandRow row;
    row.rowWidget = rowWidget;
    row.startEdit = startEdit;
    row.endEdit = endEdit;
    row.removeButton = removeButton;
    freqBandRows.append(row);

    connect(removeButton, &QPushButton::clicked, this, [this, rowWidget]() { removeFreqBandRow(rowWidget); });

    refreshFreqBandRowState();
}

void ModelLibraryEditDialog::removeFreqBandRow(QWidget *rowWidget)
{
    if (!rowWidget)
    {
        return;
    }

    for (int i = 0; i < freqBandRows.size(); ++i)
    {
        if (freqBandRows[i].rowWidget != rowWidget)
        {
            continue;
        }

        if (freqBandRows.size() == 1)
        {
            freqBandRows[i].startEdit->clear();
            freqBandRows[i].endEdit->clear();
            return;
        }

        FreqBandRow row = freqBandRows.takeAt(i);
        freqBandListLayout->removeWidget(row.rowWidget);
        row.rowWidget->deleteLater();
        break;
    }

    refreshFreqBandRowState();
}

void ModelLibraryEditDialog::clearFreqBandRows()
{
    for (const FreqBandRow &row : freqBandRows)
    {
        freqBandListLayout->removeWidget(row.rowWidget);
        row.rowWidget->deleteLater();
    }
    freqBandRows.clear();
}

void ModelLibraryEditDialog::refreshFreqBandRowState()
{
    const bool allowRemove = freqBandRows.size() > 1;
    for (const FreqBandRow &row : freqBandRows)
    {
        row.removeButton->setEnabled(allowRemove);
    }
}

void ModelLibraryEditDialog::handleConfirmClicked()
{
    const ModelLibraryUpdateRequest request = buildUpdateRequest(0);
    emit saveRequested(request);
    accept();
}
