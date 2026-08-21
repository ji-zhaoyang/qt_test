'use strict';

function simpleheat(canvas) {
    if (!(this instanceof simpleheat)) {
        return new simpleheat(canvas);
    }

    this._canvas = canvas = typeof canvas === 'string' ? document.getElementById(canvas) : canvas;
    this._ctx = canvas.getContext('2d');
    this._width = canvas.width;
    this._height = canvas.height;
    this._max = 1;
    this._data = [];
}

simpleheat.prototype = {
    defaultRadius: 25,

    defaultGradient: {
        0.4: 'blue',
        0.6: 'cyan',
        0.7: 'lime',
        0.8: 'yellow',
        1.0: 'red'
    },

    data: function (data) {
        this._data = data;
        return this;
    },

    max: function (max) {
        this._max = max;
        return this;
    },

    add: function (point) {
        this._data.push(point);
        return this;
    },

    clear: function () {
        this._data = [];
        return this;
    },

    radius: function (r, blur) {
        blur = blur === undefined ? 15 : blur;

        var circle = this._circle = this._createCanvas();
        var ctx = circle.getContext('2d');
        var r2 = this._r = r + blur;

        circle.width = circle.height = r2 * 2;
        ctx.shadowOffsetX = ctx.shadowOffsetY = r2 * 2;
        ctx.shadowBlur = blur;
        ctx.shadowColor = 'black';
        ctx.beginPath();
        ctx.arc(-r2, -r2, r, 0, Math.PI * 2, true);
        ctx.closePath();
        ctx.fill();

        return this;
    },

    resize: function () {
        this._width = this._canvas.width;
        this._height = this._canvas.height;
    },

    gradient: function (grad) {
        var canvas = this._createCanvas();
        var ctx = canvas.getContext('2d');
        var gradient = ctx.createLinearGradient(0, 0, 0, 256);
        var i;

        canvas.width = 1;
        canvas.height = 256;

        for (i in grad) {
            if (Object.prototype.hasOwnProperty.call(grad, i)) {
                gradient.addColorStop(+i, grad[i]);
            }
        }

        ctx.fillStyle = gradient;
        ctx.fillRect(0, 0, 1, 256);
        this._grad = ctx.getImageData(0, 0, 1, 256).data;

        return this;
    },

    draw: function (minOpacity) {
        if (!this._circle) {
            this.radius(this.defaultRadius);
        }
        if (!this._grad) {
            this.gradient(this.defaultGradient);
        }

        var ctx = this._ctx;
        var i;
        var len;
        var p;
        var colored;

        ctx.clearRect(0, 0, this._width, this._height);

        for (i = 0, len = this._data.length; i < len; i += 1) {
            p = this._data[i];
            ctx.globalAlpha = Math.min(
                Math.max(p[2] / this._max, minOpacity === undefined ? 0.05 : minOpacity),
                1
            );
            ctx.drawImage(this._circle, p[0] - this._r, p[1] - this._r);
        }

        colored = ctx.getImageData(0, 0, this._width, this._height);
        this._colorize(colored.data, this._grad);
        ctx.putImageData(colored, 0, 0);

        return this;
    },

    _colorize: function (pixels, gradient) {
        var i;
        var len;
        var j;

        for (i = 0, len = pixels.length; i < len; i += 4) {
            j = pixels[i + 3] * 4;
            if (j) {
                pixels[i] = gradient[j];
                pixels[i + 1] = gradient[j + 1];
                pixels[i + 2] = gradient[j + 2];
            }
        }
    },

    _createCanvas: function () {
        return document.createElement('canvas');
    }
};

window.simpleheat = simpleheat;
