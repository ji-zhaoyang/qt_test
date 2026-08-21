var directionFindingState = {
    visible: false,
    targetId: 0,
    targetName: '',
    commandSequence: 0
};
var precisionStrikeState = {
    commandSequence: 0,
    activeTargets: {},
    pendingTargetId: null,
    pendingEnabled: false,
    countdownTimer: 0
};
var wideJamState = {
    commandSequence: 0,
    activeTargets: {},
    pendingTargetId: null,
    pendingEnabled: false
};
var videoTakeoverState = {
    commandSequence: 0,
    activeTargets: {},
    pendingTargetId: null,
    pendingEnabled: false
};

function getTargetSerialNumber(target) {
    return String(target && target.targetUniqueId || '').trim();
}

function getWideJamSn(target) {
    var serial = getTargetSerialNumber(target);
    if (serial) {
        return serial;
    }

    var targetId = Number(target && target.targetId);
    if (isFinite(targetId) && targetId >= 0) {
        return String(targetId);
    }
    return '';
}

function getWideJamStateKey(target) {
    return target ? getTargetDisplayId(target) : '';
}

function getPrecisionStrikeStateKey(target) {
    return target ? getTargetDisplayId(target) : '';
}

function getVideoTakeoverStateKey(target) {
    return target ? getTargetDisplayId(target) : '';
}

function getPrecisionStrikeExpiresAt(target) {
    var key = getPrecisionStrikeStateKey(target);
    var expiresAt = key ? Number(precisionStrikeState.activeTargets[key]) : 0;
    if (!key || !isFinite(expiresAt) || expiresAt <= 0) {
        return 0;
    }
    if (expiresAt <= Date.now()) {
        delete precisionStrikeState.activeTargets[key];
        return 0;
    }
    return expiresAt;
}

function getPrecisionStrikeRemainingMs(target) {
    var expiresAt = getPrecisionStrikeExpiresAt(target);
    return expiresAt > 0 ? Math.max(0, expiresAt - Date.now()) : 0;
}

function isPrecisionStrikeActive(target) {
    return getPrecisionStrikeRemainingMs(target) > 0;
}

function isWideJamActive(target) {
    var key = getWideJamStateKey(target);
    return !!(key && wideJamState.activeTargets[key]);
}

function isVideoTakeoverActive(target) {
    var key = getVideoTakeoverStateKey(target);
    return !!(key && videoTakeoverState.activeTargets[key]);
}

function syncPrecisionStrikeCountdownTimer() {
    var hasActiveTarget = false;
    var keys = Object.keys(precisionStrikeState.activeTargets);
    for (var i = 0; i < keys.length; ++i) {
        var expiresAt = Number(precisionStrikeState.activeTargets[keys[i]]);
        if (isFinite(expiresAt) && expiresAt > Date.now()) {
            hasActiveTarget = true;
        } else {
            delete precisionStrikeState.activeTargets[keys[i]];
        }
    }

    if (!hasActiveTarget) {
        if (precisionStrikeState.countdownTimer) {
            window.clearInterval(precisionStrikeState.countdownTimer);
            precisionStrikeState.countdownTimer = 0;
        }
        return;
    }

    if (!precisionStrikeState.countdownTimer) {
        precisionStrikeState.countdownTimer = window.setInterval(function() {
            syncPrecisionStrikeCountdownTimer();
            updateTargetDetail(getSelectedDroneTarget());
        }, 1000);
    }
}

function formatDirectionPowerValue(value) {
    var num = Number(value);
    if (!isFinite(num)) {
        return '-';
    }
    return num.toFixed(1);
}

function calculateDirectionPercent(omniPower, directionalPower, calibrationValue) {
    var omni = Number(omniPower);
    var directional = Number(directionalPower);
    var calibration = Number(calibrationValue);
    if (!isFinite(omni) || !isFinite(directional) || !isFinite(calibration) || calibration === 0) {
        return null;
    }
    var percent = ((directional - omni) / calibration) * 100;
    if (!isFinite(percent) || percent < 0) {
        return null;
    }
    return Math.min(100, percent);
}

function setDirectionDialogVisible(visible) {
    var dialog = document.getElementById('direction-dialog');
    dialog.style.display = visible ? 'block' : 'none';
    directionFindingState.visible = visible;
}

function updateDirectionDialogProgress(percent) {
    var progressFill = document.getElementById('direction-dialog-progress-fill');
    var progressText = document.getElementById('direction-dialog-progress-text');
    var numericPercent = Number(percent);
    if (!isFinite(numericPercent) || numericPercent < 0) {
        progressFill.style.width = '0%';
        progressText.textContent = '';
        return;
    }
    var normalized = Math.min(100, numericPercent);
    progressFill.style.width = normalized.toFixed(1) + '%';
    progressText.textContent = Math.round(normalized) + '%';
}

function updateDirectionDialogContent(targetName, message, omniPower, directionalPower, percent) {
    document.getElementById('direction-dialog-target').textContent = targetName || '-';
    document.getElementById('direction-dialog-message').textContent = message || '';
    document.getElementById('direction-dialog-omni-power').textContent = formatDirectionPowerValue(omniPower);
    document.getElementById('direction-dialog-directional-power').textContent = formatDirectionPowerValue(directionalPower);
    updateDirectionDialogProgress(percent);
}

function getDroneType(target) {
    return Number(target && target.droneType);
}

function isSpectrumDrone(target) {
    return getDroneType(target) === 0;
}

function hasDirectionalAction(target) {
    return isSpectrumDrone(target);
}

function canShowWideJam(target) {
    var droneType = getDroneType(target);
    return droneType === 0 || droneType === 1 || droneType === 2 ||
           droneType === 3 || droneType === 4;
}

function canShowVideoTakeover(target) {
    var targetId = Number(target && target.targetId);
    var frequencyKhz = Math.round(Number(target && target.frequencyKhz));
    return isFinite(targetId) && targetId >= 0 && isFinite(frequencyKhz) && frequencyKhz > 0;
}

function hasPrecisionStrike(target) {
    var droneType = getDroneType(target);
    var identifyTimestamp = Number(target && target.identifyTimestamp);
    return (droneType === 1 || droneType === 3) && identifyTimestamp > 0 && !!getTargetSerialNumber(target);
}

function getTargetActionText(target) {
    if (hasDirectionalAction(target)) {
        return '测向';
    }
    if (hasPrecisionStrike(target)) {
        return '精准';
    }
    return '干扰';
}

document.getElementById('detail-whitelist-btn').addEventListener('click', function() {
    var target = getSelectedDroneTarget();
    if (!target) {
        showTargetActionHint('请先选择目标', true);
        return;
    }
    var serial = getTargetSerialNumber(target);
    var recordKey = getTargetDisplayId(target);
    if (!serial && !recordKey) {
        showTargetActionHint('目标序列号为空', true);
        return;
    }
    showTargetActionHint('', false);
    sendWhitelistAddCommand(serial, recordKey);
});

document.getElementById('detail-wide-jam-btn').addEventListener('click', function() {
    var target = getSelectedDroneTarget();
    var targetId = Number(target && target.targetId);
    var frequencyKhz = Math.round(Number(target && target.frequencyKhz));
    var enabled = !isWideJamActive(target);
    var sn = getWideJamSn(target);
    if (!target || !isFinite(targetId) || targetId < 0 || !isFinite(frequencyKhz) || frequencyKhz <= 0 || !sn) {
        showTargetActionHint('宽频干扰参数不完整', true);
        return;
    }
    if (wideJamState.pendingTargetId !== null && wideJamState.pendingTargetId === targetId) {
        updateWideJamButtonState(target);
        return;
    }
    wideJamState.pendingTargetId = targetId;
    wideJamState.pendingEnabled = enabled;
    showTargetActionHint('', false);
    sendWideJamCommand(enabled, targetId, frequencyKhz, sn);
});

document.getElementById('detail-video-takeover-btn').addEventListener('click', function() {
    var target = getSelectedDroneTarget();
    var targetId = Number(target && target.targetId);
    var frequencyKhz = Math.round(Number(target && target.frequencyKhz));
    var enabled = !isVideoTakeoverActive(target);
    if (!target || !isFinite(targetId) || targetId < 0 || !isFinite(frequencyKhz) || frequencyKhz <= 0) {
        showTargetActionHint('图传接管参数不完整', true);
        return;
    }
    if (videoTakeoverState.pendingTargetId !== null && videoTakeoverState.pendingTargetId === targetId) {
        updateVideoTakeoverButtonState(target);
        return;
    }
    videoTakeoverState.pendingTargetId = targetId;
    videoTakeoverState.pendingEnabled = enabled;
    showTargetActionHint('', false);
    sendVideoTakeoverCommand(enabled, targetId, frequencyKhz);
});

document.getElementById('detail-directional-btn').addEventListener('click', function() {
    var target = getSelectedDroneTarget();
    var targetId = Number(target && target.targetId);
    if (!target || !isFinite(targetId) || targetId < 0) {
        return;
    }
    directionFindingState.targetId = targetId;
    directionFindingState.targetName = target.targetName || ('ID: ' + String(targetId));
    sendDirectionFindingCommand(true, targetId);
});

document.getElementById('detail-precision-strike-btn').addEventListener('click', function() {
    var target = getSelectedDroneTarget();
    var targetId = Number(target && target.targetId);
    var timestamp = Number(target && target.identifyTimestamp);
    var droneType = getDroneType(target);
    var enabled = !isPrecisionStrikeActive(target);
    var sn = getTargetSerialNumber(target);
    if (!target || !isFinite(targetId) || targetId < 0 || !isFinite(timestamp) || timestamp <= 0 ||
        !isFinite(droneType) || !sn) {
        showTargetActionHint('精准打击参数不完整', true);
        return;
    }
    if (precisionStrikeState.pendingTargetId !== null && precisionStrikeState.pendingTargetId === targetId) {
        updatePrecisionStrikeButtonState(target);
        return;
    }
    precisionStrikeState.pendingTargetId = targetId;
    precisionStrikeState.pendingEnabled = enabled;
    showTargetActionHint('', false);
    sendPrecisionStrikeCommand(enabled, targetId, timestamp, droneType, sn);
});

document.getElementById('direction-dialog-close').addEventListener('click', function() {
    if (!directionFindingState.visible) {
        return;
    }
    sendDirectionFindingCommand(false, directionFindingState.targetId);
});
