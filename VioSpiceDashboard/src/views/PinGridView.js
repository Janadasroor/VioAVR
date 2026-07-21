import { $ } from "../core/Utils.js";

export class PinGridView {
  init(){ const g=$('#pin-grid'); g.innerHTML=''; this.leds=[];
    for(let i=0;i<32;i++){ const d=document.createElement('div'); d.className='pin-item';
      const port=String.fromCharCode(65+Math.floor(i/8));
      d.innerHTML=`<div class="pin-led"></div><span class="pin-label">P${port}${i%8}</span>`;
      g.appendChild(d); this.leds.push(d.querySelector('.pin-led')); } }
  update(t){ (t.digital_outputs||[]).slice(0,32).forEach((v,i)=>this.leds[i]?.classList.toggle('high',v===1)); }
}
