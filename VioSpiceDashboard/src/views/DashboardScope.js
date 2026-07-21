import { $ } from "../core/Utils.js";
import { fitCanvas } from "../core/CanvasUtils.js";

export class DashboardScope {
  init(){ this.canvas=$('#scope-canvas'); this.a=new Array(300).fill(.5); this.d=new Array(300).fill(0);
    this._toggles=new Uint16Array(128); this._prevOut=new Uint8Array(128); this._activeIdx=8; this._sampleCount=0;
    this._fit=()=>this._draw(true); window.addEventListener('resize',this._fit); this._draw(true); }
  onShow(){ this._draw(true); }
  _pickActive(douts){
    if(!douts) return;
    for(let i=0;i<Math.min(128,douts.length);i++){
      if(douts[i]!==this._prevOut[i]){ this._toggles[i]++; this._prevOut[i]=douts[i]; }
    }
    this._sampleCount++;
    if(this._sampleCount%10===0){
      let best=0, bestIdx=this._activeIdx;
      for(let i=0;i<Math.min(32,this._toggles.length);i++){
        if(this._toggles[i]>best){ best=this._toggles[i]; bestIdx=i; }
        if(this._sampleCount%100===0) this._toggles[i]=this._toggles[i]>>1;
      }
      this._activeIdx=bestIdx;
      const p=String.fromCharCode(65+Math.floor(bestIdx/8));
      $('#scope-dig-label').textContent=`P${p}${bestIdx%8}`;
    }
  }
  update(t){ this.a.push(t.dac?t.dac.voltage:.5); this.a.shift();
    this._pickActive(t.digital_outputs);
    const v=t.digital_outputs&&t.digital_outputs[this._activeIdx]===1?1:0;
    this.d.push(v); this.d.shift(); this._draw(); }
  _draw(force=false){
    const r=this.canvas.parentElement.getBoundingClientRect();
    if(r.width<10) return;
    if(force||this.canvas.width!==Math.floor(r.width*(devicePixelRatio||1))){ const f=fitCanvas(this.canvas); this.ctx=f.ctx; this.w=f.w; this.h=f.h; }
    const ctx=this.ctx,w=this.w,h=this.h; if(!ctx)return;
    ctx.clearRect(0,0,w,h);
    ctx.strokeStyle='rgba(148,163,184,.08)'; ctx.lineWidth=1;
    for(let x=0;x<w;x+=40){ctx.beginPath();ctx.moveTo(x,0);ctx.lineTo(x,h);ctx.stroke();}
    for(let y=0;y<h;y+=32){ctx.beginPath();ctx.moveTo(0,y);ctx.lineTo(w,y);ctx.stroke();}
    const analogH=h*.62, step=w/300;
    ctx.strokeStyle='#22d3ee'; ctx.lineWidth=2; ctx.shadowColor='rgba(34,211,238,.6)'; ctx.shadowBlur=6;
    ctx.beginPath();
    this.a.forEach((v,i)=>{ const x=i*step,y=analogH-v*(analogH*.86)-6; i?ctx.lineTo(x,y):ctx.moveTo(x,y); });
    ctx.stroke(); ctx.shadowBlur=0;
    const base=h-24, amp=34;
    ctx.strokeStyle='#8b5cf6'; ctx.lineWidth=2; ctx.shadowColor='rgba(139,92,246,.6)'; ctx.shadowBlur=5;
    ctx.beginPath();
    this.d.forEach((v,i)=>{ const x=i*step,y=base-v*amp;
      if(i&&this.d[i-1]!==v)ctx.lineTo(x,base-this.d[i-1]*amp),ctx.lineTo(x,y);
      i?ctx.lineTo(x,y):ctx.moveTo(x,y); });
    ctx.stroke(); ctx.shadowBlur=0;
    const port=String.fromCharCode(65+Math.floor(this._activeIdx/8));
    ctx.fillStyle='rgba(148,163,184,.55)'; ctx.font='9px JetBrains Mono';
    ctx.fillText('5V',6,14); ctx.fillText('0V',6,analogH); ctx.fillText(`P${port}${this._activeIdx%8}`,6,base-amp-4);
  }
}
