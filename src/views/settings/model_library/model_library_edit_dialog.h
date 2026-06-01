#ifndef MODEL_LIBRARY_EDIT_DIALOG_H
#define MODEL_LIBRARY_EDIT_DIALOG_H

#include "network/core/protocol_types.h"

#include <QDialog>
#include <QVector>

class QComboBox;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QVBoxLayout;
class QWidget;

class ModelLibraryEditDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit ModelLibraryEditDialog(QWidget *parent = nullptr);

    void clearForCreate();
    void setRecord(const ModelLibraryRecord &record);
    ModelLibraryRecord record() const;
    ModelLibraryUpdateRequest buildUpdateRequest(int deleteFlag = 0) const;

  signals:
    void saveRequested(const ModelLibraryUpdateRequest &request);

  private:
    struct FreqBandRow
    {
        QWidget *rowWidget = nullptr;
        QLineEdit *startEdit = nullptr;
        QLineEdit *endEdit = nullptr;
        QPushButton *removeButton = nullptr;
    };

    void setupUi();
    QWidget *createFieldRow(const QString &labelText, QWidget *editor) const;
    void addFreqBandRow(const ModelLibraryFreqBand &band = ModelLibraryFreqBand());
    void removeFreqBandRow(QWidget *rowWidget);
    void clearFreqBandRows();
    void refreshFreqBandRowState();
    void handleConfirmClicked();

    int currentType;
    QLineEdit *nameEdit;
    QSpinBox *sensitivitySpinBox;
    QComboBox *enableComboBox;
    QScrollArea *freqBandScrollArea;
    QWidget *freqBandListWidget;
    QVBoxLayout *freqBandListLayout;
    QPushButton *addFreqBandButton;
    QPushButton *cancelButton;
    QPushButton *confirmButton;
    QVector<FreqBandRow> freqBandRows;
};

#endif // MODEL_LIBRARY_EDIT_DIALOG_H
