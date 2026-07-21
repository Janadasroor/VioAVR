export class Command { execute(){} undo(){} }
export class HistoryManager {
  constructor(){ this.stack=[]; this.redoStack=[]; this.onChange=null; }
  execute(cmd){ cmd.execute(); this.stack.push(cmd); this.redoStack=[]; if(this.stack.length>60)this.stack.shift(); this.onChange&&this.onChange(); }
  undo(){ const c=this.stack.pop(); if(c){c.undo(); this.redoStack.push(c); this.onChange&&this.onChange();} }
  redo(){ const c=this.redoStack.pop(); if(c){c.execute(); this.stack.push(c); this.onChange&&this.onChange();} }
}
