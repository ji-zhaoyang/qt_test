var map = L.map('map', {
    zoomControl: true,
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

var pilotMarker = null;
var qrCodeInstance = null;

function buildAmapNavigationUri(lng, lat, name) {
    return 'https://uri.amap.com/marker?position=' +
        encodeURIComponent(String(lng) + ',' + String(lat)) +
        '&name=' + encodeURIComponent(name || '飞手位置');
}

function formatCoordinate(value) {
    var num = Number(value);
    if (!isFinite(num)) {
        return '--';
    }
    return num.toFixed(6);
}

function renderQrCode(text) {
    var container = document.getElementById('qrcode');
    if (!container) {
        return;
    }

    container.innerHTML = '';
    qrCodeInstance = new QRCode(container, {
        text: text,
        width: 160,
        height: 160,
        colorDark: '#000000',
        colorLight: '#ffffff',
        correctLevel: QRCode.CorrectLevel.M
    });
}

function setPilotLocation(lat, lng, label) {
    var safeLat = Number(lat);
    var safeLng = Number(lng);
    var title = label || '飞手位置';

    document.getElementById('pilot-lng').textContent = formatCoordinate(safeLng);
    document.getElementById('pilot-lat').textContent = formatCoordinate(safeLat);

    if (pilotMarker) {
        map.removeLayer(pilotMarker);
        pilotMarker = null;
    }

    pilotMarker = L.circleMarker([safeLat, safeLng], {
        radius: 10,
        weight: 3,
        color: '#ffffff',
        fillColor: '#f2994a',
        fillOpacity: 1
    }).addTo(map).bindTooltip(title, { direction: 'top' });

    map.setView([safeLat, safeLng], 16);
    renderQrCode(buildAmapNavigationUri(safeLng, safeLat, title));
}
