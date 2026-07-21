import { $, Toast } from "../core/Utils.js";

export class NvmView {
  init(){
    const host=$('#view-nvm');
    host.innerHTML=`<div class="nvm-page">
      <div class="nvm-top">
        <div class="nvm-stat"><label>FLASH</label><div class="v">32 KB</div></div>
        <div class="nvm-stat"><label>EEPROM</label><div class="v">1 KB</div></div>
        <div class="nvm-stat"><label>BOOTLOADER</label><div class="v">2 KB</div></div>
        <div class="nvm-stat" style="display:flex;align-items:center;gap:10px;justify-content:center">
          <button class="btn-action" id="nvm-program">Program</button>
          <button class="btn-action btn-danger" id="nvm-erase">Chip Erase</button>
        </div>
      </div>
      <div class="card"><div class="card-header"><h3>Flash Block Map</h3><span class="tag" id="flash-usage">0% USED</span></div>
        <div class="mem-grid" id="flash-grid" style="grid-template-columns:repeat(32,1fr)"></div>
        <div class="mem-legend"><span><i style="background:#141d30"></i>Erased</span><span><i style="background:#2b3a5c"></i>Metadata</span><span><i style="background:#6d4ac2"></i>Programmed</span></div></div>
      <div class="card"><div class="card-header"><h3>EEPROM Map</h3><span class="tag" id="eeprom-usage">0% USED</span></div>
        <div class="mem-grid" id="eeprom-grid" style="grid-template-columns:repeat(32,1fr)"></div></div>
    </div>`;
    this.flash=this._buildGrid($('#flash-grid'),1024);
    this.eeprom=this._buildGrid($('#eeprom-grid'),512);
    $('#nvm-program').onclick=()=>this.program();
    $('#nvm-erase').onclick=()=>{ this.flash.forEach(c=>c.className='mem-cell'); this.eeprom.forEach(c=>c.className='mem-cell');
      this._usage(); Toast.show('Chip erase complete','warning'); window.__vio.log.add('NVM: chip erase executed','warning'); };
    window.__vio.sim.on('hexLoaded',()=>this.program());
  }
  _buildGrid(host,n){
    const cells=[];
    for(let i=0;i<n;i++){ const d=document.createElement('div'); d.className='mem-cell'; host.appendChild(d); cells.push(d); }
    return cells;
  }
  program(){
    const seed=(Date.now()%100000)/100000;
    this.flash.forEach((c,i)=>{ const v=((i*2654435761)>>>0)%100; const roll=(v+seed*100)%100;
      c.className='mem-cell '+(roll<38?'u2':roll<50?'u1':''); });
    this.eeprom.forEach((c,i)=>{ const v=((i*40503)>>>0)%100;
      c.className='mem-cell '+(v<14?'u2':v<24?'u1':''); });
    let row=0;
    const sweep=setInterval(()=>{
      for(let x=0;x<32;x++){ const c=this.flash[row*32+x]; if(c){c.classList.add('writing'); setTimeout(()=>c.classList.remove('writing'),450);} }
      if(++row>=32) clearInterval(sweep);
    },40);
    this._usage();
    window.__vio.log.add('NVM: flash program sequence started','info');
  }
  _usage(){
    const pct=Math.round(this.flash.filter(c=>c.classList.contains('u2')).length/this.flash.length*100);
    $('#flash-usage').textContent=pct+'% USED';
    const epct=Math.round(this.eeprom.filter(c=>c.classList.contains('u2')).length/this.eeprom.length*100);
    $('#eeprom-usage').textContent=epct+'% USED';
  }
}
