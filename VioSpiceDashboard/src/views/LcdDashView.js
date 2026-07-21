import { $, $$, svgEl } from "../core/Utils.js";
import { digitGroup } from "../schematic/ChipFactory.js";

export class LcdDashView {
  init(){ const host=$('#dash-lcd'); host.innerHTML=''; this.digits=[];
    for(let d=0;d<4;d++){ const wrap=document.createElement('div'); wrap.className='lcd-digit';
      const svg=svgEl('svg',{viewBox:'0 0 60 80'},wrap); digitGroup(svg,'');
      host.appendChild(wrap); this.digits.push(wrap); } }
  update(lcd){
    const on=(d,i)=>lcd&&lcd.enabled?((lcd.data[i%4]>>(d*2+(i>>2)))&1)===1:false;
    this.digits.forEach((w,d)=>{ $$('.lcd-seg',w).forEach(seg=>{
      const id=seg.dataset.seg;
      if(id==='DP'){seg.classList.remove('active');return;}
      seg.classList.toggle('active',on(d,'ABCDEFG'.indexOf(id)));
    }); });
    $('#lcd-pill').textContent=lcd&&lcd.enabled?'ACTIVE':'STANDBY';
  }
}
