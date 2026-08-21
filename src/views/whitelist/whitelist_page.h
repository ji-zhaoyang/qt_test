#ifndef WHITELIST_PAGE_H
#define WHITELIST_PAGE_H

#include <QVector>
#include <QWidget>

class QLabel;
class QPushButton;
class QComboBox;
class QTableWidget;
class WhitelistRepository;

class WhitelistPage : public QWidget
{
    Q_OBJECT

  public:
    struct WhitelistRecord
    {
        int id = 0;
        QString serialNumber;
        QString recordKey;
        QString modelName;
        QString remarks;
        QString effectiveTime;
        QString effectiveArea;
    };

    explicit WhitelistPage(QWidget *parent = nullptr);

    void setRepository(WhitelistRepository *repository);
    void refreshRecords();

  protected:
    void showEvent(QShowEvent *event) override;

  private:
    void setupUi();
    void setupConnections();
    void reloadCurrentPage();
    void updatePaginationControls();
    void showCreateDialog();
    void showEditDialog(const WhitelistRecord &record);
    void showDeleteConfirmDialog(const WhitelistRecord &record);
    void showMessageDialog(const QString &message, const QString &title = QStringLiteral("提示"));
    QString displayEffectiveTime(const QString &value) const;
    QString displayEffectiveArea(const QString &value) const;
    WhitelistRecord recordAtRow(int row) const;

    WhitelistRepository *repository;
    QPushButton *addButton;
    QTableWidget *table;
    QLabel *pageInfoLabel;
    QPushButton *prevPageButton;
    QPushButton *nextPageButton;
    QComboBox *pageSizeCombo;

    QVector<WhitelistRecord> currentRecords;
    int totalRecords;
    int currentPage;
    int pageSize;
};

#endif // WHITELIST_PAGE_H
