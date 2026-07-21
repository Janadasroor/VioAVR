import { $, $$, openModal } from "./Utils.js";

export function openERCDialog(issues,onRun){
  const filters={error:true,warning:true};
  const m=openModal(`
    <div class="dialog-header"><h2>Electrical Rules Check</h2><button class="btn-close">×</button></div>
    <div class="erc-filters">
      <label class="filter-item"><input type="checkbox" data-f="error" checked><span>Errors (${issues.filter(i=>i.level==='error').length})</span></label>
      <label class="filter-item"><input type="checkbox" data-f="warning" checked><span>Warnings (${issues.filter(i=>i.level==='warning').length})</span></label>
    </div>
    <div class="erc-list"></div>
    <div class="dialog-footer"><button class="btn-action cancel">Fix Issues</button><button class="btn-primary run">Ignore &amp; Run</button></div>`);
  const list=m.dialog.querySelector('.erc-list');
  const render=()=>{
    const vis=issues.filter(i=>filters[i.level]);
    list.innerHTML = vis.length
      ? vis.map(i=>`<div class="erc-item ${i.level}"><div>${i.level==='error'?'❌':'⚠️'}</div><div><div class="erc-msg">${i.message}</div><div class="erc-meta">Scope: ${i.chipId||'netlist'}</div></div></div>`).join('')
      : '<div class="erc-empty">No issues match the current filters.</div>';
  };
  $$('.erc-filters input',m.dialog).forEach(inp=>inp.onchange=()=>{filters[inp.dataset.f]=inp.checked;render();});
  m.dialog.querySelector('.cancel').onclick=m.close;
  m.dialog.querySelector('.run').onclick=()=>{m.close();onRun();};
  render();
}
