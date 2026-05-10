/**
 * ERCManager.js
 * Performs Electrical Rules Check (ERC) on the schematic.
 * Returns a list of warnings and errors.
 */

import { getComponent } from '../schematic/ComponentLibrary.js';

export class ERCManager {
  static check(chips, wires) {
    const issues = [];

    // 1. Connectivity Map
    const pinConnections = new Map(); // pinElement -> [wires]
    
    wires.forEach(w => {
      const s = w.startPin;
      const e = w.endPin;
      if (s) {
        if (!pinConnections.has(s)) pinConnections.set(s, []);
        pinConnections.get(s).push(w);
      }
      if (e) {
        if (!pinConnections.has(e)) pinConnections.set(e, []);
        pinConnections.get(e).push(w);
      }
    });

    chips.forEach(chip => {
      const type = chip.dataset.type;
      const name = chip.dataset.label || chip.dataset.name || type;
      const def = getComponent(type);
      if (!def) return;

      let powerConnected = false;
      let hasPowerPins = false;

      def.pins.forEach(pDef => {
        const pinEl = chip.querySelector(`.pin-group[data-id="${pDef.id}"]`);
        const connections = pinConnections.get(pinEl) || [];
        const isConnected = connections.length > 0;

        // Rule: Floating Input/IO
        if (!isConnected && (pDef.type === 'io' || pDef.type === 'ctrl' || pDef.type === 'analog')) {
          // Exceptions: NC pins or optional peripherals
          if (!pDef.id.startsWith('NC') && !['AREF', 'RESET'].includes(pDef.id)) {
            issues.push({
              level: 'warning',
              message: `Floating pin: ${name}.${pDef.id}`,
              chipId: chip.dataset.id
            });
          }
        }

        // Track power connectivity
        if (pDef.type === 'power') {
          hasPowerPins = true;
          if (isConnected) powerConnected = true;
        }
      });

      // Rule: No Power (Ignore if using Virtual Supply)
      const isVirtualPower = def.pins.some(p => ['VCC', 'VDD', 'GND', 'VSS'].includes(p.id));
      if (hasPowerPins && !powerConnected && !['vsrc', 'gnd'].includes(type) && !isVirtualPower) {
        issues.push({
          level: 'error',
          message: `Power failure: ${name} has no VCC/GND connections.`,
          chipId: chip.dataset.id
        });
      }
    });

    // Rule: VCC to GND Short (Direct)
    wires.forEach((w, i) => {
        const sPin = w.startPin?.dataset?.id;
        const ePin = w.endPin?.dataset?.id;
        if ((sPin === 'VCC' && ePin === 'GND') || (sPin === 'GND' && ePin === 'VCC')) {
            issues.push({
                level: 'error',
                message: `Direct Short: VCC and GND are tied together in Wire #${i}.`,
                wireIndex: i
            });
        }
    });

    return issues;
  }
}
