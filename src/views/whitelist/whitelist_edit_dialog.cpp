#include "whitelist_edit_dialog.h"

#include "components/datetime_picker_popup.h"

#include <QAbstractSpinBox>
#include <QCheckBox>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDate>
#include <QDateTimeEdit>
#include <QDebug>
#include <QDir>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <QVBoxLayout>
#include <QWebEnginePage>
#include <QWebEngineView>

namespace
{
QString primaryButtonStyle()
{
    return QStringLiteral("QPushButton { background-color: #f2994a; color: #ffffff; border: none; border-radius: 2px; "
                          "padding: 8px 18px; font-size: 13px; font-weight: bold; }"
                          "QPushButton:hover { background-color: #f6a85f; }");
}

QString secondaryButtonStyle()
{
    return QStringLiteral("QPushButton { background-color: #1b1d22; color: #d6d7da; border: 1px solid #3b3e46; "
                          "border-radius: 2px; padding: 8px 18px; font-size: 13px; }"
                          "QPushButton:hover { background-color: #252830; }");
}

QString inputStyle()
{
    return QStringLiteral("background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; border-radius: 2px; "
                          "padding: 8px 10px; font-size: 13px;");
}

QString flatInputStyle()
{
    return QStringLiteral("background-color: #101113; color: #ffffff; border: 1px solid #2d2d2d; border-radius: 2px; "
                          "padding: 0 10px; font-size: 13px;");
}

QString checkBoxStyle()
{
    return QStringLiteral("QCheckBox { color: #d6d7da; font-size: 13px; spacing: 8px; }"
                          "QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #555; border-radius: 2px; "
                          "background: #101113; }"
                          "QCheckBox::indicator:checked { background: #f2994a; border-color: #f2994a; }");
}

QString dateTimeEditStyle()
{
    return QStringLiteral("QDateTimeEdit { %1 }"
                          "QDateTimeEdit::drop-down { subcontrol-origin: padding; subcontrol-position: top right; "
                          "width: 28px; border-left: 1px solid #2a2a2a; }"
                          "QDateTimeEdit::down-arrow { image: none; }")
        .arg(flatInputStyle());
}

QLabel *makeFieldLabel(const QString &text, bool required, QWidget *parent)
{
    QLabel *label = new QLabel(parent);
    if (required)
    {
        label->setText(QStringLiteral("<span style='color:#ff4d4f;'>*</span> %1").arg(text));
        label->setTextFormat(Qt::RichText);
    }
    else
    {
        label->setText(text);
    }
    label->setStyleSheet(QStringLiteral("font-size: 13px; color: #d6d7da;"));
    label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    return label;
}

bool isUnsetDateTimeEdit(const QDateTimeEdit *edit)
{
    return edit && edit->dateTime() == edit->minimumDateTime();
}
} // namespace

WhitelistEditDialog::WhitelistEditDialog(QWidget *parent)
    : QWidget(parent), editing(false), editingId(0), mapReady_(false), panelWidget_(nullptr), contentRow_(nullptr),
      contentLayout_(nullptr), formColumn_(nullptr), mapColumn_(nullptr), dateRow_(nullptr), titleLabel_(nullptr), hintLabel_(nullptr),
      serialEdit_(nullptr), remarksEdit_(nullptr), permanentCheck_(nullptr), unlimitedAreaCheck_(nullptr),
      startTimeEdit_(nullptr), endTimeEdit_(nullptr), timePicker_(nullptr), mapLayout_(nullptr), mapWebView_(nullptr),
      cancelButton_(nullptr), confirmButton_(nullptr)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAttribute(Qt::WA_DeleteOnClose, true);
    hide();
    setStyleSheet(QStringLiteral("QWidget { background-color: rgba(0, 0, 0, 120); }"));
    setupUi();
    setCreateMode();
}

QDateTimeEdit *WhitelistEditDialog::createDateTimeEdit(const QString &placeholderText)
{
    QDateTimeEdit *edit = new QDateTimeEdit(dateRow_);
    edit->setDisplayFormat(QStringLiteral("yyyy-MM-dd"));
    edit->setCalendarPopup(false);
    edit->setButtonSymbols(QAbstractSpinBox::NoButtons);
    edit->setKeyboardTracking(false);
    edit->setMinimumDateTime(QDateTime(QDate(2000, 1, 1), QTime(0, 0, 0)));
    edit->setSpecialValueText(placeholderText);
    edit->setDateTime(edit->minimumDateTime());
    edit->setFixedHeight(32);
    edit->setStyleSheet(dateTimeEditStyle());
    edit->setReadOnly(true);
    return edit;
}

void WhitelistEditDialog::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addStretch();

    panelWidget_ = new QWidget(this);
    panelWidget_->setObjectName(QStringLiteral("whitelistEditPanel"));
    panelWidget_->setAttribute(Qt::WA_StyledBackground, true);
    panelWidget_->setStyleSheet(QStringLiteral(
        "QWidget#whitelistEditPanel { background-color: #1b1d22; color: #f0f0f0; border: 1px solid #2b2f36; "
        "border-radius: 8px; }"));
    mainLayout->addWidget(panelWidget_, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    QVBoxLayout *layout = new QVBoxLayout(panelWidget_);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(16);

    QWidget *titleBar = new QWidget(panelWidget_);
    QHBoxLayout *titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(0, 0, 0, 0);
    titleBarLayout->setSpacing(8);

    titleLabel_ = new QLabel(titleBar);
    titleLabel_->setStyleSheet(QStringLiteral("color: #ffffff; font-size: 18px; font-weight: bold;"));
    titleBarLayout->addWidget(titleLabel_);
    titleBarLayout->addStretch();

    QPushButton *closeButton = new QPushButton(QStringLiteral("×"), titleBar);
    closeButton->setFixedSize(28, 28);
    closeButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: transparent; color: #c7cbd3; border: none; font-size: 18px; font-weight: bold; }"
        "QPushButton:hover { color: #ffffff; }"));
    connect(closeButton, &QPushButton::clicked, this,
            [this]()
            {
                if (timePicker_)
                {
                    timePicker_->hidePopup();
                }
                emit cancelled();
                close();
            });
    titleBarLayout->addWidget(closeButton);
    layout->addWidget(titleBar);

    contentRow_ = new QWidget(panelWidget_);
    contentLayout_ = new QHBoxLayout(contentRow_);
    contentLayout_->setContentsMargins(0, 0, 0, 0);
    contentLayout_->setSpacing(18);

    formColumn_ = new QWidget(contentRow_);
    formColumn_->setMinimumWidth(360);
    QGridLayout *formLayout = new QGridLayout(formColumn_);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setHorizontalSpacing(12);
    formLayout->setVerticalSpacing(14);
    formLayout->setColumnStretch(1, 1);

    serialEdit_ = new QLineEdit(formColumn_);
    serialEdit_->setStyleSheet(inputStyle());
    serialEdit_->setPlaceholderText(QStringLiteral("请输入"));
    remarksEdit_ = new QLineEdit(formColumn_);
    remarksEdit_->setStyleSheet(inputStyle());
    remarksEdit_->setPlaceholderText(QStringLiteral("请输入"));

    permanentCheck_ = new QCheckBox(QStringLiteral("永久有效"), formColumn_);
    permanentCheck_->setStyleSheet(checkBoxStyle());
    permanentCheck_->setChecked(true);

    dateRow_ = new QWidget(formColumn_);
    QHBoxLayout *dateLayout = new QHBoxLayout(dateRow_);
    dateLayout->setContentsMargins(0, 0, 0, 0);
    dateLayout->setSpacing(8);
    startTimeEdit_ = createDateTimeEdit(QStringLiteral("开始日期"));
    endTimeEdit_ = createDateTimeEdit(QStringLiteral("结束日期"));
    startTimeEdit_->setFixedWidth(142);
    endTimeEdit_->setFixedWidth(142);
    QLabel *dateSeparator = new QLabel(QStringLiteral("~"), dateRow_);
    dateSeparator->setStyleSheet(QStringLiteral("color: #9da3ad; font-size: 13px;"));
    dateLayout->addWidget(startTimeEdit_);
    dateLayout->addWidget(dateSeparator);
    dateLayout->addWidget(endTimeEdit_);
    dateLayout->addStretch();

    unlimitedAreaCheck_ = new QCheckBox(QStringLiteral("不限制"), formColumn_);
    unlimitedAreaCheck_->setStyleSheet(checkBoxStyle());
    unlimitedAreaCheck_->setChecked(true);

    formLayout->addWidget(makeFieldLabel(QStringLiteral("无人机序列号:"), true, formColumn_), 0, 0);
    formLayout->addWidget(serialEdit_, 0, 1);
    formLayout->addWidget(makeFieldLabel(QStringLiteral("备注:"), false, formColumn_), 1, 0);
    formLayout->addWidget(remarksEdit_, 1, 1);
    formLayout->addWidget(makeFieldLabel(QStringLiteral("有效时间:"), true, formColumn_), 2, 0);
    formLayout->addWidget(permanentCheck_, 2, 1);
    formLayout->addWidget(new QWidget(formColumn_), 3, 0);
    formLayout->addWidget(dateRow_, 3, 1);
    formLayout->addWidget(makeFieldLabel(QStringLiteral("有效区域:"), true, formColumn_), 4, 0);
    formLayout->addWidget(unlimitedAreaCheck_, 4, 1);
    formLayout->setRowStretch(5, 1);

    mapColumn_ = new QWidget(contentRow_);
    mapColumn_->setMinimumWidth(420);
    mapLayout_ = new QVBoxLayout(mapColumn_);
    mapLayout_->setContentsMargins(0, 0, 0, 0);

    contentLayout_->addWidget(formColumn_, 0);
    contentLayout_->addWidget(mapColumn_, 1);
    layout->addWidget(contentRow_, 1);

    hintLabel_ = new QLabel(panelWidget_);
    hintLabel_->setStyleSheet(QStringLiteral("color: #f2994a; font-size: 12px;"));
    hintLabel_->hide();
    layout->addWidget(hintLabel_);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    cancelButton_ = new QPushButton(QStringLiteral("取消"), panelWidget_);
    cancelButton_->setStyleSheet(secondaryButtonStyle());
    confirmButton_ = new QPushButton(QStringLiteral("确定"), panelWidget_);
    confirmButton_->setStyleSheet(primaryButtonStyle());
    buttonLayout->addWidget(cancelButton_);
    buttonLayout->addWidget(confirmButton_);
    layout->addLayout(buttonLayout);

    timePicker_ = new DateTimePickerPopup(this, this);
    timePicker_->registerDateTimeEdit(startTimeEdit_);
    timePicker_->registerDateTimeEdit(endTimeEdit_);

    connect(cancelButton_, &QPushButton::clicked, this,
            [this]()
            {
                if (timePicker_)
                {
                    timePicker_->hidePopup();
                }
                emit cancelled();
                close();
            });
    connect(confirmButton_, &QPushButton::clicked, this, &WhitelistEditDialog::tryConfirm);
    connect(permanentCheck_, &QCheckBox::toggled, this,
            [this](bool permanent)
            {
                syncTimeSectionVisibility(permanent);
                if (!permanent)
                {
                    const QDateTime now = QDateTime::currentDateTime();
                    if (isUnsetDateTimeEdit(startTimeEdit_))
                    {
                        startTimeEdit_->setDateTime(QDateTime(now.date(), QTime(0, 0, 0)));
                    }
                    if (isUnsetDateTimeEdit(endTimeEdit_))
                    {
                        endTimeEdit_->setDateTime(QDateTime(now.date().addDays(30), QTime(23, 59, 59)));
                    }
                }
                else if (timePicker_)
                {
                    timePicker_->hidePopup();
                }
                updatePanelGeometry();
            });
    connect(unlimitedAreaCheck_, &QCheckBox::toggled, this,
            [this](bool unlimited)
            {
                if (timePicker_)
                {
                    timePicker_->hidePopup();
                }
                if (unlimited)
                {
                    currentAreaShapeJson_.clear();
                }

                QTimer::singleShot(0, this,
                                   [this, unlimited]()
                                   {
                                       applyAreaMode(unlimited);
                                   });
            });

    syncTimeSectionVisibility(true);
    syncAreaSectionVisibility(true);
}

void WhitelistEditDialog::ensureMapWebView()
{
    if (mapWebView_ || !mapLayout_)
    {
        return;
    }

    qDebug() << "[WhitelistEdit] lazy-create map WebEngineView";
    mapWebView_ = new QWebEngineView(mapColumn_);
    mapWebView_->setFocusPolicy(Qt::NoFocus);
    mapWebView_->setStyleSheet(
        QStringLiteral("background-color: #eef1f5; border: 1px solid #2b2f36; border-radius: 4px;"));
    mapLayout_->addWidget(mapWebView_);

    const QString webPath =
        QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/assets/web/whitelist_area.html"));
    qDebug() << "[WhitelistEdit] loading map:" << webPath;
    mapWebView_->load(QUrl::fromLocalFile(webPath));
    connect(mapWebView_, &QWebEngineView::loadFinished, this,
            [this](bool ok)
            {
                if (!mapWebView_)
                {
                    return;
                }

                mapReady_ = ok;
                qDebug() << "[WhitelistEdit] map loadFinished ok=" << ok;
                if (ok)
                {
                    loadAreaShapeToMap();
                    mapWebView_->page()->runJavaScript(
                        QStringLiteral("if (typeof map !== 'undefined' && map.invalidateSize) { map.invalidateSize(); }"));
                }
            });
    connect(mapWebView_->page(), &QWebEnginePage::titleChanged, this,
            [this](const QString &title)
            {
                if (!mapWebView_ || !title.startsWith(QStringLiteral("AREA:")))
                {
                    return;
                }

                const QString payload = title.mid(5);
                currentAreaShapeJson_ = payload == QStringLiteral("null") ? QString() : payload;
            });
}

void WhitelistEditDialog::suspendMapWebView()
{
    if (!mapWebView_)
    {
        return;
    }

    qDebug() << "[WhitelistEdit] suspend map WebEngineView";
    mapWebView_->clearFocus();
    mapWebView_->setEnabled(false);
    mapWebView_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

void WhitelistEditDialog::resumeMapWebView()
{
    if (!mapWebView_)
    {
        return;
    }

    qDebug() << "[WhitelistEdit] resume map WebEngineView";
    mapWebView_->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    mapWebView_->setEnabled(true);
    mapWebView_->show();
    if (mapReady_ && mapWebView_->page())
    {
        mapWebView_->page()->runJavaScript(
            QStringLiteral("setTimeout(function(){ if (typeof map !== 'undefined' && map.invalidateSize) map.invalidateSize(); }, 50);"));
    }
}

void WhitelistEditDialog::attachMapColumn()
{
    if (!contentLayout_ || !mapColumn_)
    {
        return;
    }

    if (contentLayout_->indexOf(mapColumn_) >= 0)
    {
        mapColumn_->setMinimumWidth(420);
        mapColumn_->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        mapColumn_->setEnabled(true);
        mapColumn_->show();
        return;
    }

    mapColumn_->setMinimumWidth(420);
    mapColumn_->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    contentLayout_->addWidget(mapColumn_, 1);
    mapColumn_->setEnabled(true);
    mapColumn_->show();
}

void WhitelistEditDialog::detachMapColumn()
{
    if (!contentLayout_ || !mapColumn_)
    {
        return;
    }

    contentLayout_->removeWidget(mapColumn_);
    mapColumn_->setEnabled(false);
    mapColumn_->setMinimumSize(0, 0);
    mapColumn_->setMaximumSize(0, 0);
    mapColumn_->hide();
}

void WhitelistEditDialog::applyAreaMode(bool unlimited)
{
    syncAreaSectionVisibility(unlimited);
    updatePanelGeometry();
    if (panelWidget_)
    {
        panelWidget_->raise();
    }
    raise();
    if (window())
    {
        window()->raise();
        window()->activateWindow();
    }
    if (cancelButton_)
    {
        cancelButton_->setFocus(Qt::OtherFocusReason);
    }
}

void WhitelistEditDialog::schedulePanelGeometryUpdate()
{
    QTimer::singleShot(0, this, &WhitelistEditDialog::updatePanelGeometry);
}

void WhitelistEditDialog::setCreateMode()
{
    editing = false;
    editingId = 0;
    titleLabel_->setText(QStringLiteral("新增"));
    serialEdit_->clear();
    serialEdit_->setReadOnly(false);
    remarksEdit_->clear();
    permanentCheck_->setChecked(true);
    {
        const QSignalBlocker blocker(unlimitedAreaCheck_);
        unlimitedAreaCheck_->setChecked(true);
    }
    startTimeEdit_->setDateTime(startTimeEdit_->minimumDateTime());
    endTimeEdit_->setDateTime(endTimeEdit_->minimumDateTime());
    currentAreaShapeJson_.clear();
    hintLabel_->hide();
    syncTimeSectionVisibility(true);
    syncAreaSectionVisibility(true);
    updatePanelGeometry();
    loadAreaShapeToMap();
}

void WhitelistEditDialog::setEditMode(const WhitelistPage::WhitelistRecord &record)
{
    editing = true;
    editingId = record.id;
    titleLabel_->setText(QStringLiteral("编辑"));
    serialEdit_->setText(record.serialNumber);
    serialEdit_->setReadOnly(true);
    remarksEdit_->setText(record.remarks);
    applyEffectiveTime(record.effectiveTime);
    applyEffectiveArea(record.effectiveArea);
    hintLabel_->hide();
    updatePanelGeometry();
    loadAreaShapeToMap();
}

void WhitelistEditDialog::applyEffectiveTime(const QString &value)
{
    if (value == QStringLiteral("permanent") || value.trimmed().isEmpty())
    {
        permanentCheck_->setChecked(true);
        syncTimeSectionVisibility(true);
        return;
    }

    permanentCheck_->setChecked(false);
    const QStringList parts = value.split(QLatin1Char('|'));
    if (parts.size() >= 3 && parts.at(0) == QStringLiteral("range"))
    {
        QDateTime start = QDateTime::fromString(parts.at(1), Qt::ISODate);
        QDateTime end = QDateTime::fromString(parts.at(2), Qt::ISODate);
        if (!start.isValid())
        {
            const QDate startDate = QDate::fromString(parts.at(1), Qt::ISODate);
            if (startDate.isValid())
            {
                start = QDateTime(startDate, QTime(0, 0, 0));
            }
        }
        if (!end.isValid())
        {
            const QDate endDate = QDate::fromString(parts.at(2), Qt::ISODate);
            if (endDate.isValid())
            {
                end = QDateTime(endDate, QTime(23, 59, 59));
            }
        }
        if (start.isValid())
        {
            startTimeEdit_->setDateTime(start);
        }
        if (end.isValid())
        {
            endTimeEdit_->setDateTime(end);
        }
    }
    syncTimeSectionVisibility(false);
}

void WhitelistEditDialog::applyEffectiveArea(const QString &value)
{
    const QSignalBlocker blocker(unlimitedAreaCheck_);
    if (value == QStringLiteral("unlimited") || value.trimmed().isEmpty())
    {
        unlimitedAreaCheck_->setChecked(true);
        currentAreaShapeJson_.clear();
    }
    else
    {
        unlimitedAreaCheck_->setChecked(false);
        currentAreaShapeJson_ = value;
    }

    syncAreaSectionVisibility(unlimitedAreaCheck_->isChecked());
    if (!unlimitedAreaCheck_->isChecked())
    {
        ensureMapWebView();
    }
}

QString WhitelistEditDialog::buildEffectiveTime() const
{
    if (permanentCheck_->isChecked())
    {
        return QStringLiteral("permanent");
    }

    return QStringLiteral("range|%1|%2")
        .arg(startTimeEdit_->dateTime().toString(Qt::ISODate), endTimeEdit_->dateTime().toString(Qt::ISODate));
}

QString WhitelistEditDialog::buildEffectiveArea() const
{
    if (unlimitedAreaCheck_->isChecked())
    {
        return QStringLiteral("unlimited");
    }

    return currentAreaShapeJson_;
}

WhitelistPage::WhitelistRecord WhitelistEditDialog::record() const
{
    WhitelistPage::WhitelistRecord result;
    result.id = editingId;
    result.serialNumber = serialEdit_->text().trimmed();
    result.recordKey = result.serialNumber;
    result.modelName = QStringLiteral("");
    result.remarks = remarksEdit_->text().trimmed();
    result.effectiveTime = buildEffectiveTime();
    result.effectiveArea = buildEffectiveArea();
    return result;
}

void WhitelistEditDialog::syncTimeSectionVisibility(bool permanent)
{
    if (dateRow_)
    {
        dateRow_->setVisible(!permanent);
    }
    if (permanent && timePicker_)
    {
        timePicker_->hidePopup();
    }
}

void WhitelistEditDialog::syncAreaSectionVisibility(bool unlimited)
{
    if (unlimited)
    {
        detachMapColumn();
        suspendMapWebView();
        return;
    }

    attachMapColumn();
    ensureMapWebView();
    resumeMapWebView();
    loadAreaShapeToMap();
}

void WhitelistEditDialog::loadAreaShapeToMap()
{
    if (!mapWebView_ || !mapReady_)
    {
        return;
    }

    if (currentAreaShapeJson_.trimmed().isEmpty())
    {
        mapWebView_->page()->runJavaScript(QStringLiteral("if (typeof setWhitelistAreaShape === 'function') "
                                                          "setWhitelistAreaShape(null);"));
        return;
    }

    mapWebView_->page()->runJavaScript(
        QStringLiteral("if (typeof setWhitelistAreaShape === 'function') setWhitelistAreaShape(%1);")
            .arg(currentAreaShapeJson_));
}

void WhitelistEditDialog::showOverlay()
{
    qDebug() << "[WhitelistEdit] showOverlay";
    if (parentWidget())
    {
        setGeometry(parentWidget()->rect());
        raise();
    }
    updatePanelGeometry();
    show();
    if (serialEdit_)
    {
        serialEdit_->setFocus();
    }
    if (mapWebView_ && mapWebView_->page())
    {
        mapWebView_->page()->runJavaScript(
            QStringLiteral("setTimeout(function(){ if (typeof map !== 'undefined' && map.invalidateSize) map.invalidateSize(); }, "
                           "150);"));
    }
}

void WhitelistEditDialog::tryConfirm()
{
    if (serialEdit_->text().trimmed().isEmpty())
    {
        hintLabel_->setText(QStringLiteral("请输入无人机序列号"));
        hintLabel_->show();
        serialEdit_->setFocus();
        return;
    }

    if (!permanentCheck_->isChecked())
    {
        if (isUnsetDateTimeEdit(startTimeEdit_) || isUnsetDateTimeEdit(endTimeEdit_))
        {
            hintLabel_->setText(QStringLiteral("请选择有效时间"));
            hintLabel_->show();
            return;
        }
        if (startTimeEdit_->dateTime() > endTimeEdit_->dateTime())
        {
            hintLabel_->setText(QStringLiteral("开始时间不能晚于结束时间"));
            hintLabel_->show();
            return;
        }
    }

    if (!unlimitedAreaCheck_->isChecked())
    {
        ensureMapWebView();
        if (!mapWebView_ || !mapWebView_->page())
        {
            hintLabel_->setText(QStringLiteral("请绘制有效区域"));
            hintLabel_->show();
            return;
        }

        mapWebView_->page()->runJavaScript(
            QStringLiteral("(function(){"
                           "if (typeof finalizeWhitelistAreaShape === 'function') {"
                           "  var shape = finalizeWhitelistAreaShape();"
                           "} else if (typeof getWhitelistAreaShape === 'function') {"
                           "  var shape = getWhitelistAreaShape();"
                           "} else {"
                           "  var shape = null;"
                           "}"
                           "return shape ? JSON.stringify(shape) : null;"
                           "})()"),
            [this](const QVariant &result)
            {
                const QString shapeJson = result.toString();
                currentAreaShapeJson_ = shapeJson == QStringLiteral("null") ? QString() : shapeJson;
                finishConfirm();
            });
        return;
    }

    finishConfirm();
}

void WhitelistEditDialog::finishConfirm()
{
    if (!unlimitedAreaCheck_->isChecked() && currentAreaShapeJson_.trimmed().isEmpty())
    {
        hintLabel_->setText(QStringLiteral("请绘制有效区域"));
        hintLabel_->show();
        return;
    }

    hintLabel_->hide();
    emit confirmed(record());
    close();
}

void WhitelistEditDialog::updatePanelGeometry()
{
    if (!parentWidget() || !panelWidget_)
    {
        return;
    }

    const QSize parentSize = parentWidget()->size();
    const bool showMap = !unlimitedAreaCheck_->isChecked();
    const int panelWidth =
        showMap ? qBound(860, parentSize.width() - 80, 1180) : qBound(480, qMin(540, parentSize.width() - 80), 620);
    const int panelHeight =
        showMap ? qBound(520, parentSize.height() - 80, 700)
                : qBound(permanentCheck_->isChecked() ? 380 : 420, qMin(500, parentSize.height() - 120), 560);
    panelWidget_->setFixedSize(panelWidth, panelHeight);

    if (mapWebView_)
    {
        mapWebView_->setMinimumHeight(showMap ? panelHeight - 170 : 0);
    }

    if (timePicker_)
    {
        timePicker_->updatePosition();
    }
}

void WhitelistEditDialog::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (timePicker_ && timePicker_->isVisible())
    {
        timePicker_->hidePopup();
    }
    updatePanelGeometry();
}

void WhitelistEditDialog::closeEvent(QCloseEvent *event)
{
    qDebug() << "[WhitelistEdit] closeEvent";
    if (timePicker_)
    {
        timePicker_->hidePopup();
    }

    if (mapWebView_)
    {
        mapWebView_->disconnect(this);
        if (QWebEnginePage *page = mapWebView_->page())
        {
            page->disconnect(this);
        }
        mapWebView_->hide();
    }

    QWidget::closeEvent(event);
}
