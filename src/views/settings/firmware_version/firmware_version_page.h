#ifndef FIRMWARE_VERSION_PAGE_H
#define FIRMWARE_VERSION_PAGE_H

#include <QMap>
#include <QWidget>

class QLabel;
class QVBoxLayout;

class FirmwareVersionPage : public QWidget
{
    Q_OBJECT

  public:
    explicit FirmwareVersionPage(QWidget *parent = nullptr);

  public slots:
    void updateFirmwareVersions(const QString &appVersion, const QString &fpgaVersion, const QString &gpuVersion);
    void updateDeviceSerial(const QString &serialText);

  private:
    void setupUi();
    void addInfoRow(QVBoxLayout *cardLayout, QWidget *cardWidget, const QString &labelText, const QString &valueText);
    QWidget *createSeparatorLine(QWidget *parent) const;
    QString titleStyle() const;
    QString rowLabelStyle() const;
    QString rowValueStyle() const;
    QString buttonStyle() const;

    QMap<QString, QLabel *> valueLabels;
};

#endif // FIRMWARE_VERSION_PAGE_H
