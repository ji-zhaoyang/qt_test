function setQtCommandTitle(command) {
    document.title = command;
}

function sendDirectionFindingCommand(enabled, targetId) {
    directionFindingState.commandSequence += 1;
    setQtCommandTitle(
        'CMD:DIRECTION_FINDING:' + (enabled ? '1' : '0') + ':' +
        String(targetId) + ':' + String(directionFindingState.commandSequence)
    );
}

function sendPrecisionStrikeCommand(enabled, targetId, timestamp, type, sn) {
    precisionStrikeState.commandSequence += 1;
    setQtCommandTitle(
        'CMD:PRECISION_STRIKE:' + (enabled ? '1' : '0') + ':' +
        String(targetId) + ':' + String(timestamp) + ':' + String(type) + ':' +
        encodeURIComponent(String(sn || '')) + ':' + String(precisionStrikeState.commandSequence)
    );
}

function sendWideJamCommand(enabled, targetId, frequencyKhz, sn) {
    wideJamState.commandSequence += 1;
    setQtCommandTitle(
        'CMD:WIDE_JAM:' + (enabled ? '1' : '0') + ':' +
        String(targetId) + ':' + String(frequencyKhz) + ':' +
        encodeURIComponent(String(sn || '')) + ':' + String(wideJamState.commandSequence)
    );
}

function updateDroneTargetFromQt(target) {
    if (!target || typeof target !== 'object') {
        return;
    }

    var id = getTargetDisplayId(target);
    if (!id) {
        return;
    }

    if (target.disappeared) {
        scheduleRemoveDroneTarget(id);
        return;
    }

    cancelScheduledRemoval(id);
    setTargetPanelVisible(true);

    var normalizedTarget = Object.assign({}, target);
    normalizedTarget.sequence = ++targetSequence;
    droneTargets[id] = normalizedTarget;
    ensureTargetOrder(id);
    scheduleRemoveDroneTarget(id);

    if (!selectedTargetId) {
        selectedTargetId = id;
    }
    renderTargetPanel();
}

function updateDroneDirectionFindingResponseFromQt(response) {
    if (!response || typeof response !== 'object') {
        return;
    }

    var targetId = Number(response.targetId);
    var target = getTargetByNumericId(targetId) || getSelectedDroneTarget();
    var targetName = target && target.targetName ? target.targetName : (directionFindingState.targetName || '-');
    var message = String(response.message || '');
    if (response.success && response.enabled) {
        directionFindingState.targetId = targetId;
        directionFindingState.targetName = targetName;
        updateDirectionDialogContent(targetName, '', '-', '-', 0);
        setDirectionDialogVisible(true);
        return;
    }

    if (response.success && !response.enabled) {
        setDirectionDialogVisible(false);
        updateDirectionDialogContent('-', '', '-', '-', 0);
        directionFindingState.targetId = 0;
        directionFindingState.targetName = '';
        return;
    }

    if (directionFindingState.visible) {
        document.getElementById('direction-dialog-message').textContent = message;
    }
}

function updateDroneDirectionPowerReportFromQt(report) {
    if (!report || typeof report !== 'object') {
        return;
    }

    var targetId = Number(report.targetId);
    var target = getTargetByNumericId(targetId) || getSelectedDroneTarget();
    var targetName = target && target.targetName ? target.targetName : (directionFindingState.targetName || '-');
    var percent = calculateDirectionPercent(report.omniPower, report.directionalPower, report.calibrationValue);
    directionFindingState.targetId = targetId;
    directionFindingState.targetName = targetName;
    updateDirectionDialogContent(targetName, '', report.omniPower, report.directionalPower, percent);
    setDirectionDialogVisible(true);
}

function updateDronePrecisionStrikeResponseFromQt(response) {
    if (!response || typeof response !== 'object') {
        return;
    }

    var targetId = Number(response.targetId);
    var target = getTargetByNumericId(targetId) || getSelectedDroneTarget();
    var message = String(response.message || '').trim();
    if (response.success && target) {
        precisionStrikeState.pendingTargetId = null;
        precisionStrikeState.pendingEnabled = false;
        if (response.enabled) {
            precisionStrikeState.activeTargets[getPrecisionStrikeStateKey(target)] = Date.now() + 120000;
            syncPrecisionStrikeCountdownTimer();
            updateTargetDetail(target);
        } else {
            delete precisionStrikeState.activeTargets[getPrecisionStrikeStateKey(target)];
            syncPrecisionStrikeCountdownTimer();
            updateTargetDetail(target);
            showTargetActionHint('精准打击已关闭', false);
        }
        return;
    }

    if (precisionStrikeState.pendingTargetId === targetId) {
        precisionStrikeState.pendingTargetId = null;
        precisionStrikeState.pendingEnabled = false;
    }
    updateTargetDetail(getSelectedDroneTarget());
    showTargetActionHint(message || '精准打击执行失败', true);
}

function updateDroneWideBandJammingResponseFromQt(response) {
    if (!response || typeof response !== 'object') {
        return;
    }

    var targetId = Number(response.targetId);
    var target = getTargetByNumericId(targetId) || getSelectedDroneTarget();
    var message = String(response.message || '').trim();
    if (response.success && target) {
        wideJamState.pendingTargetId = null;
        wideJamState.pendingEnabled = false;
        if (response.enabled) {
            wideJamState.activeTargets[getWideJamStateKey(target)] = true;
            updateTargetDetail(target);
            showTargetActionHint('宽频干扰已开启', false);
        } else {
            delete wideJamState.activeTargets[getWideJamStateKey(target)];
            updateTargetDetail(target);
            showTargetActionHint('宽频干扰已关闭', false);
        }
        return;
    }

    if (wideJamState.pendingTargetId === targetId) {
        wideJamState.pendingTargetId = null;
        wideJamState.pendingEnabled = false;
    }
    updateTargetDetail(getSelectedDroneTarget());
    showTargetActionHint(message || '宽频干扰执行失败', true);
}

function updateMarker(lat, lng, alt) {
    if (typeof lat !== 'number' || typeof lng !== 'number' || isNaN(lat) || isNaN(lng)) {
        return;
    }

    if (lat < -90 || lat > 90 || lng < -180 || lng > 180) {
        return;
    }

    var newPos = [lat, lng];
    myMarker.setLatLng(newPos);
    currentDevicePosition.lat = lat;
    currentDevicePosition.lng = lng;
    currentDevicePosition.alt = alt;
    rerenderAllDroneTargets();
}

function updateDashboard(yaw, pitch) {
    if (typeof yaw !== 'number' || typeof pitch !== 'number') {
        return;
    }

    document.getElementById('needle-pitch').style.transform = 'rotate(' + String(pitch - 135) + 'deg)';
    document.getElementById('needle-yaw').style.transform = 'rotate(' + String(yaw - 45) + 'deg)';
    document.getElementById('val-pitch').innerText = pitch.toFixed(0) + '°';
    document.getElementById('val-yaw').innerText = yaw.toFixed(0) + '°';
}

var isFullscreen = false;
document.getElementById('btn-fullscreen').addEventListener('click', function() {
    isFullscreen = !isFullscreen;
    setQtCommandTitle(isFullscreen ? 'CMD:FULLSCREEN_ON' : 'CMD:FULLSCREEN_OFF');
});
