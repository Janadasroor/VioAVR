/**
 * AppShell.js
 * Manages top-level UI concerns: navigation, view transitions,
 * toast notifications, session timer, and breadcrumbs.
 */
import { LibraryView } from '../views/LibraryView.js';
import { LogicView } from '../views/LogicView.js';
import { TerminalView } from '../views/TerminalView.js';
import { ActuatorView } from '../views/ActuatorView.js';
import { LogicPageView } from '../views/LogicPageView.js';
import { AnalogScopeView } from '../views/AnalogScopeView.js';

export class AppShell {
  constructor() {
    this.activeViewId = 'view-dashboard';
    this.startTime = Date.now();
    
    // Views
    this.registers = new RegisterView();
    this.pins = new PinGridView();
    this.controls = new ControlView();
    this.logic = new LogicView();
    this.terminal = new TerminalView();
    this.actuators = new ActuatorView();
    this.library = new LibraryView();
    this.logicPage = new LogicPageView();
    this.analogPage = new AnalogScopeView();
  }

  init(sim) {
    this._setupNavigation();
    this._setupSidebarAutoCollapse();
    this._startTimer();

    // Initialize Views
    this.registers.init();
    this.pins.init();
    this.library.init();
    this.logicPage.init();
    this.analogPage.init();

    // Wire up SimController events
    if (sim) {
        sim.on('telemetry', (data) => {
            this.registers.update(data);
            this.pins.update(data);
            this.logic.update(data.digital_outputs);
            this.logicPage.update(data.digital_outputs);
            this.terminal.update(data.eusart);
            this.actuators.update(data);
            this.analogPage.update(data.dac);
        });
        sim.on('stateChanged', ({ running }) => {
            this.controls.updateState(running);
        });
        sim.on('connectionChanged', ({ connected }) => {
            this.controls.updateConnection(connected);
        });
        sim.on('analysisTriggered', () => {
            this.switchView('view-logic');
            document.body.classList.add('fault-active');
            this.logicPage.freeze();
            this.analogPage.freeze();
            this.showToast('Hardware Event: Fault Analysis Triggered', 'error');
        });
        sim.on('stateChanged', ({ running }) => {
            if (running) {
                document.body.classList.remove('fault-active');
                this.logicPage.unfreeze();
                this.analogPage.unfreeze();
            }
            this.controls.updateState(running);
        });
    }

    this.showToast('Simulator Environment Ready', 'success');
  }

  // ─── View Management ───────────────────────────────────────────────────────

  switchView(viewId) {
    if (this.activeViewId === viewId) return;
    const targetView = document.getElementById(viewId);
    if (!targetView) return;

    // Deactivate all
    document.querySelectorAll('.view').forEach(v => {
      v.classList.remove('active');
      v.style.opacity = '0';
      v.style.transform = 'scale(0.98)';
    });

    // Activate target
    targetView.classList.add('active');
    this.activeViewId = viewId;

    // Update breadcrumb
    const label = document.querySelector(`[data-view="${viewId}"]`)?.dataset.label || '';
    const crumb = document.querySelector('.breadcrumb .current');
    if (crumb) crumb.textContent = label;

    // Animate in
    requestAnimationFrame(() => {
      targetView.style.opacity = '1';
      targetView.style.transform = 'scale(1)';
    });

    this.emit('viewChanged', { viewId });
  }

  // ─── Toast System ──────────────────────────────────────────────────────────

  showToast(message, type = 'info') {
    const toast = document.createElement('div');
    toast.className = `toast toast-${type}`;
    toast.innerHTML = `<div class="toast-icon"></div><div class="toast-content">${message}</div>`;
    document.body.appendChild(toast);

    requestAnimationFrame(() => {
      toast.classList.add('show');
      setTimeout(() => {
        toast.classList.remove('show');
        setTimeout(() => toast.remove(), 500);
      }, 3000);
    });
  }

  showContextMenu(x, y, items) {
    if (window.__vio.contextMenu) {
      window.__vio.contextMenu.show(x, y, items);
    }
  }

  // ─── Event Bus (simple) ────────────────────────────────────────────────────

  _listeners = {};

  on(event, fn) {
    if (!this._listeners[event]) this._listeners[event] = [];
    this._listeners[event].push(fn);
  }

  emit(event, data) {
    (this._listeners[event] || []).forEach(fn => fn(data));
  }

  // ─── Private ───────────────────────────────────────────────────────────────

  _setupNavigation() {
    document.querySelectorAll('.nav-item[data-view]').forEach(item => {
      item.addEventListener('click', () => {
        document.querySelectorAll('.nav-item').forEach(i => i.classList.remove('active'));
        item.classList.add('active');
        this.switchView(item.dataset.view);
      });
    });
  }

  _startTimer() {
    const el = document.querySelector('.session-timer');
    if (!el) return;
    setInterval(() => {
      const elapsed = Math.floor((Date.now() - this.startTime) / 1000);
      const h = String(Math.floor(elapsed / 3600)).padStart(2, '0');
      const m = String(Math.floor((elapsed % 3600) / 60)).padStart(2, '0');
      const s = String(elapsed % 60).padStart(2, '0');
      el.textContent = `Session: ${h}:${m}:${s}`;
    }, 1000);
  }

  _setupSidebarAutoCollapse() {
    const sidebar = document.querySelector('.sidebar');
    if (!sidebar) return;

    window.addEventListener('mousemove', (e) => {
      const x = e.clientX;
      if (x > 400) {
        sidebar.classList.add('collapsed');
      } else if (x < 100) {
        sidebar.classList.remove('collapsed');
      }
    });
  }
}
