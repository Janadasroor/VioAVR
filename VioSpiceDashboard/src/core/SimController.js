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
    this.eusartHistory = [];
    this.dacHistory = new Array(50).fill(0.5); // Default to mid-range
  }

    this._setupListeners();
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
    this.emit('stateChanged', { running: true });
    this.shell.showToast('Simulation Running', 'success');
  }

  pause() {
    this.isRunning = false;
    this.client.send('stop');
    this.emit('stateChanged', { running: false });
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
        setTimeout(() => {
            this.client.removeListener(handler);
            resolve([]);
        }, 2000);
    });
  }

  // ─── Event Handling ────────────────────────────────────────────────────────

  _listeners = {};
  on(event, fn) {
    if (!this._listeners[event]) this._listeners[event] = [];
    this._listeners[event].push(fn);
  }
  emit(event, data) {
    (this._listeners[event] || []).forEach(fn => fn(data));
  }

  _setupListeners() {
    this.client.connect();
    this.client.on('status', ({ connected }) => {
      this.emit('connectionChanged', { connected });
    });

    this.client.on('message', (data) => {
      this._handleTelemetry(data);
    });
  }

  _handleTelemetry(data) {
    if (!data.cpu) return;

    // Update internal state
    this.telemetry.pc = data.cpu.pc;
    this.telemetry.sp = data.cpu.sp;
    this.telemetry.sreg = data.cpu.sreg;
    this.telemetry.cycles = data.cpu.cycles;
    this.telemetry.gprs = data.cpu.gprs;
    this.telemetry.digital_outputs = data.digital_outputs;

    // Notify subscribers
    this.emit('telemetry', {
        ...this.telemetry,
        lcd: data.lcd,
        eusart: data.eusart,
        dac: data.dac
    });
  }
}
