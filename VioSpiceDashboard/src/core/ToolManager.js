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
      status.textContent = name === 'wire' ? 'Wiring Mode (Click Pins)' : 'Selection Mode';
      status.classList.toggle('active', name === 'wire');
    }
    
    // Update cursor
    const svg = document.getElementById('schematic-svg');
    if (svg) {
      svg.style.cursor = name === 'wire' ? 'crosshair' : 'default';
    }
  }

  getTool() {
    return this.currentTool;
  }
}
