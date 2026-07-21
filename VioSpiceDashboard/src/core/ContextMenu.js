import { $ } from "./Utils.js";

export class ContextMenu {
  constructor(){
    this.el=document.createElement('div');
    this.el.className='context-menu';
    this.el.style.display='none';
    document.body.appendChild(this.el);
    window.addEventListener('mousedown',e=>{ if(!this.el.contains(e.target)) this.close(); });
  }
  show(x,y,items){
    if(!items||!items.length) return;
    this.el.innerHTML='';
    items.forEach(it=>{
      if(it.type==='divider'){ const d=document.createElement('div'); d.className='menu-divider'; this.el.appendChild(d); return; }
      const b=document.createElement('button');
      b.className=`menu-item ${it.className||''}`;
      b.innerHTML=`<span class="menu-icon">${it.icon||''}</span><span class="menu-label">${it.label}</span><span class="menu-shortcut">${it.shortcut||''}</span>`;
      b.onmousedown=e=>{ e.stopPropagation(); this.close(); it.action&&it.action(); };
      this.el.appendChild(b);
    });
    this.el.style.display='flex';
    const r=this.el.getBoundingClientRect();
    this.el.style.left = (x+r.width>innerWidth ? x-r.width : x)+'px';
    this.el.style.top  = (y+r.height>innerHeight ? y-r.height : y)+'px';
    requestAnimationFrame(()=>this.el.classList.add('show'));
  }
  close(){ this.el.classList.remove('show'); this.el.style.display='none'; }
}