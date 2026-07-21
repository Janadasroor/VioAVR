import { $, $$, Toast, openModal } from "./Utils.js";
import { getComponent } from "../schematic/ComponentLibrary.js";

export function openPropertyDialog(chip){
  const def=getComponent(chip.dataset.type); if(!def) return;
  const fields=(def.properties||[]).map(p=>`
    <div class="property-group"><label>${p.label}</label>
    <input type="${p.type==='number'?'number':'text'}" data-prop="${p.id}" value="${chip.dataset[p.id] ?? p.default}"></div>`).join('');
  const m=openModal(`
    <div class="dialog-header"><h2>${def.label} Properties</h2><button class="btn-close">×</button></div>
    <div class="dialog-body">
      <div class="property-group"><label>Instance Name</label><input type="text" data-prop="name" value="${chip.dataset.name||def.label}"></div>
      ${fields}
    </div>
    <div class="dialog-footer"><button class="btn-action cancel">Cancel</button><button class="btn-primary save">Apply Changes</button></div>`);
  m.dialog.querySelector('.cancel').onclick=m.close;
  m.dialog.querySelector('.save').onclick=()=>{
    $$('input[data-prop]',m.dialog).forEach(inp=>{ chip.dataset[inp.dataset.prop]=inp.value; });
    const lbl=chip.querySelector('.node-label');
    if(lbl) lbl.textContent=(chip.dataset.name||def.label).toUpperCase();
    Toast.show('Properties updated','success');
    window.__vio.editor._updatePropertyPanel();
    m.close();
  };
}
