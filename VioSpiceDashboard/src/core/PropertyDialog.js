/**
 * PropertyDialog.js
 * A glassmorphic modal for editing component properties.
 */

import { getComponent } from '../schematic/ComponentLibrary.js';

export class PropertyDialog {
    constructor() {
        this.overlay = document.createElement('div');
        this.overlay.className = 'modal-overlay';
        this.overlay.style.display = 'none';

        this.dialog = document.createElement('div');
        this.dialog.className = 'property-dialog glass';
        this.overlay.appendChild(this.dialog);

        document.body.appendChild(this.overlay);
        
        this.overlay.onclick = (e) => {
            if (e.target === this.overlay) this.close();
        };
    }

    open(chip) {
        this.currentChip = chip;
        const type = chip.dataset.type;
        const def = getComponent(type);
        
        this.dialog.innerHTML = `
            <div class="dialog-header">
                <h2>${def ? def.label : 'Component'} Properties</h2>
                <button class="btn-close">×</button>
            </div>
            <div class="dialog-body">
                <div class="property-group">
                    <label>Instance Name</label>
                    <input type="text" id="prop-name" value="${chip.dataset.name || ''}">
                </div>
                <div id="custom-props"></div>
            </div>
            <div class="dialog-footer">
                <button class="btn-primary" id="btn-save-props">Apply Changes</button>
            </div>
        `;

        const customContainer = this.dialog.querySelector('#custom-props');
        if (def && def.properties) {
            def.properties.forEach(p => {
                const row = document.createElement('div');
                row.className = 'property-group';
                const val = chip.dataset[p.id] ?? p.default;
                row.innerHTML = `
                    <label>${p.label}</label>
                    <input type="${p.type === 'number' ? 'number' : 'text'}" 
                           id="prop-${p.id}" 
                           value="${val}">
                `;
                customContainer.appendChild(row);
            });
        }

        this.overlay.style.display = 'flex';
        
        this.dialog.querySelector('.btn-close').onclick = () => this.close();
        this.dialog.querySelector('#btn-save-props').onclick = () => this.save();
    }

    save() {
        const chip = this.currentChip;
        const type = chip.dataset.type;
        const def = getComponent(type);

        // Save name
        const newName = this.dialog.querySelector('#prop-name').value;
        chip.dataset.name = newName;
        const labelText = chip.querySelector('.node-label');
        if (labelText) labelText.textContent = newName;

        // Save custom properties
        if (def && def.properties) {
            def.properties.forEach(p => {
                const input = this.dialog.querySelector(`#prop-${p.id}`);
                if (input) {
                    chip.dataset[p.id] = input.value;
                }
            });
        }

        window.__vio.shell.showToast('Properties updated', 'success');
        this.close();
        
        // Trigger editor update
        if (window.__vio.editor) {
            window.__vio.editor._updatePropertyPanel();
        }
    }

    close() {
        this.overlay.style.display = 'none';
    }
}
