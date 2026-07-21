import { $ } from "../core/Utils.js";

export class LogView {
  constructor(){ this.el=$('#log-list'); this.count=0; this.start=Date.now(); }
  add(msg,type='info'){
    const el=document.createElement('div'); el.className=`log-entry ${type}`;
    const s=Math.floor((Date.now()-this.start)/1000);
    const ts=`${String(Math.floor(s/60)).padStart(2,'0')}:${String(s%60).padStart(2,'0')}`;
    el.innerHTML=`<span class="log-time">[${ts}]</span><span>${msg}</span>`;
    this.el.appendChild(el);
    while(this.el.children.length>200) this.el.firstChild.remove();
    this.el.scrollTop=this.el.scrollHeight;
    $('#log-count').textContent=++this.count;
  }
}
