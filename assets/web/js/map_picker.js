// 初始化地图选点，默认中心点杭州
var map = L.map('map').setView([30.342, 120.089], 15);

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
var satelliteGroup = L.layerGroup([baseSatelliteLayer, overlayLayer]);

// 默认显示深色态势图
darkMapGroup.addTo(map);

var isSatelliteMode = false;
document.getElementById('btn-toggle-map').addEventListener('click', function() {
    if (isSatelliteMode) {
        map.removeLayer(satelliteGroup);
        darkMapGroup.addTo(map);
        isSatelliteMode = false;
        this.title = "切换卫星实景图";
    } else {
        map.removeLayer(darkMapGroup);
        satelliteGroup.addTo(map);
        isSatelliteMode = true;
        this.title = "切换深色态势图";
    }
});

// 自定义图标 (类似百度地图/高德地图的选点图标)
var pickerIcon = L.divIcon({
    className: 'custom-picker-icon',
    html: "<div style='background-color: #ff9900; width: 16px; height: 16px; border-radius: 50%; border: 3px solid white; box-shadow: 0 0 6px rgba(0,0,0,0.8);'></div>",
    iconSize: [22, 22],
    iconAnchor: [11, 11]
});

var currentMarker = null;

// Qt 端调用此函数设置初始点
function setInitialPoint(lat, lng) {
    if (currentMarker) {
        map.removeLayer(currentMarker);
    }
    currentMarker = L.marker([lat, lng], {icon: pickerIcon}).addTo(map);
    map.setView([lat, lng], 15);
    // 更新 Qt 端显示的经纬度
    document.title = "COORD:" + lng.toFixed(6) + "," + lat.toFixed(6);
}

// 监听地图点击事件
map.on('click', function(e) {
    var lat = e.latlng.lat;
    var lng = e.latlng.lng;
    
    if (currentMarker) {
        currentMarker.setLatLng(e.latlng);
    } else {
        currentMarker = L.marker(e.latlng, {icon: pickerIcon}).addTo(map);
    }
    
    // 通过 document.title 将坐标发送回 Qt 端
    document.title = "COORD:" + lng.toFixed(6) + "," + lat.toFixed(6);
});
