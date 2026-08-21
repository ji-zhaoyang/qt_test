(function () {
    'use strict';

    var DEFAULT_CENTER = [30.342, 120.089];
    var DEFAULT_ZOOM = 13;
    var map = null;
    var darkMapGroup = null;
    var heatLayers = [];
    var markerLayer = null;
    var plotMarkers = [];
    var showMarkers = false;
    var mapReady = false;
    var pendingCenter = null;

    function isValidCoordinate(lat, lng) {
        return isFinite(lat) && isFinite(lng) &&
            lat >= -90 && lat <= 90 &&
            lng >= -180 && lng <= 180 &&
            !(Math.abs(lat) < 0.000001 && Math.abs(lng) < 0.000001);
    }

    function escapeHtml(text) {
        return String(text)
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;')
            .replace(/'/g, '&#39;');
    }

    function getPixelByMeter(meter, leafletMap) {
        var bound = leafletMap.getBounds();
        var northWest = bound.getNorthWest();
        var northEast = bound.getNorthEast();
        var xMeter = leafletMap.distance(northWest, northEast);
        var size = leafletMap.getSize();
        return (size.x / xMeter) * meter;
    }

    function genHeatLayerByReside(markers, layers, leafletMap) {
        if (!leafletMap || !markers.length) {
            return;
        }

        var radiusMeters = 200;
        var max = 0;
        var data = markers.map(function (item) {
            if (item.value > max) {
                max = item.value;
            }
            return [item.lat, item.lng, item.value];
        });
        var radius = getPixelByMeter(radiusMeters, leafletMap);
        var option = {
            minOpacity: 0.05,
            radius: radius,
            blur: radius,
            max: max || 1
        };

        if (!layers.length) {
            layers[0] = window.genHeatLayer(data, option).addTo(leafletMap);
        } else {
            layers[0].setLatLngs(data);
            layers[0].setOptions(option);
            layers[0].redraw();
        }
    }

    function clearHeatLayer() {
        if (heatLayers.length && map) {
            map.removeLayer(heatLayers[0]);
            heatLayers.length = 0;
        }
    }

    function clearMarkerLayer() {
        if (markerLayer && map) {
            map.removeLayer(markerLayer);
            markerLayer = null;
        }
    }

    function mergePlotPoints(items) {
        var grouped = {};

        items.forEach(function (item) {
            var lat = Number(item.lat);
            var lng = Number(item.lng);
            if (!isValidCoordinate(lat, lng)) {
                return;
            }

            var key = lat.toFixed(5) + '_' + lng.toFixed(5);
            if (grouped[key]) {
                grouped[key].createTimeArr.push(item.createTime || '');
                grouped[key].value += 1;
            } else {
                grouped[key] = {
                    lat: lat,
                    lng: lng,
                    targetId: item.targetId || '',
                    model: item.droneModel || '',
                    value: 1,
                    createTimeArr: [item.createTime || '']
                };
            }
        });

        return Object.keys(grouped).map(function (key) {
            return grouped[key];
        });
    }

    function setEmptyVisible(visible) {
        var emptyNode = document.getElementById('stats-heatmap-empty');
        if (emptyNode) {
            emptyNode.style.display = visible ? 'flex' : 'none';
        }
    }

    function fitMapToPlots() {
        if (!map || !plotMarkers.length) {
            return;
        }

        var bounds = L.latLngBounds(plotMarkers.map(function (item) {
            return [item.lat, item.lng];
        }));
        map.fitBounds(bounds.pad(0.18));
    }

    function panToCenter(lat, lng) {
        if (!map || !isValidCoordinate(lat, lng)) {
            return;
        }
        map.setView([lat, lng], Math.max(map.getZoom(), DEFAULT_ZOOM));
    }

    function renderMarkers() {
        clearMarkerLayer();
        if (!map || !plotMarkers.length) {
            return;
        }

        markerLayer = L.layerGroup();
        plotMarkers.forEach(function (item) {
            var marker = L.circleMarker([item.lat, item.lng], {
                radius: Math.min(10, 4 + item.value),
                weight: 2,
                color: '#ffffff',
                fillColor: '#00A0FF',
                fillOpacity: 0.85
            });
            var latestTime = item.createTimeArr[item.createTimeArr.length - 1] || '';
            marker.bindPopup(
                '<div class="stats-heatmap-popup">' +
                '<div>' + escapeHtml(latestTime.split('T')[0] + ' ' + (latestTime.split('T')[1] || '').substring(0, 8)) + '</div>' +
                '<div>' + escapeHtml(item.model || '未知机型') + '</div>' +
                '<div>频次：' + item.value + '</div>' +
                '</div>'
            );
            markerLayer.addLayer(marker);
        });
        markerLayer.addTo(map);
    }

    function renderPlotLayers() {
        if (!map) {
            return;
        }

        clearHeatLayer();
        clearMarkerLayer();

        if (!plotMarkers.length) {
            setEmptyVisible(true);
            if (pendingCenter) {
                panToCenter(pendingCenter.lat, pendingCenter.lng);
            }
            return;
        }

        setEmptyVisible(false);

        if (showMarkers) {
            renderMarkers();
        } else {
            genHeatLayerByReside(plotMarkers, heatLayers, map);
        }
    }

    function createTileLayers() {
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
        return L.layerGroup([baseDarkLayer, labelsLayer]);
    }

    function initMap() {
        if (map) {
            return map;
        }

        map = L.map('stats-heatmap-map', {
            zoomControl: false,
            attributionControl: false
        }).setView(DEFAULT_CENTER, DEFAULT_ZOOM);

        darkMapGroup = createTileLayers();
        darkMapGroup.addTo(map);

        map.on('zoomend', function () {
            if (!showMarkers && plotMarkers.length) {
                genHeatLayerByReside(plotMarkers, heatLayers, map);
            }
        });

        mapReady = true;
        if (pendingCenter) {
            panToCenter(pendingCenter.lat, pendingCenter.lng);
            pendingCenter = null;
        }

        return map;
    }

    function refreshMapSize() {
        if (!map) {
            return;
        }
        map.invalidateSize(false);
        renderPlotLayers();
        if (plotMarkers.length) {
            fitMapToPlots();
        }
    }

    function bootHeatmapMap() {
        initMap();
        bindControls();
        renderPlotLayers();
        window.setTimeout(refreshMapSize, 120);
    }

    function bindControls() {
        var toggle = document.getElementById('stats-heatmap-marker-toggle');
        var locateButton = document.getElementById('stats-heatmap-locate');

        if (toggle && toggle.dataset.bound !== '1') {
            toggle.dataset.bound = '1';
            toggle.addEventListener('change', function () {
                showMarkers = !!toggle.checked;
                renderPlotLayers();
            });
        }

        if (locateButton && locateButton.dataset.bound !== '1') {
            locateButton.dataset.bound = '1';
            locateButton.addEventListener('click', function () {
                if (plotMarkers.length) {
                    fitMapToPlots();
                    return;
                }
                if (pendingCenter) {
                    panToCenter(pendingCenter.lat, pendingCenter.lng);
                } else {
                    map && map.setView(DEFAULT_CENTER, DEFAULT_ZOOM);
                }
            });
        }
    }

    window.updateStatsPlotFromQt = function (payload) {
        var items = (payload && payload.items) || [];
        plotMarkers = mergePlotPoints(items);

        if (payload && isValidCoordinate(Number(payload.centerLat), Number(payload.centerLng))) {
            pendingCenter = {
                lat: Number(payload.centerLat),
                lng: Number(payload.centerLng)
            };
        }

        initMap();
        bindControls();
        renderPlotLayers();

        window.setTimeout(function () {
            refreshMapSize();
        }, 120);
    };

    document.addEventListener('DOMContentLoaded', function () {
        bootHeatmapMap();

        var statsPage = document.querySelector('.stats-page');
        if (statsPage) {
            statsPage.addEventListener('scroll', function () {
                window.setTimeout(refreshMapSize, 80);
            });
        }
    });

    window.addEventListener('resize', function () {
        window.setTimeout(refreshMapSize, 80);
    });
})();
