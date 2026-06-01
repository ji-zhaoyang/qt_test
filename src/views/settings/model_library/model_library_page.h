#ifndef MODEL_LIBRARY_PAGE_H
#define MODEL_LIBRARY_PAGE_H

#include "network/core/protocol_types.h"

#include <QVector>
#include <QWidget>
#include <cstdint>

class QComboBox;
class QFrame;
class QGraphicsOpacityEffect;
class QLabel;
class QPropertyAnimation;
class QPushButton;
class QResizeEvent;
class QStackedLayout;
class QTimer;
class QVBoxLayout;
class QWidget;

class ModelLibraryEditDialog;

class ModelLibraryPage : public QWidget
{
    Q_OBJECT

  public:
    explicit ModelLibraryPage(QWidget *parent = nullptr);

  signals:
    void requestSaveModelLibraryMode(uint8_t mode);
    void requestQueryModelLibraryMode();
    void requestQueryModelLibraryRecords(int current, int size);
    void requestUpdateModelLibraryRecord(const ModelLibraryUpdateRequest &request);

  public slots:
    void updateModelLibraryMode(uint8_t mode);
    void updateModelLibraryRecords(const ModelLibraryPageResult &result);
    void showSaveResult(bool success, const QString &message);
    void showRecordSaveResult(bool success, const QString &message);

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private:
    void setupUi();
    QWidget *createHeaderRow(QWidget *parent) const;
    QWidget *createRecordRow(const ModelLibraryRecord &record, int rowIndex);
    void ensureToastWidget();
    void updateToastPosition();
    void showToastResult(bool success, const QString &message);
    void renderRecordList();
    void updatePaginationState();
    void queryPage(int page);
    void queryCurrentPage();
    void openEditDialog(int rowIndex);
    void submitDeleteRecord(int rowIndex);
    QString formatFreqBands(const QVector<ModelLibraryFreqBand> &freqBands) const;
    QString enableText(int enable) const;
    QString extractDisplayMessage(bool success, const QString &message) const;
    int indexFromMode(uint8_t mode) const;
    uint8_t modeFromIndex(int index) const;
    int totalPages() const;
    QString titleStyle() const;
    QString comboBoxStyle() const;
    QString primaryButtonStyle() const;
    QString headerTextStyle() const;
    QString emptyIconStyle() const;
    QString emptyTextStyle() const;

    QComboBox *libraryTypeComboBox;
    QPushButton *setButton;
    QPushButton *refreshButton;
    QPushButton *previousPageButton;
    QPushButton *nextPageButton;
    QLabel *recordCountLabel;
    QLabel *pageInfoLabel;
    QFrame *tableBodyFrame;
    QStackedLayout *tableContentStack;
    QWidget *recordListContainer;
    QVBoxLayout *recordListLayout;
    QLabel *emptyIconLabel;
    QLabel *emptyTextLabel;
    QWidget *emptyContainer;
    QWidget *toastWidget;
    QLabel *toastIconLabel;
    QLabel *toastTextLabel;
    QTimer *toastHideTimer;
    QGraphicsOpacityEffect *toastOpacityEffect;
    QPropertyAnimation *toastFadeInAnimation;
    QPropertyAnimation *toastFadeOutAnimation;
    ModelLibraryEditDialog *editDialog;
    QVector<ModelLibraryRecord> recordCache;
    int currentPage;
    int pageSize;
    int totalRecords;
    bool lastRecordOperationWasDelete;
};

#endif // MODEL_LIBRARY_PAGE_H
