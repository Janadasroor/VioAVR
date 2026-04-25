import { Command } from '../core/History.js';
import { updateWirePath } from './Renderer.js';

export class AddChipCommand extends Command {
  constructor(factory, parent, x, y, type, name, pins) {
    super();
    this.factory = factory;
    this.parent = parent;
    this.x = x; this.y = y; this.type = type; this.name = name; this.pins = pins;
    this.chip = null;
  }
  execute() {
    if (!this.chip) {
      this.chip = this.factory(this.parent, this.x, this.y, this.type, this.name, this.pins);
    } else {
      this.parent.appendChild(this.chip);
    }
  }
  undo() { this.chip.remove(); }
}

export class DeleteChipCommand extends Command {
  constructor(chip, parent) {
    super();
    this.chip = chip;
    this.parent = parent;
  }
  execute() { this.chip.remove(); }
  undo() { this.parent.appendChild(this.chip); }
}

export class RotateChipCommand extends Command {
  constructor(chip, oldRot, newRot, onUpdate) {
    super();
    this.chip = chip;
    this.oldRot = oldRot;
    this.newRot = newRot;
    this.onUpdate = onUpdate;
  }
  execute() {
    this.chip.dataset.rotation = this.newRot;
    const x = this.chip.dataset.x;
    const y = this.chip.dataset.y;
    this.chip.setAttribute('transform', `translate(${x}, ${y}) rotate(${this.newRot}, 75, ${parseFloat(this.chip.querySelector('.node-body').getAttribute('height')) / 2})`);
    if (this.onUpdate) this.onUpdate();
  }
  undo() {
    this.chip.dataset.rotation = this.oldRot;
    const x = this.chip.dataset.x;
    const y = this.chip.dataset.y;
    this.chip.setAttribute('transform', `translate(${x}, ${y}) rotate(${this.oldRot}, 75, ${parseFloat(this.chip.querySelector('.node-body').getAttribute('height')) / 2})`);
    if (this.onUpdate) this.onUpdate();
  }
}

export class MoveChipCommand extends Command {
  constructor(chip, oldX, oldY, newX, newY, onUpdate) {
    super();
    this.chip = chip;
    this.oldX = oldX; this.oldY = oldY;
    this.newX = newX; this.newY = newY;
    this.onUpdate = onUpdate;
  }
  execute() {
    this.chip.setAttribute('transform', `translate(${this.newX}, ${this.newY})`);
    this.chip.dataset.x = this.newX;
    this.chip.dataset.y = this.newY;
    if (this.onUpdate) this.onUpdate();
  }
  undo() {
    this.chip.setAttribute('transform', `translate(${this.oldX}, ${this.oldY})`);
    this.chip.dataset.x = this.oldX;
    this.chip.dataset.y = this.oldY;
    if (this.onUpdate) this.onUpdate();
  }
}

export class AddWireCommand extends Command {
  constructor(wire, parent) {
    super();
    this.wire = wire;
    this.parent = parent;
  }
  execute() { this.parent.appendChild(this.wire); }
  undo() { this.wire.remove(); }
}

export class DeleteWireCommand extends Command {
  constructor(wire, parent) {
    super();
    this.wire = wire;
    this.parent = parent;
  }
  execute() { this.wire.remove(); }
  undo() { this.parent.appendChild(this.wire); }
}

export class UpdateWirePointsCommand extends Command {
  constructor(wire, oldPoints, newPoints) {
    super();
    this.wire = wire;
    this.oldPoints = oldPoints;
    this.newPoints = newPoints;
  }
  execute() {
    this.wire.dataset.points = JSON.stringify(this.newPoints);
    updateWirePath(this.wire);
  }
  undo() {
    this.wire.dataset.points = JSON.stringify(this.oldPoints);
    updateWirePath(this.wire);
  }
}

export class SplitWireCommand extends Command {
  constructor(parent, oldWire, newWires) {
    super();
    this.parent = parent;
    this.oldWire = oldWire;
    this.newWires = newWires;
  }
  execute() {
    this.oldWire.remove();
    this.newWires.forEach(w => this.parent.appendChild(w));
  }
  undo() {
    this.newWires.forEach(w => w.remove());
    this.parent.appendChild(this.oldWire);
  }
}

export class MoveWireSegmentCommand extends Command {
  constructor(wire, oldData, newData) {
    super();
    this.wire = wire;
    this.oldData = oldData;
    this.newData = newData;
  }
  execute() {
    this.wire.dataset.points = JSON.stringify(this.newData.points);
    updateWirePath(this.wire);
  }
  undo() {
    this.wire.dataset.points = JSON.stringify(this.oldData.points);
    updateWirePath(this.wire);
  }
}
