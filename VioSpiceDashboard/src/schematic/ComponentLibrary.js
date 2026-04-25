/**
 * ComponentLibrary.js
 * Single source of truth for all component definitions
 * (pin layouts, metadata, icons). Decoupled from the canvas/editor.
 */

// ─── Component Definitions ─────────────────────────────────────────────────

import { DYNAMIC_MCUS } from './McuLibrary.js';

// ─── Component Definitions ─────────────────────────────────────────────────

const COMPONENTS = {
  // Hardcoded core components
  ...DYNAMIC_MCUS,
  
  // Custom overrides or non-AVR components
  lcd: {
    label: 'LCD Glass',
    category: 'Display',
    color: 'var(--accent)',
    properties: [
      { id: 'controller', label: 'Controller', type: 'text', default: 'HD44780' },
      { id: 'cols',       label: 'Columns',    type: 'number', default: 16 },
      { id: 'rows',       label: 'Rows',       type: 'number', default: 2 }
    ],
    pins: [
      { id: 'VDD',  side: 'left', type: 'power' },
      { id: 'VSS',  side: 'left', type: 'power' },
      { id: 'V0',   side: 'left', type: 'analog'},
      { id: 'RS',   side: 'left', type: 'ctrl'  },
      { id: 'RW',   side: 'left', type: 'ctrl'  },
      { id: 'E',    side: 'left', type: 'ctrl'  },
      ...['D0','D1','D2','D3','D4','D5','D6','D7'].map(id => ({ id, side: 'right', type: 'io' })),
      { id: 'A',   side: 'right', type: 'power' },
      { id: 'K',   side: 'right', type: 'power' },
    ],
  },

  res: {
    label: 'Resistor',
    category: 'Passive',
    color: 'var(--warning)',
    properties: [
      { id: 'value', label: 'Resistance (Ω)', type: 'text', default: '10k' }
    ],
    pins: [
      { id: '1', side: 'left',  type: 'io' },
      { id: '2', side: 'right', type: 'io' },
    ],
  },

  cap: {
    label: 'Capacitor',
    category: 'Passive',
    color: 'var(--warning)',
    properties: [
      { id: 'value', label: 'Capacitance (F)', type: 'text', default: '100u' }
    ],
    pins: [
      { id: '+', side: 'left',  type: 'io' },
      { id: '-', side: 'right', type: 'io' },
    ],
  },

  vsrc: {
    label: 'V-Source',
    category: 'Source',
    color: 'var(--success)',
    properties: [
      { id: 'voltage', label: 'Voltage (V)', type: 'number', default: 5.0 }
    ],
    pins: [
      { id: '+', side: 'left',  type: 'power' },
      { id: '-', side: 'right', type: 'power' },
    ],
  },

  gnd: {
    label: 'GND',
    category: 'Power',
    color: 'var(--text-dim)',
    properties: [],
    pins: [
      { id: 'GND', side: 'left', type: 'power' },
    ],
  },
  led: {
    label: 'LED',
    category: 'Output',
    color: 'var(--danger)',
    properties: [
      { id: 'color', label: 'Color', type: 'text', default: '#ff0000' },
      { id: 'v_f',   label: 'Forward Voltage (V)', type: 'number', default: 2.0 }
    ],
    pins: [
      { id: 'A', side: 'left',  type: 'power' },
      { id: 'K', side: 'right', type: 'power' },
    ],
  },

  button: {
    label: 'Push Button',
    category: 'Input',
    color: 'var(--info)',
    properties: [],
    pins: [
      { id: '1', side: 'left',  type: 'io' },
      { id: '2', side: 'right', type: 'io' },
    ],
  },

  seven_seg: {
    label: '7-Segment Display',
    category: 'Display',
    color: 'var(--danger)',
    properties: [
      { id: 'common', label: 'Common', type: 'text', default: 'Cathode' }
    ],
    pins: [
      { id: 'A',   side: 'left',  type: 'io' },
      { id: 'B',   side: 'left',  type: 'io' },
      { id: 'C',   side: 'left',  type: 'io' },
      { id: 'D',   side: 'left',  type: 'io' },
      { id: 'E',   side: 'right', type: 'io' },
      { id: 'F',   side: 'right', type: 'io' },
      { id: 'G',   side: 'right', type: 'io' },
      { id: 'DP',  side: 'right', type: 'io' },
      { id: 'COM', side: 'left',  type: 'power' },
    ],
  },

  pot: {
    label: 'Potentiometer',
    category: 'Passive',
    color: 'var(--warning)',
    properties: [
      { id: 'value', label: 'Resistance (Ω)', type: 'text', default: '10k' }
    ],
    pins: [
      { id: '1', side: 'left',  type: 'io' },
      { id: 'W', side: 'right', type: 'analog' },
      { id: '2', side: 'left',  type: 'io' },
    ],
  },

  buzzer: {
    label: 'Buzzer',
    category: 'Output',
    color: 'var(--text-dim)',
    properties: [
      { id: 'freq', label: 'Resonance (Hz)', type: 'number', default: 4000 }
    ],
    pins: [
      { id: '+', side: 'left',  type: 'power' },
      { id: '-', side: 'right', type: 'power' },
    ],
  },
};

// ─── Pin type color map ────────────────────────────────────────────────────

export const PIN_COLORS = {
  power:  '#ff5252',
  analog: '#ffea00',
  ctrl:   '#7c4dff',
  osc:    '#00e5ff',
  io:     'rgba(255, 255, 255, 0.3)',
};

// ─── Public API ────────────────────────────────────────────────────────────

export function getComponent(type) {
  const def = COMPONENTS[type];
  return def ? { type, ...def } : null;
}

export function getAllComponents() {
  return Object.entries(COMPONENTS).map(([type, def]) => ({ type, ...def }));
}
