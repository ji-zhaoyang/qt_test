#include "authorization_info_page.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QtGlobal>
#include <QVBoxLayout>

AuthorizationInfoPage::AuthorizationInfoPage(QWidget *parent)
    : QWidget(parent), attachmentLabel(nullptr), uploadButton(nullptr), saveButton(nullptr), usageLimitValueLabel(nullptr),
      usageTimeValueLabel(nullptr), usageCountValueLabel(nullptr)
{
    setupUi();
}

void AuthorizationInfoPage::setupUi()
{
    setObjectName("authorizationInfoPage");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#authorizationInfoPage { background-color: #202020; color: #ffffff; }");

    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(6, 10, 12, 18);
    pageLayout->setSpacing(10);
    pageLayout->setAlignment(Qt::AlignTop);

    pageLayout->addWidget(createUploadSection(this));
    pageLayout->addWidget(createUsageSection(this));
    pageLayout->addStretch();
}

QWidget *AuthorizationInfoPage::createUploadSection(QWidget *parent)
{
    QWidget *section = new QWidget(parent);
    QVBoxLayout *sectionLayout = new QVBoxLayout(section);
    sectionLayout->setContentsMargins(0, 0, 0, 0);
    sectionLayout->setSpacing(8);

    QLabel *sectionTitle = new QLabel(QStringLiteral("上传秘钥"), section);
    sectionTitle->setStyleSheet(sectionTitleStyle());
    sectionLayout->addWidget(sectionTitle);

    QFrame *card = new QFrame(section);
    card->setStyleSheet("QFrame { background-color: #2a2d33; border-radius: 0px; }");
    sectionLayout->addWidget(card);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    QWidget *row = new QWidget(card);
    row->setStyleSheet("background-color: #2a2d33; border-bottom: 1px solid #3a3d42;");
    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(20, 14, 16, 14);
    rowLayout->setSpacing(12);

    attachmentLabel = new QLabel(QStringLiteral("* 附件"), row);
    attachmentLabel->setStyleSheet(cellTextStyle());
    rowLayout->addWidget(attachmentLabel);
    rowLayout->addStretch();

    uploadButton = new QPushButton(QStringLiteral("  上传"), row);
    uploadButton->setFixedSize(156, 34);
    uploadButton->setStyleSheet(uploadButtonStyle());
    rowLayout->addWidget(uploadButton);

    cardLayout->addWidget(row);

    QWidget *footer = new QWidget(card);
    footer->setStyleSheet("background-color: #2a2d33;");
    QHBoxLayout *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(16, 18, 16, 18);
    footerLayout->setSpacing(0);

    saveButton = new QPushButton(QStringLiteral("保存"), footer);
    saveButton->setFixedSize(124, 34);
    saveButton->setStyleSheet(actionButtonStyle());
    footerLayout->addWidget(saveButton, 0, Qt::AlignLeft);
    footerLayout->addStretch();

    cardLayout->addWidget(footer);
    return section;
}

QWidget *AuthorizationInfoPage::createUsageSection(QWidget *parent)
{
    QWidget *section = new QWidget(parent);
    QVBoxLayout *sectionLayout = new QVBoxLayout(section);
    sectionLayout->setContentsMargins(0, 0, 0, 0);
    sectionLayout->setSpacing(8);

    QLabel *sectionTitle = new QLabel(QStringLiteral("设备使用时间查询"), section);
    sectionTitle->setStyleSheet(sectionTitleStyle());
    sectionLayout->addWidget(sectionTitle);

    QFrame *card = new QFrame(section);
    card->setStyleSheet("QFrame { background-color: #0d0d0d; border-radius: 6px; }");
    sectionLayout->addWidget(card);

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);
    cardLayout->addWidget(createUsageHeaderRow(card));
    cardLayout->addWidget(createUsageDataRow(card));

    return section;
}

QWidget *AuthorizationInfoPage::createUsageHeaderRow(QWidget *parent) const
{
    QWidget *headerRow = new QWidget(parent);
    headerRow->setStyleSheet("background-color: #151515; border-top-left-radius: 6px; border-top-right-radius: 6px;");

    QHBoxLayout *headerLayout = new QHBoxLayout(headerRow);
    headerLayout->setContentsMargins(10, 12, 10, 12);
    headerLayout->setSpacing(0);

    auto createHeaderLabel = [this, headerRow](const QString &text, bool withDivider)
    {
        QLabel *label = new QLabel(text, headerRow);
        label->setStyleSheet(withDivider ? headerTextStyle() + QStringLiteral("border-right: 1px solid #2c2c2c;")
                                         : headerTextStyle());
        return label;
    };

    headerLayout->addWidget(createHeaderLabel(QStringLiteral("使用期限"), true), 4);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("剩余使用时间"), true), 5);
    headerLayout->addWidget(createHeaderLabel(QStringLiteral("剩余使用次数"), false), 5);

    return headerRow;
}

QWidget *AuthorizationInfoPage::createUsageDataRow(QWidget *parent)
{
    QWidget *row = new QWidget(parent);
    row->setStyleSheet("background-color: #0d0d0d; border-top: 1px solid #272727;");

    QHBoxLayout *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(10, 12, 10, 12);
    rowLayout->setSpacing(0);

    auto createValueLabel = [row, this](const QString &text)
    {
        QLabel *label = new QLabel(text, row);
        label->setStyleSheet(valueTextStyle());
        return label;
    };

    usageLimitValueLabel = createValueLabel(QStringLiteral("--"));
    usageTimeValueLabel = createValueLabel(QStringLiteral("--"));
    usageCountValueLabel = createValueLabel(QStringLiteral("--"));

    rowLayout->addWidget(usageLimitValueLabel, 4);
    rowLayout->addWidget(usageTimeValueLabel, 5);
    rowLayout->addWidget(usageCountValueLabel, 5);

    return row;
}

void AuthorizationInfoPage::updateDeviceUsageInfo(const DeviceUsageInfo &info)
{
    if (!usageLimitValueLabel || !usageTimeValueLabel || !usageCountValueLabel)
    {
        return;
    }

    usageLimitValueLabel->setText(formatUsageLimit(info.limit));
    usageTimeValueLabel->setText(formatRemainingTime(info.limit, info.remainingTimeSeconds));
    usageCountValueLabel->setText(formatRemainingCount(info.limit, info.remainingCount));
}

QString AuthorizationInfoPage::titleStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 16px; font-weight: bold;");
}

QString AuthorizationInfoPage::sectionTitleStyle() const
{
    return titleStyle();
}

QString AuthorizationInfoPage::headerTextStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 14px; font-weight: bold; padding-left: 10px;");
}

QString AuthorizationInfoPage::cellTextStyle() const
{
    return QStringLiteral("color: #f0f0f0; font-size: 14px;");
}

QString AuthorizationInfoPage::valueTextStyle() const
{
    return QStringLiteral("color: #ffffff; font-size: 14px; padding-left: 10px;");
}

QString AuthorizationInfoPage::actionButtonStyle() const
{
    return QStringLiteral("QPushButton { background-color: #f0f0f0; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #dfdfdf; }");
}

QString AuthorizationInfoPage::uploadButtonStyle() const
{
    return QStringLiteral("QPushButton { background-color: #f0f0f0; color: #000000; border: none; border-radius: 2px; "
                          "font-size: 14px; font-weight: bold; text-align: center; }"
                          "QPushButton:hover { background-color: #dfdfdf; }");
}

QString AuthorizationInfoPage::formatUsageLimit(int limit) const
{
    return limit == 1 ? QStringLiteral("永久") : QStringLiteral("有期限");
}

QString AuthorizationInfoPage::formatRemainingTime(int limit, int remainingTimeSeconds) const
{
    if (limit == 1)
    {
        return QStringLiteral("永久");
    }

    const int safeSeconds = qMax(0, remainingTimeSeconds);
    const int days = safeSeconds / 86400;
    const int hours = (safeSeconds % 86400) / 3600;
    const int minutes = (safeSeconds % 3600) / 60;
    const int seconds = safeSeconds % 60;

    QStringList parts;
    if (days > 0)
    {
        parts << QStringLiteral("%1天").arg(days);
    }
    if (hours > 0 || !parts.isEmpty())
    {
        parts << QStringLiteral("%1小时").arg(hours);
    }
    if (minutes > 0 || !parts.isEmpty())
    {
        parts << QStringLiteral("%1分").arg(minutes);
    }
    parts << QStringLiteral("%1秒").arg(seconds);
    return parts.join(QString());
}

QString AuthorizationInfoPage::formatRemainingCount(int limit, int remainingCount) const
{
    if (limit == 1)
    {
        return QStringLiteral("永久");
    }

    return QString::number(qMax(0, remainingCount));
}
