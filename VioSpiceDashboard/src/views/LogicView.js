/**
 * LogicView.js
 * Renders logic analyzer traces on the schematic.
 */
export class LogicView {
    constructor(canvas) {
        this.canvas = canvas || document.getElementById('schematic-content');
        this.logicHistory = Array.from({ length: 8 }, () => new Array(100).fill(0));
    }

    update(pins) {
        if (!pins) return;
        const analyzerNodes = this.canvas.querySelectorAll('.node-type-logic_analyzer');
        analyzerNodes.forEach(node => {
            for (let i = 0; i < 8; i++) {
                const val = pins[i] === 1 ? 1 : 0;
                this.logicHistory[i].push(val);
                if (this.logicHistory[i].length > 100) this.logicHistory[i].shift();

                const trace = node.querySelector(`.logic-trace-${i}`);
                if (trace) {
                    const yBase = 35 + i * 15, width = 175, step = width / 100;
                    let d = `M 45 ${yBase}`;
                    this.logicHistory[i].forEach((v, idx) => {
                        const x = 45 + idx * step;
                        const y = v === 1 ? yBase - 8 : yBase;
                        if (idx > 0 && v !== this.logicHistory[i][idx-1]) {
                            d += ` L ${x} ${v === 1 ? yBase : yBase - 8}`;
                        }
                        d += ` L ${x} ${y}`;
                    });
                    trace.setAttribute('d', d);
                }
            }
        });
    }
}
