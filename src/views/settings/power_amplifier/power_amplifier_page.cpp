#include "power_amplifier_page.h"
#include <QDebug>
#include <QDoubleValidator>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
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
constexpr int kDividerColumn = 1;
constexpr int kPowerAmplifierChannelCount = 6;

QString formatDoubleValue(double value)
{
    return QString::number(value, 'g', 10);
}
} // namespace

PowerAmplifierPage::PowerAmplifierPage(QWidget *parent)
    : QWidget(parent), queryButton(nullptr), applyButton(nullptr), cancelButton(nullptr), confirmButton(nullptr),
      editMode(false), toastWidget(nullptr), toastIconLabel(nullptr), toastTextLabel(nullptr), toastHideTimer(nullptr),
      toastOpacityEffect(nullptr), toastFadeInAnimation(nullptr), toastFadeOutAnimation(nullptr)
{
    currentParams = PowerAmplifierParamList(kPowerAmplifierChannelCount);
    setupUi();
}

void PowerAmplifierPage::setupUi()
{
    setObjectName("powerAmplifierPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#powerAmplifierPage { background-color: #202020; color: #ffffff; }");

    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(12, 10, 12, 18);
    pageLayout->setSpacing(12);
    pageLayout->setAlignment(Qt::AlignTop);

    QLabel *pageTitle = new QLabel(QStringLiteral("功放设置"), this);
    pageTitle->setStyleSheet(titleStyle());
    pageLayout->addWidget(pageTitle);

    QFrame *tableCard = new QFrame(this);
    tableCard->setStyleSheet("QFrame { background-color: #0d0d0d; border-radius: 6px; }");
    pageLayout->addWidget(tableCard);

    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(0, 0, 0, 0);
    tableLayout->setSpacing(0);

    tableLayout->addWidget(createHeaderRow(tableCard));
    for (int i = 0; i < kPowerAmplifierChannelCount; ++i)
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
    connect(queryButton, &QPushButton::clicked, this, &PowerAmplifierPage::handleQueryClicked);
    footerLayout->addWidget(queryButton);

    applyButton = new QPushButton(QStringLiteral("设置"), footerRow);
    applyButton->setFixedSize(224, 34);
    applyButton->setStyleSheet(actionButtonStyle());
    connect(applyButton, &QPushButton::clicked, this, &PowerAmplifierPage::handleApplyClicked);
    footerLayout->addWidget(applyButton);

    cancelButton = new QPushButton(QStringLiteral("取消"), footerRow);
    cancelButton->setFixedSize(224, 34);
    cancelButton->setStyleSheet(actionButtonStyle());
    connect(cancelButton, &QPushButton::clicked, this, &PowerAmplifierPage::handleCancelClicked);
    footerLayout->addWidget(cancelButton);

    confirmButton = new QPushButton(QStringLiteral("确认"), footerRow);
    confirmButton->setFixedSize(224, 34);
    confirmButton->setStyleSheet(actionButtonStyle());
    connect(confirmButton, &QPushButton::clicked, this, &PowerAmplifierPage::handleConfirmClicked);
    footerLayout->addWidget(confirmButton);

    tableLayout->addWidget(footerRow);
    pageLayout->addStretch();
    setEditMode(false);
}

void PowerAmplifierPage::updatePowerAmplifierParams(const PowerAmplifierParamList &params)
{
    currentParams = PowerAmplifierParamList(kPowerAmplifierChannelCount);
    for (int i = 0; i < kPowerAmplifierChannelCount; ++i)
    {
        currentParams[i] = i < params.size() ? params.at(i) : PowerAmplifierParam();
    }
    syncDisplayFromParams(currentParams);
    syncEditsFromCurrentParams();
}

void PowerAmplifierPage::showSaveResult(bool success, const QString &message)
{
    showToastResult(success, message);
}

void PowerAmplifierPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateToastPosition();
}

QWidget *PowerAmplifierPage::createHeaderRow(QWidget *parent) const
{
    QWidget *headerRow = new QWidget(parent);
    headerRow->setStyleSheet("background-color: #151515; border-top-left-radius: 6px; border-top-right-radius: 6px;");

    QGridLayout *headerLayout = new QGridLayout(headerRow);
    headerLayout->setContentsMargins(14, 14, 14, 14);
    headerLayout->setHorizontalSpacing(0);
    headerLayout->setVerticalSpacing(0);

    auto createHeaderLabel = [this, headerRow](const QString &text)
    {
        QLabel *label = new QLabel(text, headerRow);
        label->setStyleSheet(headerTextStyle());
        return label;
    };

    auto createDivider = [headerRow]()
    {
        QFrame *divider = new QFrame(headerRow);
        divider->setFrameShape(QFrame::VLine);
        divider->setStyleSheet("background-color: #2c2c2c; max-width: 1px;");
        return divider;
    };

    headerLayout->setColumnStretch(0, 4);
    headerLayout->setColumnStretch(2, 4);
    headerLayout->setColumnStretch(4, 4);
    headerLayout->setColumnStretch(6, 4);
    headerLayout->setColumnStretch(8, 4);
    headerLayout->setColumnMinimumWidth(1, kDividerColumn);
    headerLayout->setColumnMinimumWidth(3, kDividerColumn);
    headerLayout->setColumnMinimumWidth(5, kDividerColumn);
    headerLayout->setColumnMinimumWidth(7, kDividerColumn);

    headerLayout->addWidget(createHeaderLabel(QStringLiteral("序号")), 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    headerLayout->addWidget(createDivider(), 0, 1);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("K")), 0, 2, Qt::AlignLeft | Qt::AlignVCenter);
    headerLayout->addWidget(createDivider(), 0, 3);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("B")), 0, 4, Qt::AlignLeft | Qt::AlignVCenter);
    headerLayout->addWidget(createDivider(), 0, 5);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("输出功率")), 0, 6, Qt::AlignLeft | Qt::AlignVCenter);
    headerLayout->addWidget(createDivider(), 0, 7);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("att")), 0, 8, Qt::AlignLeft | Qt::AlignVCenter);

    return headerRow;
}

QWidget *PowerAmplifierPage::createDataRow(QWidget *parent, int index)
{
    QWidget *row = new QWidget(parent);
    row->setStyleSheet("background-color: #0d0d0d; border-top: 1px solid #272727;");

    QGridLayout *rowLayout = new QGridLayout(row);
    rowLayout->setContentsMargins(14, 12, 14, 12);
    rowLayout->setHorizontalSpacing(0);
    rowLayout->setVerticalSpacing(0);

    auto createCell = [row](const QString &text, const QString &style)
    {
        QLabel *label = new QLabel(text, row);
        label->setStyleSheet(style);
        return label;
    };

    auto createDivider = [row]()
    {
        QFrame *divider = new QFrame(row);
        divider->setFrameShape(QFrame::VLine);
        divider->setStyleSheet("background-color: #232323; max-width: 1px;");
        return divider;
    };

    rowLayout->setColumnStretch(0, 4);
    rowLayout->setColumnStretch(2, 4);
    rowLayout->setColumnStretch(4, 4);
    rowLayout->setColumnStretch(6, 4);
    rowLayout->setColumnStretch(8, 4);
    rowLayout->setColumnMinimumWidth(1, kDividerColumn);
    rowLayout->setColumnMinimumWidth(3, kDividerColumn);
    rowLayout->setColumnMinimumWidth(5, kDividerColumn);
    rowLayout->setColumnMinimumWidth(7, kDividerColumn);

    QLineEdit *kEdit = createValueEdit(row);
    QLabel *kValueLabel = createValueDisplayLabel(row);
    QLineEdit *bEdit = createValueEdit(row);
    QLabel *bValueLabel = createValueDisplayLabel(row);
    QLabel *outpowerLabel = createOutpowerLabel(row);
    QLineEdit *attEdit = createValueEdit(row);
    QLabel *attValueLabel = createValueDisplayLabel(row);

    kValueLabels.append(kValueLabel);
    kEdits.append(kEdit);
    bValueLabels.append(bValueLabel);
    bEdits.append(bEdit);
    outpowerLabels.append(outpowerLabel);
    attValueLabels.append(attValueLabel);
    attEdits.append(attEdit);

    rowLayout->addWidget(createCell(QStringLiteral("PA%1").arg(index + 1), cellTextStyle()), 0, 0, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(createDivider(), 0, 1);
    rowLayout->addWidget(kValueLabel, 0, 2, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(kEdit, 0, 2, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(createDivider(), 0, 3);
    rowLayout->addWidget(bValueLabel, 0, 4, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(bEdit, 0, 4, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(createDivider(), 0, 5);
    rowLayout->addWidget(outpowerLabel, 0, 6, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(createDivider(), 0, 7);
    rowLayout->addWidget(attValueLabel, 0, 8, Qt::AlignLeft | Qt::AlignVCenter);
    rowLayout->addWidget(attEdit, 0, 8, Qt::AlignLeft | Qt::AlignVCenter);

    return row;
}

QLineEdit *PowerAmplifierPage::createValueEdit(QWidget *parent) const
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

QLabel *PowerAmplifierPage::createValueDisplayLabel(QWidget *parent) const
{
    QLabel *label = new QLabel(QStringLiteral("0"), parent);
    label->setFixedWidth(128);
    label->setStyleSheet(valueTextStyle());
    return label;
}

QLabel *PowerAmplifierPage::createOutpowerLabel(QWidget *parent) const
{
    QLabel *label = new QLabel(QStringLiteral("0"), parent);
    label->setFixedWidth(128);
    label->setStyleSheet(valueTextStyle());
    return label;
}

bool PowerAmplifierPage::buildSavePayload(PowerAmplifierParamList &params, QString &errorMessage) const
{
    params.clear();
    for (int i = 0; i < kPowerAmplifierChannelCount; ++i)
    {
        bool okK = false;
        bool okB = false;
        bool okAtt = false;
        const double k = i < kEdits.size() ? kEdits.at(i)->text().trimmed().toDouble(&okK) : 0.0;
        const double b = i < bEdits.size() ? bEdits.at(i)->text().trimmed().toDouble(&okB) : 0.0;
        const double att = i < attEdits.size() ? attEdits.at(i)->text().trimmed().toDouble(&okAtt) : 0.0;
        if (!okK)
        {
            errorMessage = QStringLiteral("PA%1 的 K 参数不能为空。").arg(i + 1);
            return false;
        }
        if (!okB)
        {
            errorMessage = QStringLiteral("PA%1 的 B 参数不能为空。").arg(i + 1);
            return false;
        }
        if (!okAtt || att < 0.0 || att > 31.5)
        {
            errorMessage = QStringLiteral("PA%1 的 att 参数需在 0-31.5 之间。").arg(i + 1);
            return false;
        }

        PowerAmplifierParam param;
        param.k = k;
        param.b = b;
        param.att = att;
        param.outpower = i < outpowerLabels.size() ? outpowerLabels.at(i)->text().trimmed().toDouble() : 0.0;
        params.append(param);
    }

    return true;
}

void PowerAmplifierPage::handleQueryClicked()
{
    qDebug() << "[PowerAmplifierPage] 点击功放设置查询按钮";
    emit requestQueryPowerAmplifierParams();
}

void PowerAmplifierPage::handleApplyClicked()
{
    syncEditsFromCurrentParams();
    setEditMode(true);
}

void PowerAmplifierPage::handleCancelClicked()
{
    syncEditsFromCurrentParams();
    setEditMode(false);
}

void PowerAmplifierPage::handleConfirmClicked()
{
    PowerAmplifierParamList params;
    QString errorMessage;
    if (!buildSavePayload(params, errorMessage))
    {
        showToastResult(false, errorMessage);
        return;
    }

    currentParams = params;
    syncDisplayFromParams(currentParams);
    setEditMode(false);
    emit requestSavePowerAmplifierParams(params);
}

void PowerAmplifierPage::setEditMode(bool editing)
{
    editMode = editing;

    for (int i = 0; i < kEdits.size(); ++i)
    {
        if (i < kValueLabels.size())
        {
            kValueLabels.at(i)->setVisible(!editing);
        }
        kEdits.at(i)->setVisible(editing);
    }
    for (int i = 0; i < bEdits.size(); ++i)
    {
        if (i < bValueLabels.size())
        {
            bValueLabels.at(i)->setVisible(!editing);
        }
        bEdits.at(i)->setVisible(editing);
    }
    for (int i = 0; i < attEdits.size(); ++i)
    {
        if (i < attValueLabels.size())
        {
            attValueLabels.at(i)->setVisible(!editing);
        }
        attEdits.at(i)->setVisible(editing);
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

void PowerAmplifierPage::syncEditsFromCurrentParams()
{
    for (int i = 0; i < kPowerAmplifierChannelCount; ++i)
    {
        const PowerAmplifierParam param = i < currentParams.size() ? currentParams.at(i) : PowerAmplifierParam();
        if (i < kEdits.size())
        {
            kEdits.at(i)->setText(formatDoubleValue(param.k));
        }
        if (i < bEdits.size())
        {
            bEdits.at(i)->setText(formatDoubleValue(param.b));
        }
        if (i < attEdits.size())
        {
            attEdits.at(i)->setText(formatDoubleValue(param.att));
        }
    }
}

void PowerAmplifierPage::syncDisplayFromParams(const PowerAmplifierParamList &params)
{
    for (int i = 0; i < kPowerAmplifierChannelCount; ++i)
    {
        const PowerAmplifierParam param = i < params.size() ? params.at(i) : PowerAmplifierParam();
        if (i < kValueLabels.size())
        {
            kValueLabels.at(i)->setText(formatDoubleValue(param.k));
        }
        if (i < bValueLabels.size())
        {
            bValueLabels.at(i)->setText(formatDoubleValue(param.b));
        }
        if (i < outpowerLabels.size())
        {
            outpowerLabels.at(i)->setText(formatDoubleValue(param.outpower));
        }
        if (i < attValueLabels.size())
        {
            attValueLabels.at(i)->setText(formatDoubleValue(param.att));
        }
    }
}

void PowerAmplifierPage::ensureToastWidget()
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

void PowerAmplifierPage::updateToastPosition()
{
    if (!toastWidget)
    {
        return;
    }

    toastWidget->adjustSize();
    const int x = qMax(0, (width() - toastWidget->width()) / 2);
    toastWidget->move(x, 18);
}

void PowerAmplifierPage::showToastResult(bool success, const QString &message)
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

QString PowerAmplifierPage::extractDisplayMessage(bool success, const QString &message) const
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

QString PowerAmplifierPage::titleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold;");
}

QString PowerAmplifierPage::headerTextStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 14px; font-weight: bold; padding-left: 14px;");
}

QString PowerAmplifierPage::cellTextStyle() const
{
    return QStringLiteral("color: #f0f0f0; font-size: 14px; padding-left: 14px;");
}

QString PowerAmplifierPage::valueTextStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 14px; padding-left: 14px;");
}

QString PowerAmplifierPage::actionButtonStyle() const
{
    return QStringLiteral("QPushButton { background-color: #f0f0f0; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #dfdfdf; }");
}
