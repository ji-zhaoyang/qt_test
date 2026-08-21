#include "video_takeover_facade.h"

#include "home_web_bridge.h"
#include "video_frame_pipeline.h"
#include "video_takeover_panel_controller.h"
#include "video_takeover_widget.h"

#include <QWebEngineView>

VideoTakeoverFacade::VideoTakeoverFacade(QWidget *hostWidget, QWebEngineView *mapWebView, QObject *parent)
    : QObject(parent), mapWebView_(mapWebView)
{
    controller_ = new VideoTakeoverPanelController(this);
    pipeline_ = new VideoFramePipeline(this);
    widget_ = new VideoTakeoverWidget(hostWidget);
    wireInternal();
}

void VideoTakeoverFacade::setWebBridge(HomeWebBridge *bridge)
{
    webBridge_ = bridge;
}

void VideoTakeoverFacade::setMapPageLoaded(bool loaded)
{
    mapPageLoaded_ = loaded;
}

void VideoTakeoverFacade::updateMapGeometry()
{
    if (!widget_ || !mapWebView_)
    {
        return;
    }

    widget_->setAnchorGeometry(mapWebView_->geometry());
}

void VideoTakeoverFacade::onUserRequest(bool enabled, quint32 frequencyKhz, quint32 targetId)
{
    controller_->onUserRequest(enabled, frequencyKhz, targetId);
}

void VideoTakeoverFacade::on290(quint32 targetId, bool enabled, bool success, const QString &msg)
{
    if (mapPageLoaded_ && webBridge_)
    {
        webBridge_->sendVideoTakeoverResponse(targetId, enabled, success, msg);
    }
    controller_->onDeviceResponse(targetId, enabled, success, msg);
}

void VideoTakeoverFacade::on291(const QByteArray &jpegPayload, const QSize &frameSize)
{
    controller_->onVideoFrame(jpegPayload, frameSize);
}

void VideoTakeoverFacade::onConnectionLost()
{
    controller_->onConnectionLost();
}

void VideoTakeoverFacade::wireInternal()
{
    connect(controller_, &VideoTakeoverPanelController::takeoverRequested, this, &VideoTakeoverFacade::takeoverRequested);
    connect(controller_, &VideoTakeoverPanelController::toastRequested, this, &VideoTakeoverFacade::toastRequested);
    connect(controller_, &VideoTakeoverPanelController::panelShowRequested, this,
            [this](quint32 targetId, quint32 frequencyKhz, const QString &metaText)
            {
                updateMapGeometry();
                widget_->showSession(targetId, frequencyKhz, metaText);
            });
    connect(controller_, &VideoTakeoverPanelController::panelHideRequested, widget_, &VideoTakeoverWidget::hideSession);
    connect(controller_, &VideoTakeoverPanelController::panelMetaChanged, widget_, &VideoTakeoverWidget::updateMeta);
    connect(controller_, &VideoTakeoverPanelController::panelFrameReady, pipeline_, &VideoFramePipeline::submitFrame);
    connect(controller_, &VideoTakeoverPanelController::panelClearRequested, pipeline_, &VideoFramePipeline::invalidatePending);
    connect(controller_, &VideoTakeoverPanelController::panelClearRequested, widget_, &VideoTakeoverWidget::clearFrame);
    connect(pipeline_, &VideoFramePipeline::frameReady, widget_, &VideoTakeoverWidget::displayFrame);
    connect(pipeline_, &VideoFramePipeline::decodeFailed, widget_, &VideoTakeoverWidget::showDecodeFailed);
    connect(widget_, &VideoTakeoverWidget::closeRequested, this,
            [this](quint32 targetId, quint32 frequencyKhz)
            {
                controller_->onUserRequest(false, frequencyKhz, targetId);
            });
}
