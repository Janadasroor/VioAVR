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

// ─── Application Initialization ─────────────────────────────────────────────

const shell = new AppShell();
const client = new NetworkClient('ws://localhost:8080');
const sim = new SimController(client, shell);
const editor = new SchematicEditor(document.getElementById('canvas-container'));
const contextMenu = new ContextMenu();
const propertyDialog = new PropertyDialog();
const statusDialog = new SimulationStatusDialog(client);

// Global exposure for debugging and cross-module access
window.__vio = {
  shell,
  client,
  sim,
  editor,
  contextMenu,
  dialogs: {
    properties: propertyDialog,
    status: statusDialog
  }
};

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

// ── Init ─────────────────────────────────────────────────────────────────────
shell.init(sim);
sim.init();
editor.init();

// ── Expose globally for debugging ────────────────────────────────────────────
Object.assign(window.__vio, { shell, sim, editor, contextMenu, client });
