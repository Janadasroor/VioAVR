import { $ } from "../core/Utils.js";
import { Command } from "../core/History.js";

export class AddChipCommand extends Command {
  constructor(make,parent,x,y,type,def){super();Object.assign(this,{make,parent,x,y,type,def});this.chip=null;}
  execute(){ this.chip = this.chip ? this.parent.appendChild(this.chip) : this.make(this.parent,this.x,this.y,this.type,this.def); }
  undo(){ this.chip.remove(); }
}
export class DeleteChipCommand extends Command {
  constructor(chip,parent,findWires){super();this.chip=chip;this.parent=parent;this.findWires=findWires;this.wires=[];}
  execute(){ this.wires=this.findWires(this.chip); this.wires.forEach(w=>w.remove()); this.chip.remove(); }
  undo(){ this.parent.appendChild(this.chip); this.wires.forEach(w=>this.parent.appendChild(w)); }
}
export class MoveChipCommand extends Command {
  constructor(chip,oldX,oldY,newX,newY,cb){super();Object.assign(this,{chip,oldX,oldY,newX,newY,cb});}
  _apply(x,y){ this.chip.dataset.x=x; this.chip.dataset.y=y;
    const r=parseInt(this.chip.dataset.rotation)||0;
    const bw=parseFloat(this.chip.dataset.bw)/2, bh=parseFloat(this.chip.dataset.bh)/2;
    this.chip.setAttribute('transform',`translate(${x}, ${y})${r?` rotate(${r}, ${bw}, ${bh})`:''}`);
    this.cb&&this.cb(); }
  execute(){ this._apply(this.newX,this.newY); }
  undo(){ this._apply(this.oldX,this.oldY); }
}
export class RotateChipCommand extends Command {
  constructor(chip,oldR,newR,cb){super();Object.assign(this,{chip,oldR,newR,cb});}
  _apply(r){ this.chip.dataset.rotation=r;
    const bw=parseFloat(this.chip.dataset.bw)/2, bh=parseFloat(this.chip.dataset.bh)/2;
    this.chip.setAttribute('transform',`translate(${this.chip.dataset.x}, ${this.chip.dataset.y}) rotate(${r}, ${bw}, ${bh})`);
    this.cb&&this.cb(); }
  execute(){ this._apply(this.newR); } undo(){ this._apply(this.oldR); }
}
export class AddWireCommand extends Command {
  constructor(wire,parent){super();this.wire=wire;this.parent=parent;}
  execute(){ this.parent.appendChild(this.wire); } undo(){ this.wire.remove(); }
}
export class DeleteWireCommand extends Command {
  constructor(wire,parent){super();this.wire=wire;this.parent=parent;}
  execute(){ this.wire.remove(); } undo(){ this.parent.appendChild(this.wire); }
}