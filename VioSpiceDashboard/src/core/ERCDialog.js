/**
 * ERCDialog.js
 * Visual interface for Electrical Rules Check (ERC) results.
 * Supports filtering and ignore/fix actions.
 */

export class ERCDialog {
  constructor() {
    this.issues = [];
    this.filters = { error: true, warning: true };
    this.onConfirm = null;
  }

  open(issues, onConfirm) {
    this.issues = issues;
    this.onConfirm = onConfirm;
    
    this._render();
  }

  _render() {
    // Remove old overlay if exists
    document.querySelector('.erc-dialog-overlay')?.remove();

    const overlay = document.createElement('div');
    overlay.className = 'erc-dialog-overlay';
    
    overlay.innerHTML = `
      <div class="erc-dialog">
        <div class="erc-header">
          <h2>Electrical Rules Check</h2>
          <button class="close-btn" style="background:none; border:none; color:var(--text-dim); font-size:24px; cursor:pointer;">&times;</button>
        </div>
        <div class="erc-filters">
          <label class="filter-item">
            <input type="checkbox" data-filter="error" ${this.filters.error ? 'checked' : ''}>
            <span>Show Errors</span>
          </label>
          <label class="filter-item">
            <input type="checkbox" data-filter="warning" ${this.filters.warning ? 'checked' : ''}>
            <span>Show Warnings</span>
          </label>
        </div>
        <div class="erc-list" id="erc-items-container">
          <!-- Populated dynamically -->
        </div>
        <div class="erc-footer">
          <button class="btn-secondary close-dialog">Fix Issues</button>
          <button class="btn-primary run-anyway">Ignore & Run</button>
        </div>
      </div>
    `;

    document.body.appendChild(overlay);

    // Event Listeners
    overlay.querySelector('.close-btn').onclick = () => overlay.remove();
    overlay.querySelector('.close-dialog').onclick = () => overlay.remove();
    overlay.querySelector('.run-anyway').onclick = () => {
      if (this.onConfirm) this.onConfirm();
      overlay.remove();
    };

    overlay.querySelectorAll('.erc-filters input').forEach(input => {
      input.onchange = () => {
        this.filters[input.dataset.filter] = input.checked;
        this._populateItems(overlay.querySelector('#erc-items-container'));
      };
    });

    this._populateItems(overlay.querySelector('#erc-items-container'));
  }

  _populateItems(container) {
    container.innerHTML = '';
    const filtered = this.issues.filter(i => this.filters[i.level]);

    if (filtered.length === 0) {
      container.innerHTML = '<div style="padding:40px; text-align:center; color:var(--text-dim);">No issues matching current filters.</div>';
      return;
    }

    filtered.forEach(issue => {
      const el = document.createElement('div');
      el.className = `erc-item ${issue.level}`;
      el.innerHTML = `
        <div class="erc-icon">${issue.level === 'error' ? '❌' : '⚠️'}</div>
        <div class="erc-info">
          <div class="erc-msg">${issue.message}</div>
          <div class="erc-meta">Location: ${issue.chipId || 'Global'}</div>
        </div>
      `;
      container.appendChild(el);
    });
  }
}
