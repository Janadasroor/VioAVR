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
