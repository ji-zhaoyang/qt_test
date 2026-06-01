#include "direction_calibration_value_page.h"
#include <QDebug>
#include <QDoubleValidator>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
constexpr int kDirectionCalibrationValueCount = 6;

QString fieldNameForIndex(int index)
{
    static const QStringList names = {
        QStringLiteral("定标值1  (0~1000M)"),
        QStringLiteral("定标值2  (1000~2000M)"),
        QStringLiteral("定标值3  (2000~3000M)"),
        QStringLiteral("定标值4  (3000~4000M)"),
        QStringLiteral("定标值5  (4000~5000M)"),
        QStringLiteral("定标值6  (5000~6000M)"),
    };
    return index >= 0 && index < names.size() ? names.at(index) : QString();
}

QString formatValue(float value)
{
    return QString::number(value, 'g', 10);
}
} // namespace

DirectionCalibrationValuePage::DirectionCalibrationValuePage(QWidget *parent)
    : QWidget(parent), queryButton(nullptr), applyButton(nullptr), cancelButton(nullptr), confirmButton(nullptr),
      editMode(false), toastWidget(nullptr), toastIconLabel(nullptr), toastTextLabel(nullptr), toastHideTimer(nullptr),
      toastOpacityEffect(nullptr), toastFadeInAnimation(nullptr), toastFadeOutAnimation(nullptr)
{
    currentValues = DirectionCalibrationValueList(kDirectionCalibrationValueCount, 0.0f);
    setupUi();
}

void DirectionCalibrationValuePage::setupUi()
{
    setObjectName("directionCalibrationValuePage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#directionCalibrationValuePage { background-color: #202020; color: #ffffff; }");

    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(12, 10, 12, 18);
    pageLayout->setSpacing(12);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel *pageTitle = new QLabel(QStringLiteral("测向定标值设置"), this);
    pageTitle->setStyleSheet(titleStyle());
    pageLayout->addWidget(pageTitle);

    QFrame *tableCard = new QFrame(this);
    tableCard->setStyleSheet("QFrame { background-color: #0d0d0d; border-radius: 6px; }");
    pageLayout->addWidget(tableCard);

    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);

    tableLayout->addWidget(createHeaderRow(tableCard));
    for (int i = 0; i < kDirectionCalibrationValueCount; ++i)
    {
        tableLayout->addWidget(createDataRow(tableCard, i));
    }

    QWidget *footerRow = new QWidget(tableCard);
    footerRow->setStyleSheet("background-color: #2a2d33; border-bottom-left-radius: 6px; border-bottom-right-radius: 6px;");
    QHBoxLayout *footerLayout = new QHBoxLayout(footerRow);
    footerLayout->setContentsMargins(12, 18, 12, 18);
    footerLayout->setSpacing(18);
    footerLayout->addStretch();

    queryButton = new QPushButton(QStringLiteral("查询"), footerRow);
    queryButton->setFixedSize(224, 34);
    queryButton->setStyleSheet(actionButtonStyle());
    connect(queryButton, &QPushButton::clicked, this, &DirectionCalibrationValuePage::handleQueryClicked);
    footerLayout->addWidget(queryButton);

    applyButton = new QPushButton(QStringLiteral("设置"), footerRow);
    applyButton->setFixedSize(224, 34);
    applyButton->setStyleSheet(actionButtonStyle());
    connect(applyButton, &QPushButton::clicked, this, &DirectionCalibrationValuePage::handleApplyClicked);
    footerLayout->addWidget(applyButton);

    cancelButton = new QPushButton(QStringLiteral("取消"), footerRow);
    cancelButton->setFixedSize(224, 34);
    cancelButton->setStyleSheet(actionButtonStyle());
    connect(cancelButton, &QPushButton::clicked, this, &DirectionCalibrationValuePage::handleCancelClicked);
    footerLayout->addWidget(cancelButton);

    confirmButton = new QPushButton(QStringLiteral("确认"), footerRow);
    confirmButton->setFixedSize(224, 34);
    confirmButton->setStyleSheet(actionButtonStyle());
    connect(confirmButton, &QPushButton::clicked, this, &DirectionCalibrationValuePage::handleConfirmClicked);
    footerLayout->addWidget(confirmButton);

    tableLayout->addWidget(footerRow);
    pageLayout->addStretch();
    setEditMode(false);
}

void DirectionCalibrationValuePage::updateDirectionCalibrationValues(const DirectionCalibrationValueList &values)
{
    currentValues = DirectionCalibrationValueList(kDirectionCalibrationValueCount, 0.0f);
    for (int i = 0; i < kDirectionCalibrationValueCount; ++i)
    {
        currentValues[i] = i < values.size() ? values.at(i) : 0.0f;
    }
    syncDisplayFromValues(currentValues);
    syncEditsFromCurrentValues();
}

void DirectionCalibrationValuePage::showSaveResult(bool success, const QString &message)
{
    if (success)
    {
        DirectionCalibrationValueList values;
        QString errorMessage;
        if (buildSavePayload(values, errorMessage))
        {
            currentValues = values;
            syncDisplayFromValues(currentValues);
            syncEditsFromCurrentValues();
        }
        setEditMode(false);
    }
    showToastResult(success, message);
}

void DirectionCalibrationValuePage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateToastPosition();
}

QWidget *DirectionCalibrationValuePage::createHeaderRow(QWidget *parent) const
{
    QWidget *headerRow = new QWidget(parent);
    headerRow->setStyleSheet("background-color: #151515; border-top-left-radius: 6px; border-top-right-radius: 6px;");

    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(14, 14, 14, 14);
    headerLayout->setSpacing(0);

    QLabel *fieldHeader = new QLabel(QStringLiteral("字段名"), headerRow);
    fieldHeader->setStyleSheet(headerTextStyle() + QStringLiteral("border-right: 1px solid #2c2c2c;"));
    headerLayout->addWidget(fieldHeader, 11);

    QLabel *valueHeader = new QLabel(QStringLiteral("值"), headerRow);
    valueHeader->setStyleSheet(headerTextStyle());
    headerLayout->addWidget(valueHeader, 4);

    return headerRow;
}

QWidget *DirectionCalibrationValuePage::createDataRow(QWidget *parent, int index)
{
    QWidget *row = new QWidget(parent);
    row->setStyleSheet("background-color: #0d0d0d; border-top: 1px solid #272727;");

    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(14, 12, 14, 12);
    rowLayout->setSpacing(0);

    QLabel *fieldLabel = new QLabel(fieldNameForIndex(index), row);
    fieldLabel->setStyleSheet(cellTextStyle());

    QLabel *valueLabel = createValueDisplayLabel(row);
    QLineEdit *valueEdit = createValueEdit(row);
    valueLabels.append(valueLabel);
    valueEdits.append(valueEdit);

    rowLayout->addWidget(fieldLabel, 11);
    rowLayout->addWidget(valueLabel, 4, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(valueEdit, 4, Qt::AlignLeft | Qt::AlignVCenter);

    return row;
}

QLineEdit *DirectionCalibrationValuePage::createValueEdit(QWidget *parent) const
{
    QLineEdit *edit = new QLineEdit(parent);
    edit->setFixedSize(128, 32);
    edit->setStyleSheet("QLineEdit { background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; "
                        "border-radius: 2px; padding: 0 10px; font-size: 14px; }");
    auto *validator = new QDoubleValidator(-999999.0, 999999.0, 4, edit);
    validator->setNotation(QDoubleValidator::StandardNotation);
    edit->setValidator(validator);
    return edit;
}

QLabel *DirectionCalibrationValuePage::createValueDisplayLabel(QWidget *parent) const
{
    QLabel *label = new QLabel(QStringLiteral("0"), parent);
    label->setFixedWidth(128);
    label->setStyleSheet(valueTextStyle());
    return label;
}

bool DirectionCalibrationValuePage::buildSavePayload(DirectionCalibrationValueList &values, QString &errorMessage) const
{
    values.clear();
    for (int i = 0; i < kDirectionCalibrationValueCount; ++i)
    {
        bool ok = false;
        const float value = i < valueEdits.size() ? valueEdits.at(i)->text().trimmed().toFloat(&ok) : 0.0f;
        if (!ok)
        {
            errorMessage = QStringLiteral("定标值%1 不能为空。").arg(i + 1);
            return false;
        }
        values.append(value);
    }
    return true;
}

void DirectionCalibrationValuePage::handleQueryClicked()
{
    qDebug() << "[DirectionCalibrationValuePage] 点击测向定标值查询按钮";
    emit requestQueryDirectionCalibrationValues();
}

void DirectionCalibrationValuePage::handleApplyClicked()
{
    syncEditsFromCurrentValues();
    setEditMode(true);
}

void DirectionCalibrationValuePage::handleCancelClicked()
{
    syncEditsFromCurrentValues();
    setEditMode(false);
}

void DirectionCalibrationValuePage::handleConfirmClicked()
{
    DirectionCalibrationValueList values;
    QString errorMessage;
    if (!buildSavePayload(values, errorMessage))
    {
        showToastResult(false, errorMessage);
        return;
    }

    emit requestSaveDirectionCalibrationValues(values);
}

void DirectionCalibrationValuePage::setEditMode(bool editing)
{
    editMode = editing;
    for (int i = 0; i < valueEdits.size(); ++i)
    {
        if (i < valueLabels.size())
        {
            valueLabels.at(i)->setVisible(!editing);
        }
        valueEdits.at(i)->setVisible(editing);
    }

    if (queryButton)
    {
        queryButton->setVisible(!editing);
    }
    if (applyButton)
    {
        applyButton->setVisible(!editing);
    }
    if (cancelButton)
    {
        cancelButton->setVisible(editing);
    }
    if (confirmButton)
    {
        confirmButton->setVisible(editing);
    }
}

void DirectionCalibrationValuePage::syncEditsFromCurrentValues()
{
    for (int i = 0; i < valueEdits.size(); ++i)
    {
        const float value = i < currentValues.size() ? currentValues.at(i) : 0.0f;
        valueEdits.at(i)->setText(formatValue(value));
    }
}

void DirectionCalibrationValuePage::syncDisplayFromValues(const DirectionCalibrationValueList &values)
{
    for (int i = 0; i < valueLabels.size(); ++i)
    {
        const float value = i < values.size() ? values.at(i) : 0.0f;
        valueLabels.at(i)->setText(formatValue(value));
    }
}

void DirectionCalibrationValuePage::ensureToastWidget()
{
    if (toastWidget)
    {
        return;
    }

    toastWidget = new QWidget(this);
    toastWidget->setAttribute(Qt::WA_StyledBackground, true);
    toastWidget->setStyleSheet("background-color: rgba(23, 23, 23, 235); border-radius: 12px;");

    QHBoxLayout *layout = new QHBoxLayout(toastWidget);
    layout->setContentsMargins(18, 12, 18, 12);
    layout->setSpacing(10);

    toastIconLabel = new QLabel(toastWidget);
    toastIconLabel->setFixedSize(20, 20);
    toastIconLabel->setAlignment(Qt::AlignCenter);

    toastTextLabel = new QLabel(toastWidget);
    toastTextLabel->setStyleSheet("color: #ffffff; font-size: 16px; font-weight: 600;");

    layout->addWidget(toastIconLabel);
    layout->addWidget(toastTextLabel);
    toastWidget->hide();

    toastOpacityEffect = new QGraphicsOpacityEffect(toastWidget);
    toastOpacityEffect->setOpacity(0.0);
    toastWidget->setGraphicsEffect(toastOpacityEffect);

    toastFadeInAnimation = new QPropertyAnimation(toastOpacityEffect, "opacity", this);
    toastFadeInAnimation->setDuration(180);
    toastFadeInAnimation->setStartValue(0.0);
    toastFadeInAnimation->setEndValue(1.0);

    toastFadeOutAnimation = new QPropertyAnimation(toastOpacityEffect, "opacity", this);
    toastFadeOutAnimation->setDuration(220);
    toastFadeOutAnimation->setStartValue(1.0);
    toastFadeOutAnimation->setEndValue(0.0);
    connect(toastFadeOutAnimation, &QPropertyAnimation::finished, this,
            [this]()
            {
                if (toastWidget && toastOpacityEffect && toastOpacityEffect->opacity() <= 0.01)
                {
                    toastWidget->hide();
                }
            });

    toastHideTimer = new QTimer(this);
    toastHideTimer->setSingleShot(true);
    connect(toastHideTimer, &QTimer::timeout, this,
            [this]()
            {
                if (toastFadeInAnimation)
                {
                    toastFadeInAnimation->stop();
                }
                if (toastFadeOutAnimation)
                {
                    toastFadeOutAnimation->start();
                }
            });
}

void DirectionCalibrationValuePage::updateToastPosition()
{
    if (!toastWidget)
    {
        return;
    }

    toastWidget->adjustSize();
    const int x = qMax(0, (width() - toastWidget->width()) / 2);
    toastWidget->move(x, 18);
}

void DirectionCalibrationValuePage::showToastResult(bool success, const QString &message)
{
    ensureToastWidget();

    toastIconLabel->setText(success ? QStringLiteral("✓") : QStringLiteral("!"));
    toastIconLabel->setStyleSheet(success
                                      ? "background-color: #67c23a; color: #ffffff; font-size: 14px; font-weight: bold; "
                                        "border-radius: 10px;"
                                      : "background-color: #ff9f55; color: #ffffff; font-size: 14px; font-weight: bold; "
                                        "border-radius: 10px;");
    toastTextLabel->setText(extractDisplayMessage(success, message));
    updateToastPosition();

    if (toastFadeOutAnimation)
    {
        toastFadeOutAnimation->stop();
    }
    if (toastFadeInAnimation)
    {
        toastFadeInAnimation->stop();
    }
    if (toastHideTimer)
    {
        toastHideTimer->stop();
    }
    if (toastOpacityEffect)
    {
        toastOpacityEffect->setOpacity(0.0);
    }

    toastWidget->show();
    toastWidget->raise();
    if (toastFadeInAnimation)
    {
        toastFadeInAnimation->start();
    }
    if (toastHideTimer)
    {
        toastHideTimer->start(1800);
    }
}

QString DirectionCalibrationValuePage::extractDisplayMessage(bool success, const QString &message) const
{
    if (message.trimmed().isEmpty())
    {
        return success ? QStringLiteral("设置成功") : QStringLiteral("设置失败");
    }

    const QString trimmed = message.trimmed();
    const int infoIndex = trimmed.indexOf("Info:");
    if (infoIndex >= 0)
    {
        const QString infoText = trimmed.mid(infoIndex + 5).trimmed();
        if (!infoText.isEmpty())
        {
            return infoText;
        }
    }

    return success ? QStringLiteral("设置成功") : trimmed;
}

QString DirectionCalibrationValuePage::titleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold;");
}

QString DirectionCalibrationValuePage::headerTextStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 14px; font-weight: bold; padding-left: 14px;");
}

QString DirectionCalibrationValuePage::cellTextStyle() const
{
    return QStringLiteral("color: #f0f0f0; font-size: 14px; padding-left: 14px;");
}

QString DirectionCalibrationValuePage::valueTextStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 14px; padding-left: 14px;");
}

QString DirectionCalibrationValuePage::actionButtonStyle() const
{
    return QStringLiteral("QPushButton { background-color: #f0f0f0; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #dfdfdf; }");
}
