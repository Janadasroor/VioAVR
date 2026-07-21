import { $, $$, Emitter } from "./Utils.js";

export class AppShell extends Emitter {
  constructor(){ super(); this.active='view-dashboard'; this.start=Date.now(); }
  init(){
    $$('.nav-item[data-view]').forEach(item=>item.addEventListener('click',()=>{
      $$('.nav-item').forEach(i=>i.classList.remove('active'));
      item.classList.add('active');
      this.switchView(item.dataset.view);
    }));
    setInterval(()=>{
      const s=Math.floor((Date.now()-this.start)/1000);
      $('#session-timer').textContent=`Session: ${String(Math.floor(s/3600)).padStart(2,'0')}:${String(Math.floor(s%3600/60)).padStart(2,'0')}:${String(s%60).padStart(2,'0')}`;
    },1000);
  }
  switchView(viewId){
    if(this.active===viewId) return;
    const target=document.getElementById(viewId); if(!target) return;
    $$('.view').forEach(v=>v.classList.remove('active'));
    target.classList.add('active');
    this.active=viewId;
    const label=$(`[data-view="${viewId}"]`)?.dataset.label||'';
    const crumb=$('.breadcrumb .current'); if(crumb) crumb.textContent=label;
    this.emit('viewChanged',{viewId});
  }
}