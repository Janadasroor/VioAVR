/**
 * TerminalView.js
 * Manages EUSART terminal display on the schematic.
 */
export class TerminalView {
    constructor(canvas) {
        this.canvas = canvas || document.getElementById('schematic-content');
        this.history = [];
    }

    update(eusart) {
        if (!eusart || !eusart.data) return;
        this.history.push(eusart.data);
        if (this.history.length > 6) this.history.shift();

        const terminalNodes = this.canvas.querySelectorAll('.node-type-eusart_terminal');
        terminalNodes.forEach(node => {
            this.history.forEach((line, i) => {
                const lineEl = node.querySelector(`.terminal-line-${i}`);
                if (lineEl) lineEl.textContent = `> ${line}`;
            });
        });
    }
}
