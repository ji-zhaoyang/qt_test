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

// 解决离线环境下缺少默认图片的问题，使用 CSS 画一个红色的圆形点
var customIcon = L.divIcon({
    className: 'custom-div-icon',
    html: "<div style='background-color: red; width: 16px; height: 16px; border-radius: 50%; border: 3px solid white; box-shadow: 0 0 6px rgba(0,0,0,0.8);'></div>",
    iconSize: [22, 22],
    iconAnchor: [11, 11]
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

window.addEventListener('resize', function() {
    map.invalidateSize();
});
