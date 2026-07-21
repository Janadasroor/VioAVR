import { $, hexFmt } from "../core/Utils.js";

export class RegisterView {
  init(){
    this.pc=$('#reg-pc');this.sp=$('#reg-sp');this.sreg=$('#reg-sreg');this.cyc=$('#reg-cycles');
    const grid=$('#gpr-grid'); grid.innerHTML=''; this.gpr=[];
    for(let i=0;i<32;i++){ const d=document.createElement('div'); d.className='gpr-item';
      d.innerHTML=`<label>R${i}</label><span>00</span>`; grid.appendChild(d); this.gpr.push(d.querySelector('span')); }
  }
  update(t){
    this.pc.textContent=hexFmt(t.pc); this.sp.textContent=hexFmt(t.sp);
    this.sreg.textContent=hexFmt(t.sreg,2); this.cyc.textContent=t.cycles.toLocaleString();
    (t.gprs||[]).forEach((v,i)=>{ if(this.gpr[i])this.gpr[i].textContent=(v&0xff).toString(16).toUpperCase().padStart(2,'0'); });
  }
}
