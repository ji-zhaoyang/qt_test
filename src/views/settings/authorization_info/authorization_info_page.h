#ifndef AUTHORIZATION_INFO_PAGE_H
#define AUTHORIZATION_INFO_PAGE_H

#include <QWidget>
#include "network/core/protocol_types.h"

class QLabel;
class QPushButton;

class AuthorizationInfoPage : public QWidget
{
    Q_OBJECT

  public:
    explicit AuthorizationInfoPage(QWidget *parent = nullptr);
    void updateDeviceUsageInfo(const DeviceUsageInfo &info);

  private:
    void setupUi();
    QWidget *createUploadSection(QWidget *parent);
    QWidget *createUsageSection(QWidget *parent);
    QWidget *createUsageHeaderRow(QWidget *parent) const;
    QWidget *createUsageDataRow(QWidget *parent);
    QString titleStyle() const;
    QString sectionTitleStyle() const;
    QString headerTextStyle() const;
    QString cellTextStyle() const;
    QString valueTextStyle() const;
    QString actionButtonStyle() const;
    QString uploadButtonStyle() const;
    QString formatUsageLimit(int limit) const;
    QString formatRemainingTime(int limit, int remainingTimeSeconds) const;
    QString formatRemainingCount(int limit, int remainingCount) const;

    QLabel *attachmentLabel;
    QPushButton *uploadButton;
    QPushButton *saveButton;
    QLabel *usageLimitValueLabel;
    QLabel *usageTimeValueLabel;
    QLabel *usageCountValueLabel;
};

#endif // AUTHORIZATION_INFO_PAGE_H
