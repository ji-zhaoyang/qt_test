#include "whitelist_page.h"
#include "whitelist_edit_dialog.h"
#include "repositories/whitelist_repository.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <functional>

namespace
{
QString orangeButtonStyle()
{
    return QStringLiteral("QPushButton { background-color: #f2994a; color: #ffffff; border: none; border-radius: 2px; "
                          "padding: 0 16px; font-size: 13px; font-weight: bold; min-height: 34px; }"
                          "QPushButton:hover { background-color: #f6a85f; }");
}

QString ghostButtonStyle()
{
    return QStringLiteral("QPushButton { background-color: #16181c; color: #ffffff; border: 1px solid #30333a; "
                          "border-radius: 2px; padding: 0 14px; font-size: 13px; min-height: 30px; }"
                          "QPushButton:hover { background-color: #21242a; }");
}

QString linkButtonStyle()
{
    return QStringLiteral("QPushButton { background: transparent; color: #4ea1ff; border: none; font-size: 13px; padding: 0 6px; }"
                          "QPushButton:hover { color: #7bb8ff; }");
}

class WhitelistConfirmOverlay : public QWidget
{
  public:
    explicit WhitelistConfirmOverlay(QWidget *parent)
        : QWidget(parent), titleLabel_(nullptr), messageLabel_(nullptr), errorLabel_(nullptr), cancelButton_(nullptr)
    {
        setAttribute(Qt::WA_StyledBackground, true);
        setAttribute(Qt::WA_DeleteOnClose, true);
        setStyleSheet(QStringLiteral("background-color: rgba(0, 0, 0, 120);"));

        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addStretch();

        QWidget *panel = new QWidget(this);
        panel->setAttribute(Qt::WA_StyledBackground, true);
        panel->setFixedSize(400, 168);
        panel->setStyleSheet(QStringLiteral(
            "background-color: #1b1d22; color: #f0f0f0; border: 1px solid #2b2f36; border-radius: 8px;"));

        QVBoxLayout *panelLayout = new QVBoxLayout(panel);
        panelLayout->setContentsMargins(24, 20, 24, 20);
        panelLayout->setSpacing(12);

        titleLabel_ = new QLabel(QStringLiteral("确认删除"), panel);
        titleLabel_->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 18px; font-weight: bold;"));
        panelLayout->addWidget(titleLabel_);

        messageLabel_ = new QLabel(panel);
        messageLabel_->setWordWrap(true);
        messageLabel_->setStyleSheet(QStringLiteral("color: #d6d7da; font-size: 14px;"));
        panelLayout->addWidget(messageLabel_);

        errorLabel_ = new QLabel(panel);
        errorLabel_->setStyleSheet(QStringLiteral("color: #ff7875; font-size: 12px;"));
        errorLabel_->hide();
        panelLayout->addWidget(errorLabel_);

        panelLayout->addStretch();

        QHBoxLayout *buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();

        cancelButton_ = new QPushButton(QStringLiteral("取消"), panel);
        cancelButton_->setStyleSheet(QStringLiteral("QPushButton { background-color: #1b1d22; color: #d6d7da; "
                                                    "border: 1px solid #3b3e46; border-radius: 2px; padding: 8px 18px; "
                                                    "font-size: 13px; min-width: 72px; }"
                                                    "QPushButton:hover { background-color: #252830; }"));
        QPushButton *confirmButton = new QPushButton(QStringLiteral("确定"), panel);
        confirmButton->setStyleSheet(QStringLiteral("QPushButton { background-color: #f2994a; color: #ffffff; "
                                                    "border: none; border-radius: 2px; padding: 8px 18px; "
                                                    "font-size: 13px; font-weight: bold; min-width: 72px; }"
                                                    "QPushButton:hover { background-color: #f6a85f; }"));
        buttonLayout->addWidget(cancelButton_);
        buttonLayout->addWidget(confirmButton);
        panelLayout->addLayout(buttonLayout);

        layout->addWidget(panel, 0, Qt::AlignCenter);
        layout->addStretch();

        connect(cancelButton_, &QPushButton::clicked, this, &QWidget::close);
        connect(confirmButton, &QPushButton::clicked, this,
                [this]()
                {
                    if (confirmHandler_)
                    {
                        confirmHandler_();
                    }
                });
    }

    void setConfirmHandler(const std::function<void()> &handler)
    {
        confirmHandler_ = handler;
    }

    void setTitle(const QString &title)
    {
        if (titleLabel_)
        {
            titleLabel_->setText(title);
        }
    }

    void setMessage(const QString &message)
    {
        if (messageLabel_)
        {
            messageLabel_->setText(message);
        }
    }

    void setAlertMode(bool alertOnly)
    {
        if (cancelButton_)
        {
            cancelButton_->setVisible(!alertOnly);
        }
    }

    void showError(const QString &message)
    {
        if (errorLabel_)
        {
            errorLabel_->setText(message);
            errorLabel_->show();
        }
    }

    void showOverlay()
    {
        if (parentWidget())
        {
            setGeometry(parentWidget()->rect());
            raise();
        }
        show();
    }

  protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        if (parentWidget())
        {
            setGeometry(parentWidget()->rect());
        }
    }

  private:
    QLabel *titleLabel_;
    QLabel *messageLabel_;
    QLabel *errorLabel_;
    QPushButton *cancelButton_;
    std::function<void()> confirmHandler_;
};
} // namespace

WhitelistPage::WhitelistPage(QWidget *parent)
    : QWidget(parent), repository(nullptr), addButton(nullptr), table(nullptr),
      pageInfoLabel(nullptr), prevPageButton(nullptr), nextPageButton(nullptr), pageSizeCombo(nullptr), totalRecords(0),
      currentPage(1), pageSize(10)
{
    setStyleSheet(QStringLiteral("background-color: #111214;"));
    setupUi();
    setupConnections();
}

void WhitelistPage::setRepository(WhitelistRepository *whitelistRepository)
{
    if (repository == whitelistRepository)
    {
        return;
    }

    if (repository)
    {
        disconnect(repository, nullptr, this, nullptr);
    }

    repository = whitelistRepository;
    if (repository)
    {
        connect(repository, &WhitelistRepository::changed, this, &WhitelistPage::refreshRecords);
    }

    refreshRecords();
}

void WhitelistPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshRecords();
}

void WhitelistPage::setupUi()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    addButton = new QPushButton(QStringLiteral("+ 新增"), this);
    addButton->setStyleSheet(orangeButtonStyle());
    addButton->setFixedHeight(34);
    toolbarLayout->addWidget(addButton);
    toolbarLayout->addStretch();
    layout->addLayout(toolbarLayout);

    QFrame *tableFrame = new QFrame(this);
    tableFrame->setStyleSheet(QStringLiteral("QFrame { background-color: #111214; border: 1px solid #2b2f36; }"));
    QVBoxLayout *tableFrameLayout = new QVBoxLayout(tableFrame);
    tableFrameLayout->setContentsMargins(0, 0, 0, 0);

    table = new QTableWidget(tableFrame);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels(
        {QStringLiteral("序号"), QStringLiteral("无人机序列号"), QStringLiteral("有效时间"), QStringLiteral("有效区域"),
         QStringLiteral("备注"), QStringLiteral("操作")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setShowGrid(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    table->horizontalHeader()->setStyleSheet(
        QStringLiteral("QHeaderView::section { background-color: #1b1d22; color: #9aa0a6; padding: 10px 12px; "
                       "border: none; border-bottom: 1px solid #2b2f36; font-size: 13px; }"));
    table->setStyleSheet(
        QStringLiteral("QTableWidget { background-color: #111214; color: #f0f0f0; border: none; font-size: 13px; gridline-color: #1f2329; "
                       "selection-background-color: #252830; selection-color: #f0f0f0; outline: none; }"
                       "QTableWidget::item { padding: 8px 12px; border-bottom: 1px solid #26292f; }"
                       "QTableWidget::item:selected { background-color: #252830; color: #f0f0f0; }"));
    table->setColumnWidth(0, 70);
    table->setColumnWidth(1, 220);
    table->setColumnWidth(2, 120);
    table->setColumnWidth(3, 120);
    table->setColumnWidth(4, 160);
    tableFrameLayout->addWidget(table);
    layout->addWidget(tableFrame, 1);

    QHBoxLayout *paginationLayout = new QHBoxLayout();
    pageInfoLabel = new QLabel(QStringLiteral("共 0 条"), this);
    pageInfoLabel->setStyleSheet(QStringLiteral("color: #c4c7cc; font-size: 13px;"));
    prevPageButton = new QPushButton(QStringLiteral("上一页"), this);
    nextPageButton = new QPushButton(QStringLiteral("下一页"), this);
    prevPageButton->setStyleSheet(ghostButtonStyle());
    nextPageButton->setStyleSheet(ghostButtonStyle());
    pageSizeCombo = new QComboBox(this);
    pageSizeCombo->addItem(QStringLiteral("10 条/页"), 10);
    pageSizeCombo->addItem(QStringLiteral("20 条/页"), 20);
    pageSizeCombo->addItem(QStringLiteral("50 条/页"), 50);
    pageSizeCombo->setStyleSheet(
        QStringLiteral("QComboBox { background-color: #16181c; color: #ffffff; border: 1px solid #30333a; "
                       "border-radius: 2px; padding: 4px 8px; min-width: 96px; }"));

    paginationLayout->addWidget(pageInfoLabel);
    paginationLayout->addStretch();
    paginationLayout->addWidget(prevPageButton);
    paginationLayout->addWidget(nextPageButton);
    paginationLayout->addWidget(pageSizeCombo);
    layout->addLayout(paginationLayout);
}

void WhitelistPage::showDeleteConfirmDialog(const WhitelistRecord &record)
{
    if (!repository)
    {
        return;
    }

    QWidget *overlayParent = window();
    if (!overlayParent)
    {
        overlayParent = this;
    }

    WhitelistConfirmOverlay *overlay = new WhitelistConfirmOverlay(overlayParent);
    overlay->setTitle(QStringLiteral("确认删除"));
    overlay->setMessage(QStringLiteral("确定删除白名单「%1」吗？").arg(record.serialNumber));
    overlay->setConfirmHandler(
        [this, overlay, record]()
        {
            if (!repository)
            {
                overlay->close();
                return;
            }

            if (!repository->removeById(record.id))
            {
                overlay->showError(QStringLiteral("删除失败，请重试"));
                return;
            }

            overlay->close();
            refreshRecords();
        });
    overlay->showOverlay();
}

void WhitelistPage::showMessageDialog(const QString &message, const QString &title)
{
    QWidget *overlayParent = window();
    if (!overlayParent)
    {
        overlayParent = this;
    }

    WhitelistConfirmOverlay *overlay = new WhitelistConfirmOverlay(overlayParent);
    overlay->setTitle(title);
    overlay->setMessage(message);
    overlay->setAlertMode(true);
    overlay->setConfirmHandler([overlay]() { overlay->close(); });
    overlay->showOverlay();
}

void WhitelistPage::showCreateDialog()
{
    if (!repository)
    {
        return;
    }

    QWidget *overlayParent = window();
    if (!overlayParent)
    {
        overlayParent = this;
    }

    WhitelistEditDialog *dialog = new WhitelistEditDialog(overlayParent);
    dialog->setCreateMode();
    connect(dialog, &WhitelistEditDialog::confirmed, this,
            [this](const WhitelistRecord &record)
            {
                if (!repository)
                {
                    return;
                }

                if (!repository->insertRecord(record))
                {
                    showMessageDialog(repository->lastError().trimmed().isEmpty()
                                          ? QStringLiteral("新增失败，序列号可能已存在")
                                          : repository->lastError());
                    return;
                }

                currentPage = 1;
                reloadCurrentPage();
            });
    dialog->showOverlay();
}

void WhitelistPage::showEditDialog(const WhitelistRecord &record)
{
    if (!repository)
    {
        return;
    }

    QWidget *overlayParent = window();
    if (!overlayParent)
    {
        overlayParent = this;
    }

    WhitelistEditDialog *dialog = new WhitelistEditDialog(overlayParent);
    dialog->setEditMode(record);
    connect(dialog, &WhitelistEditDialog::confirmed, this,
            [this](const WhitelistRecord &editedRecord)
            {
                if (!repository)
                {
                    return;
                }

                if (!repository->updateRecord(editedRecord))
                {
                    showMessageDialog(repository->lastError().trimmed().isEmpty() ? QStringLiteral("保存失败")
                                                                                  : repository->lastError());
                    return;
                }

                reloadCurrentPage();
            });
    dialog->showOverlay();
}

void WhitelistPage::setupConnections()
{
    connect(addButton, &QPushButton::clicked, this, &WhitelistPage::showCreateDialog);

    connect(prevPageButton, &QPushButton::clicked, this,
            [this]()
            {
                if (currentPage > 1)
                {
                    --currentPage;
                    reloadCurrentPage();
                }
            });

    connect(nextPageButton, &QPushButton::clicked, this,
            [this]()
            {
                const int totalPages = qMax(1, (totalRecords + pageSize - 1) / pageSize);
                if (currentPage < totalPages)
                {
                    ++currentPage;
                    reloadCurrentPage();
                }
            });

    connect(pageSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int)
            {
                pageSize = pageSizeCombo->currentData().toInt();
                if (pageSize <= 0)
                {
                    pageSize = 10;
                }
                currentPage = 1;
                reloadCurrentPage();
            });
}

void WhitelistPage::refreshRecords()
{
    if (!repository)
    {
        currentRecords.clear();
        totalRecords = 0;
        table->setRowCount(0);
        updatePaginationControls();
        return;
    }

    totalRecords = repository->countAll();
    const int totalPages = qMax(1, (totalRecords + pageSize - 1) / pageSize);
    if (currentPage > totalPages)
    {
        currentPage = totalPages;
    }
    reloadCurrentPage();
}

void WhitelistPage::reloadCurrentPage()
{
    if (!repository)
    {
        return;
    }

    currentRecords = repository->queryPage(currentPage, pageSize);
    table->clearSelection();
    table->setRowCount(currentRecords.size());

    for (int row = 0; row < currentRecords.size(); ++row)
    {
        const WhitelistRecord &record = currentRecords.at(row);
        const int displayIndex = (currentPage - 1) * pageSize + row + 1;

        table->setItem(row, 0, new QTableWidgetItem(QString::number(displayIndex)));
        table->setItem(row, 1, new QTableWidgetItem(record.serialNumber));
        table->setItem(row, 2, new QTableWidgetItem(displayEffectiveTime(record.effectiveTime)));
        table->setItem(row, 3, new QTableWidgetItem(displayEffectiveArea(record.effectiveArea)));
        table->setItem(row, 4, new QTableWidgetItem(record.remarks));

        QWidget *actionWidget = new QWidget(table);
        actionWidget->setAutoFillBackground(false);
        actionWidget->setStyleSheet(QStringLiteral("background: transparent;"));
        QHBoxLayout *actionLayout = new QHBoxLayout(actionWidget);
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->setSpacing(4);

        QPushButton *editButton = new QPushButton(QStringLiteral("编辑"), actionWidget);
        QPushButton *deleteButton = new QPushButton(QStringLiteral("删除"), actionWidget);
        editButton->setStyleSheet(linkButtonStyle());
        deleteButton->setStyleSheet(linkButtonStyle());
        actionLayout->addWidget(editButton);
        actionLayout->addWidget(deleteButton);
        actionLayout->addStretch();
        table->setCellWidget(row, 5, actionWidget);

        connect(editButton, &QPushButton::clicked, this,
                [this, row]()
                {
                    if (!repository || row < 0 || row >= currentRecords.size())
                    {
                        return;
                    }

                    showEditDialog(currentRecords.at(row));
                });

        connect(deleteButton, &QPushButton::clicked, this,
                [this, row]()
                {
                    if (!repository || row < 0 || row >= currentRecords.size())
                    {
                        return;
                    }

                    const WhitelistRecord record = currentRecords.at(row);
                    showDeleteConfirmDialog(record);
                });
    }

    updatePaginationControls();
}

void WhitelistPage::updatePaginationControls()
{
    pageInfoLabel->setText(QStringLiteral("共 %1 条").arg(totalRecords));
    const int totalPages = qMax(1, (totalRecords + pageSize - 1) / pageSize);
    prevPageButton->setEnabled(currentPage > 1);
    nextPageButton->setEnabled(currentPage < totalPages);
}

QString WhitelistPage::displayEffectiveTime(const QString &value) const
{
    if (value == QStringLiteral("permanent"))
    {
        return QStringLiteral("永久有效");
    }

    const QStringList parts = value.split(QLatin1Char('|'));
    if (parts.size() >= 3 && parts.at(0) == QStringLiteral("range"))
    {
        const QDateTime start = QDateTime::fromString(parts.at(1), Qt::ISODate);
        const QDateTime end = QDateTime::fromString(parts.at(2), Qt::ISODate);
        if (start.isValid() && end.isValid())
        {
            return QStringLiteral("%1 ~ %2")
                .arg(start.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                     end.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")));
        }
        return QStringLiteral("%1 ~ %2").arg(parts.at(1), parts.at(2));
    }

    return value;
}

QString WhitelistPage::displayEffectiveArea(const QString &value) const
{
    if (value == QStringLiteral("unlimited"))
    {
        return QStringLiteral("不限制");
    }

    if (value.trimmed().startsWith(QLatin1Char('{')) || value.trimmed().startsWith(QLatin1Char('[')))
    {
        return QStringLiteral("自定义区域");
    }

    return value;
}

WhitelistPage::WhitelistRecord WhitelistPage::recordAtRow(int row) const
{
    if (row >= 0 && row < currentRecords.size())
    {
        return currentRecords.at(row);
    }
    return WhitelistRecord();
}
