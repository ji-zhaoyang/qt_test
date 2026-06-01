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

// 在杭州的位置画一个红色图标，并把它保存到 myMarker 变量里
var myMarker = L.marker([30.342, 120.089], {icon: customIcon}).addTo(map);

// ==========================================
// 【供 Qt C++ 调用的核心更新接口】
// ==========================================
function updateMarker(lat, lng, alt) {
    console.log("js: 接收到 Qt 传入的坐标: lat=" + lat + " lng=" + lng + " alt=" + alt);

    if (typeof lat !== 'number' || typeof lng !== 'number' || isNaN(lat) || isNaN(lng)) {
        return;
    }

    if (lat < -90 || lat > 90 || lng < -180 || lng > 180) {
        console.warn("js: 坐标超出合法范围，已忽略: lat=" + lat + " lng=" + lng);
        return;
    }

    var newPos = [lat, lng];
    myMarker.setLatLng(newPos);

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
    // 【修改】：使用 myMarker.getLatLng() 获取坐标，然后通过 setView 或者 panTo 移动过去
    var currentPos = myMarker.getLatLng();
    map.setView(currentPos, map.getZoom(), { animate: true }); // 使用 setView 并保持当前缩放级别
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
