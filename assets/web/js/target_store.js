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
var kSingleTargetRemovalDelayMs = 20000;

function setWarningClearDelayMs(delayMs) {
    var normalized = Number(delayMs);
    if (!isFinite(normalized) || normalized < 0) {
        return;
    }
    kSingleTargetRemovalDelayMs = normalized;
}

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

function getTargetDisplayId(target) {
    var rawId = String(target.targetUniqueId || '').trim();
    if (rawId) {
        return rawId;
    }
    return String(target.targetId || '');
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

