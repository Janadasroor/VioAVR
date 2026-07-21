import { $, $$, svgEl } from "../core/Utils.js";
import { digitGroup } from "../schematic/ChipFactory.js";

export class LcdGlassView {
  init(){
    const host=$('#view-lcd');
    host.innerHTML=`<div class="lcd-page">
      <div class="card"><div class="card-header"><h3>LCD Glass — 4 Digit Module</h3><span class="status-pill" id="lcd-big-pill">STANDBY</span></div>
        <div class="lcd-container"><div class="lcd-glass-frame"><div class="lcd-digits lcd-big" id="lcd-big"></div></div></div></div>
      <div class="card"><div class="card-header"><h3>Segment Matrix</h3><span class="tag">COM × SEG</span></div>
        <table class="lcd-matrix" id="lcd-matrix"></table>
        <div class="lcd-meta" id="lcd-meta">ENABLED <b>—</b> · DUTY <b>—</b> · SEGMENTS <b>—</b></div></div>
    </div>`;
    const big=$('#lcd-big'); this.digits=[];
    for(let d=0;d<4;d++){ const w=document.createElement('div'); w.className='lcd-digit';
      const svg=svgEl('svg',{viewBox:'0 0 60 80'},w); digitGroup(svg,''); big.appendChild(w); this.digits.push(w); }
    const tbl=$('#lcd-matrix'); this.cells=[];
    let html='<tr><th></th>'+Array.from({length:8},(_,s)=>`<th>SEG${s}</th>`).join('')+'</tr>';
    for(let c=0;c<4;c++){ html+=`<tr><th>COM${c}</th>`+Array.from({length:8},(_,s)=>`<td><div class="lcd-cell" data-c="${c}" data-s="${s}"></div></td>`).join('')+'</tr>'; }
    tbl.innerHTML=html;
    this.cells=$$('.lcd-cell',tbl);
  }
  update(lcd){
    const on=(d,i)=>lcd&&lcd.enabled?((lcd.data[i%4]>>(d*2+(i>>2)))&1)===1:false;
    this.digits.forEach((w,d)=>$$('.lcd-seg',w).forEach(seg=>{
      const id=seg.dataset.seg; if(id==='DP'){seg.classList.remove('active');return;}
      seg.classList.toggle('active',on(d,'ABCDEFG'.indexOf(id)));
    }));
    this.cells.forEach(cell=>{
      const c=+cell.dataset.c,s=+cell.dataset.s;
      cell.classList.toggle('on',!!(lcd&&lcd.enabled&&((lcd.data[c]>>s)&1)));
    });
    $('#lcd-big-pill').textContent=lcd&&lcd.enabled?'ACTIVE':'STANDBY';
    $('#lcd-meta').innerHTML=`ENABLED <b>${lcd&&lcd.enabled?'YES':'NO'}</b> · DUTY <b>${lcd?lcd.duty:'—'}</b> · SEGMENTS <b>${lcd?lcd.segments:'—'}</b>`;
  }
}
