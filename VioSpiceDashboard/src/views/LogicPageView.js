/**
 * LogicPageView.js
 * Manages the dedicated Logic Analyzer view.
 */
export class LogicPageView {
    constructor() {
        this.container = document.getElementById('view-logic');
        this.history = Array.from({ length: 8 }, () => new Array(200).fill(0));
        this.canvas = null;
        this.ctx = null;
    }

    init() {
        if (!this.container) return;
        this.container.innerHTML = `
            <div class="logic-page-container">
                <div class="logic-toolbar">
                    <div class="toolbar-group">
                        <span class="toolbar-label">Timebase</span>
                        <select class="logic-select"><option>1ms/div</option><option>10ms/div</option></select>
                    </div>
                    <div class="toolbar-group">
                        <span class="toolbar-label">Trigger</span>
                        <select class="logic-select"><option>CH0 Rising</option><option>None</option></select>
                    </div>
                </div>
                <div class="logic-viewport">
                    <canvas id="logic-page-canvas"></canvas>
                </div>
            </div>`;
        
        this.canvas = document.getElementById('logic-page-canvas');
        if (this.canvas) {
            this.ctx = this.canvas.getContext('2d');
            this._resize();
            window.addEventListener('resize', () => this._resize());
        }
    }

    _resize() {
        if (!this.canvas) return;
        const rect = this.canvas.parentElement.getBoundingClientRect();
        this.canvas.width = rect.width;
        this.canvas.height = rect.height;
    }

    update(pins) {
        if (!pins || !this.ctx) return;

        // Update history
        for (let i = 0; i < 8; i++) {
            this.history[i].push(pins[i] === 1 ? 1 : 0);
            if (this.history[i].length > 200) this.history[i].shift();
        }

        this._render();
    }

    _render() {
        const { width, height } = this.canvas;
        const ctx = this.ctx;
        ctx.clearRect(0, 0, width, height);

        const chHeight = height / 8;
        const step = width / 200;

        ctx.strokeStyle = '#00ff00';
        ctx.lineWidth = 2;

        for (let i = 0; i < 8; i++) {
            const yBase = (i + 1) * chHeight - 10;
            const trace = this.history[i];

            ctx.beginPath();
            ctx.moveTo(0, trace[0] ? yBase - 30 : yBase);

            trace.forEach((val, xIdx) => {
                const x = xIdx * step;
                const y = val ? yBase - 30 : yBase;
                
                if (xIdx > 0 && val !== trace[xIdx - 1]) {
                    ctx.lineTo(x, trace[xIdx - 1] ? yBase - 30 : yBase);
                    ctx.lineTo(x, y);
                } else {
                    ctx.lineTo(x, y);
                }
            });
            ctx.stroke();

            // Label
            ctx.fillStyle = 'rgba(255,255,255,0.5)';
            ctx.font = '10px JetBrains Mono';
            ctx.fillText(`CH${i}`, 10, yBase - 35);
        }

        // Grid lines
        ctx.strokeStyle = 'rgba(255,255,255,0.05)';
        ctx.lineWidth = 1;
        for (let x = 0; x < width; x += 50) {
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, height);
            ctx.stroke();
        }
    }
}
