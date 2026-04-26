/**
 * RegisterView.js
 * Renders CPU registers (PC, SP, SREG) and the GPR grid (R0-R31).
 */
export class RegisterView {
    constructor() {
        this.pcEl = document.getElementById('reg-pc');
        this.spEl = document.getElementById('reg-sp');
        this.sregEl = document.getElementById('reg-sreg');
        this.cyclesEl = document.getElementById('reg-cycles');
        this.gprGrid = document.getElementById('gpr-grid');
        this.gprEls = [];
    }

    init() {
        this._setupGprGrid();
    }

    _setupGprGrid() {
        if (!this.gprGrid) return;
        this.gprGrid.innerHTML = '';
        for (let i = 0; i < 32; i++) {
            const item = document.createElement('div');
            item.className = 'gpr-item';
            item.innerHTML = `<label>R${i}</label><span>00</span>`;
            this.gprGrid.appendChild(item);
            this.gprEls[i] = item.querySelector('span');
        }
    }

    update(telemetry) {
        if (this.pcEl) this.pcEl.textContent = `0x${telemetry.pc.toString(16).toUpperCase().padStart(4, '0')}`;
        if (this.spEl) this.spEl.textContent = `0x${telemetry.sp.toString(16).toUpperCase().padStart(4, '0')}`;
        if (this.sregEl) this.sregEl.textContent = `0x${telemetry.sreg.toString(16).toUpperCase().padStart(2, '0')}`;
        if (this.cyclesEl) this.cyclesEl.textContent = telemetry.cycles.toLocaleString();

        if (telemetry.gprs) {
            telemetry.gprs.forEach((val, i) => {
                if (this.gprEls[i]) {
                    this.gprEls[i].textContent = val.toString(16).toUpperCase().padStart(2, '0');
                }
            });
        }
    }
}
