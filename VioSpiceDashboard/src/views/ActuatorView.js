/**
 * ActuatorView.js
 * Manages motors, LCDs, and DAC visual states.
 */
export class ActuatorView {
    constructor(canvas) {
        this.canvas = canvas || document.getElementById('schematic-content');
        this.dacHistory = new Array(50).fill(0.5);
    }

    update(data) {
        if (!data) return;
        this._updateLcdDisplays(data.lcd);
        this._updateMotors(data.digital_outputs);
        this._updateDacMonitor(data.dac);
    }

    _updateMotors(pins) {
        if (!pins) return;
        // DC Motor - Pin 16
        const dcNodes = this.canvas.querySelectorAll('.node-type-motor_dc');
        dcNodes.forEach(node => {
            node.classList.toggle('motor-dc-running', pins[16] === 1);
        });

        // Stepper - Pins 18-21
        const stepperNodes = this.canvas.querySelectorAll('.node-type-motor_stepper');
        stepperNodes.forEach(node => {
            const p = [pins[18], pins[19], pins[20], pins[21]];
            let angle = 0;
            if (p[0]) angle = 0; else if (p[1]) angle = 90; else if (p[2]) angle = 180; else if (p[3]) angle = 270;
            const shaft = node.querySelector('.motor-shaft');
            if (shaft) shaft.style.transform = `rotate(${angle}deg)`;
        });
    }

    _updateDacMonitor(dac) {
        const dacNodes = this.canvas.querySelectorAll('.node-type-dac_monitor');
        if (dacNodes.length === 0) return;

        const voltage = dac ? dac.voltage : 0.5;
        this.dacHistory.push(voltage);
        if (this.dacHistory.length > 50) this.dacHistory.shift();

        dacNodes.forEach(node => {
            const wave = node.querySelector('.dac-wave');
            if (wave) {
                const width = 140, height = 60, step = width / 50;
                const points = this.dacHistory.map((v, i) => {
                    const x = 10 + i * step;
                    const y = 30 + (1 - v) * height;
                    return `${x},${y}`;
                }).join(' ');
                wave.setAttribute('points', points);
            }
            const valueEl = node.querySelector('.dac-value');
            if (valueEl) {
                valueEl.textContent = `${(voltage * 5.0).toFixed(2)}V`;
            }
        });
    }

    _updateLcdDisplays(lcd) {
        if (!lcd || !lcd.enabled) {
            this.canvas.querySelectorAll('.lcd-seg').forEach(s => s.classList.remove('active'));
            return;
        }
        const nodes = this.canvas.querySelectorAll('.node-type-lcd_glass');
        nodes.forEach(node => {
            for (let d = 0; d < 4; d++) {
                const segments = ['A','B','C','D','E','F','G','DP'];
                segments.forEach((s, i) => {
                    const com = i % 4, seg_idx = (d * 2) + Math.floor(i / 4);
                    const isActive = (lcd.data[com] & (1 << seg_idx)) !== 0;
                    const path = node.querySelector(`.digit-${d} .seg-${s}`);
                    if (path) path.classList.toggle('active', isActive);
                });
            }
        });
    }
}
