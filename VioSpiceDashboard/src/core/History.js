export class Command {
  execute() {}
  undo() {}
}

export class HistoryManager {
  constructor() {
    this.stack = [];
    this.redoStack = [];
    this.onChange = null;
  }

  execute(command) {
    command.execute();
    this.stack.push(command);
    this.redoStack = []; // Clear redo on new action
    if (this.stack.length > 50) this.stack.shift();
    this.onChange?.();
  }

  undo() {
    const cmd = this.stack.pop();
    if (cmd) {
      cmd.undo();
      this.redoStack.push(cmd);
      this.onChange?.();
    }
  }

  redo() {
    const cmd = this.redoStack.pop();
    if (cmd) {
      cmd.execute();
      this.stack.push(cmd);
      this.onChange?.();
    }
  }
}
