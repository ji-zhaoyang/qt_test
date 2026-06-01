#ifndef DATA_COLLECTION_PAGE_H
#define DATA_COLLECTION_PAGE_H

#include <QWidget>

class QButtonGroup;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QVBoxLayout;

class DataCollectionPage : public QWidget
{
    Q_OBJECT

  public:
    explicit DataCollectionPage(QWidget *parent = nullptr);

  private:
    void setupUi();
    QWidget *createSeparatorLine(QWidget *parent) const;
    QWidget *createFormRow(QWidget *parent, const QString &labelText, QWidget *fieldWidget) const;
    QWidget *createModeRow(QWidget *parent);
    QWidget *createTimeRow(QWidget *parent);
    QWidget *createFilesSection(QWidget *parent);
    QString titleStyle() const;
    QString rowLabelStyle() const;
    QString inputStyle() const;
    QString actionButtonStyle() const;
    QString secondaryButtonStyle() const;
    QString listTitleStyle() const;
    QString emptyStateTextStyle() const;

    QLineEdit *fileNameEdit;
    QRadioButton *collectRadio;
    QRadioButton *realTimeRadio;
    QButtonGroup *modeGroup;
    QSpinBox *collectionTimeSpinBox;
    QLineEdit *channelEdit;
    QPushButton *startButton;
    QPushButton *clearButton;
    QListWidget *fileListWidget;
    QLabel *emptyStateIconLabel;
    QLabel *emptyStateTextLabel;
};

#endif // DATA_COLLECTION_PAGE_H
