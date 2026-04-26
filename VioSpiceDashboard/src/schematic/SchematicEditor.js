/**
 * SchematicEditor.js
 * Owns only the schematic canvas: drag-and-drop, wire drawing,
 * chip interaction, and keyboard shortcuts within the canvas context.
 * All UI shell concerns (navigation, toasts) are delegated to AppShell.
 */
import { HistoryManager } from '../core/History.js';
import { Viewport } from '../core/Viewport.js';
import { ToolManager } from '../core/ToolManager.js';
import { createChip } from './ChipFactory.js';
import { createWaypointWire, updateWirePath, getAbsPinPos } from './Renderer.js';
import { calculateOrthogonalPoints, simplifyPath, getClosestSegmentIndex } from './Router.js';
import * as Cmds from './Commands.js';
import { getComponent, getAllComponents } from './ComponentLibrary.js';

const GRID = 20;

export class SchematicEditor {
  constructor(shell) {
    this.shell = shell;
    this.history = new HistoryManager();
    this.history.onChange = () => this._autoSave();
    this.svg = document.getElementById('schematic-canvas');
    this.canvas = document.getElementById('schematic-content');
    this.viewport = new Viewport('schematic-canvas', 'schematic-viewport');
    this.tools = new ToolManager();

    this._drag = null;
    this._wire = null;
    this._selectedWire = null;
    this._selectedChip = null;
    this._clipboard = null;
    this._mousePos = { x: 0, y: 0 };
    this._justDragged = false;
    this._boxStart = null;
  }

  init() {
    this._buildLibraryPanel();
    this._setupToolbar();
    this._setupCanvasEvents();
    this._setupKeyboard();
    this.viewport.update();

    // Restore previous state if it exists
    this._restoreAutoSave();

    // Re-calibrate viewport when entering schematic view
    this.shell.on('viewChanged', ({ viewId }) => {
      if (viewId === 'view-schematic') this.viewport.update();
    });
  }

  _autoSave() {
    const data = this.serialize();
    localStorage.setItem('viospice_schematic_state', data);
  }

  _restoreAutoSave() {
    const data = localStorage.getItem('viospice_schematic_state');
    if (data) {
      this.deserialize(data);
    } else {
      this.loadDemo();
    }
  }

  // ─── Load Preset ──────────────────────────────────────────────────────────

  loadDemo() {
    this.canvas.innerHTML = '';
    this.history = new HistoryManager();
    this.history.onChange = () => this._autoSave();

    const mcu = getComponent('atmega6490p') || getComponent('mcu');
    const speedometer = getComponent('lcd_glass');
    const tripComputer = getComponent('glcd_128x64');
    const coolingFan = getComponent('motor_dc');
    const gaugeStepper = getComponent('motor_stepper');
    const infotainment = getComponent('keypad_4x4');
    const analyzer = getComponent('logic_analyzer');

    // Central Intelligence
    this._placeComponent(mcu, 400, 300);

    // Instrument Cluster
    this._placeComponent(speedometer, 50, 50);
    this._placeComponent(tripComputer, 350, 50);
    this._placeComponent(gaugeStepper, 700, 50);

    // Controls & Actuators
    this._placeComponent(infotainment, 50, 350);
    this._placeComponent(coolingFan, 750, 350);

    // Debugging Tools
    this._placeComponent(analyzer, 400, 600);

    // AUTO-WIRE THE DEMO
    this.autoWireDemo();

    this.viewport.zoomFit(this.canvas.querySelectorAll('.schematic-node'));
    this.shell.showToast('Automotive Cluster Auto-Wired!', 'success');
  }

  autoWireDemo() {
    const nodes = Array.from(this.canvas.querySelectorAll('.schematic-node'));
    const getNodesByType = (type) => nodes.filter(n => n.dataset.type === type);
    
    const mcu = nodes.find(n => n.dataset.type === 'atmega6490p' || n.dataset.type === 'mcu');
    if (!mcu) return;

    const mcuId = mcu.dataset.id;

    // 1. Wire Logic Analyzer to PORTA (Pins 0-7)
    const analyzer = getNodesByType('logic_analyzer')[0];
    if (analyzer) {
        for (let i = 0; i < 8; i++) {
            this._createWire(mcuId, `P${i}`, analyzer.dataset.id, `CH${i}`);
        }
    }

    // 2. Wire DC Motor to Pin 16 (PORTC.0)
    const dcMotor = getNodesByType('motor_dc')[0];
    if (dcMotor) {
        this._createWire(mcuId, 'P16', dcMotor.dataset.id, '+');
    }

    // 3. Wire Stepper to Pins 18-21 (PORTD.2-5)
    const stepper = getNodesByType('motor_stepper')[0];
    if (stepper) {
        this._createWire(mcuId, 'P18', stepper.dataset.id, 'A+');
        this._createWire(mcuId, 'P19', stepper.dataset.id, 'A-');
        this._createWire(mcuId, 'P20', stepper.dataset.id, 'B+');
        this._createWire(mcuId, 'P21', stepper.dataset.id, 'B-');
    }

    // 4. Wire LCD Segments (Exhaustive)
    const glass = getNodesByType('lcd_glass')[0];
    if (glass) {
        // Wire first 8 segments as a demo
        for (let i = 0; i < 8; i++) {
            this._createWire(mcuId, `P${30+i}`, glass.dataset.id, `SEG${i}`);
        }
    }
  }

  _createWire(fromNodeId, fromPinId, toNodeId, toPinId) {
    // This is a helper to programmatically create a wire in the model
    const wire = {
      id: `wire-${Date.now()}-${Math.random()}`,
      from: { nodeId: fromNodeId, pinId: fromPinId },
      to: { nodeId: toNodeId, pinId: toPinId },
      segments: [] // Let the router handle it
    };
    this.shell.project.activeSchematic.wires.push(wire);
    this.render(); // Full re-render to show new wires
  }

  // ─── Library Panel ────────────────────────────────────────────────────────

  _buildLibraryPanel() {
    const list = document.querySelector('.chip-list');
    if (!list) return;
    list.innerHTML = '';

    const grouped = {};
    getAllComponents().forEach(comp => {
      if (!grouped[comp.category]) grouped[comp.category] = [];
      grouped[comp.category].push(comp);
    });

    Object.entries(grouped).forEach(([category, comps]) => {
      const header = document.createElement('div');
      header.className = 'library-category';
      header.textContent = category;
      list.appendChild(header);

      comps.forEach(comp => {
        const item = document.createElement('div');
        item.className = 'chip-item';
        item.draggable = true;
        item.dataset.type = comp.type;
        item.innerHTML = `
          <div class="chip-icon" style="--chip-color: ${comp.color}">
            <span>${comp.label.substring(0, 3).toUpperCase()}</span>
          </div>
          <div class="chip-info">
            <span class="chip-name">${comp.label}</span>
            <span class="chip-pins">${comp.pins.length} pins</span>
          </div>`;

        item.addEventListener('dragstart', e => e.dataTransfer.setData('type', comp.type));
        list.appendChild(item);
      });
    });

    // Search logic
    const searchInput = document.getElementById('library-search');
    searchInput?.addEventListener('input', e => {
      const query = e.target.value.toLowerCase();
      const items = list.querySelectorAll('.chip-item');
      const categories = list.querySelectorAll('.library-category');
      
      items.forEach(item => {
        const name = item.querySelector('.chip-name').textContent.toLowerCase();
        const matches = name.includes(query);
        item.style.display = matches ? 'flex' : 'none';
      });

      // Hide category headers if they have no visible items
      categories.forEach(cat => {
        let hasVisible = false;
        let next = cat.nextElementSibling;
        while (next && !next.classList.contains('library-category')) {
          if (next.style.display !== 'none') {
            hasVisible = true;
            break;
          }
          next = next.nextElementSibling;
        }
        cat.style.display = hasVisible ? 'block' : 'none';
      });
    });
  }

  // ─── Component Placement ─────────────────────────────────────────────────

  _placeComponent(compDef, x, y) {
    this.history.execute(new Cmds.AddChipCommand(
      (parent, px, py, type, name, pins) => {
        const g = this._createChipWithEvents(parent, px, py, type, name, pins);
        if (compDef.properties) {
          compDef.properties.forEach(p => {
            g.dataset[p.id] = p.default;
          });
        }
        return g;
      },
      this.canvas, x, y, compDef.type, compDef.label, compDef.pins
    ));
  }

  _createChipWithEvents(parent, x, y, type, name, pins) {
    const g = createChip(parent, x, y, type, name, pins);
    let currentX = x, currentY = y;

    g.addEventListener('mousedown', e => {
      if (e.button !== 0) return; // Only left click
      e.stopPropagation();
      this._selectChip(g);
      const pos = this.viewport.screenToWorld(e.clientX, e.clientY);
      this._drag = { target: g, offsetX: pos.x - currentX, offsetY: pos.y - currentY, initialX: currentX, initialY: currentY };
    });

    g.addEventListener('dblclick', e => {
      e.stopPropagation();
      window.__vio.dialogs?.properties?.open(g);
    });

    g.querySelectorAll('.pin-group').forEach(pinG => {
      pinG.addEventListener('mousedown', e => {
        e.stopPropagation();
        if (e.button !== 0) return;
        
        // Allow starting a wire even in 'select' mode
        if (this.tools.getTool() !== 'wire') {
          this.tools.setTool('wire');
        }
        this._handlePinClick(pinG);
      });
      // Prevent click from bubbling to canvas and adding redundant waypoints
      pinG.addEventListener('click', e => e.stopPropagation());
      
      pinG.addEventListener('mouseenter', () => {
        pinG.querySelector('.node-pin')?.setAttribute('r', '6');
      });
      pinG.addEventListener('mouseleave', () => {
        pinG.querySelector('.node-pin')?.setAttribute('r', '4');
      });
    });

    // Update currentX/Y on move so drag offset stays accurate
    const observer = new MutationObserver(() => {
      currentX = parseInt(g.dataset.x) || currentX;
      currentY = parseInt(g.dataset.y) || currentY;
    });
    observer.observe(g, { attributes: true, attributeFilter: ['data-x', 'data-y'] });

    return g;
  }

  _selectChip(chip) {
    this._clearSelection();
    this._selectedChip = chip;
    chip.classList.add('node-selected');
    this._updatePropertyPanel();
  }

  _clearSelection() {
    this.canvas.querySelectorAll('.node-selected, .wire-selected').forEach(el => el.classList.remove('node-selected', 'wire-selected'));
    this._selectedChip = null;
    this._selectedWire = null;
    this._updatePropertyPanel();
  }

  // ─── Wire Drawing ─────────────────────────────────────────────────────────

  _handlePinClick(pin) {
    if (!this._wire) {
      this._wire = { startPin: pin, intermediatePoints: [], flip: false };
      const ghost = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      ghost.id = 'ghost-wire';
      ghost.setAttribute('stroke', 'var(--primary)');
      ghost.setAttribute('stroke-width', '2');
      ghost.setAttribute('fill', 'none');
      ghost.setAttribute('stroke-dasharray', '5,3');
      ghost.style.pointerEvents = 'none';
      
      // Initialize path immediately so it doesn't wait for mousemove
      const startPos = getAbsPinPos(pin);
      ghost.setAttribute('d', `M ${startPos.x} ${startPos.y} L ${startPos.x} ${startPos.y}`);
      
      this.canvas.appendChild(ghost);
    } else {
      if (this._wire.startPin === pin) return; // Prevent self-connections
      const endPos = getAbsPinPos(pin);
      const pts = this._buildWirePoints(endPos);
      const wire = createWaypointWire(this._wire.startPin, pin, pts);
      this.history.execute(new Cmds.AddWireCommand(wire, this.canvas));
      this._cancelWire();
    }
  }

  _buildWirePoints(endPos) {
    const startPos = getAbsPinPos(this._wire.startPin);
    const pts = [startPos, ...this._wire.intermediatePoints];
    const last = pts[pts.length - 1];
    const seg = calculateOrthogonalPoints(last, endPos, this._wire.flip ? 'v' : 'h');
    seg.shift();
    pts.push(...seg);
    return simplifyPath(pts);
  }

  serialize() {
    const chipNodes = Array.from(this.canvas.querySelectorAll('.schematic-node'));
    chipNodes.forEach((g, i) => g.dataset.id = `chip_${i}`);

    const chips = chipNodes.map(g => {
      // Collect all non-standard dataset properties (component properties)
      const props = {};
      Object.keys(g.dataset).forEach(key => {
        if (!['id', 'type', 'x', 'y', 'label'].includes(key)) {
          props[key] = g.dataset[key];
        }
      });
      return {
        id: g.dataset.id,
        type: g.dataset.type,
        x: parseInt(g.dataset.x),
        y: parseInt(g.dataset.y),
        properties: props
      };
    });

    const wires = Array.from(this.canvas.querySelectorAll('.wire-group')).map(w => {
      const startChip = w.startPin?.closest('.schematic-node');
      const endChip = w.endPin?.closest('.schematic-node');
      return {
        start: startChip ? { chipId: startChip.dataset.id, pinId: w.startPin.dataset.id } : null,
        end: endChip ? { chipId: endChip.dataset.id, pinId: w.endPin.dataset.id } : null,
        points: JSON.parse(w.dataset.points)
      };
    });

    return JSON.stringify({ chips, wires }, null, 2);
  }

  deserialize(json) {
    try {
      const data = JSON.parse(json);
      this.canvas.innerHTML = '';
      this.history = new HistoryManager();
      this.history.onChange = () => this._autoSave();
      this._clearSelection();

      const chipMap = {};
      data.chips.forEach(c => {
        const compDef = getComponent(c.type);
        if (compDef) {
          const g = this._createChipWithEvents(this.canvas, c.x, c.y, c.type, compDef.label, compDef.pins);
          // Restore properties
          if (c.properties) {
            Object.assign(g.dataset, c.properties);
          }
          chipMap[c.id] = g;
        }
      });

      data.wires.forEach(w => {
        const startChip = chipMap[w.start?.chipId];
        const endChip = chipMap[w.end?.chipId];
        const startPin = startChip?.querySelector(`.pin-group[data-id="${w.start.pinId}"]`);
        const endPin = endChip?.querySelector(`.pin-group[data-id="${w.end.pinId}"]`);
        
        const wire = createWaypointWire(startPin, endPin, w.points);
        this.canvas.appendChild(wire);
      });

      this.shell.showToast('Schematic loaded successfully', 'success');
    } catch (err) {
      console.error(err);
      this.shell.showToast('Failed to load schematic', 'error');
    }
  }

  _cancelWire() {
    document.getElementById('ghost-wire')?.remove();
    this._wire = null;
  }

  _updateGhost(pos) {
    const ghost = document.getElementById('ghost-wire');
    if (!ghost || !this._wire) return;
    const startPos = getAbsPinPos(this._wire.startPin);
    const last = this._wire.intermediatePoints.length > 0
      ? this._wire.intermediatePoints[this._wire.intermediatePoints.length - 1]
      : startPos;
    const seg = calculateOrthogonalPoints(last, pos, this._wire.flip ? 'v' : 'h');
    let d = `M ${startPos.x} ${startPos.y}`;
    this._wire.intermediatePoints.forEach(p => d += ` L ${p.x} ${p.y}`);
    seg.shift();
    seg.forEach(p => d += ` L ${p.x} ${p.y}`);
    ghost.setAttribute('d', d);
  }

  // ─── Canvas Events ────────────────────────────────────────────────────────

  _setupCanvasEvents() {
    this.svg.addEventListener('mousedown', e => this._onMouseDown(e));
    this.svg.addEventListener('click', e => this._onCanvasClick(e));
    window.addEventListener('mousemove', e => this._onMouseMove(e));
    window.addEventListener('mouseup', e => this._onMouseUp(e));

    window.addEventListener('wheel', e => {
      if (e.ctrlKey) {
        e.preventDefault();
        this.viewport.zoom(e.deltaY > 0 ? -0.1 : 0.1, e.clientX, e.clientY);
      }
    }, { passive: false });

    const dropZone = this.svg;
    dropZone?.addEventListener('dragover', e => e.preventDefault());
    dropZone?.addEventListener('drop', e => {
      e.preventDefault();
      const type = e.dataTransfer.getData('type');
      const comp = getComponent(type);
      if (!comp) return;
      const pos = this.viewport.screenToWorld(e.clientX, e.clientY);
      const sx = Math.round(pos.x / GRID) * GRID;
      const sy = Math.round(pos.y / GRID) * GRID;
      this._placeComponent(comp, sx, sy);
    });
  }

  _onMouseDown(e) {
    if (e.button !== 0) return;
    const isBackground = e.target === this.svg || e.target.tagName === 'rect';
    if (isBackground && (this.tools.current === 'select' || this.tools.current === 'zoom-box')) {
      const pos = this.viewport.screenToWorld(e.clientX, e.clientY);
      this._boxStart = pos;
      
      const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      rect.id = 'selection-box';
      rect.setAttribute('fill', 'rgba(0, 162, 255, 0.1)');
      rect.setAttribute('stroke', 'var(--primary)');
      rect.setAttribute('stroke-width', '1');
      rect.setAttribute('stroke-dasharray', '4,2');
      this.canvas.appendChild(rect);
    }
  }

  _onCanvasClick(e) {
    if (this._justDragged) return;
    if (this._wire) {
      const pos = this.viewport.screenToWorld(e.clientX, e.clientY);
      const snapped = { x: Math.round(pos.x / GRID) * GRID, y: Math.round(pos.y / GRID) * GRID };
      const last = this._wire.intermediatePoints.length > 0
        ? this._wire.intermediatePoints[this._wire.intermediatePoints.length - 1]
        : getAbsPinPos(this._wire.startPin);
      const seg = calculateOrthogonalPoints(last, snapped, this._wire.flip ? 'v' : 'h');
      seg.shift();
      this._wire.intermediatePoints.push(...seg);
      this._updateGhost(pos);
    } else {
      this._clearSelection();
      this._trySelectWire(e);
    }
  }

  _trySelectWire(e) {
    const pos = this.viewport.screenToWorld(e.clientX, e.clientY);
    for (const group of document.querySelectorAll('.wire-group')) {
      const points = JSON.parse(group.dataset.points || '[]');
      const idx = getClosestSegmentIndex(pos.x, pos.y, points);
      if (idx !== -1) {
        this._selectedWire = group;
        group.classList.add('wire-selected');
        group.dataset.selectedSegment = idx;
        updateWirePath(group);
        break;
      }
    }
  }

  _onMouseMove(e) {
    const pos = this.viewport.screenToWorld(e.clientX, e.clientY);
    this._mousePos = pos;

    if (this._drag) {
      const nx = Math.round((pos.x - this._drag.offsetX) / GRID) * GRID;
      const ny = Math.round((pos.y - this._drag.offsetY) / GRID) * GRID;
      this._drag.target.setAttribute('transform', `translate(${nx}, ${ny})`);
      this._drag.target.dataset.x = nx;
      this._drag.target.dataset.y = ny;
      this._updateConnectedWires(this._drag.target);
    } else if (this._boxStart) {
      const rect = document.getElementById('selection-box');
      if (rect) {
        const x = Math.min(this._boxStart.x, pos.x);
        const y = Math.min(this._boxStart.y, pos.y);
        const w = Math.abs(this._boxStart.x - pos.x);
        const h = Math.abs(this._boxStart.y - pos.y);
        rect.setAttribute('x', x);
        rect.setAttribute('y', y);
        rect.setAttribute('width', w);
        rect.setAttribute('height', h);
      }
    }

    if (this._wire) this._updateGhost(pos);
  }

  _onMouseUp(e) {
    if (this._drag) {
      const nx = parseInt(this._drag.target.dataset.x);
      const ny = parseInt(this._drag.target.dataset.y);
      if (nx !== this._drag.initialX || ny !== this._drag.initialY) {
        this.history.execute(new Cmds.MoveChipCommand(
          this._drag.target,
          this._drag.initialX, this._drag.initialY,
          nx, ny,
          () => this._updateConnectedWires(this._drag.target)
        ));
      }
      this._drag = null;
      this._justDragged = true;
      setTimeout(() => this._justDragged = false, 50);
    }

    if (this._boxStart) {
      const pos = this.viewport.screenToWorld(e.clientX, e.clientY);
      const x = Math.min(this._boxStart.x, pos.x);
      const y = Math.min(this._boxStart.y, pos.y);
      const w = Math.abs(this._boxStart.x - pos.x);
      const h = Math.abs(this._boxStart.y - pos.y);

      if (w > 5 && h > 5) {
        if (this.tools.current === 'zoom-box') {
          this.viewport.zoomToRect({ x, y, width: w, height: h });
          this.tools.setTool('select');
        } else if (this.tools.current === 'select') {
          this._selectInRect(x, y, w, h);
        }
      }

      document.getElementById('selection-box')?.remove();
      this._boxStart = null;
    }
  }

  _selectInRect(x, y, w, h) {
    const chips = this.canvas.querySelectorAll('.schematic-node');
    this._clearSelection();
    chips.forEach(chip => {
      const cx = parseInt(chip.dataset.x);
      const cy = parseInt(chip.dataset.y);
      if (cx >= x && cx <= x + w && cy >= y && cy <= y + h) {
        chip.classList.add('node-selected');
      }
    });
  }

  _updateConnectedWires(chip) {
    document.querySelectorAll('.wire-group').forEach(w => {
      if (w.startPin?.closest('.schematic-node') === chip ||
          w.endPin?.closest('.schematic-node') === chip) {
        updateWirePath(w);
      }
    });
  }

  // ─── Keyboard ─────────────────────────────────────────────────────────────

  _setupKeyboard() {
    window.addEventListener('keydown', e => {
      if (e.target.tagName === 'INPUT') return;

      const key = e.key.toLowerCase();
      if (key === 's') this.tools.setTool('select');
      if (key === 'w') this.tools.setTool('wire');
      if (key === 'r' && this._selectedChip) this._rotateSelectedChip();
      if (key === 'f') this.viewport.zoomFit(this.canvas.querySelectorAll('.schematic-node'));
      if (e.key === 'Escape') { this._cancelWire(); this._clearSelection(); }
      if (e.key === ' ' && this._wire) { this._wire.flip = !this._wire.flip; e.preventDefault(); }
      
      if (e.ctrlKey && key === 'c') this._copy();
      if (e.ctrlKey && key === 'v') this._paste();
      if (e.ctrlKey && key === 'd') { e.preventDefault(); this._duplicate(); }

      if (e.ctrlKey && key === 'z') { e.preventDefault(); this.history.undo(); }
      if (e.ctrlKey && (key === 'y' || (e.shiftKey && key === 'z'))) { e.preventDefault(); this.history.redo(); }
      if ((e.key === 'Delete' || e.key === 'Backspace')) {
        if (this._selectedWire) this._deleteSelectedWire();
        if (this._selectedChip) this._deleteSelectedChip();
      }
    });
  }

  _rotateSelectedChip() {
    const chip = this._selectedChip;
    const oldRot = parseInt(chip.dataset.rotation) || 0;
    const newRot = (oldRot + 90) % 360;
    this.history.execute(new Cmds.RotateChipCommand(
      chip, oldRot, newRot,
      () => this._updateConnectedWires(chip)
    ));
  }

  _clearCanvas() {
    if (confirm('Are you sure you want to clear the entire canvas?')) {
      // In a real app, this would be a single macro-command
      this.canvas.innerHTML = '';
      this.history = new HistoryManager();
      this._clearSelection();
      this._cancelWire();
      this.shell.showToast('Canvas cleared', 'info');
    }
  }

  _copy() {
    if (this._selectedChip) {
      this._clipboard = { type: this._selectedChip.dataset.type };
      this.shell.showToast(`Copied ${this._selectedChip.dataset.label}`, 'info');
    }
  }

  _paste() {
    if (this._clipboard) {
      const comp = getComponent(this._clipboard.type);
      if (comp) {
        const x = Math.round(this._mousePos.x / GRID) * GRID;
        const y = Math.round(this._mousePos.y / GRID) * GRID;
        this._placeComponent(comp, x, y);
      }
    }
  }

  _duplicate() {
    if (this._selectedChip) {
      const comp = getComponent(this._selectedChip.dataset.type);
      if (comp) {
        const x = parseInt(this._selectedChip.dataset.x) + GRID * 2;
        const y = parseInt(this._selectedChip.dataset.y) + GRID * 2;
        this._placeComponent(comp, x, y);
      }
    }
  }

  _deleteSelectedChip() {
    const chip = this._selectedChip;
    this.history.execute(new Cmds.DeleteChipCommand(chip, this.canvas));
    this._selectedChip = null;
  }

  _deleteSelectedWire() {
    const wire = this._selectedWire;
    const segIndex = parseInt(wire.dataset.selectedSegment);
    this.history.execute(new Cmds.DeleteWireCommand(wire, this.canvas));
    this._selectedWire = null;
  }

  // ─── Toolbar ──────────────────────────────────────────────────────────────

  _setupToolbar() {
    document.getElementById('tool-select')?.addEventListener('click', () => this.tools.setTool('select'));
    document.getElementById('tool-wire')?.addEventListener('click', () => this.tools.setTool('wire'));
    document.getElementById('tool-zoom-box')?.addEventListener('click', () => this.tools.setTool('zoom-box'));
    document.getElementById('zoom-in')?.addEventListener('click', () => this.viewport.zoom(0.1));
    document.getElementById('zoom-out')?.addEventListener('click', () => this.viewport.zoom(-0.1));
    document.getElementById('zoom-fit')?.addEventListener('click', () =>
      this.viewport.zoomFit(this.canvas.querySelectorAll('.schematic-node')));
    
    document.getElementById('btn-export')?.addEventListener('click', () => {
      const json = this.serialize();
      const blob = new Blob([json], { type: 'application/json' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = 'schematic.json';
      a.click();
      URL.revokeObjectURL(url);
    });

    document.getElementById('btn-import')?.addEventListener('click', () => {
      const input = document.createElement('input');
      input.type = 'file';
      input.accept = '.json';
      input.onchange = e => {
        const file = e.target.files[0];
        const reader = new FileReader();
        reader.onload = re => this.deserialize(re.target.result);
        reader.readAsText(file);
      };
      input.click();
    });

    document.getElementById('btn-undo')?.addEventListener('click', () => this.history.undo());
    document.getElementById('btn-redo')?.addEventListener('click', () => this.history.redo());
    document.getElementById('btn-clear')?.addEventListener('click', () => this._clearCanvas());

    // Sidebar Tab Switching
    document.querySelectorAll('.panel-tab').forEach(tab => {
      tab.addEventListener('click', () => {
        const target = tab.dataset.tab;
        
        // Update tabs
        document.querySelectorAll('.panel-tab').forEach(t => t.classList.remove('active'));
        tab.classList.add('active');
        
        // Update content
        document.querySelectorAll('.panel-tab-content').forEach(c => c.classList.remove('active'));
        document.getElementById(`tab-${target}`)?.classList.add('active');
      });
    });
  }

  _updatePropertyPanel() {
    const list = document.getElementById('property-list');
    const label = document.getElementById('node-type-label');
    if (!list || !label) return;

    if (!this._selectedChip) {
      label.textContent = 'No selection';
      list.innerHTML = '<div class="property-placeholder">Select a component to edit its properties.</div>';
      return;
    }

    const chip = this._selectedChip;
    const type = chip.dataset.type;
    const def = getComponent(type);
    
    label.textContent = def ? def.label : type.toUpperCase();
    list.innerHTML = '';

    // Label property (common to all)
    this._createPropertyField(list, 'label', 'Instance Name', chip.dataset.name || '', (val) => {
      chip.dataset.name = val;
      const text = chip.querySelector('.node-label');
      if (text) text.textContent = val;
    });

    if (def && def.properties) {
      def.properties.forEach(p => {
        const currentVal = chip.dataset[p.id] ?? p.default;
        this._createPropertyField(list, p.id, p.label, currentVal, (val) => {
          chip.dataset[p.id] = val;
        });
      });
    }
  }

  _createPropertyField(parent, id, labelText, value, onChange) {
    const group = document.createElement('div');
    group.className = 'property-group';
    
    const label = document.createElement('label');
    label.textContent = labelText;
    
    const inputWrapper = document.createElement('div');
    inputWrapper.className = 'property-input-wrapper';

    const input = document.createElement('input');
    input.type = 'text';
    input.value = value;
    input.addEventListener('change', () => onChange(input.value));
    
    inputWrapper.appendChild(input);

    // Special action for firmware binary
    if (id === 'binary') {
        const actions = document.createElement('div');
        actions.className = 'property-actions';

        const btnBrowse = document.createElement('button');
        btnBrowse.className = 'btn-browse';
        btnBrowse.innerHTML = '📂 Browse';
        btnBrowse.onclick = async () => {
            const files = await window.__vio.sim.listHexFiles();
            if (files.length === 0) {
                window.__vio.shell.showToast('No .hex files found in workspace', 'warning');
                return;
            }
            // Simple prompt for now, but with the list
            const selection = prompt("Found HEX files:\n" + files.join("\n") + "\n\nEnter filename:", input.value);
            if (selection) {
                input.value = selection;
                onChange(selection);
            }
        };

        const btnFlash = document.createElement('button');
        btnFlash.className = 'btn-flash';
        btnFlash.innerHTML = '⚡ Flash';
        btnFlash.onclick = () => {
            if (input.value) window.__vio.sim.loadHex(input.value);
        };

        actions.appendChild(btnBrowse);
        actions.appendChild(btnFlash);
        inputWrapper.appendChild(actions);
    }

    group.appendChild(label);
    group.appendChild(inputWrapper);
    parent.appendChild(group);
  }
}
