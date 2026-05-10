/**
 * main.js — Bootstrap only.
 * Instantiates modules and wires them together.
 * No business logic lives here.
 */
import './style.css';
import { NetworkClient } from './core/NetworkClient.js';
import { AppShell } from './core/AppShell.js';
import { SimController } from './core/SimController.js';
import { ContextMenu } from './core/ContextMenu.js';
import { PropertyDialog } from './core/PropertyDialog.js';
import { SimulationStatusDialog } from './core/SimulationStatusDialog.js';
import { SchematicEditor } from './schematic/SchematicEditor.js';
import { ProjectView } from './schematic/ProjectView.js';
import { ERCManager } from './core/ERCManager.js';
import { ERCDialog } from './core/ERCDialog.js';

// ─── Application Initialization ─────────────────────────────────────────────

const shell = new AppShell();
const client = new NetworkClient('ws://localhost:8080');
const sim = new SimController();
const editor = new SchematicEditor(document.getElementById('canvas-container'));

// Global exposure for debugging and cross-module access
window.__vio = {
  shell,
  client,
  sim,
  editor
};

const contextMenu = new ContextMenu();
const propertyDialog = new PropertyDialog();
const statusDialog = new SimulationStatusDialog(client);
const projectView = new ProjectView(editor);
const ercDialog = new ERCDialog();

Object.assign(window.__vio, {
  contextMenu,
  dialogs: {
    properties: propertyDialog,
    status: statusDialog
  }
});

// ── Wire cross-module actions ────────────────────────────────────────────────
document.getElementById('btn-load')?.addEventListener('click', () => {
  const path = prompt("Enter the path to the .hex file:", "tests/core/firmware/test.hex");
  if (path) {
    sim.loadHex(path);
    shell.showToast(`Loading: ${path.split('/').pop()}`, 'info');
  }
});

document.getElementById('btn-reset')?.addEventListener('click', () => {
  sim.reset();
});

document.getElementById('btn-play')?.addEventListener('click', () => {
  if (!sim.isRunning) {
    const issues = ERCManager.check(editor.getChips(), editor.getWires());
    if (issues.length > 0) {
      ercDialog.open(issues, () => sim.toggle());
      return;
    }
  }
  sim.toggle();
});

// ── Init ─────────────────────────────────────────────────────────────────────
shell.init(sim);
sim.init(client);
editor.init();

// ── Expose globally for debugging ────────────────────────────────────────────
Object.assign(window.__vio, { shell, sim, editor, contextMenu, client });

// ── Simulation Results Handling ──────────────────────────────────────────────
sim.on('telemetry', (data) => {
    // 1. Update Schematic Visuals
    editor.update(data);

    // 2. Update Results Panel
    const simTime = document.getElementById('res-sim-time');
    const cpuCycles = document.getElementById('res-cpu-cycles');
    const signalList = document.getElementById('res-signal-list');

    if (simTime) simTime.textContent = (data.cycles / 16000000).toFixed(6) + 's';
    if (cpuCycles) cpuCycles.textContent = data.cycles.toLocaleString();

    if (signalList && data.digital_outputs) {
        // Show only active/named pins to avoid clutter
        let html = '';
        const chips = editor.getChips();
        chips.forEach(chip => {
            const type = chip.dataset.type;
            const name = chip.dataset.name || chip.dataset.label || type;
            // For MCU, show all IO ports
            if (type.startsWith('atmega') || type.startsWith('at90')) {
                data.digital_outputs.forEach((val, idx) => {
                    const port = String.fromCharCode(65 + Math.floor(idx / 8));
                    const bit = idx % 8;
                    const valClass = val === 1 ? 'val-high' : 'val-low';
                    html += `<tr><td>PORT${port}${bit} (${name})</td><td class="${valClass}">${val === 1 ? 'HIGH' : 'LOW'}</td></tr>`;
                });
            }
        });
        // Limit to first 20 entries for performance
        signalList.innerHTML = html;
    }
});
