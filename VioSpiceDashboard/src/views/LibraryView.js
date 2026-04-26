import { getAllComponents } from '../schematic/ComponentLibrary.js';

/**
 * LibraryView.js
 * Manages the component library sidebar panel.
 */
export class LibraryView {
    constructor() {
        this.listEl = document.querySelector('.chip-list');
        this.searchEl = document.getElementById('library-search');
    }

    init() {
        this._buildLibraryPanel();
        this._setupSearch();
    }

    _buildLibraryPanel() {
        if (!this.listEl) return;
        this.listEl.innerHTML = '';

        const grouped = {};
        getAllComponents().forEach(comp => {
            if (!grouped[comp.category]) grouped[comp.category] = [];
            grouped[comp.category].push(comp);
        });

        Object.entries(grouped).forEach(([category, comps]) => {
            const header = document.createElement('div');
            header.className = 'library-category';
            header.textContent = category;
            this.listEl.appendChild(header);

            comps.forEach(comp => {
                const item = document.createElement('div');
                item.className = 'chip-item';
                item.draggable = true;
                item.dataset.type = comp.type;
                item.innerHTML = `
                    <div class="chip-icon" style="--chip-color: ${comp.color}">
                        <span>${comp.label.substring(0, 3).toUpperCase()}</span>
                    </div>
                    <div class="chip-info">
                        <span class="chip-name">${comp.label}</span>
                        <span class="chip-pins">${comp.pins.length} pins</span>
                    </div>`;

                item.addEventListener('dragstart', e => e.dataTransfer.setData('type', comp.type));
                this.listEl.appendChild(item);
            });
        });
    }

    _setupSearch() {
        this.searchEl?.addEventListener('input', e => {
            const query = e.target.value.toLowerCase();
            const items = this.listEl.querySelectorAll('.chip-item');
            const categories = this.listEl.querySelectorAll('.library-category');
            
            items.forEach(item => {
                const name = item.querySelector('.chip-name').textContent.toLowerCase();
                item.style.display = name.includes(query) ? 'flex' : 'none';
            });

            categories.forEach(cat => {
                let hasVisible = false;
                let next = cat.nextElementSibling;
                while (next && !next.classList.contains('library-category')) {
                    if (next.style.display !== 'none') {
                        hasVisible = true;
                        break;
                    }
                    next = next.nextElementSibling;
                }
                cat.style.display = hasVisible ? 'block' : 'none';
            });
        });
    }
}
