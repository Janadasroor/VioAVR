import { NetworkClient } from './NetworkClient.js';

/**
 * SimController.js
 * Manages simulation state (run/pause/reset) and interfaces
 * with the hardware-backed VioSpice Gateway.
 */
export class SimController {
  constructor(shell) {
    this.shell = shell;
    this.client = new NetworkClient();
    this.isRunning = false;
    
    // Telemetry storage
    this.telemetry = {
      pc: 0,
      sp: 0,
      sreg: 0,
      cycles: 0,
      gprs: new Array(32).fill(0),
      digital_outputs: new Array(128).fill(0)
    };

    // Logic Analyzer Buffer (last 100 samples for 8 channels)
    this.logicHistory = Array.from({ length: 8 }, () => new Array(100).fill(0));
  }

  init() {
    this.client.connect();

    this.client.on('status', ({ connected }) => {
      this._updateConnectionStatus(connected);
    });

    this.client.on('message', (data) => {
      this._handleTelemetry(data);
    });

    this._setupGrids();

    document.getElementById('btn-play')?.addEventListener('click', () => this.toggle());
    document.getElementById('btn-reset')?.addEventListener('click', () => this.reset());
  }

  // ─── Public API ────────────────────────────────────────────────────────────

  toggle() {
    this.isRunning ? this.pause() : this.run();
  }

  run() {
    this.isRunning = true;
    this.client.send('run');
    this._updatePlayBtn(true);
    this.shell.showToast('Simulation Running', 'success');
  }

  pause() {
    this.isRunning = false;
    this.client.send('stop');
    this._updatePlayBtn(false);
    this.shell.showToast('Simulation Paused', 'info');
  }

  reset() {
    this.client.send('reset');
    this.shell.showToast('CPU Reset Triggered', 'warning');
  }

  loadHex(path) {
    this.client.send('load', { path });
  }

  exportTrace() {
    this.client.send('vcd');
    this.shell.showToast('VCD Tracing Toggled', 'info');
  }

  async listHexFiles() {
    return new Promise((resolve) => {
        const handler = (data) => {
            if (data.type === 'hex_list') {
                this.client.removeListener(handler);
                resolve(data.files);
            }
        };
        this.client.onMessage(handler);
        this.client.send('list_hex');
        // Timeout
        setTimeout(() => {
            this.client.removeListener(handler);
            resolve([]);
        }, 2000);
    });
  }

  // ─── Private ───────────────────────────────────────────────────────────────

  _handleTelemetry(data) {
    if (!data.cpu) return;

    // Update internal state
    this.telemetry.pc = data.cpu.pc;
    this.telemetry.sp = data.cpu.sp;
    this.telemetry.sreg = data.cpu.sreg;
    this.telemetry.cycles = data.cpu.cycles;
    this.telemetry.gprs = data.cpu.gprs;
    this.telemetry.digital_outputs = data.digital_outputs;

    // Update UI
    this._updateRegisters();
    this._updateGprGrid();
    this._updatePinGrid();
    this._updateLcdDisplays(data.lcd);
    this._updateLogicAnalyzer();
    this._updateActuators();
  }

  _updateActuators() {
    // 1. DC Motor (Cooling Fan) - Sniff Pin 16
    const dcNode = document.querySelector('.node-type-motor_dc');
    if (dcNode) {
        const isRunning = this.telemetry.digital_outputs[16] === 1;
        dcNode.classList.toggle('motor-dc-running', isRunning);
    }

    // 2. Stepper Motor (Gauge) - Sniff Pins 18-21 (4 phases)
    const stepperNode = document.querySelector('.node-type-motor_stepper');
    if (stepperNode) {
        const p1 = this.telemetry.digital_outputs[18];
        const p2 = this.telemetry.digital_outputs[19];
        const p3 = this.telemetry.digital_outputs[20];
        const p4 = this.telemetry.digital_outputs[21];
        
        // Simple heuristic for stepper position based on active phase
        let angle = 0;
        if (p1) angle = 0;
        else if (p2) angle = 90;
        else if (p3) angle = 180;
        else if (p4) angle = 270;
        
        const shaft = stepperNode.querySelector('.motor-shaft');
        if (shaft) {
            shaft.style.transform = `rotate(${angle}deg)`;
        }
    }
  }

  _updateLogicAnalyzer() {
    const analyzerNode = document.querySelector('.node-type-logic_analyzer');
    if (!analyzerNode) return;

    // Sniff first 8 pins (usually PORTA)
    for (let i = 0; i < 8; i++) {
      const isHigh = this.telemetry.digital_outputs[i] === 1;
      this.logicHistory[i].push(isHigh ? 1 : 0);
      if (this.logicHistory[i].length > 100) this.logicHistory[i].shift();

      const trace = analyzerNode.querySelector(`.logic-trace-${i}`);
      if (trace) {
        const yBase = 35 + i * 15;
        const width = 175;
        const step = width / 100;
        
        let d = `M 45 ${yBase}`;
        this.logicHistory[i].forEach((val, idx) => {
          const x = 45 + idx * step;
          const y = val === 1 ? yBase - 8 : yBase;
          // Use square wave jumps
          if (idx > 0 && val !== this.logicHistory[i][idx-1]) {
            d += ` L ${x} ${val === 1 ? yBase : yBase - 8}`;
          }
          d += ` L ${x} ${y}`;
        });
        trace.setAttribute('d', d);
      }
    }
  }

  _updateLcdDisplays(lcd) {
    if (!lcd || !lcd.enabled) {
      document.querySelectorAll('.lcd-seg').forEach(s => s.classList.remove('active'));
      return;
    }

    const nodes = document.querySelectorAll('.node-type-lcd_glass');
    nodes.forEach(node => {
      // 1/4 Duty Decoding Logic
      for (let d = 0; d < 4; d++) {
        const seg_start = d * 2;
        const segments = ['A','B','C','D','E','F','G','DP'];
        
        segments.forEach((s, i) => {
          const com = i % 4;
          const seg_off = Math.floor(i / 4);
          const seg_idx = seg_start + seg_off;
          
          const dr_idx = com;
          const isActive = (lcd.data[dr_idx] & (1 << seg_idx)) !== 0;
          
          const path = node.querySelector(`.digit-${d} .seg-${s}`);
          if (path) {
            path.classList.toggle('active', isActive);
          }
        });
      }
    });
  }

  _setupGrids() {
    // Setup GPR Grid
    const gprGrid = document.getElementById('gpr-grid');
    if (gprGrid) {
      gprGrid.innerHTML = '';
      for (let i = 0; i < 32; i++) {
        const item = document.createElement('div');
        item.className = 'gpr-item';
        item.innerHTML = `<label>R${i}</label><span id="reg-r${i}">00</span>`;
        gprGrid.appendChild(item);
      }
    }

    // Setup Pin Grid
    const pinGrid = document.getElementById('pin-grid');
    if (pinGrid) {
      pinGrid.innerHTML = '';
      // We'll show the first 32 pins for now (usually enough for most chips)
      for (let i = 0; i < 32; i++) {
        const item = document.createElement('div');
        item.className = 'pin-item';
        item.innerHTML = `
          <div class="pin-led" id="pin-led-${i}"></div>
          <span class="pin-label">P${i}</span>
        `;
        pinGrid.appendChild(item);
      }
    }
  }

  _updateGprGrid() {
    for (let i = 0; i < 32; i++) {
      const val = this.telemetry.gprs[i] || 0;
      this._set(`reg-r${i}`, val.toString(16).toUpperCase().padStart(2, '0'));
    }
  }

  _updatePinGrid() {
    for (let i = 0; i < 32; i++) {
      const el = document.getElementById(`pin-led-${i}`);
      if (el) {
        const isHigh = this.telemetry.digital_outputs[i] === 1;
        el.classList.toggle('high', isHigh);
      }
    }
  }

  _updateConnectionStatus(connected) {
    const badge = document.querySelector('.status-badge');
    if (badge) {
      badge.textContent = connected ? 'Simulator: Connected' : 'Simulator: Offline';
      badge.className = `status-badge ${connected ? 'online' : 'offline'}`;
    }
  }

  _updatePlayBtn(running) {
    const btn = document.getElementById('btn-play');
    if (!btn) return;
    btn.classList.toggle('running', running);
    btn.innerHTML = running
      ? `<svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor" style="margin-right:8px"><rect x="6" y="4" width="4" height="16"></rect><rect x="14" y="4" width="4" height="16"></rect></svg>Pause`
      : `<svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor" style="margin-right:8px"><path d="M8 5v14l11-7z"/></svg>Run Simulation`;
  }

  _updateRegisters() {
    this._set('reg-cycles', this.telemetry.cycles.toLocaleString());
    this._set('reg-pc', `0x${this.telemetry.pc.toString(16).toUpperCase().padStart(4, '0')}`);
    this._set('reg-sp', `0x${this.telemetry.sp.toString(16).toUpperCase().padStart(4, '0')}`);
    this._set('reg-sreg', `0x${this.telemetry.sreg.toString(16).toUpperCase().padStart(2, '0')}`);
  }

  _set(id, val) {
    const el = document.getElementById(id);
    if (el) el.textContent = val;
  }
}
