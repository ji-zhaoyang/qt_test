(function () {
    'use strict';

    var PIE_COLORS = ['#5470c6', '#91cc75', '#fac858', '#ee6666', '#73c0de', '#3ba272', '#fc8452', '#9a60b4'];

    var chartStore = {
        counter: null,
        detection: null
    };

    var KPI_GROUPS = [
        {
            key: 'total',
            color: '#00A0FF',
            metrics: [
                { title: '总发现架次', field: 'trackTotalCount', integer: true },
                { title: '总架数', field: 'droneTotalCount', integer: true, sub: true }
            ]
        },
        {
            key: 'dailyMax',
            color: '#FFFFFF',
            metrics: [
                { title: '单日最多架次', field: 'trackDailyMaxCount', integer: true },
                { title: '单日最多架数', field: 'droneDailyMaxCount', integer: true, sub: true }
            ]
        },
        {
            key: 'dailyAvg',
            color: '#34FDFF',
            metrics: [
                { title: '平均每日架次', field: 'trackDailyAvgCount', integer: false },
                { title: '平均每日架数', field: 'droneDailyAvgCount', integer: false, sub: true }
            ]
        },
        {
            key: 'stayTotal',
            color: '#FFE20C',
            metrics: [{ title: '滞留总时长(分钟)', field: 'totalStayTime', integer: false }]
        },
        {
            key: 'stayMax',
            color: '#FFFFFF',
            metrics: [{ title: '单次最长时长(分钟)', field: 'maxStayTime', integer: false }]
        },
        {
            key: 'stayAvg',
            color: '#45CBAE',
            metrics: [{ title: '滞留平均时长(分钟)', field: 'avgStayTime', integer: false }]
        }
    ];

    function formatValue(value, integer) {
        var num = Number(value);
        if (!isFinite(num)) {
            return integer ? '0' : '0.00';
        }
        return integer ? String(Math.round(num)) : num.toFixed(2);
    }

    function formatDateLabel(dateText) {
        if (!dateText) {
            return '';
        }
        var parts = String(dateText).split('T')[0].split('-');
        if (parts.length !== 3) {
            return dateText;
        }
        return parts[1] + '-' + parts[2];
    }

    function formatHourLabel(dateText) {
        if (!dateText) {
            return '';
        }
        var chunks = String(dateText).split('T');
        if (chunks.length > 1 && chunks[1]) {
            return chunks[1].substring(0, 5);
        }
        return '00:00';
    }

    function formatBucketLabel(dateText, granularity) {
        return granularity === 'hour' ? formatHourLabel(dateText) : formatDateLabel(dateText);
    }

    function formatTooltipTitle(dateText, granularity) {
        if (!dateText) {
            return '';
        }
        if (granularity === 'hour') {
            var chunks = String(dateText).split('T');
            var datePart = chunks[0] || dateText;
            var timePart = chunks.length > 1 && chunks[1] ? chunks[1].substring(0, 5) : '00:00';
            return datePart + ' ' + timePart;
        }
        return String(dateText).split('T')[0];
    }

    function buildDailyChartPayload(items, valueKeys, granularity) {
        var labels = [];
        var series = valueKeys.map(function (cfg) {
            return {
                name: cfg.name,
                color: cfg.color,
                fill: cfg.fill,
                data: [],
                yAxis: cfg.yAxis || 'left',
                integerAxis: !!cfg.integerAxis
            };
        });

        (items || []).forEach(function (item) {
            labels.push(formatBucketLabel(item.date, granularity));
            valueKeys.forEach(function (cfg, index) {
                var raw = item[cfg.field];
                series[index].data.push(Number(raw) || 0);
            });
        });

        var rawDates = (items || []).map(function (item) {
            return String(item.date || '');
        });

        return {
            labels: labels,
            rawDates: rawDates,
            series: series,
            granularity: granularity || 'day'
        };
    }

    function seriesHasPositiveValues(series) {
        return (series || []).some(function (s) {
            return (s.data || []).some(function (value) {
                return Number(value) > 0;
            });
        });
    }

    function seriesMaxValue(seriesItem) {
        var maxValue = 0;
        (seriesItem.data || []).forEach(function (value) {
            maxValue = Math.max(maxValue, Number(value) || 0);
        });
        return maxValue;
    }

    function getXLabelIndices(labelCount, plotWidth, granularity) {
        if (labelCount <= 1) {
            return [0];
        }

        var minGap = granularity === 'hour' ? 44 : 52;
        var maxLabels = Math.max(2, Math.floor(plotWidth / minGap));
        var step = Math.max(1, Math.ceil((labelCount - 1) / (maxLabels - 1)));
        var indices = [];
        var index = 0;

        for (index = 0; index < labelCount; index += step) {
            indices.push(index);
        }
        if (indices[indices.length - 1] !== labelCount - 1) {
            indices.push(labelCount - 1);
        }
        return indices;
    }

    function drawEmptyMessage(ctx, rect, message) {
        ctx.fillStyle = '#8c96a3';
        ctx.font = '13px Microsoft YaHei, sans-serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(message || '暂无数据', rect.width / 2, rect.height / 2);
    }

    function drawXAxisLabels(ctx, labels, xAt, plotTop, plotH, plotWidth, granularity) {
        var labelIndices = getXLabelIndices(labels.length, plotWidth, granularity);
        ctx.fillStyle = '#aeb6c4';
        ctx.font = '11px Microsoft YaHei, sans-serif';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'top';
        labelIndices.forEach(function (index) {
            ctx.fillText(labels[index], xAt(index), plotTop + plotH + 8);
        });
    }

    function drawYAxisTicks(ctx, options) {
        var gridCount = 4;
        var i;
        var gy;
        var tickValue;

        ctx.strokeStyle = 'rgba(255,255,255,0.08)';
        ctx.lineWidth = 1;
        for (i = 0; i <= gridCount; i += 1) {
            gy = options.plotTop + (options.plotH * i) / gridCount;
            ctx.beginPath();
            ctx.moveTo(options.plotLeft, gy);
            ctx.lineTo(options.plotLeft + options.plotW, gy);
            ctx.stroke();
        }

        for (i = 0; i <= gridCount; i += 1) {
            gy = options.plotTop + (options.plotH * i) / gridCount;
            tickValue = options.maxValue * (1 - i / gridCount);
            ctx.font = '11px Microsoft YaHei, sans-serif';
            ctx.textBaseline = 'middle';

            if (options.side === 'left') {
                ctx.fillStyle = options.color || '#8c96a3';
                ctx.textAlign = 'right';
                ctx.fillText(formatValue(tickValue, options.integerAxis), options.plotLeft - 8, gy);
            } else {
                ctx.fillStyle = options.color || '#8c96a3';
                ctx.textAlign = 'left';
                ctx.fillText(formatValue(tickValue, options.integerAxis), options.plotLeft + options.plotW + 8, gy);
            }
        }
    }

    function traceSmoothCurve(ctx, points) {
        if (!points.length) {
            return;
        }
        if (points.length === 1) {
            ctx.lineTo(points[0].x, points[0].y);
            return;
        }
        if (points.length === 2) {
            ctx.lineTo(points[1].x, points[1].y);
            return;
        }

        var i;
        var p0;
        var p1;
        var p2;
        var p3;
        var cp1x;
        var cp1y;
        var cp2x;
        var cp2y;

        for (i = 0; i < points.length - 1; i += 1) {
            p0 = points[i === 0 ? i : i - 1];
            p1 = points[i];
            p2 = points[i + 1];
            p3 = points[i + 2] || p2;
            cp1x = p1.x + (p2.x - p0.x) / 6;
            cp1y = p1.y + (p2.y - p0.y) / 6;
            cp2x = p2.x - (p3.x - p1.x) / 6;
            cp2y = p2.y - (p3.y - p1.y) / 6;
            ctx.bezierCurveTo(cp1x, cp1y, cp2x, cp2y, p2.x, p2.y);
        }
    }

    function drawSeriesLine(ctx, seriesItem, points, plotTop, plotH, hoverIndex) {
        var gradient = ctx.createLinearGradient(0, plotTop, 0, plotTop + plotH);
        gradient.addColorStop(0, seriesItem.fill || seriesItem.color);
        gradient.addColorStop(1, 'rgba(61, 72, 115, 0)');

        ctx.beginPath();
        ctx.moveTo(points[0].x, plotTop + plotH);
        ctx.lineTo(points[0].x, points[0].y);
        traceSmoothCurve(ctx, points);
        ctx.lineTo(points[points.length - 1].x, plotTop + plotH);
        ctx.closePath();
        ctx.fillStyle = gradient;
        ctx.fill();

        ctx.beginPath();
        ctx.moveTo(points[0].x, points[0].y);
        traceSmoothCurve(ctx, points);
        ctx.strokeStyle = seriesItem.color;
        ctx.lineWidth = 2;
        ctx.lineJoin = 'round';
        ctx.lineCap = 'round';
        ctx.stroke();

        points.forEach(function (point, index) {
            var active = hoverIndex === index;
            ctx.beginPath();
            ctx.arc(point.x, point.y, active ? 4.5 : 2.5, 0, Math.PI * 2);
            ctx.fillStyle = seriesItem.color;
            ctx.fill();
            if (active) {
                ctx.strokeStyle = '#ffffff';
                ctx.lineWidth = 1.5;
                ctx.stroke();
            }
        });
    }

    function ensureTooltip(containerId) {
        var container = document.getElementById(containerId);
        if (!container) {
            return null;
        }
        var tooltip = container.querySelector('.stats-chart-tooltip');
        if (!tooltip) {
            tooltip = document.createElement('div');
            tooltip.className = 'stats-chart-tooltip';
            container.appendChild(tooltip);
        }
        return tooltip;
    }

    function hideTooltip(containerId) {
        var tooltip = ensureTooltip(containerId);
        if (tooltip) {
            tooltip.classList.remove('stats-chart-tooltip--visible');
        }
    }

    function showTooltip(containerId, event, layout, hoverIndex) {
        var tooltip = ensureTooltip(containerId);
        var container = document.getElementById(containerId);
        if (!tooltip || !container || hoverIndex < 0) {
            hideTooltip(containerId);
            return;
        }

        var title = formatTooltipTitle(layout.rawDates[hoverIndex] || layout.labels[hoverIndex] || '', layout.granularity);
        var rows = layout.series.map(function (seriesItem) {
            var value = seriesItem.data[hoverIndex];
            return (
                '<div class="stats-chart-tooltip__row">' +
                '<span class="stats-chart-tooltip__dot" style="background:' + seriesItem.color + '"></span>' +
                '<span>' + seriesItem.name + '：' + formatValue(value, seriesItem.integerAxis) + '</span>' +
                '</div>'
            );
        }).join('');

        tooltip.innerHTML =
            '<div class="stats-chart-tooltip__title">' + title + '</div>' + rows;
        tooltip.classList.add('stats-chart-tooltip--visible');

        var containerRect = container.getBoundingClientRect();
        var tooltipRect = tooltip.getBoundingClientRect();
        var offsetX = event.clientX - containerRect.left + 14;
        var offsetY = event.clientY - containerRect.top - tooltipRect.height - 12;

        if (offsetX + tooltipRect.width > containerRect.width - 8) {
            offsetX = event.clientX - containerRect.left - tooltipRect.width - 14;
        }
        if (offsetY < 8) {
            offsetY = event.clientY - containerRect.top + 14;
        }

        tooltip.style.left = Math.max(8, offsetX) + 'px';
        tooltip.style.top = Math.max(8, offsetY) + 'px';
    }

    function findNearestIndex(x, layout) {
        if (x < layout.plotLeft || x > layout.plotLeft + layout.plotW) {
            return -1;
        }

        var bestIndex = 0;
        var bestDistance = Infinity;
        layout.xPositions.forEach(function (px, index) {
            var distance = Math.abs(x - px);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestIndex = index;
            }
        });
        return bestIndex;
    }

    function bindChartInteraction(containerId, canvas) {
        if (canvas.dataset.statsBound === '1') {
            return;
        }
        canvas.dataset.statsBound = '1';
        canvas.style.cursor = 'crosshair';

        canvas.addEventListener('mousemove', function (event) {
            var chartKey = containerId === 'counter-chart' ? 'counter' : 'detection';
            var chartState = chartStore[chartKey];
            var layout = chartState && chartState._layout;
            if (!layout || layout.empty) {
                hideTooltip(containerId);
                return;
            }

            var rect = canvas.getBoundingClientRect();
            var hoverIndex = findNearestIndex(event.clientX - rect.left, layout);
            if (hoverIndex < 0) {
                chartState._hoverIndex = -1;
                drawLineChart(containerId, chartState, -1);
                hideTooltip(containerId);
                return;
            }

            chartState._hoverIndex = hoverIndex;
            drawLineChart(containerId, chartState, hoverIndex);
            showTooltip(containerId, event, layout, hoverIndex);
        });

        canvas.addEventListener('mouseleave', function () {
            var chartKey = containerId === 'counter-chart' ? 'counter' : 'detection';
            var chartState = chartStore[chartKey];
            if (chartState) {
                chartState._hoverIndex = -1;
                drawLineChart(containerId, chartState, -1);
            }
            hideTooltip(containerId);
        });
    }

    function ensureCanvas(containerId) {
        var container = document.getElementById(containerId);
        if (!container) {
            return null;
        }
        var canvas = container.querySelector('canvas');
        if (!canvas) {
            canvas = document.createElement('canvas');
            container.appendChild(canvas);
        }
        bindChartInteraction(containerId, canvas);
        return { container: container, canvas: canvas };
    }

    function drawHoverGuide(ctx, x, plotTop, plotH) {
        ctx.save();
        ctx.strokeStyle = 'rgba(255,255,255,0.22)';
        ctx.lineWidth = 1;
        ctx.setLineDash([4, 4]);
        ctx.beginPath();
        ctx.moveTo(x, plotTop);
        ctx.lineTo(x, plotTop + plotH);
        ctx.stroke();
        ctx.restore();
    }

    function drawLineChart(containerId, chartState, hoverIndex) {
        var parts = ensureCanvas(containerId);
        if (!parts) {
            return;
        }

        if (typeof hoverIndex !== 'number') {
            hoverIndex = chartState._hoverIndex >= 0 ? chartState._hoverIndex : -1;
        }

        var rect = parts.container.getBoundingClientRect();
        if (rect.width <= 0 || rect.height <= 0) {
            return;
        }

        var dpr = window.devicePixelRatio || 1;
        parts.canvas.width = Math.max(1, Math.floor(rect.width * dpr));
        parts.canvas.height = Math.max(1, Math.floor(rect.height * dpr));
        parts.canvas.style.width = rect.width + 'px';
        parts.canvas.style.height = rect.height + 'px';

        var ctx = parts.canvas.getContext('2d');
        ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
        ctx.clearRect(0, 0, rect.width, rect.height);

        var labels = chartState.labels || [];
        var rawDates = chartState.rawDates || labels;
        var series = chartState.series || [];
        chartState._layout = { empty: true };

        if (!labels.length || !series.length) {
            drawEmptyMessage(ctx, rect, chartState.emptyText);
            hideTooltip(containerId);
            return;
        }

        if (chartState.treatZeroAsEmpty && !seriesHasPositiveValues(series)) {
            drawEmptyMessage(ctx, rect, chartState.emptyText || '暂无数据');
            hideTooltip(containerId);
            return;
        }

        var dualAxis = !!chartState.dualAxis;
        var padding = dualAxis
            ? { top: 18, right: 46, bottom: 30, left: 46 }
            : { top: 18, right: 16, bottom: 30, left: 42 };
        var plotLeft = padding.left;
        var plotTop = padding.top;
        var plotW = Math.max(10, rect.width - padding.left - padding.right);
        var plotH = Math.max(10, rect.height - padding.top - padding.bottom);

        var leftSeries = series.filter(function (s) {
            return (s.yAxis || 'left') === 'left';
        });
        var rightSeries = series.filter(function (s) {
            return s.yAxis === 'right';
        });
        var maxLeft = 1;
        var maxRight = 1;

        leftSeries.forEach(function (s) {
            maxLeft = Math.max(maxLeft, seriesMaxValue(s));
        });
        if (dualAxis) {
            rightSeries.forEach(function (s) {
                maxRight = Math.max(maxRight, seriesMaxValue(s));
            });
        } else {
            series.forEach(function (s) {
                maxLeft = Math.max(maxLeft, seriesMaxValue(s));
            });
        }

        function xAt(index) {
            if (labels.length === 1) {
                return plotLeft + plotW / 2;
            }
            return plotLeft + (plotW * index) / (labels.length - 1);
        }

        function yAtLeft(value) {
            var ratio = (Number(value) || 0) / maxLeft;
            return plotTop + plotH - plotH * ratio;
        }

        function yAtRight(value) {
            var ratio = (Number(value) || 0) / maxRight;
            return plotTop + plotH - plotH * ratio;
        }

        function yAtSingle(value) {
            return yAtLeft(value);
        }

        var leftAxisSeries = leftSeries[0];
        var rightAxisSeries = rightSeries[0];
        var xPositions = labels.map(function (_label, index) {
            return xAt(index);
        });
        var layoutSeries = [];

        drawYAxisTicks(ctx, {
            side: 'left',
            plotLeft: plotLeft,
            plotTop: plotTop,
            plotW: plotW,
            plotH: plotH,
            maxValue: maxLeft,
            integerAxis: leftAxisSeries ? leftAxisSeries.integerAxis : false,
            color: dualAxis && leftAxisSeries ? leftAxisSeries.color : '#8c96a3'
        });

        if (dualAxis) {
            drawYAxisTicks(ctx, {
                side: 'right',
                plotLeft: plotLeft,
                plotTop: plotTop,
                plotW: plotW,
                plotH: plotH,
                maxValue: maxRight,
                integerAxis: rightAxisSeries ? rightAxisSeries.integerAxis : true,
                color: rightAxisSeries ? rightAxisSeries.color : '#8c96a3'
            });
        }

        if (hoverIndex >= 0 && hoverIndex < xPositions.length) {
            drawHoverGuide(ctx, xPositions[hoverIndex], plotTop, plotH);
        }

        series.forEach(function (s) {
            var yAt = dualAxis
                ? (s.yAxis === 'right' ? yAtRight : yAtLeft)
                : yAtSingle;
            var points = (s.data || []).map(function (value, index) {
                return { x: xAt(index), y: yAt(value), value: value };
            });
            if (!points.length) {
                return;
            }
            layoutSeries.push({
                name: s.name,
                color: s.color,
                data: s.data,
                integerAxis: !!s.integerAxis
            });
            drawSeriesLine(ctx, s, points, plotTop, plotH, hoverIndex);
        });

        drawXAxisLabels(ctx, labels, xAt, plotTop, plotH, plotW, chartState.granularity || 'day');

        chartState._layout = {
            empty: false,
            plotLeft: plotLeft,
            plotTop: plotTop,
            plotW: plotW,
            plotH: plotH,
            labels: labels,
            rawDates: rawDates,
            series: layoutSeries,
            xPositions: xPositions,
            granularity: chartState.granularity || 'day'
        };
    }

    function redrawAllCharts() {
        if (chartStore.counter) {
            chartStore.counter._hoverIndex = -1;
            hideTooltip('counter-chart');
            drawLineChart('counter-chart', chartStore.counter, -1);
        }
        if (chartStore.detection) {
            chartStore.detection._hoverIndex = -1;
            hideTooltip('detection-chart');
            drawLineChart('detection-chart', chartStore.detection, -1);
        }
    }

    function renderPie(items) {
        var pieNode = document.getElementById('model-pie');
        var legendNode = document.getElementById('model-legend');
        if (!pieNode || !legendNode) {
            return;
        }

        pieNode.innerHTML = '';
        legendNode.innerHTML = '';

        if (!items || !items.length) {
            pieNode.innerHTML = '<div class="stats-empty">暂无机型数据</div>';
            return;
        }

        var total = items.reduce(function (sum, item) {
            return sum + Number(item.count || 0);
        }, 0);
        if (total <= 0) {
            pieNode.innerHTML = '<div class="stats-empty">暂无机型数据</div>';
            return;
        }

        var radius = 88;
        var cx = 100;
        var cy = 100;
        var startAngle = -Math.PI / 2;
        var paths = [];

        items.forEach(function (item, index) {
            var value = Number(item.count || 0);
            var slice = (value / total) * Math.PI * 2;
            var endAngle = startAngle + slice;
            var x1 = cx + Math.cos(startAngle) * radius;
            var y1 = cy + Math.sin(startAngle) * radius;
            var x2 = cx + Math.cos(endAngle) * radius;
            var y2 = cy + Math.sin(endAngle) * radius;
            var largeArc = slice > Math.PI ? 1 : 0;
            var color = PIE_COLORS[index % PIE_COLORS.length];
            paths.push(
                '<path d="M' + cx + ' ' + cy + ' L' + x1 + ' ' + y1 +
                ' A' + radius + ' ' + radius + ' 0 ' + largeArc + ' 1 ' + x2 + ' ' + y2 +
                ' Z" fill="' + color + '"></path>'
            );

            var percent = ((value / total) * 100).toFixed(2);
            var legendItem = document.createElement('div');
            legendItem.className = 'stats-legend__item';
            legendItem.innerHTML =
                '<span class="stats-legend__dot" style="background:' + color + '"></span>' +
                '<span class="stats-legend__name" title="' + item.model + '">' + item.model + '</span>' +
                '<span class="stats-legend__value">' + percent + '% (' + value + ')</span>';
            legendNode.appendChild(legendItem);
            startAngle = endAngle;
        });

        pieNode.innerHTML =
            '<svg viewBox="0 0 200 200" role="img" aria-label="机型统计饼图">' + paths.join('') + '</svg>';
    }

    function renderTrackCards(data) {
        var container = document.getElementById('track-cards');
        if (!container) {
            return;
        }

        container.innerHTML = '';
        KPI_GROUPS.forEach(function (group) {
            var card = document.createElement('div');
            card.className = 'stats-kpi-card';

            group.metrics.forEach(function (metric) {
                var metricNode = document.createElement('div');
                metricNode.className = 'stats-kpi-card__metric';

                var title = document.createElement('div');
                title.className = 'stats-kpi-card__title';
                title.textContent = metric.title;

                var value = document.createElement('div');
                value.className = 'stats-kpi-card__value' + (metric.sub ? ' stats-kpi-card__value--sub' : '');
                value.style.color = group.color;
                value.textContent = formatValue(data[metric.field], metric.integer);

                metricNode.appendChild(title);
                metricNode.appendChild(value);
                card.appendChild(metricNode);
            });

            container.appendChild(card);
        });
    }

    function renderCounterChart(items, granularity) {
        chartStore.counter = buildDailyChartPayload(items, [
            {
                name: '反制次数',
                field: 'count',
                color: '#00A0FF',
                fill: 'rgba(0,160,255,0.35)',
                integerAxis: true
            }
        ], granularity || 'day');
        chartStore.counter.treatZeroAsEmpty = true;
        chartStore.counter.emptyText = '暂无反制记录';
        drawLineChart('counter-chart', chartStore.counter);
    }

    function renderDetectionChart(items, granularity) {
        chartStore.detection = buildDailyChartPayload(items, [
            {
                name: '滞留时长(分钟)',
                field: 'stayTime',
                color: '#FFE20C',
                fill: 'rgba(255,226,12,0.35)',
                yAxis: 'left'
            },
            {
                name: '探测频次',
                field: 'trackCount',
                color: '#34FDFF',
                fill: 'rgba(52,253,255,0.25)',
                yAxis: 'right',
                integerAxis: true
            }
        ], granularity || 'day');
        chartStore.detection.dualAxis = true;
        drawLineChart('detection-chart', chartStore.detection);
    }

    function renderInitialState() {
        var page = document.querySelector('.stats-page');
        var pieNode = document.getElementById('model-pie');
        var legendNode = document.getElementById('model-legend');
        var trackCards = document.getElementById('track-cards');

        if (page) {
            page.classList.add('stats-page--loading');
        }
        if (pieNode) {
            pieNode.innerHTML = '<div class="stats-empty stats-empty--pending">加载中...</div>';
        }
        if (legendNode) {
            legendNode.innerHTML = '';
        }
        if (trackCards) {
            trackCards.innerHTML = '<div class="stats-empty stats-empty--pending stats-empty--wide">加载中...</div>';
        }

        chartStore.counter = {
            labels: [],
            series: [],
            emptyText: '加载中...'
        };
        chartStore.detection = {
            labels: [],
            series: [],
            emptyText: '加载中...'
        };
        drawLineChart('counter-chart', chartStore.counter);
        drawLineChart('detection-chart', chartStore.detection);
    }

    window.updateStatsModelFromQt = function (payload) {
        renderPie((payload && payload.items) || []);
    };

    window.updateStatsTrackFromQt = function (payload) {
        renderTrackCards(payload || {});
    };

    window.updateStatsDetectionDailyFromQt = function (payload) {
        renderDetectionChart((payload && payload.items) || [], payload && payload.granularity);
    };

    window.updateStatsCounterDailyFromQt = function (payload) {
        renderCounterChart((payload && payload.items) || [], payload && payload.granularity);
    };

    window.setStatsLoadingFromQt = function (payload) {
        var page = document.querySelector('.stats-page');
        if (!page) {
            return;
        }
        page.classList.toggle('stats-page--loading', !!(payload && payload.loading));
    };

    window.addEventListener('resize', redrawAllCharts);

    document.addEventListener('DOMContentLoaded', renderInitialState);
})();
