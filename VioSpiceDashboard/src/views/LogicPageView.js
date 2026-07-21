import { $ } from "../core/Utils.js";
import { fitCanvas } from "../core/CanvasUtils.js";

export class LogicAnalyzerView {
  init(){
    const host=$('#view-logic');
    host.innerHTML=`<div class="instrument-layout">
      <div class="instrument-toolbar">
        <div class="tb-group"><label>TIMEBASE</label><select><option>1 ms/div</option><option>5 ms/div</option><option>10 ms/div</option></select></div>
        <div class="tb-group"><label>TRIGGER</label><select><option>CH0 Rising</option><option>CH0 Falling</option><option>None</option></select></div>
        <button class="btn-action" id="la-single">Single</button>
        <span class="tb-status" id="la-status">● RUNNING</span>
      </div>
      <div class="instrument-viewport" id="la-viewport"><canvas id="logic-page-canvas"></canvas></div>
    </div>`;
    this.canvas=$('#logic-page-canvas'); this.viewport=$('#la-viewport');
    this.hist=Array.from({length:8},()=>new Array(240).fill(0));
    this.frozen=false;
    $('#la-single').onclick=()=>this.frozen?this.unfreeze():this.freeze(true);
    window.addEventListener('resize',()=>this._draw(true));
  }
  onShow(){ this._draw(true); }
  freeze(manual){ this.frozen=true; this.viewport.classList.add('frozen');
    const s=$('#la-status'); s.textContent='■ FROZEN'; s.classList.add('frozen'); }
  unfreeze(){ this.frozen=false; this.viewport.classList.remove('frozen');
    const s=$('#la-status'); s.textContent='● RUNNING'; s.classList.remove('frozen'); }
  update(pins){ if(!pins||this.frozen) return;
    for(let i=0;i<8;i++){ this.hist[i].push(pins[i]===1?1:0); this.hist[i].shift(); }
    this._draw(); }
  _draw(force=false){
    const r=this.viewport.getBoundingClientRect(); if(r.width<10)return;
    if(force||!this.ctx){ const f=fitCanvas(this.canvas); this.ctx=f.ctx; this.w=f.w; this.h=f.h; }
    const ctx=this.ctx,w=this.w,h=this.h; if(!ctx)return;
    ctx.clearRect(0,0,w,h);
    const ch=h/8, step=w/240;
    ctx.strokeStyle='rgba(148,163,184,.06)';
    for(let x=0;x<w;x+=60){ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,h);ctx.stroke();}
    for(let i=0;i<8;i++){
      const yB=(i+1)*ch-12;
      ctx.strokeStyle='#22c55e'; ctx.lineWidth=1.6; ctx.shadowColor='rgba(34,197,94,.5)'; ctx.shadowBlur=4;
      ctx.beginPath();
      this.hist[i].forEach((v,xi)=>{ const x=xi*step,y=yB-v*(ch*.55);
        if(xi&&this.hist[i][xi-1]!==v)ctx.lineTo(x,yB-this.hist[i][xi-1]*(ch*.55)),ctx.lineTo(x,y);
        xi?ctx.lineTo(x,y):ctx.moveTo(x,y); });
      ctx.stroke(); ctx.shadowBlur=0;
      ctx.fillStyle='rgba(148,163,184,.6)'; ctx.font='9px JetBrains Mono';
      ctx.fillText('CH'+i,8,yB-ch*.55-4);
    }
  }
}
