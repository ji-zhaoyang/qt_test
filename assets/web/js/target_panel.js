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
