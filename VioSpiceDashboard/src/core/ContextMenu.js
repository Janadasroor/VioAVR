/**
 * ContextMenu.js
 * A premium, glassmorphic context menu for the VioSpice Dashboard.
 */
export class ContextMenu {
  constructor() {
    this.element = null;
    this.isOpen = false;
    this._setupElement();
  }

  _setupElement() {
    this.element = document.createElement('div');
    this.element.className = 'context-menu';
    this.element.style.display = 'none';
    document.body.appendChild(this.element);

    window.addEventListener('click', () => this.close());
    window.addEventListener('contextmenu', (e) => {
      // Allow default context menu on input/textarea
      if (e.target.tagName === 'INPUT' || e.target.tagName === 'TEXTAREA') return;
      
      e.preventDefault();
      this.show(e.clientX, e.clientY, this._getMenuItems(e.target));
    });
  }

  show(x, y, items) {
    if (!items || items.length === 0) return;

    this.element.innerHTML = '';
    items.forEach(item => {
      if (item.type === 'divider') {
        const div = document.createElement('div');
        div.className = 'menu-divider';
        this.element.appendChild(div);
        return;
      }

      const btn = document.createElement('button');
      btn.className = `menu-item ${item.className || ''}`;
      btn.innerHTML = `
        <span class="menu-icon">${item.icon || ''}</span>
        <span class="menu-label">${item.label}</span>
        <span class="menu-shortcut">${item.shortcut || ''}</span>
      `;
      btn.onclick = (e) => {
        e.stopPropagation();
        item.action();
        this.close();
      };
      this.element.appendChild(btn);
    });

    this.element.style.display = 'flex';
    this.isOpen = true;

    // Position adjustment to keep menu in viewport
    const rect = this.element.getBoundingClientRect();
    if (x + rect.width > window.innerWidth) x -= rect.width;
    if (y + rect.height > window.innerHeight) y -= rect.height;

    this.element.style.left = `${x}px`;
    this.element.style.top = `${y}px`;
    
    requestAnimationFrame(() => {
        this.element.classList.add('show');
    });
  }

  close() {
    if (!this.isOpen) return;
    this.element.classList.remove('show');
    setTimeout(() => {
        if (!this.element.classList.contains('show')) {
            this.element.style.display = 'none';
        }
    }, 200);
    this.isOpen = false;
  }

  _getMenuItems(target) {
    const items = [];
    const vio = window.__vio;

    // 1. Component Actions (Priority)
    const chipNode = target.closest('.schematic-node');
    if (chipNode && vio.editor) {
        items.push({ 
            label: `Component: ${chipNode.dataset.label}`, 
            icon: '📦', 
            className: 'header',
            action: () => {} 
        });
        items.push({ type: 'divider' });
        items.push({ 
            label: 'Rotate', 
            icon: '🔄', 
            shortcut: 'R',
            action: () => {
                vio.editor._selectedChip = chipNode;
                vio.editor._rotateSelectedChip();
            } 
        });
        items.push({ 
            label: 'Duplicate', 
            icon: '👯', 
            shortcut: 'Ctrl+D',
            action: () => {
                vio.editor._selectedChip = chipNode;
                vio.editor._duplicate();
            } 
        });
        items.push({ 
            label: 'Properties', 
            icon: '⚙️', 
            action: () => {
                vio.editor._selectChip(chipNode);
                vio.shell.switchView('view-schematic');
            } 
        });
        items.push({ 
            label: 'Load Firmware', 
            icon: '⚡', 
            action: () => {
                const path = prompt('Enter HEX file path:', chipNode.dataset.binary || '');
                if (path) {
                    chipNode.dataset.binary = path;
                    vio.sim.loadHex(path);
                }
            } 
        });
        items.push({ type: 'divider' });
        items.push({ 
            label: 'Hot-Swap: AT90PWM316', 
            icon: '⚡', 
            action: () => vio.editor.swapComponent(chipNode.dataset.id, 'at90pwm316') 
        });
        items.push({ 
            label: 'Hot-Swap: ATmega6490P', 
            icon: '📟', 
            action: () => vio.editor.swapComponent(chipNode.dataset.id, 'atmega6490p') 
        });
        items.push({ type: 'divider' });
        items.push({ 
            label: 'Delete', 
            icon: '🗑️', 
            shortcut: 'Del',
            action: () => {
                vio.editor._selectedChip = chipNode;
                vio.editor._deleteSelectedChip();
            } 
        });
        return items; // Return early if it's a chip
    }

    // 2. Global items
    items.push({ 
        label: 'Refresh Dashboard', 
        icon: '♻️', 
        action: () => window.location.reload() 
    });

    // 3. Schematic items
    if (target.closest('.schematic-canvas')) {
        items.push({ type: 'divider' });
        items.push({ 
            label: 'Add Component', 
            icon: '➕', 
            action: () => {
                vio.shell.switchView('view-schematic');
                // Open the library tab
                document.querySelector('[data-tab="library"]')?.click();
            } 
        });
        items.push({ 
            label: 'Export Netlist', 
            icon: '📄', 
            action: () => document.getElementById('btn-export')?.click() 
        });
        items.push({ 
            label: 'Zoom Fit', 
            icon: '🔍', 
            shortcut: 'F',
            action: () => vio.editor.viewport.zoomFit(vio.editor.canvas.querySelectorAll('.schematic-node'))
        });
    }

    // 4. Register items
    if (target.closest('.state-card')) {
        items.push({ type: 'divider' });
        items.push({ 
            label: 'Reset Registers', 
            icon: '🧹', 
            action: () => vio.sim.reset() 
        });
        items.push({ 
            label: 'Export Trace', 
            icon: '📈', 
            action: () => vio.sim.exportTrace() 
        });
    }

    return items;
  }
}
