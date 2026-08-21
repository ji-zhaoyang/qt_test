var map = L.map('map', { zoomControl: true }).setView([30.342, 120.089], 14);

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
L.layerGroup([baseDarkLayer, labelsLayer]).addTo(map);

var drawMode = '';
var currentLayer = null;
var polygonPoints = [];
var polygonMarkers = [];
var tempLayer = null;
var draftLayerGroup = L.layerGroup().addTo(map);
var circleCenter = null;
var rectStart = null;

var shapeStyle = {
    color: '#f2994a',
    weight: 2,
    fillColor: '#f2994a',
    fillOpacity: 0.25
};

function clearTempLayers() {
    if (tempLayer) {
        draftLayerGroup.removeLayer(tempLayer);
        map.removeLayer(tempLayer);
        tempLayer = null;
    }
}

function clearPolygonDraft() {
    polygonPoints = [];
    polygonMarkers = [];
    clearTempLayers();
    draftLayerGroup.clearLayers();
}

function removeCurrentShape() {
    if (currentLayer) {
        map.removeLayer(currentLayer);
        currentLayer = null;
    }
    notifyAreaChanged();
}

function setDrawMode(mode) {
    drawMode = mode;
    circleCenter = null;
    rectStart = null;
    clearPolygonDraft();
    clearTempLayers();
    document.querySelectorAll('.area-tool-btn').forEach(function(btn) {
        btn.classList.remove('active');
    });
    if (mode === 'polygon') {
        document.getElementById('tool-polygon').classList.add('active');
        map.doubleClickZoom.disable();
    } else if (mode === 'circle') {
        document.getElementById('tool-circle').classList.add('active');
        map.doubleClickZoom.enable();
    } else if (mode === 'rectangle') {
        document.getElementById('tool-rectangle').classList.add('active');
        map.doubleClickZoom.enable();
    } else {
        map.doubleClickZoom.enable();
    }
}

function latLngPairsFromLayer(layer, type) {
    if (type === 'circle') {
        var center = layer.getLatLng();
        return {
            type: 'circle',
            center: [center.lat, center.lng],
            radius: layer.getRadius()
        };
    }
    if (type === 'rectangle') {
        var bounds = layer.getBounds();
        return {
            type: 'rectangle',
            bounds: [
                [bounds.getSouthWest().lat, bounds.getSouthWest().lng],
                [bounds.getNorthEast().lat, bounds.getNorthEast().lng]
            ]
        };
    }
    var latlngs = layer.getLatLngs();
    var ring = Array.isArray(latlngs[0]) ? latlngs[0] : latlngs;
    return {
        type: 'polygon',
        points: ring.map(function(point) {
            return [point.lat, point.lng];
        })
    };
}

function commitShape(layer, type) {
    removeCurrentShape();
    currentLayer = layer;
    notifyAreaChanged();
    setDrawMode('');
}

function finishPolygon() {
    if (polygonPoints.length < 3) {
        return;
    }
    var layer = L.polygon(polygonPoints, shapeStyle).addTo(map);
    clearPolygonDraft();
    commitShape(layer, 'polygon');
}

function notifyAreaChanged() {
    var shape = getWhitelistAreaShape();
    document.title = shape ? ('AREA:' + JSON.stringify(shape)) : 'AREA:null';
}

function getWhitelistAreaShape() {
    if (currentLayer) {
        if (currentLayer instanceof L.Circle) {
            return latLngPairsFromLayer(currentLayer, 'circle');
        }
        if (currentLayer instanceof L.Rectangle) {
            return latLngPairsFromLayer(currentLayer, 'rectangle');
        }
        if (currentLayer instanceof L.Polygon) {
            return latLngPairsFromLayer(currentLayer, 'polygon');
        }
    }

    if (drawMode === 'polygon' && polygonPoints.length >= 3) {
        return {
            type: 'polygon',
            points: polygonPoints.map(function(point) {
                return [point.lat, point.lng];
            })
        };
    }

    return null;
}

function finalizeWhitelistAreaShape() {
    if (drawMode === 'polygon' && polygonPoints.length >= 3) {
        finishPolygon();
    }
    return getWhitelistAreaShape();
}

function setWhitelistAreaShape(shape) {
    removeCurrentShape();
    clearPolygonDraft();
    clearTempLayers();
    setDrawMode('');
    if (!shape || typeof shape !== 'object') {
        notifyAreaChanged();
        return;
    }

    if (shape.type === 'circle' && shape.center && shape.radius) {
        currentLayer = L.circle(shape.center, Object.assign({ radius: shape.radius }, shapeStyle)).addTo(map);
        map.fitBounds(currentLayer.getBounds(), { padding: [24, 24] });
    } else if (shape.type === 'rectangle' && shape.bounds && shape.bounds.length === 2) {
        currentLayer = L.rectangle(shape.bounds, shapeStyle).addTo(map);
        map.fitBounds(currentLayer.getBounds(), { padding: [24, 24] });
    } else if (shape.type === 'polygon' && shape.points && shape.points.length >= 3) {
        currentLayer = L.polygon(shape.points, shapeStyle).addTo(map);
        map.fitBounds(currentLayer.getBounds(), { padding: [24, 24] });
    }
    notifyAreaChanged();
}

map.on('click', function(event) {
    if (drawMode === 'polygon') {
        polygonPoints.push(event.latlng);
        var marker = L.circleMarker(event.latlng, {
            radius: 4,
            color: '#f2994a',
            fillColor: '#f2994a',
            fillOpacity: 1,
            weight: 1
        });
        draftLayerGroup.addLayer(marker);
        polygonMarkers.push(marker);
        clearTempLayers();
        if (polygonPoints.length >= 3) {
            tempLayer = L.polygon(polygonPoints, shapeStyle);
        } else if (polygonPoints.length >= 2) {
            tempLayer = L.polyline(polygonPoints, shapeStyle);
        }
        if (tempLayer) {
            draftLayerGroup.addLayer(tempLayer);
        }
        return;
    }

    if (drawMode === 'circle') {
        if (!circleCenter) {
            circleCenter = event.latlng;
            return;
        }
        var radius = circleCenter.distanceTo(event.latlng);
        clearTempLayers();
        commitShape(L.circle(circleCenter, Object.assign({ radius: radius }, shapeStyle)).addTo(map), 'circle');
        circleCenter = null;
        return;
    }

    if (drawMode === 'rectangle') {
        if (!rectStart) {
            rectStart = event.latlng;
            return;
        }
        var bounds = L.latLngBounds(rectStart, event.latlng);
        clearTempLayers();
        commitShape(L.rectangle(bounds, shapeStyle).addTo(map), 'rectangle');
        rectStart = null;
    }
});

map.on('mousemove', function(event) {
    if (drawMode === 'circle' && circleCenter) {
        clearTempLayers();
        tempLayer = L.circle(circleCenter, Object.assign({
            radius: circleCenter.distanceTo(event.latlng)
        }, shapeStyle)).addTo(map);
    }
    if (drawMode === 'rectangle' && rectStart) {
        clearTempLayers();
        tempLayer = L.rectangle(L.latLngBounds(rectStart, event.latlng), shapeStyle).addTo(map);
    }
});

map.on('dblclick', function(event) {
    if (drawMode === 'polygon' && polygonPoints.length >= 3) {
        if (event.originalEvent) {
            event.originalEvent.preventDefault();
        }
        finishPolygon();
    }
});

document.getElementById('tool-polygon').addEventListener('click', function() {
    if (drawMode === 'polygon' && polygonPoints.length >= 3) {
        finishPolygon();
        return;
    }
    setDrawMode('polygon');
});
document.getElementById('tool-circle').addEventListener('click', function() {
    setDrawMode('circle');
});
document.getElementById('tool-rectangle').addEventListener('click', function() {
    setDrawMode('rectangle');
});
document.getElementById('tool-delete').addEventListener('click', function() {
    removeCurrentShape();
    clearPolygonDraft();
    clearTempLayers();
    circleCenter = null;
    rectStart = null;
    setDrawMode('');
});

setTimeout(function() {
    map.invalidateSize();
}, 120);
