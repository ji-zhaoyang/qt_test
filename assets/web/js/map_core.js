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

renderTargetPanel();
