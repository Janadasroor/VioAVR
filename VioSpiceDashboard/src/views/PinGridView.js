/**
 * PinGridView.js
 * Renders the LED grid for physical I/O pins.
 */
export class PinGridView {
    constructor() {
        this.container = document.getElementById('pin-grid');
        this.leds = [];
    }

    init() {
        if (!this.container) return;
        this.container.innerHTML = '';
        // Show 32 pins by default
        for (let i = 0; i < 32; i++) {
            const item = document.createElement('div');
            item.className = 'pin-item';
            item.innerHTML = `
                <div class="pin-led" id="pin-led-${i}"></div>
                <span class="pin-label">P${i}</span>
            `;
            this.container.appendChild(item);
            this.leds[i] = item.querySelector('.pin-led');
        }
    }

    update(telemetry) {
        if (!telemetry.digital_outputs) return;
        telemetry.digital_outputs.forEach((val, i) => {
            if (this.leds[i]) {
                this.leds[i].classList.toggle('high', val === 1);
            }
        });
    }
}
