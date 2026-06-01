#ifndef SYSTEM_LOG_PAGE_H
#define SYSTEM_LOG_PAGE_H

#include <QWidget>

class QComboBox;
class QPushButton;
class QVBoxLayout;

class SystemLogPage : public QWidget
{
    Q_OBJECT

  public:
    explicit SystemLogPage(QWidget *parent = nullptr);

  private:
    void setupUi();
    QWidget *createHeaderRow(QWidget *parent) const;
    QWidget *createLogRow(QWidget *parent, const QString &contentText, const QString &timeText) const;
    QWidget *createFooterRow(QWidget *parent);
    QPushButton *createPageButton(QWidget *parent, const QString &text, bool active = false) const;
    QPushButton *createActionButton(QWidget *parent, const QString &text) const;
    QString titleStyle() const;
    QString headerTextStyle() const;
    QString contentTextStyle() const;
    QString timeTextStyle() const;
    QString paginationTextStyle() const;

    QPushButton *clearLogButton;
    QPushButton *exportLogButton;
    QComboBox *pageSizeComboBox;
};

#endif // SYSTEM_LOG_PAGE_H
