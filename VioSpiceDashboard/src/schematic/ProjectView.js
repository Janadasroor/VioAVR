/**
 * ProjectView.js
 * Manages the Project tab in the sidebar, providing pre-configured
 * tests and examples for the user to load.
 */

const TESTS = [
  {
    id: 'test_interrupt',
    name: 'External Interrupt Stress',
    desc: 'Verifies INT0 response time and vector jumping under load.',
    badge: 'Hardware Fidelity',
    type: 'test',
    details: {
      pins: ['PD2: External Interrupt 0 (Input)'],
      jobs: ['Interrupt Vector Validation', 'Context Switch Timing', 'SREG Preservation']
    },
    data: {
      chips: [
        { id: 'mcu', type: 'atmega328p', x: 100, y: 100, properties: { binary: 'tests/test_interrupt.hex' } },
        { id: 'btn', type: 'button', x: 400, y: 100 }
      ],
      wires: [
        { start: { chipId: 'mcu', pinId: 'PD2' }, end: { chipId: 'btn', pinId: '1' }, points: [{x:250, y:230}, {x:400, y:230}] }
      ]
    }
  },
  {
    id: 'test_uart',
    name: 'UART Protocol Validation',
    desc: 'Checks baud rate accuracy and frame parity handling.',
    badge: 'Protocol Compliance',
    type: 'test',
    details: {
      pins: ['PD1: TX (Output)', 'PD0: RX (Input)'],
      jobs: ['Baud Rate Tolerance', 'Frame Parity Check', 'FIFO Buffer Overflow']
    },
    data: {
      chips: [
        { id: 'mcu', type: 'atmega328p', x: 100, y: 100, properties: { binary: 'tests/firmware/uart_test_compare.hex' } },
        { id: 'term', type: 'eusart_terminal', x: 450, y: 100 }
      ],
      wires: [
        { start: { chipId: 'mcu', pinId: 'PD1' }, end: { chipId: 'term', pinId: 'RX' }, points: [{x:250, y:215}, {x:450, y:215}] }
      ]
    }
  }
];

const EXAMPLES = [
  {
    id: 'ex_blinky',
    name: 'Blinky (Hello World)',
    desc: 'The classic LED toggle. Uses PB0 on ATmega328P.',
    badge: 'Getting Started',
    type: 'example',
    details: {
      pins: ['PB0: Status LED (Output)'],
      jobs: ['Basic I/O Toggling', 'Wait-Loop Timing', 'GPIO Direction Setup']
    },
    data: {
      chips: [
        { id: 'mcu', type: 'atmega328p', x: 100, y: 100, properties: { binary: 'tests/blink.hex' } },
        { id: 'led', type: 'led', x: 420, y: 100 }
      ],
      wires: [
        { start: { chipId: 'mcu', pinId: 'PB0' }, end: { chipId: 'led', pinId: 'A' }, points: [{x:250, y:215}, {x:420, y:215}] }
      ]
    }
  },
  {
    id: 'ex_counter',
    name: '7-Segment Counter',
    desc: 'Interactive counter driven by Timer1 PWM interrupts.',
    badge: 'Display Control',
    type: 'example',
    details: {
      pins: ['PB0-PB6: Segment Bus', 'PB7: Decimal Point'],
      jobs: ['7-Segment Multiplexing', 'Timer1 CTC Mode', 'Pattern Translation']
    },
    data: {
      chips: [
        { id: 'mcu', type: 'atmega328p', x: 100, y: 100, properties: { binary: 'tests/firmware/timer1_interrupt_compare.hex' } },
        { id: 'seg', type: 'seven_seg', x: 420, y: 150 }
      ],
      wires: [
        { start: { chipId: 'mcu', pinId: 'PB0' }, end: { chipId: 'seg', pinId: 'A' }, points: [{x:250, y:215}, {x:420, y:215}] },
        { start: { chipId: 'mcu', pinId: 'PB1' }, end: { chipId: 'seg', pinId: 'B' }, points: [{x:250, y:237}, {x:420, y:237}] }
      ]
    }
  },
  {
    id: 'ex_adc_pwm',
    name: 'ADC to PSC Bridge',
    desc: 'High-speed motor control loop using AT90PWM power stage.',
    badge: 'Mixed Signal',
    details: {
      pins: ['PD1: Status LED', 'ADC0: Control Input', 'PSC0: Power Output'],
      jobs: ['ADC Sampling Jitter', 'PSC Synchronization', 'PWM Waveform Shaping']
    },
    type: 'example',
    data: {
      chips: [
        { id: 'mcu', type: 'at90pwm316', x: 100, y: 100, properties: { binary: 'tests/firmware/adc_pwm.hex' } },
        { id: 'led', type: 'led', x: 450, y: 100 },
        { id: 'pot', type: 'pot', x: 100, y: 400 }
      ],
      wires: [
        { start: { chipId: 'mcu', pinId: 'PD1' }, end: { chipId: 'led', pinId: 'A' }, points: [{x:250, y:215}, {x:450, y:215}] }
      ]
    }
  }
];

export class ProjectView {
  constructor(editor) {
    this.editor = editor;
    this.testList = document.getElementById('test-list');
    this.exampleList = document.getElementById('example-list');
    
    this.render();
  }

  render() {
    this.testList.innerHTML = '';
    this.exampleList.innerHTML = '';

    TESTS.forEach(test => this._createItem(test, this.testList));
    EXAMPLES.forEach(ex => this._createItem(ex, this.exampleList));
  }

  _createItem(item, container) {
    const el = document.createElement('div');
    el.className = 'project-item';
    el.innerHTML = `
      <div class="project-item-header">
        <div class="name">${item.name}</div>
        <button class="info-btn" title="View Details">
          <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
            <circle cx="12" cy="12" r="10"/><path d="M12 16v-4M12 8h.01"/>
          </svg>
        </button>
      </div>
      <div class="desc">${item.desc}</div>
      <div class="badge badge-${item.type}">${item.badge}</div>
    `;
    
    // Load project on click (unless info button clicked)
    el.onclick = (e) => {
      if (e.target.closest('.info-btn')) {
        this.showDetails(item);
        return;
      }
      if (confirm(`Load project "${item.name}"? Current schematic will be cleared.`)) {
        this.editor.deserialize(JSON.stringify(item.data));
        window.__vio.shell.showToast(`Loaded: ${item.name}`, 'success');
      }
    };
    
    container.appendChild(el);
  }

  showDetails(item) {
    const modal = document.createElement('div');
    modal.className = 'project-details-overlay';
    modal.innerHTML = `
      <div class="project-details-card">
        <div class="details-header">
          <h3>${item.name}</h3>
          <button class="close-btn">&times;</button>
        </div>
        <div class="details-content">
          <div class="details-section">
            <label>Pin Configuration</label>
            <ul>${item.details.pins.map(p => `<li>${p}</li>`).join('')}</ul>
          </div>
          <div class="details-section">
            <label>Operational Jobs</label>
            <ul>${item.details.jobs.map(j => `<li>${j}</li>`).join('')}</ul>
          </div>
        </div>
        <div class="details-footer">
          <button class="btn-primary load-direct">Load Example</button>
        </div>
      </div>
    `;

    modal.querySelector('.close-btn').onclick = () => modal.remove();
    modal.querySelector('.load-direct').onclick = () => {
        this.editor.deserialize(JSON.stringify(item.data));
        window.__vio.shell.showToast(`Loaded: ${item.name}`, 'success');
        modal.remove();
    };

    document.body.appendChild(modal);
  }
}
