// 初始化地图，设置中心点经纬度（杭州西湖附近：30.25, 120.15）和缩放级别
// 【注意】：根据实际离线包层级调整缩放级别
var map = L.map('map').setView([30.342, 120.089], 15);

// ==================== 1. 深色态势图 (2d 文件夹组合) ====================
// 底图：浅色带地形路网的底图
var baseDarkLayer = L.tileLayer('../../../map/2d/satellite/{z}/{x}/{y}.jpg', {
    maxZoom: 16, 
    minZoom: 1,
    errorTileUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII='
});

// 标注层：纯文字透明层
var labelsLayer = L.tileLayer('../../../map/2d/{z}/{x}/{y}.png', {
    maxZoom: 16,
    minZoom: 1,
    errorTileUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII='
});

// 将底图和文字层打包成“深色态势图组合”
var darkMapGroup = L.layerGroup([baseDarkLayer, labelsLayer]);

// ==================== 2. 卫星实景图 (3d 文件夹组合) ====================
// 底图：真正的航拍卫星照片
var baseSatelliteLayer = L.tileLayer('../../../map/3d/satellite/{z}/{x}/{y}.jpg', {
    maxZoom: 16,
    minZoom: 1,
    errorTileUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII='
});

// 标注层：带路网黄线和文字的透明层
var overlayLayer = L.tileLayer('../../../map/3d/overlay/{z}/{x}/{y}.png', {
    maxZoom: 16,
    minZoom: 1,
    errorTileUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII='
});

// 将航拍底图和路网文字层打包成“卫星实景图组合”
var satelliteGroup = L.layerGroup([baseSatelliteLayer, overlayLayer]);

// ==================== 3. 初始显示与图层切换控制 ====================
// 默认显示深色态势图组合
darkMapGroup.addTo(map);

// 移除 Leaflet 原生的右上角图层控制器，改为左下角按钮控制
var isSatelliteMode = false;
document.getElementById('btn-toggle-map').addEventListener('click', function() {
    if (isSatelliteMode) {
        // 当前是卫星图，切回深色态势图
        map.removeLayer(satelliteGroup);
        darkMapGroup.addTo(map);
        isSatelliteMode = false;
        this.title = "切换卫星实景图"; // 修改鼠标悬停提示
        // 可以按需修改按钮的文字或颜色，比如： this.innerText = "🗺️";
    } else {
        // 当前是态势图，切到卫星图
        map.removeLayer(darkMapGroup);
        satelliteGroup.addTo(map);
        isSatelliteMode = true;
        this.title = "切换深色态势图";
    }
});

// 解决离线环境下缺少默认图片的问题，使用 CSS 画一个红色的圆形点
var customIcon = L.divIcon({
    className: 'custom-div-icon',
    html: "<div style='background-color: red; width: 16px; height: 16px; border-radius: 50%; border: 3px solid white; box-shadow: 0 0 6px rgba(0,0,0,0.8);'></div>",
    iconSize: [22, 22],
    iconAnchor: [11, 11] // 让圆心对准坐标点
});

var droneTargetIcon = L.divIcon({
    className: 'drone-target-icon',
    html: "<div style='background-color: #ff9900; width: 14px; height: 14px; border-radius: 50%; border: 3px solid #111111; box-shadow: 0 0 6px rgba(255,153,0,0.8);'></div>",
    iconSize: [20, 20],
    iconAnchor: [10, 10]
});

var fallbackDroneTargetIcon = L.divIcon({
    className: 'drone-target-icon-fallback',
    html: "<div style='background-color: #ffd36a; width: 14px; height: 14px; border-radius: 50%; border: 3px dashed #111111; box-shadow: 0 0 8px rgba(255,211,106,0.9);'></div>",
    iconSize: [20, 20],
    iconAnchor: [10, 10]
});

var droneMarkers = {};
var droneTargets = {};
var selectedTargetId = '';
var currentDevicePosition = {
    lat: 30.342,
    lng: 120.089,
    alt: 0
};
var targetSequence = 0;
var targetOrder = [];
var pendingRemovalTimers = {};
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
// 任意目标超过预警消除时间没有收到新上报，就按过期目标移除
var kSingleTargetRemovalDelayMs = 20000;

function setWarningClearDelayMs(delayMs) {
    var normalized = Number(delayMs);
    if (!isFinite(normalized) || normalized < 0) {
        return;
    }
    kSingleTargetRemovalDelayMs = normalized;
}

function setTargetPanelVisible(visible) {
    var layoutNode = document.querySelector('.app-layout');
    if (!layoutNode) {
        return;
    }
    layoutNode.classList.toggle('app-layout--panel-hidden', !visible);
    setTimeout(function() {
        map.invalidateSize();
    }, 0);
    setTimeout(function() {
        map.invalidateSize();
    }, 200);
}

function scheduleTargetPanelHide() {
    if (Object.keys(droneTargets).length > 0) {
        setTargetPanelVisible(true);
        return;
    }
    setTargetPanelVisible(false);
}

// 在杭州的位置画一个红色图标，并把它保存到 myMarker 变量里
var myMarker = L.marker([30.342, 120.089], {icon: customIcon}).addTo(map);

window.addEventListener('load', function() {
    setTimeout(function() {
        map.invalidateSize();
    }, 0);
    setTimeout(function() {
        map.invalidateSize();
    }, 200);
});

function getTargetByNumericId(targetId) {
    var expectedId = Number(targetId);
    var ids = Object.keys(droneTargets);
    for (var i = 0; i < ids.length; ++i) {
        var target = droneTargets[ids[i]];
        if (Number(target && target.targetId) === expectedId) {
            return target;
        }
    }
    return null;
}

function getSelectedDroneTarget() {
    if (!selectedTargetId) {
        return null;
    }
    return droneTargets[selectedTargetId] || null;
}

function getTargetSerialNumber(target) {
    return String(target && target.targetUniqueId || '').trim();
}

function getWideJamStateKey(target) {
    return target ? getTargetDisplayId(target) : '';
}

function getPrecisionStrikeStateKey(target) {
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

function updatePrecisionStrikeHint(target) {
    if (!isPrecisionStrikeActive(target)) {
        return;
    }
    var remainingSeconds = Math.ceil(getPrecisionStrikeRemainingMs(target) / 1000);
    showTargetActionHint('精准打击已开启，剩余' + String(remainingSeconds) + '秒', false);
}

function showTargetActionHint(text, isError) {
    var hintNode = document.getElementById('target-action-hint');
    if (!hintNode) {
        return;
    }
    var content = String(text || '').trim();
    if (!content) {
        hintNode.textContent = '';
        hintNode.style.display = 'none';
        hintNode.classList.remove('target-detail-card__action-hint--error');
        return;
    }
    hintNode.textContent = content;
    hintNode.style.display = 'block';
    hintNode.classList.toggle('target-detail-card__action-hint--error', !!isError);
}

function clearTargetActionHintIfIdle(target) {
    var hintNode = document.getElementById('target-action-hint');
    if (!hintNode || hintNode.classList.contains('target-detail-card__action-hint--error')) {
        return;
    }

    var targetId = Number(target && target.targetId);
    var precisionPending = isFinite(targetId) && precisionStrikeState.pendingTargetId !== null &&
        precisionStrikeState.pendingTargetId === targetId;
    var wideJamPending = isFinite(targetId) && wideJamState.pendingTargetId !== null &&
        wideJamState.pendingTargetId === targetId;
    var hasActiveAction = isPrecisionStrikeActive(target) || isWideJamActive(target);
    if (!precisionPending && !wideJamPending && !hasActiveAction) {
        showTargetActionHint('', false);
    }
}

function updateWideJamButtonState(target) {
    var button = document.getElementById('detail-wide-jam-btn');
    if (!button) {
        return;
    }

    var targetId = Number(target && target.targetId);
    if (isFinite(targetId) && wideJamState.pendingTargetId !== null &&
        wideJamState.pendingTargetId === targetId) {
        button.disabled = true;
        button.textContent = wideJamState.pendingEnabled ? '开启中...' : '关闭中...';
        return;
    }

    if (isWideJamActive(target)) {
        button.disabled = false;
        button.textContent = '关闭宽频干扰';
        return;
    }

    button.disabled = false;
    button.textContent = '宽频干扰';
}

function updatePrecisionStrikeButtonState(target) {
    var button = document.getElementById('detail-precision-strike-btn');
    if (!button) {
        return;
    }

    var targetId = Number(target && target.targetId);
    if (isFinite(targetId) && precisionStrikeState.pendingTargetId !== null &&
        precisionStrikeState.pendingTargetId === targetId) {
        button.disabled = true;
        button.textContent = precisionStrikeState.pendingEnabled ? '开启中...' : '关闭中...';
        return;
    }

    if (isPrecisionStrikeActive(target)) {
        button.disabled = false;
        button.textContent = '关闭精准打击(' + String(Math.ceil(getPrecisionStrikeRemainingMs(target) / 1000)) + 's)';
        return;
    }

    button.disabled = false;
    button.textContent = '精准打击';
}

function sendDirectionFindingCommand(enabled, targetId) {
    directionFindingState.commandSequence += 1;
    document.title = 'CMD:DIRECTION_FINDING:' + (enabled ? '1' : '0') + ':' +
        String(targetId) + ':' + String(directionFindingState.commandSequence);
}

function sendPrecisionStrikeCommand(enabled, targetId, timestamp, type, sn) {
    precisionStrikeState.commandSequence += 1;
    document.title = 'CMD:PRECISION_STRIKE:' + (enabled ? '1' : '0') + ':' +
        String(targetId) + ':' + String(timestamp) + ':' + String(type) + ':' +
        encodeURIComponent(String(sn || '')) + ':' + String(precisionStrikeState.commandSequence);
}

function sendWideJamCommand(enabled, targetId, frequencyKhz, sn) {
    wideJamState.commandSequence += 1;
    document.title = 'CMD:WIDE_JAM:' + (enabled ? '1' : '0') + ':' +
        String(targetId) + ':' + String(frequencyKhz) + ':' +
        encodeURIComponent(String(sn || '')) + ':' + String(wideJamState.commandSequence);
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
    if (!isFinite(percent)) {
        return null;
    }
    if (percent < 0) {
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

document.getElementById('detail-whitelist-btn').addEventListener('click', function() {
    window.alert('白名单功能预留');
});

document.getElementById('detail-wide-jam-btn').addEventListener('click', function() {
    var target = getSelectedDroneTarget();
    var targetId = Number(target && target.targetId);
    var frequencyKhz = Math.round(Number(target && target.frequencyKhz));
    var enabled = !isWideJamActive(target);
    var sn = getTargetSerialNumber(target);
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

window.addEventListener('resize', function() {
    map.invalidateSize();
});

function isFiniteNumber(value) {
    return typeof value === 'number' && isFinite(value);
}

function isNearZeroCoordinate(lat, lng) {
    return Math.abs(lat) < 0.000001 && Math.abs(lng) < 0.000001;
}

function hasValidCoordinate(lat, lng) {
    return isFiniteNumber(lat) && isFiniteNumber(lng) &&
           lat >= -90 && lat <= 90 && lng >= -180 && lng <= 180 &&
           !isNearZeroCoordinate(lat, lng);
}

function formatFrequencyKhz(value) {
    var num = Number(value);
    if (!isFinite(num) || num <= 0) {
        return '-';
    }
    var mhz = num / 1000;
    return (Math.abs(mhz - Math.round(mhz)) < 0.001 ? mhz.toFixed(0) : mhz.toFixed(2)) + 'MHz';
}

function formatBandwidthKhz(value) {
    var num = Number(value);
    if (!isFinite(num) || num <= 0) {
        return '-';
    }
    return (num / 1000).toFixed(2) + 'MHz';
}

function formatSignal(value) {
    var num = Number(value);
    if (!isFinite(num) || num === 0) {
        return '-';
    }
    return num.toFixed(1) + 'dbm';
}

function formatDistance(value) {
    var num = Number(value);
    if (!isFinite(num) || num < 0) {
        return '-';
    }
    return num.toFixed(0) + '米';
}

function formatAngle(value) {
    var num = Number(value);
    if (!isFinite(num) || num < 0) {
        return '-';
    }
    return num.toFixed(0) + '°';
}

function formatAltitudeMeters(value) {
    var num = Number(value);
    if (!isFinite(num)) {
        return '-';
    }
    return num.toFixed(0) + '米';
}

function formatSpeedMetersPerSecond(value) {
    var num = Number(value);
    if (!isFinite(num)) {
        return '-';
    }
    return num.toFixed(0) + '米/s';
}

function formatCoordinatePair(lng, lat) {
    var longitude = Number(lng);
    var latitude = Number(lat);
    if (!isFinite(longitude) || !isFinite(latitude)) {
        return '-';
    }
    return longitude.toFixed(6) + ',' + latitude.toFixed(6);
}

function formatConfidence(value) {
    var num = Number(value);
    if (!isFinite(num) || num < 0) {
        return '-';
    }
    return num.toFixed(0) + '%';
}

function formatSignalType(target) {
    var droneType = Number(target && target.droneType);
    if (isFinite(droneType)) {
        switch (droneType) {
        case 0:
            return '频谱无人机';
        case 1:
            return '解析无人机';
        case 2:
            return 'TDOA类型无人机';
        case 3:
            return 'remoteid id解析无人机';
        case 4:
            return 'wifi无人机';
        default:
            return '类型 ' + droneType;
        }
    }
    return String(target && target.targetUniqueId || '').trim() ? 'RID信号' : '普通信号';
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

function escapeHtml(text) {
    return String(text)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;')
        .replace(/'/g, '&#39;');
}

function currentTargetStatusText(target) {
    var altitude = Number(target.altitudeFromTakeoff);
    return isFinite(altitude) && altitude > 0 ? '飞行中' : '未起飞';
}

function destinationPoint(lat, lng, distanceMeters, bearingDegrees) {
    var earthRadius = 6378137;
    var angularDistance = distanceMeters / earthRadius;
    var bearing = bearingDegrees * Math.PI / 180;
    var lat1 = lat * Math.PI / 180;
    var lng1 = lng * Math.PI / 180;

    var sinLat1 = Math.sin(lat1);
    var cosLat1 = Math.cos(lat1);
    var sinAngular = Math.sin(angularDistance);
    var cosAngular = Math.cos(angularDistance);

    var lat2 = Math.asin(sinLat1 * cosAngular + cosLat1 * sinAngular * Math.cos(bearing));
    var lng2 = lng1 + Math.atan2(
        Math.sin(bearing) * sinAngular * cosLat1,
        cosAngular - sinLat1 * Math.sin(lat2)
    );

    return {
        lat: lat2 * 180 / Math.PI,
        lng: lng2 * 180 / Math.PI
    };
}

function resolveTargetPosition(target) {
    return null;
}

function getTargetDisplayId(target) {
    var rawId = String(target.targetUniqueId || '').trim();
    if (rawId) {
        return rawId;
    }
    return String(target.targetId || '');
}

function getTargetSerialText(target) {
    var rawId = String(target && target.targetUniqueId || '').trim();
    return rawId || '-';
}

function ensureTargetMarker(targetId, target, positionInfo) {
    var marker = droneMarkers[targetId];
    var iconToUse = positionInfo && positionInfo.estimated ? fallbackDroneTargetIcon : droneTargetIcon;

    if (!marker) {
        marker = L.marker([positionInfo.lat, positionInfo.lng], {icon: iconToUse}).addTo(map);
        marker.on('click', function() {
            setSelectedTarget(targetId, true);
        });
        droneMarkers[targetId] = marker;
    } else {
        marker.setLatLng([positionInfo.lat, positionInfo.lng]);
        marker.setIcon(iconToUse);
    }

    var estimatedText = positionInfo.estimated ? '<br>位置: 由设备位置、距离和方位角估算' : '';
    marker.bindTooltip(
        '无人机: ' + (target.targetName || 'Unknown Signal') +
        '<br>ID: ' + targetId +
        '<br>距离: ' + formatDistance(target.distance) +
        '<br>方位角: ' + formatAngle(target.azimuth) +
        estimatedText,
        {direction: 'top', offset: [0, -8]}
    );
}

function removeDroneTarget(id) {
    cancelScheduledRemoval(id);
    if (droneMarkers[id]) {
        map.removeLayer(droneMarkers[id]);
        delete droneMarkers[id];
    }
    delete droneTargets[id];
    releaseTargetOrder(id);

    if (selectedTargetId === id) {
        selectedTargetId = getFirstVisibleTargetId() || Object.keys(droneTargets)[0] || '';
    }
    renderTargetPanel();
}

function scheduleRemoveDroneTarget(id) {
    cancelScheduledRemoval(id);
    pendingRemovalTimers[id] = window.setTimeout(function() {
        delete pendingRemovalTimers[id];
        removeDroneTarget(id);
    }, kSingleTargetRemovalDelayMs);
}

function cancelScheduledRemoval(id) {
    if (pendingRemovalTimers[id]) {
        window.clearTimeout(pendingRemovalTimers[id]);
        delete pendingRemovalTimers[id];
    }
}

function ensureTargetOrder(targetId) {
    if (targetOrder.indexOf(targetId) === -1) {
        targetOrder.push(targetId);
    }
}

function releaseTargetOrder(targetId) {
    targetOrder = targetOrder.filter(function(id) {
        return id !== targetId;
    });
}

function getFirstVisibleTargetId() {
    var visibleIds = targetOrder.filter(function(id) {
        return droneTargets[id];
    });
    return visibleIds[0] || '';
}

function updateTargetDetail(target) {
    var emptyNode = document.getElementById('target-detail-empty');
    var contentNode = document.getElementById('target-detail-content');
    var actionsNode = document.getElementById('target-detail-actions');
    var whitelistBtn = document.getElementById('detail-whitelist-btn');
    var wideJamBtn = document.getElementById('detail-wide-jam-btn');
    var directionalBtn = document.getElementById('detail-directional-btn');
    var precisionStrikeBtn = document.getElementById('detail-precision-strike-btn');
    if (!target) {
        emptyNode.style.display = 'flex';
        contentNode.style.display = 'none';
        actionsNode.style.display = 'none';
        showTargetActionHint('', false);
        return;
    }

    emptyNode.style.display = 'none';
    contentNode.style.display = 'block';

    document.getElementById('detail-name').textContent = target.targetName || 'Unknown Signal';
    document.getElementById('detail-serial').textContent = getTargetSerialText(target);
    document.getElementById('detail-azimuth').textContent = formatAngle(target.azimuth);
    document.getElementById('detail-target-id').textContent = String(target.targetId || 0);
    document.getElementById('detail-bandwidth').textContent = formatBandwidthKhz(target.bandwidthKhz);
    document.getElementById('detail-frequency').textContent = formatFrequencyKhz(target.frequencyKhz);
    document.getElementById('detail-signal').textContent = formatSignal(target.signalStrengthDb);
    document.getElementById('detail-distance').textContent = formatDistance(target.distance);
    document.getElementById('detail-altitude-sea-level').textContent = formatAltitudeMeters(target.altitudeSeaLevel);
    document.getElementById('detail-altitude-takeoff').textContent = formatAltitudeMeters(target.altitudeFromTakeoff);
    document.getElementById('detail-speed').textContent = formatSpeedMetersPerSecond(target.speedMetersPerSecond);
    document.getElementById('detail-coordinates').textContent =
        formatCoordinatePair(target.longitude, target.latitude);
    document.getElementById('detail-controller-coordinates').textContent =
        formatCoordinatePair(target.controllerLongitude, target.controllerLatitude);

    var showWhitelist = isSpectrumDrone(target);
    var showWideJam = canShowWideJam(target);
    var showDirectional = hasDirectionalAction(target);
    var showPrecisionStrike = hasPrecisionStrike(target);

    whitelistBtn.style.display = showWhitelist ? 'inline-flex' : 'none';
    wideJamBtn.style.display = showWideJam ? 'inline-flex' : 'none';
    directionalBtn.style.display = showDirectional ? 'inline-flex' : 'none';
    precisionStrikeBtn.style.display = showPrecisionStrike ? 'inline-flex' : 'none';
    updateWideJamButtonState(target);
    updatePrecisionStrikeButtonState(target);
    actionsNode.style.display =
        (showWhitelist || showWideJam || showDirectional || showPrecisionStrike) ? 'flex' : 'none';
    if (!showWideJam && !showPrecisionStrike) {
        showTargetActionHint('', false);
        return;
    }
    if (showPrecisionStrike) {
        updatePrecisionStrikeHint(target);
    }
    clearTargetActionHintIfIdle(target);
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

function renderTargetList() {
    var listNode = document.getElementById('target-list');
    var visibleTargetIds = targetOrder.filter(function(id) {
        return droneTargets[id];
    });

    if (!visibleTargetIds.length) {
        listNode.innerHTML = '<div class="target-list__empty">暂无目标列表</div>';
        return;
    }

    listNode.innerHTML = '';
    visibleTargetIds.forEach(function(id) {
        var target = droneTargets[id];
        var item = document.createElement('div');
        item.className = 'target-list__item' + (id === selectedTargetId ? ' target-list__item--active' : '');
        item.addEventListener('pointerdown', function(event) {
            event.preventDefault();
            setSelectedTarget(id, false);
        });

        var bandText = getTargetActionText(target) +
            ' · ' + formatAngle(target.azimuth) + ' · ' + formatFrequencyKhz(target.frequencyKhz);
        var subline = 'ID: ' + id;

        item.innerHTML =
            '<div class="target-list__content">' +
                '<div class="target-list__icon-box">' +
                    '<img class="target-list__icon" src="./images/drone.png" alt="drone">' +
                '</div>' +
                '<div class="target-list__body">' +
                    '<div class="target-list__top">' +
                        '<div class="target-list__name">' + escapeHtml(target.targetName || 'Unknown Signal') + '</div>' +
                    '</div>' +
                    '<div class="target-list__freq">' + escapeHtml(bandText) + '</div>' +
                    '<div class="target-list__meta">' + escapeHtml(subline) + '</div>' +
                '</div>' +
            '</div>';
        listNode.appendChild(item);
    });
}

function renderTargetPanel() {
    var targetIds = Object.keys(droneTargets);
    document.getElementById('target-count-badge').textContent = String(targetIds.length);
    document.getElementById('target-summary-text').textContent =
        targetIds.length ? '实时接收设备自动上报目标' : '暂无目标';

    if (targetIds.length > 0) {
        setTargetPanelVisible(true);
    } else {
        scheduleTargetPanelHide();
    }

    if (!selectedTargetId || !droneTargets[selectedTargetId]) {
        selectedTargetId = getFirstVisibleTargetId() || targetIds[0] || '';
    }

    updateTargetDetail(selectedTargetId ? droneTargets[selectedTargetId] : null);
    renderTargetList();
}

function setSelectedTarget(targetId, shouldFocus) {
    if (!droneTargets[targetId]) {
        return;
    }

    selectedTargetId = targetId;
    renderTargetPanel();

    if (shouldFocus && droneMarkers[targetId]) {
        map.panTo(droneMarkers[targetId].getLatLng(), {animate: true});
    }
}

function rerenderAllDroneTargets() {
    Object.keys(droneMarkers).forEach(function(id) {
        map.removeLayer(droneMarkers[id]);
        delete droneMarkers[id];
    });
    renderTargetPanel();
}

// ==========================================
// 【供 Qt C++ 调用的核心更新接口】
// ==========================================
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

// 暴露给 Qt 调用以更新右下角仪表的函数
function updateDashboard(yaw, pitch) {
    if (typeof yaw !== 'number' || typeof pitch !== 'number') return;
    
    // web-ppl 源码中有角度的初始偏移量逻辑 (pitch-135, horizontal-45)
    // 这里保留这个偏移量，确保指针指向正确
    document.getElementById('needle-pitch').style.transform = `rotate(${pitch - 135}deg)`;
    document.getElementById('needle-yaw').style.transform = `rotate(${yaw - 45}deg)`;
    
    document.getElementById('val-pitch').innerText = pitch.toFixed(0) + '°';
    document.getElementById('val-yaw').innerText = yaw.toFixed(0) + '°';
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

// 放大
document.getElementById('btn-zoom-in').addEventListener('click', function() {
    map.zoomIn();
});

// 缩小
document.getElementById('btn-zoom-out').addEventListener('click', function() {
    map.zoomOut();
});

// 定位 (平移回到目标中心)
document.getElementById('btn-locate').addEventListener('click', function() {
    var currentPos = myMarker.getLatLng();
    map.setView(currentPos, map.getZoom(), { animate: true });
});

// 【终极安全全屏通信】：不再使用 alert()，改用改变标题 (Title) 的方式！
// 因为 Qt WebEngine 会自动监控网页的 title 属性变化，这个过程是 100% 异步且底层绝对安全的，
// 绝不会引发 IPC channel message 的崩溃。
var isFullscreen = false;
document.getElementById('btn-fullscreen').addEventListener('click', function() {
    isFullscreen = !isFullscreen;
    if (isFullscreen) {
        document.title = "CMD:FULLSCREEN_ON";
    } else {
        document.title = "CMD:FULLSCREEN_OFF";
    }
});

renderTargetPanel();
