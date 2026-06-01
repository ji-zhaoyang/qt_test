#include "screen_flash_overlay.h"
#include <QPainter>
#include <QTimer>

ScreenFlashOverlay::ScreenFlashOverlay(QWidget *parent)
    : QWidget(parent), flashTimer(new QTimer(this)), flashingEnabled(false), flashVisible(false)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAutoFillBackground(false);
    hide();

    flashTimer->setInterval(280);
    connect(flashTimer, &QTimer::timeout, this,
            [this]()
            {
                flashVisible = !flashVisible;
                setVisible(flashVisible);
                if (flashVisible)
                {
                    raise();
                }
            });
}

void ScreenFlashOverlay::setFlashingEnabled(bool enabled)
{
    if (flashingEnabled == enabled)
    {
        if (enabled)
        {
            syncToParentGeometry();
            raise();
        }
        return;
    }

    flashingEnabled = enabled;
    if (!flashingEnabled)
    {
        flashTimer->stop();
        flashVisible = false;
        hide();
        return;
    }

    flashVisible = true;
    syncToParentGeometry();
    show();
    raise();
    flashTimer->start();
}

bool ScreenFlashOverlay::isFlashingEnabled() const
{
    return flashingEnabled;
}

void ScreenFlashOverlay::syncToParentGeometry()
{
    if (parentWidget())
    {
        setGeometry(parentWidget()->rect());
    }
}

void ScreenFlashOverlay::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setPen(Qt::NoPen);

    const int edgeWidth = 28;
    const QColor flashColor(255, 0, 0, 130);

    painter.fillRect(0, 0, width(), edgeWidth, flashColor);
    painter.fillRect(0, height() - edgeWidth, width(), edgeWidth, flashColor);
    painter.fillRect(0, edgeWidth, edgeWidth, qMax(0, height() - edgeWidth * 2), flashColor);
    painter.fillRect(width() - edgeWidth, edgeWidth, edgeWidth, qMax(0, height() - edgeWidth * 2), flashColor);
}
