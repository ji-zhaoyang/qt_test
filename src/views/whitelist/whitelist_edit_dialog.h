#ifndef WHITELIST_EDIT_DIALOG_H
#define WHITELIST_EDIT_DIALOG_H

#include "whitelist_page.h"

#include <QWidget>

class DateTimePickerPopup;
class QCheckBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QPushButton;
class QResizeEvent;
class QCloseEvent;
class QHBoxLayout;
class QVBoxLayout;
class QWebEngineView;

class WhitelistEditDialog : public QWidget
{
    Q_OBJECT

  public:
    explicit WhitelistEditDialog(QWidget *parent = nullptr);

    void setCreateMode();
    void setEditMode(const WhitelistPage::WhitelistRecord &record);
    WhitelistPage::WhitelistRecord record() const;
    void showOverlay();

  signals:
    void confirmed(const WhitelistPage::WhitelistRecord &record);
    void cancelled();

  protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

  private:
    void setupUi();
    void updatePanelGeometry();
    void schedulePanelGeometryUpdate();
    void ensureMapWebView();
    void suspendMapWebView();
    void resumeMapWebView();
    void attachMapColumn();
    void detachMapColumn();
    void applyAreaMode(bool unlimited);
    void syncTimeSectionVisibility(bool permanent);
    void syncAreaSectionVisibility(bool unlimited);
    void applyEffectiveTime(const QString &value);
    void applyEffectiveArea(const QString &value);
    QString buildEffectiveTime() const;
    QString buildEffectiveArea() const;
    void loadAreaShapeToMap();
    void tryConfirm();
    void finishConfirm();
    QDateTimeEdit *createDateTimeEdit(const QString &placeholderText);

    bool editing;
    int editingId;
    bool mapReady_;
    QString currentAreaShapeJson_;
    QWidget *panelWidget_;
    QWidget *contentRow_;
    QHBoxLayout *contentLayout_;
    QWidget *formColumn_;
    QWidget *mapColumn_;
    QWidget *dateRow_;
    QLabel *titleLabel_;
    QLabel *hintLabel_;
    QLineEdit *serialEdit_;
    QLineEdit *remarksEdit_;
    QCheckBox *permanentCheck_;
    QCheckBox *unlimitedAreaCheck_;
    QDateTimeEdit *startTimeEdit_;
    QDateTimeEdit *endTimeEdit_;
    DateTimePickerPopup *timePicker_;
    QVBoxLayout *mapLayout_;
    QWebEngineView *mapWebView_;
    QPushButton *cancelButton_;
    QPushButton *confirmButton_;
};

#endif // WHITELIST_EDIT_DIALOG_H
