const $  = (s, r=document) => r.querySelector(s);
const $$ = (s, r=document) => Array.from(r.querySelectorAll(s));
const SVG_NS = 'http://www.w3.org/2000/svg';
const GRID = 20;
const clamp = (v,a,b)=>Math.max(a,Math.min(b,v));
function svgEl(tag, attrs={}, parent){ const el=document.createElementNS(SVG_NS,tag);
  for(const [k,v] of Object.entries(attrs)) el.setAttribute(k,v);
  if(parent) parent.appendChild(el); return el; }
function hexFmt(v,w=4){ return '0x'+(v>>>0).toString(16).toUpperCase().padStart(w,'0'); }
class Emitter{
  constructor(){ this._l={}; }
  on(ev,fn){ (this._l[ev] ||= []).push(fn); return ()=>{ this._l[ev]=(this._l[ev]||[]).filter(f=>f!==fn); }; }
  emit(ev,data){ (this._l[ev]||[]).slice().forEach(fn=>{ try{ fn(data); }catch(err){ console.error(`[VioSpice] listener error (${ev})`,err); } }); }
}
const Toast = {
  show(message, type='info'){
    const t=document.createElement('div');
    t.className=`toast toast-${type}`;
    t.innerHTML=`<div class="toast-icon"></div><div>${message}</div>`;
    document.body.appendChild(t);
    requestAnimationFrame(()=>requestAnimationFrame(()=>t.classList.add('show')));
    setTimeout(()=>{ t.classList.remove('show'); setTimeout(()=>t.remove(),500); },3200);
  }
};
function openModal(html, opts={}){
  const overlay=document.createElement('div');
  overlay.className='modal-overlay';
  const dialog=document.createElement('div');
  dialog.className='dialog';
  dialog.innerHTML=html;
  overlay.appendChild(dialog);
  document.body.appendChild(overlay);
  const close=()=>{ overlay.remove(); document.removeEventListener('keydown',esc); };
  const esc=e=>{ if(e.key==='Escape') close(); };
  document.addEventListener('keydown',esc);
  overlay.addEventListener('mousedown',e=>{ if(e.target===overlay && opts.dismissable!==false) close(); });
  dialog.querySelector('.btn-close')?.addEventListener('click',close);
  return { overlay, dialog, close };
}
function confirmDialog(message, onYes, title='Please Confirm'){
  const m=openModal(`
    <div class="dialog-header"><h2>${title}</h2><button class="btn-close">×</button></div>
    <div class="dialog-body"><p style="font-size:13px;line-height:1.7;color:var(--text-main)">${message}</p></div>
    <div class="dialog-footer"><button class="btn-action cancel">Cancel</button><button class="btn-primary confirm">Confirm</button></div>`);
  m.dialog.querySelector('.cancel').onclick=m.close;
  m.dialog.querySelector('.confirm').onclick=()=>{ m.close(); onYes(); };
}
function filePickerDialog(title, files, onPick){
  const rows = files.length
    ? files.map(f=>`<div class="file-row" data-f="${f}">⚡ ${f}</div>`).join('')
    : `<div class="erc-empty">No .hex files found in workspace.</div>`;
  const m=openModal(`
    <div class="dialog-header"><h2>${title}</h2><button class="btn-close">×</button></div>
    <div class="dialog-body"><div class="file-list">${rows}</div></div>
    <div class="dialog-footer"><button class="btn-action cancel">Cancel</button></div>`);
  m.dialog.querySelector('.cancel').onclick=m.close;
  $$('.file-row',m.dialog).forEach(r=>r.onclick=()=>{ m.close(); onPick(r.dataset.f); });
}

export { $, $$, SVG_NS, GRID, clamp, svgEl, hexFmt, Emitter, Toast, openModal, confirmDialog, filePickerDialog };
