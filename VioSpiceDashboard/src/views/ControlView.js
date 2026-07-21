import { $ } from "../core/Utils.js";

export class ControlView {
  constructor(){ this.playBtn=$('#btn-play'); this.modeBadge=$('#mode-badge'); this.sideStatus=$('#sidebar-status'); }
  updateState(running){
    this.playBtn.classList.toggle('running',running);
    this.playBtn.innerHTML=running
      ?'<svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor" style="margin-right:7px"><rect x="6" y="4" width="4" height="16"/><rect x="14" y="4" width="4" height="16"/></svg>Pause'
      :'<svg width="14" height="14" viewBox="0 0 24 24" fill="currentColor" style="margin-right:7px"><path d="M8 5v14l11-7z"/></svg>Run Simulation';
  }
  updateMode(mode){
    const map={gateway:['GATEWAY LIVE','online','Simulator: Connected'],demo:['DEMO MODE','demo','Simulator: Demo Telemetry'],connecting:['CONNECTING…','demo','Simulator: Connecting']};
    const [txt,cls,side]=map[mode]||map.connecting;
    this.modeBadge.textContent=txt; this.modeBadge.className='mode-badge '+cls;
    this.sideStatus.textContent=side; this.sideStatus.className='status-badge '+cls;
  }
}
