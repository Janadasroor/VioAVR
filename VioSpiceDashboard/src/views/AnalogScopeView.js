import { $ } from "../core/Utils.js";
import { fitCanvas } from "../core/CanvasUtils.js";

export class AnalogScopeView {
  init(){
    const host=$('#view-analog');
    host.innerHTML=`<div class="scope-layout">
      <div class="scope-side">
        <div class="control-group"><label>V / DIV</label><div class="knob">1V</div></div>
        <div class="control-group"><label>T / DIV</label><div class="knob">5ms</div></div>
        <div class="control-group"><label>COUPLING</label><div class="knob">DC</div></div>
      </div>
      <div class="instrument-viewport scope-screen" id="as-viewport">
        <canvas id="analog-page-canvas"></canvas>
        <div class="scope-readout" id="as-readout">Vpp 0.00 V<br>f ≈ 0 Hz</div>
      </div>
    </div>`;
    this.canvas=$('#analog-page-canvas'); this.viewport=$('#as-viewport');
    this.hist=new Array(300).fill(.5); this.frozen=false;
    window.addEventListener('resize',()=>this._draw(true));
  }
  onShow(){ this._draw(true); }
  freeze(){ this.frozen=true; this.viewport.classList.add('frozen'); }
  unfreeze(){ this.frozen=false; this.viewport.classList.remove('frozen'); }
  update(dac){ if(this.frozen) return;
    this.hist.push(dac?dac.voltage:.5); this.hist.shift(); this._draw(); }
  _draw(force=false){
    const r=this.viewport.getBoundingClientRect(); if(r.width<10)return;
    if(force||!this.ctx){ const f=fitCanvas(this.canvas); this.ctx=f.ctx; this.w=f.w; this.h=f.h; }
    const ctx=this.ctx,w=this.w,h=this.h; if(!ctx)return;
    ctx.clearRect(0,0,w,h);
    ctx.strokeStyle='rgba(74,222,128,.07)';
    for(let x=0;x<w;x+=44){ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,h);ctx.stroke();}
    for(let y=0;y<h;y+=44){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}
    const step=w/300, col=this.frozen?'#f87171':'#4ade80';
    ctx.strokeStyle=col; ctx.lineWidth=2.4; ctx.shadowColor=col; ctx.shadowBlur=10;
    ctx.beginPath();
    this.hist.forEach((v,i)=>{ const x=i*step,y=h-(v*h*.8+h*.1); i?ctx.lineTo(x,y):ctx.moveTo(x,y); });
    ctx.stroke(); ctx.shadowBlur=0;
    const mn=Math.min(...this.hist),mx=Math.max(...this.hist);
    let cross=0; for(let i=1;i<300;i++) if(this.hist[i-1]<.5&&this.hist[i]>=.5) cross++;
    $('#as-readout').innerHTML=`Vpp ${((mx-mn)*5).toFixed(2)} V<br>f ≈ ${(cross/(300*.05)).toFixed(1)} Hz`;
  }
}
