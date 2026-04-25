/**
 * SimulationStatusDialog.js
 * Shows critical alerts when services are down or errors occur.
 */

export class SimulationStatusDialog {
    constructor(client) {
        this.client = client;
        this.overlay = document.createElement('div');
        this.overlay.className = 'status-overlay';
        this.overlay.style.display = 'none';

        this.dialog = document.createElement('div');
        this.dialog.className = 'status-dialog glass';
        this.overlay.appendChild(this.dialog);

        document.body.appendChild(this.overlay);

        // Listen for connection status
        this.client.on('status', (s) => this._handleConnection(s.connected));
        
        // Listen for bridge errors
        this.client.on('message', (m) => {
            if (m.type === 'error') this.showError(m.message);
        });
    }

    _handleConnection(connected) {
        if (!connected) {
            this.showError('Lost connection to VioSpice Gateway. Ensure the gateway service is running.');
        } else {
            // Check if we were in an error state due to connection
            if (this.currentReason?.includes('Gateway')) {
                this.hide();
            }
        }
    }

    showError(reason) {
        this.currentReason = reason;
        
        // Specialized messages for common errors
        let title = 'Simulation Error';
        let icon = '⚠️';
        let detail = reason;
        let action = '';

        if (reason.includes('SHM')) {
            title = 'VioAVR Daemon Not Found';
            icon = '📡';
            detail = 'The simulation bridge cannot connect to the hardware daemon. Is the <b>vioavr-daemon</b> running?';
            action = '<p class="status-hint">Try running: <code>./bin/viospice_daemon</code> in your terminal.</p>';
        }

        this.dialog.innerHTML = `
            <div class="status-header">
                <span class="status-icon">${icon}</span>
                <h2>${title}</h2>
            </div>
            <div class="status-body">
                <p>${detail}</p>
                ${action}
            </div>
            <div class="status-footer">
                <button class="btn-primary" onclick="window.location.reload()">Retry Connection</button>
            </div>
        `;

        this.overlay.style.display = 'flex';
    }

    hide() {
        this.overlay.style.display = 'none';
        this.currentReason = null;
    }
}
