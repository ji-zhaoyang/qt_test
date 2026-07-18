var map = L.map('map', {
    zoomControl: false,
    attributionControl: false
}).setView([30.342, 120.089], 15);

var baseDarkLayer = L.tileLayer('../../../map/2d/satellite/{z}/{x}/{y}.jpg', {
    maxZoom: 16,
    minZoom: 1,
    errorTileUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII='
});

var labelsLayer = L.tileLayer('../../../map/2d/{z}/{x}/{y}.png', {
    maxZoom: 16,
    minZoom: 1,
    errorTileUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII='
});

var darkMapGroup = L.layerGroup([baseDarkLayer, labelsLayer]);

var baseSatelliteLayer = L.tileLayer('../../../map/3d/satellite/{z}/{x}/{y}.jpg', {
    maxZoom: 16,
    minZoom: 1,
    errorTileUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII='
});

var overlayLayer = L.tileLayer('../../../map/3d/overlay/{z}/{x}/{y}.png', {
    maxZoom: 16,
    minZoom: 1,
    errorTileUrl: 'data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII='
});

var satelliteGroup = L.layerGroup([baseSatelliteLayer, overlayLayer]);
darkMapGroup.addTo(map);

var isSatelliteMode = false;
document.getElementById('btn-toggle-map').addEventListener('click', function () {
    if (isSatelliteMode) {
        map.removeLayer(satelliteGroup);
        darkMapGroup.addTo(map);
        isSatelliteMode = false;
        this.title = '切换卫星实景图';
    } else {
        map.removeLayer(darkMapGroup);
        satelliteGroup.addTo(map);
        isSatelliteMode = true;
        this.title = '切换深色态势图';
    }
});

var replayPoints = [];
var routeLine = null;
var passedRouteLine = null;
var startMarker = null;
var endMarker = null;
var activeMarker = null;
var activeHalo = null;
var statusEl = document.getElementById('replay-status');

function clearReplayLayers() {
    if (routeLine) {
        map.removeLayer(routeLine);
        routeLine = null;
    }
    if (passedRouteLine) {
        map.removeLayer(passedRouteLine);
        passedRouteLine = null;
    }
    if (startMarker) {
        map.removeLayer(startMarker);
        startMarker = null;
    }
    if (endMarker) {
        map.removeLayer(endMarker);
        endMarker = null;
    }
    if (activeMarker) {
        map.removeLayer(activeMarker);
        activeMarker = null;
    }
    if (activeHalo) {
        map.removeLayer(activeHalo);
        activeHalo = null;
    }
}

function createTerminalMarker(latlng, color, label) {
    return L.circleMarker(latlng, {
        radius: 6,
        weight: 2,
        color: '#ffffff',
        fillColor: color,
        fillOpacity: 1
    }).bindTooltip(label, {
        permanent: false,
        direction: 'top'
    });
}

function ensureActiveMarker() {
    if (!activeMarker) {
        activeMarker = L.circleMarker([30.342, 120.089], {
            radius: 9,
            weight: 3,
            color: '#ffffff',
            fillColor: '#f2994a',
            fillOpacity: 1
        }).addTo(map);
    }

    if (!activeHalo) {
        activeHalo = L.circleMarker([30.342, 120.089], {
            radius: 16,
            weight: 2,
            color: '#ffd7b1',
            fillColor: '#f2994a',
            fillOpacity: 0.18
        }).addTo(map);
    }
}

function setStatusText(text) {
    statusEl.textContent = text || '等待回放数据';
}

function setReplayData(points) {
    replayPoints = Array.isArray(points) ? points : [];
    clearReplayLayers();

    if (!replayPoints.length) {
        setStatusText('当前记录没有可用于回放的轨迹点');
        map.setView([30.342, 120.089], 15);
        return;
    }

    var latlngs = replayPoints.map(function (point) {
        return [point.lat, point.lng];
    });

    routeLine = L.polyline(latlngs, {
        color: '#6b573e',
        weight: 4,
        opacity: 0.82
    }).addTo(map);

    passedRouteLine = L.polyline([latlngs[0]], {
        color: '#f5a253',
        weight: 5,
        opacity: 0.96
    }).addTo(map);

    startMarker = createTerminalMarker(latlngs[0], '#2ecc71', '起点').addTo(map);
    endMarker = createTerminalMarker(latlngs[latlngs.length - 1], '#eb5757', '终点').addTo(map);

    if (latlngs.length === 1) {
        map.setView(latlngs[0], 16);
    } else {
        map.fitBounds(routeLine.getBounds().pad(0.2));
    }

    setReplayIndex(0);
}

function setReplayIndex(index) {
    if (!replayPoints.length) {
        return;
    }

    var safeIndex = Math.max(0, Math.min(index, replayPoints.length - 1));
    var point = replayPoints[safeIndex];
    var latlng = [point.lat, point.lng];

    ensureActiveMarker();
    activeHalo.setLatLng(latlng);
    activeMarker.setLatLng(latlng);
    activeHalo.bringToFront();
    activeMarker.bringToFront();

    if (passedRouteLine) {
        passedRouteLine.setLatLngs(replayPoints.slice(0, safeIndex + 1).map(function (item) {
            return [item.lat, item.lng];
        }));
    }

    if (map.getBounds && !map.getBounds().pad(-0.15).contains(latlng)) {
        map.panTo(latlng, { animate: false });
    }

    setStatusText((point.label || '--') + '  ' + (point.subtitle || '轨迹点'));
}
