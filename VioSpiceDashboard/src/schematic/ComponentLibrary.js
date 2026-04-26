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
    label: 'LCD 16-Pin (16x2)',
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
  lcd_glass: {
    label: 'LCD Glass (4-Digit)',
    category: 'Display',
    color: '#889988',
    properties: [
      { id: 'com_count', label: 'Commons', type: 'number', default: 4 }
    ],
    pins: [
      ...Array.from({length: 32}, (_, i) => ({ id: `SEG${i}`, side: i < 16 ? 'left' : 'right', type: 'io' })),
      ...Array.from({length: 4}, (_, i) => ({ id: `COM${i}`, side: 'left', type: 'power' })),
    ],
  },
  glcd_128x64: {
    label: 'Graphical LCD (128x64)',
    category: 'Display',
    color: '#3a58cc',
    properties: [
      { id: 'controller', label: 'Controller', type: 'text', default: 'KS0108' }
    ],
    pins: [
      { id: 'VCC',  side: 'left',  type: 'power' },
      { id: 'GND',  side: 'left',  type: 'power' },
      { id: 'RS',   side: 'left',  type: 'ctrl'  },
      { id: 'RW',   side: 'left',  type: 'ctrl'  },
      { id: 'E',    side: 'left',  type: 'ctrl'  },
      { id: 'CS1',  side: 'left',  type: 'ctrl'  },
      { id: 'CS2',  side: 'left',  type: 'ctrl'  },
      { id: 'RST',  side: 'left',  type: 'ctrl'  },
      ...['D0','D1','D2','D3','D4','D5','D6','D7'].map(id => ({ id, side: 'right', type: 'io' })),
      { id: 'VEE',  side: 'right', type: 'analog'},
      { id: 'A',    side: 'right', type: 'power' },
      { id: 'K',    side: 'right', type: 'power' },
    ],
  },
  keypad_4x4: {
    label: '4x4 Keypad',
    category: 'Input',
    color: '#333333',
    properties: [],
    pins: [
      ...['R1','R2','R3','R4'].map(id => ({ id, side: 'left', type: 'io' })),
      ...['C1','C2','C3','C4'].map(id => ({ id, side: 'right', type: 'io' })),
    ],
  },
  motor_dc: {
    label: 'DC Motor',
    category: 'Actuator',
    color: '#555555',
    properties: [
      { id: 'v_nom', label: 'Nominal Voltage (V)', type: 'number', default: 5.0 }
    ],
    pins: [
      { id: '+', side: 'left',  type: 'power' },
      { id: '-', side: 'right', type: 'power' },
    ],
  },
  motor_stepper: {
    label: 'Stepper Motor',
    category: 'Actuator',
    color: '#444444',
    properties: [
      { id: 'steps', label: 'Steps/Rev', type: 'number', default: 200 }
    ],
    pins: [
      { id: 'A+', side: 'left',  type: 'io' },
      { id: 'A-', side: 'left',  type: 'io' },
      { id: 'B+', side: 'right', type: 'io' },
      { id: 'B-', side: 'right', type: 'io' },
    ],
  },
  eeprom_i2c: {
    label: 'I2C EEPROM (24C64)',
    category: 'Memory',
    color: '#5c2d91',
    properties: [
      { id: 'address', label: 'I2C Address', type: 'text', default: '0x50' }
    ],
    pins: [
      { id: 'VCC', side: 'left',  type: 'power' },
      { id: 'GND', side: 'left',  type: 'power' },
      { id: 'SCL', side: 'right', type: 'io'    },
      { id: 'SDA', side: 'right', type: 'io'    },
      { id: 'A0',  side: 'left',  type: 'ctrl'  },
      { id: 'A1',  side: 'left',  type: 'ctrl'  },
      { id: 'A2',  side: 'left',  type: 'ctrl'  },
      { id: 'WP',  side: 'right', type: 'ctrl'  },
    ],
  },
  flash_spi: {
    label: 'SPI Flash (25LC256)',
    category: 'Memory',
    color: '#0078d4',
    properties: [],
    pins: [
      { id: 'VCC',  side: 'left',  type: 'power' },
      { id: 'GND',  side: 'left',  type: 'power' },
      { id: 'CS',   side: 'left',  type: 'ctrl'  },
      { id: 'SCK',  side: 'right', type: 'io'    },
      { id: 'SI',   side: 'right', type: 'io'    },
      { id: 'SO',   side: 'right', type: 'io'    },
      { id: 'WP',   side: 'left',  type: 'ctrl'  },
      { id: 'HOLD', side: 'right', type: 'ctrl'  },
    ],
  },
  can_transceiver: {
    label: 'CAN Transceiver (MCP2551)',
    category: 'Communication',
    color: '#d13438',
    properties: [],
    pins: [
      { id: 'VCC',  side: 'left',  type: 'power' },
      { id: 'GND',  side: 'left',  type: 'power' },
      { id: 'TXD',  side: 'left',  type: 'io'    },
      { id: 'RXD',  side: 'left',  type: 'io'    },
      { id: 'CANH', side: 'right', type: 'io'    },
      { id: 'CANL', side: 'right', type: 'io'    },
      { id: 'VREF', side: 'right', type: 'analog' },
      { id: 'RS',   side: 'right', type: 'ctrl'  },
    ],
  },
  logic_analyzer: {
    label: 'Logic Analyzer (8-Channel)',
    category: 'Instrument',
    color: '#00b140',
    properties: [
      { id: 'sample_rate', label: 'Sample Rate (MHz)', type: 'number', default: 24 }
    ],
    pins: [
      ...Array.from({length: 8}, (_, i) => ({ id: `CH${i}`, side: 'left', type: 'io' })),
      { id: 'GND', side: 'right', type: 'power' },
      { id: 'CLK', side: 'right', type: 'ctrl' },
    ],
  },
  can_debugger: {
    label: 'CAN Bus Debugger',
    category: 'Instrument',
    color: '#d13438',
    properties: [
      { id: 'baud', label: 'Baud Rate (kbps)', type: 'number', default: 500 }
    ],
    pins: [
      { id: 'CANH', side: 'left', type: 'io' },
      { id: 'CANL', side: 'left', type: 'io' },
    ],
  },
  eusart_terminal: {
    label: 'EUSART Terminal',
    category: 'Instrument',
    color: '#0078d4',
    properties: [
      { id: 'baud', label: 'Baud Rate (bps)', type: 'number', default: 9600 }
    ],
    pins: [
      { id: 'RX', side: 'left', type: 'io' },
      { id: 'TX', side: 'right', type: 'io' },
    ],
  },
  dac_monitor: {
    label: 'DAC Monitor',
    category: 'Instrument',
    color: '#107c10',
    properties: [
      { id: 'range', label: 'Voltage Range', type: 'text', default: '0-5V' }
    ],
    pins: [
      { id: 'IN', side: 'left', type: 'analog' },
      { id: 'REF', side: 'left', type: 'power' },
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
