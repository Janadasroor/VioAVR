/**
 * AnalogScopeView.js
 * Manages the dedicated Analog Oscilloscope view.
 */
export class AnalogScopeView {
    constructor() {
        this.container = document.getElementById('view-analog');
        this.history = new Array(300).fill(0.5);
        this.canvas = null;
        this.ctx = null;
        this.isFrozen = false;
        this.triggerPoint = -1;
    }

    init() {
        if (!this.container) return;
        this.container.innerHTML = `
            <div class="scope-page-container">
                <div class="scope-controls">
                    <div class="control-group">
                        <label>V/Div</label>
                        <div class="knob">1V</div>
                    </div>
                    <div class="control-group">
                        <label>T/Div</label>
                        <div class="knob">5ms</div>
                    </div>
                </div>
                <div class="scope-screen">
                    <canvas id="analog-page-canvas"></canvas>
                    <div class="screen-overlay">
                        <div class="measurement">Vpp: 3.30V</div>
                        <div class="measurement">Freq: 1.02kHz</div>
                    </div>
                </div>
            </div>`;
        
        this.canvas = document.getElementById('analog-page-canvas');
        if (this.canvas) {
            this.ctx = this.canvas.getContext('2d');
            this._resize();
        }
    }

    _resize() {
        if (!this.canvas) return;
        const rect = this.canvas.parentElement.getBoundingClientRect();
        this.canvas.width = rect.width;
        this.canvas.height = rect.height;
    }

    freeze() {
        if (this.isFrozen) return;
        this.isFrozen = true;
        this.triggerPoint = this.history.length - 1;
        this._render();
    }

    unfreeze() {
        this.isFrozen = false;
        this.triggerPoint = -1;
    }

    update(dac) {
        if (this.isFrozen || !this.ctx) return;
        const val = dac ? dac.voltage : 0.5;
        this.history.push(val);
        if (this.history.length > 300) this.history.shift();
        this._render();
    }

    _render() {
        if (!this.ctx) return;
        const { width, height } = this.canvas;
        const ctx = this.ctx;
        ctx.clearRect(0, 0, width, height);

        // Grid
        ctx.strokeStyle = 'rgba(0, 255, 0, 0.1)';
        ctx.lineWidth = 1;
        for (let x = 0; x < width; x += 40) {
            ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, height); ctx.stroke();
        }
        for (let y = 0; y < height; y += 40) {
            ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(width, y); ctx.stroke();
        }

        // Trace
        ctx.strokeStyle = this.isFrozen ? '#ff3e3e' : '#00ff41';
        ctx.lineWidth = 3;
        ctx.shadowBlur = 10;
        ctx.shadowColor = this.isFrozen ? '#ff3e3e' : '#00ff41';
        
        ctx.beginPath();
        const step = width / 300;
        this.history.forEach((v, i) => {
            const x = i * step;
            const y = height - (v * height * 0.8 + height * 0.1);
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        });
        ctx.stroke();
        ctx.shadowBlur = 0;

        // Trigger Marker
        if (this.isFrozen) {
            ctx.fillStyle = 'rgba(255, 62, 62, 0.2)';
            ctx.fillRect(0, 0, width, height);
            
            ctx.fillStyle = '#ff3e3e';
            ctx.font = 'bold 24px "Outfit", sans-serif';
            ctx.textAlign = 'center';
            ctx.fillText('FAULT TRIGGER', width / 2, height / 2);
            
            // Vertical line at trigger
            const tx = this.triggerPoint * step;
            ctx.strokeStyle = '#fff';
            ctx.setLineDash([5, 5]);
            ctx.beginPath(); ctx.moveTo(tx, 0); ctx.lineTo(tx, height); ctx.stroke();
            ctx.setLineDash([]);
        }
    }
}
