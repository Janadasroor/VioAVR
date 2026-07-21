import { getComponent } from "../schematic/ComponentLibrary.js";

export const ERCManager = {
  check(chips,wires){
    const issues=[];
    const conn=new Map();
    wires.forEach(w=>{ [w.startPin,w.endPin].forEach(p=>{ if(p){ if(!conn.has(p))conn.set(p,0); conn.set(p,conn.get(p)+1); } }); });
    chips.forEach(chip=>{
      const def=getComponent(chip.dataset.type); if(!def) return;
      const name=chip.dataset.name||def.label;
      const floating=def.pins.filter(p=>
        (p.type==='io'||p.type==='ctrl'||p.type==='analog') &&
        !p.id.startsWith('NC') && !['AREF','RESET'].includes(p.id) &&
        !conn.has(chip.querySelector(`.pin-group[data-id="${p.id}"]`)));
      if(floating.length)
        issues.push({level:'warning',message:`${floating.length} floating pin${floating.length>1?'s':''} on ${name} (${floating.slice(0,4).map(p=>p.id).join(', ')}${floating.length>4?'…':''})`,chipId:chip.dataset.id});
      const powerPins=def.pins.filter(p=>p.type==='power');
      if(powerPins.length && !powerPins.some(p=>conn.has(chip.querySelector(`.pin-group[data-id="${p.id}"]`))))
        issues.push({level:'warning',message:`${name}: no supply rail connected (virtual supply assumed)`,chipId:chip.dataset.id});
    });
    wires.forEach((w,i)=>{
      const s=w.startPin?.dataset.id, e=w.endPin?.dataset.id;
      if((['VCC','VDD'].includes(s)&&['GND','VSS'].includes(e))||(['GND','VSS'].includes(s)&&['VCC','VDD'].includes(e)))
        issues.push({level:'error',message:`Direct VCC→GND short detected on wire #${i+1}.`,wireIndex:i});
    });
    return issues;
  }
};
