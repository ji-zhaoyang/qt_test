#ifndef DATA_COLLECTION_PAGE_H
#define DATA_COLLECTION_PAGE_H

#include "network/core/protocol_types.h"
#include <QWidget>

class QButtonGroup;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSpinBox;

class DataCollectionPage : public QWidget
{
    Q_OBJECT

  public:
    explicit DataCollectionPage(QWidget *parent = nullptr);

  signals:
    void requestUploadPatternFile(const PatternUploadRequest &request);

  public slots:
    void showPatternUploadResult(bool success, const QString &message);

  private:
    void setupUi();
    void updateModeFieldVisibility();
    void refreshCurrentFileList();
    void updateFilesSectionForMode();
    int currentModeType() const;
    QString currentModeDirectoryPath() const;
    QWidget *createSeparatorLine(QWidget *parent) const;
    QWidget *createFormRow(QWidget *parent, const QString &labelText, QWidget *fieldWidget) const;
    QWidget *createModeRow(QWidget *parent);
    QWidget *createFilesSection(QWidget *parent);
    void updateFileListVisibility();
    void showStatusMessage(bool success, const QString &message);
    QString extractDisplayMessage(const QString &message) const;
    QString titleStyle() const;
    QString rowLabelStyle() const;
    QString inputStyle() const;
    QString actionButtonStyle() const;
    QString secondaryButtonStyle() const;
    QString listTitleStyle() const;
    QString emptyStateTextStyle() const;

    QRadioButton *collectRadio;
    QRadioButton *realTimeRadio;
    QButtonGroup *modeGroup;
    QLineEdit *fileNameEdit;
    QWidget *collectionTimeRow;
    QWidget *collectionTimeSeparator;
    QSpinBox *collectionTimeSpinBox;
    QWidget *channelRow;
    QWidget *channelSeparator;
    QSpinBox *channelSpinBox;
    QPushButton *startButton;
    QPushButton *clearButton;
    QLabel *filesTitleLabel;
    QListWidget *fileListWidget;
    QLabel *emptyStateIconLabel;
    QLabel *emptyStateTextLabel;
    QLabel *statusLabel;
};

#endif // DATA_COLLECTION_PAGE_H
