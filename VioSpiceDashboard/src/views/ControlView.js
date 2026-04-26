/**
 * ControlView.js
 * Manages simulation controls (Play/Pause, Reset) and connection status.
 */
export class ControlView {
    constructor() {
        this.playBtn = document.getElementById('btn-play');
        this.statusBadge = document.querySelector('.status-badge');
    }

    updateState(running) {
        if (!this.playBtn) return;
        this.playBtn.classList.toggle('running', running);
        this.playBtn.innerHTML = running
            ? `<svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor" style="margin-right:8px"><rect x="6" y="4" width="4" height="16"></rect><rect x="14" y="4" width="4" height="16"></rect></svg>Pause`
            : `<svg width="16" height="16" viewBox="0 0 24 24" fill="currentColor" style="margin-right:8px"><path d="M8 5v14l11-7z"/></svg>Run Simulation`;
    }

    updateConnection(connected) {
        if (!this.statusBadge) return;
        this.statusBadge.textContent = connected ? 'Simulator: Connected' : 'Simulator: Offline';
        this.statusBadge.className = `status-badge ${connected ? 'online' : 'offline'}`;
    }
}
