// ==========================================
// 【需求 1.7 扩展】实现距离测量功能
// ==========================================
var isMeasuring = false;
var measurePoints = []; // 保存点击的坐标点
var measureLines = []; // 保存绘制的折线
var measureMarkers = []; // 保存节点图标和距离提示
var measureTempLine = null; // 鼠标移动时的跟随线

// 计算两点之间的真实地理距离（米）
function calculateDistance(latlng1, latlng2) {
    return map.distance(latlng1, latlng2);
}

// 格式化距离显示
function formatDistance(meters) {
    if (meters > 1000) {
        return (meters / 1000).toFixed(2) + " km";
    } else {
        return meters.toFixed(0) + " m";
    }
}

// 清除测距数据和图层
function clearMeasure() {
    measurePoints = [];
    measureLines.forEach(function(line) { map.removeLayer(line); });
    measureMarkers.forEach(function(marker) { map.removeLayer(marker); });
    if (measureTempLine) { map.removeLayer(measureTempLine); measureTempLine = null; }
    measureLines = [];
    measureMarkers = [];
}

// 重新渲染整个测距线路（用于撤销操作后）
function redrawMeasure() {
    // 先清理地图上的图层，但保留数据数组
    measureLines.forEach(function(line) { map.removeLayer(line); });
    measureMarkers.forEach(function(marker) { map.removeLayer(marker); });
    measureLines = [];
    measureMarkers = [];

    var totalDist = 0;
    
    for (var i = 0; i < measurePoints.length; i++) {
        var latlng = measurePoints[i];
        var nodeIcon = L.divIcon({ className: 'measure-node-icon', iconSize: [10, 10], iconAnchor: [5, 5] });
        var marker = L.marker(latlng, { icon: nodeIcon, interactive: false }).addTo(map);
        measureMarkers.push(marker);

        if (i === 0) {
            marker.bindTooltip("起点<br>左键继续，Shift+左键删除", { permanent: true, direction: 'right', className: 'measure-tooltip', offset: [5, 0] }).openTooltip();
        } else {
            var prevPoint = measurePoints[i - 1];
            var segmentDist = calculateDistance(prevPoint, latlng);
            totalDist += segmentDist;

            var line = L.polyline([prevPoint, latlng], { color: '#000080', weight: 3, opacity: 0.8 }).addTo(map);
            measureLines.push(line);

            var tooltipContent = "<span style='color:#666;'>+" + formatDistance(segmentDist) + "</span><br>" + 
                                 "<span style='color:#000;font-size:14px;'>" + formatDistance(totalDist) + "</span>";
            
            marker.bindTooltip(tooltipContent, { permanent: true, direction: 'bottom', className: 'measure-tooltip', offset: [0, 5] }).openTooltip();
        }
    }
}

// 开启/关闭测距模式
document.getElementById('btn-measure').addEventListener('click', function() {
    isMeasuring = !isMeasuring;
    var btn = document.getElementById('btn-measure');
    
    if (isMeasuring) {
        btn.classList.add('measure-btn-active');
        map.getContainer().style.cursor = 'crosshair'; // 改变鼠标指针为十字准星
        clearMeasure(); // 开启时清空之前的历史，开始新的测量
    } else {
        btn.classList.remove('measure-btn-active');
        map.getContainer().style.cursor = ''; // 恢复默认指针
        clearMeasure(); // 关闭时直接清空所有痕迹
    }
});

// 鼠标点击事件：添加测距节点 或 撤销
map.on('click', function(e) {
    if (!isMeasuring) return;

    // 【新需求】Shift + 左键：删除上一个点（撤销）
    if (e.originalEvent.shiftKey) {
        if (measurePoints.length > 0) {
            measurePoints.pop(); // 移除最后一个点的数据
            redrawMeasure(); // 重新画一遍剩下的线
            
            // 撤销后，更新跟随虚线的起点
            if (measureTempLine) {
                if (measurePoints.length > 0) {
                    var lastPoint = measurePoints[measurePoints.length - 1];
                    measureTempLine.setLatLngs([lastPoint, e.latlng]);
                } else {
                    map.removeLayer(measureTempLine);
                    measureTempLine = null;
                }
            }
        }
        return; // 删完就退出，不要再加新点了
    }

    // 正常左键点击：添加新点
    var latlng = e.latlng;
    measurePoints.push(latlng);
    redrawMeasure();
});

// 鼠标移动事件：绘制动态跟随线
map.on('mousemove', function(e) {
    if (!isMeasuring || measurePoints.length === 0) return;
    
    var lastPoint = measurePoints[measurePoints.length - 1];
    var currentPos = e.latlng;

    if (measureTempLine) {
        measureTempLine.setLatLngs([lastPoint, currentPos]);
    } else {
        measureTempLine = L.polyline([lastPoint, currentPos], { color: '#000080', weight: 2, opacity: 0.5, dashArray: '5, 5' }).addTo(map);
    }
});
