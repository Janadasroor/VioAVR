/**
 * ToolManager.js
 * Manages the active tool state and updates the UI feedback (cursors, status).
 */
export class ToolManager {
  constructor() {
    this.currentTool = 'select';
  }

  setTool(name) {
    this.currentTool = name;
    
    // Update toolbar icons
    document.querySelectorAll('.tool-btn').forEach(btn => {
      btn.classList.toggle('active', btn.id === `tool-${name}`);
    });
    
    // Update canvas status overlay
    const status = document.getElementById('tool-status');
    if (status) {
      if (name === 'wire') {
        status.textContent = 'Wiring Mode (Click Pins)';
        status.classList.add('active');
      } else if (name === 'probe') {
        status.textContent = 'Probe Mode (Click any Pin/Wire)';
        status.classList.add('active');
      } else {
        status.textContent = 'Selection Mode';
        status.classList.remove('active');
      }
    }
    
    // Update cursor
    const canvas = document.getElementById('schematic-canvas');
    if (canvas) {
      canvas.classList.remove('tool-select', 'tool-wire', 'tool-probe', 'tool-zoom-box');
      canvas.classList.add(`tool-${name}`);
    }
  }

  getTool() {
    return this.currentTool;
  }
}
