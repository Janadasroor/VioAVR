import { $, $$, Emitter, GRID, Toast, confirmDialog, filePickerDialog, openModal, svgEl } from "../core/Utils.js";
import { Viewport } from "../core/Viewport.js";
import { HistoryManager } from "../core/History.js";
import { ERCManager } from "../core/ERCManager.js";
import { openERCDialog } from "../core/ERCDialog.js";
import { openPropertyDialog } from "../core/PropertyDialog.js";
import { getAllComponents, getComponent } from "./ComponentLibrary.js";
import { createChip } from "./ChipFactory.js";
import { calculateOrthogonalPoints, getClosestSegmentIndex, simplifyPath } from "./Router.js";
import { createWireGroup, getAbsPinPos, updateWirePath } from "./Renderer.js";
import { AddChipCommand, DeleteChipCommand, MoveChipCommand, RotateChipCommand, AddWireCommand, DeleteWireCommand } from "./Commands.js";

export class SchematicEditor extends Emitter {
  constructor(){
    super();
    this.svg=$('#schematic-canvas'); this.canvas=$('#schematic-content');
    this.viewport=new Viewport(this.svg,$('#schematic-viewport'));
    this.history=new HistoryManager();
    this.history.onChange=()=>{ this._autoSave(); this._refreshHistoryButtons(); };
    this.tool='select';
    this._drag=null; this._wire=null; this._pan=null; this._box=null;
    this._selectedChip=null; this._selectedWire=null;
    this._mouse={x:0,y:0}; this._down=null; this._moved=false;
    this._termLines=[]; this._dacHist=new Array(46).fill(.5);
  }
  init(){
    this._setupToolbar(); this._setupCanvas(); this._setupKeys(); this._buildLibrary(); this._buildProjects();
    this._restore(); this.viewport.update();
  }
  /* ── tools ── */
  setTool(name){
    this.tool=name;
    ['select','wire','probe','zoom-box'].forEach(t=>$('#tool-'+t)?.classList.toggle('active',t===name));
    this.svg.classList.remove('tool-select','tool-wire','tool-probe','tool-zoom-box');
    this.svg.classList.add('tool-'+name);
    const st=$('#tool-status');
    const map={select:['SELECTION MODE',false],wire:['WIRING MODE — CLICK PINS',true],probe:['PROBE MODE — CLICK NETS',true],'zoom-box':['ZOOM WINDOW — DRAG',true]};
    st.textContent=map[name][0]; st.classList.toggle('active',map[name][1]);
    if(name!=='wire') this._cancelWire();
  }
  /* ── chip creation ── */
  placeComponent(type,x,y,props={}){
    const def=getComponent(type); if(!def) return;
    this.history.execute(new AddChipCommand((p,px,py,t,d)=>{
      const g=this._attachChipEvents(createChip(p,px,py,t,d));
      (d.properties||[]).forEach(pr=>{ g.dataset[pr.id]=props[pr.id] ?? pr.default; });
      if(props.name){ g.dataset.name=props.name; const l=g.querySelector('.node-label'); if(l)l.textContent=props.name.toUpperCase(); }
      return g;
    },this.canvas,x,y,type,def));
  }
  _attachChipEvents(g){
    g.addEventListener('mousedown',e=>{
      if(e.button!==0) return;
      e.stopPropagation();
      this._selectChip(g);
      const pos=this.viewport.screenToWorld(e.clientX,e.clientY);
      this._drag={target:g,ox:pos.x-(+g.dataset.x),oy:pos.y-(+g.dataset.y),ix:+g.dataset.x,iy:+g.dataset.y};
      this._down={x:e.clientX,y:e.clientY,mode:'chip'}; this._moved=false;
    });
    g.addEventListener('dblclick',e=>{ e.stopPropagation(); openPropertyDialog(g); });
    g.addEventListener('contextmenu',e=>{
      e.preventDefault(); e.stopPropagation();
      this._selectChip(g);
      this._chipMenu(e.clientX,e.clientY,g);
    });
    $$('.pin-group',g).forEach(pg=>{
      pg.addEventListener('mousedown',e=>{
        e.stopPropagation();
        if(e.button!==0) return;
        this._down={x:e.clientX,y:e.clientY,mode:'pin',pin:pg}; this._moved=false;
      });
      pg.addEventListener('mouseenter',()=>pg.querySelector('.node-pin').setAttribute('r',5.5));
      pg.addEventListener('mouseleave',()=>pg.querySelector('.node-pin').setAttribute('r',3.6));
    });
    return g;
  }
  _chipMenu(x,y,g){
    const vio=window.__vio;
    vio.contextMenu.show(x,y,[
      {label:g.dataset.name||g.dataset.label,icon:'📦',className:'header'},
      {type:'divider'},
      {label:'Rotate 90°',icon:'🔄',shortcut:'R',action:()=>this._rotateSelected()},
      {label:'Duplicate',icon:'👯',shortcut:'Ctrl+D',action:()=>this._duplicate()},
      {label:'Properties',icon:'⚙️',action:()=>openPropertyDialog(g)},
      {label:'Load Firmware…',icon:'⚡',action:async()=>{
        const files=await vio.sim.listHexFiles();
        filePickerDialog('Select HEX file',files,f=>{ g.dataset.binary=f; vio.sim.loadHex(f); });
      }},
      {type:'divider'},
      {label:'Delete',icon:'🗑️',shortcut:'Del',className:'danger',action:()=>this._deleteChip(g)},
    ]);
  }
  /* ── selection ── */
  _selectChip(chip){ this._clearSelection(); this._selectedChip=chip; chip.classList.add('node-selected'); this._updatePropertyPanel(); }
  _clearSelection(){
    $$('.node-selected,.wire-selected',this.canvas).forEach(el=>el.classList.remove('node-selected','wire-selected'));
    this._selectedChip=null; this._selectedWire=null; this._updatePropertyPanel();
  }
  _deleteChip(chip){
    this.history.execute(new DeleteChipCommand(chip,this.canvas,c=>this.getWires().filter(w=>w.startPin?.closest('.schematic-node')===c||w.endPin?.closest('.schematic-node')===c)));
    this._clearSelection();
  }
  _rotateSelected(){
    const chip=this._selectedChip; if(!chip) return;
    const oldR=parseInt(chip.dataset.rotation)||0;
    this.history.execute(new RotateChipCommand(chip,oldR,(oldR+90)%360,()=>this._updateWiresFor(chip)));
  }
  _duplicate(){
    const chip=this._selectedChip; if(!chip) return;
    this.placeComponent(chip.dataset.type,(+chip.dataset.x)+GRID*2,(+chip.dataset.y)+GRID*2,{...chip.dataset});
  }
  /* ── wires ── */
  getChips(){ return $$('.schematic-node',this.canvas); }
  getWires(){ return $$('.wire-group',this.canvas); }
  _startWire(ref){
    this._wire={start:ref,waypoints:[],flip:false};
    const pos=ref.nodeType?getAbsPinPos(ref):ref.pos;
    const ghost=svgEl('path',{id:'ghost-wire',stroke:'var(--accent)','stroke-width':2,fill:'none','stroke-dasharray':'5,4','pointer-events':'none'});
    ghost.setAttribute('d',`M ${pos.x} ${pos.y} L ${pos.x} ${pos.y}`);
    this.canvas.appendChild(ghost);
  }
  _cancelWire(){ $('#ghost-wire')?.remove(); this._wire=null; }
  _wireStartPos(){ const s=this._wire.start; return s.nodeType?getAbsPinPos(s):{...s.pos}; }
  _updateGhost(cursor){
    const ghost=$('#ghost-wire'); if(!ghost||!this._wire) return;
    const start=this._wireStartPos();
    const last=this._wire.waypoints.length?this._wire.waypoints[this._wire.waypoints.length-1]:start;
    const seg=calculateOrthogonalPoints(last,cursor,this._wire.flip?'v':'h');
    let d=`M ${start.x} ${start.y}`;
    this._wire.waypoints.forEach(p=>d+=` L ${p.x} ${p.y}`);
    seg.shift(); seg.forEach(p=>d+=` L ${p.x} ${p.y}`);
    ghost.setAttribute('d',d);
  }
  _buildWirePoints(endPos){
    const start=this._wireStartPos();
    const pts=[start,...this._wire.waypoints];
    const last=pts[pts.length-1];
    const seg=calculateOrthogonalPoints(last,endPos,this._wire.flip?'v':'h');
    seg.shift(); pts.push(...seg);
    return simplifyPath(pts);
  }
  _finishWire(endRef){
    const endPos=endRef.nodeType?getAbsPinPos(endRef):{...endRef.pos};
    const pts=this._buildWirePoints(endPos);
    if(pts.length>=2){
      const wire=createWireGroup(this._wire.start,endRef,pts);
      this.history.execute(new AddWireCommand(wire,this.canvas));
    }
    this._cancelWire();
  }
  _addWaypoint(worldPos){
    const snapped={x:Math.round(worldPos.x/GRID)*GRID,y:Math.round(worldPos.y/GRID)*GRID};
    const last=this._wire.waypoints.length?this._wire.waypoints[this._wire.waypoints.length-1]:this._wireStartPos();
    const seg=calculateOrthogonalPoints(last,snapped,this._wire.flip?'v':'h');
    seg.shift(); this._wire.waypoints.push(...seg);
    this._updateGhost(worldPos);
  }
  _junctionFromWire(wire,pos){
    const j={wire,pos:{...pos}};
    const dot=svgEl('circle',{class:'junction-dot',cx:pos.x,cy:pos.y,r:3.4,fill:'var(--primary)'},wire);
    return j;
  }
  _selectWireAt(worldPos){
    for(const w of this.getWires()){
      const pts=JSON.parse(w.dataset.points||'[]');
      const idx=getClosestSegmentIndex(worldPos.x,worldPos.y,pts);
      if(idx!==-1){
        this._clearSelection();
        this._selectedWire=w; w.classList.add('wire-selected');
        w.dataset.selectedSegment=idx; updateWirePath(w);
        return true;
      }
    }
    return false;
  }
  _updateWiresFor(chip){
    this.getWires().forEach(w=>{
      if(w.startPin?.closest('.schematic-node')===chip||w.endPin?.closest('.schematic-node')===chip) updateWirePath(w);
    });
  }
  /* ── probe ── */
  _probePin(pin){
    const node=pin.closest('.schematic-node');
    const state=this._getPinState(pin,this.getWires(),window.__vio.sim.telemetry.digital_outputs);
    const hi=state===1;
    Toast.show(`PROBE · ${node?.dataset.name||node?.dataset.label||'NET'}.${pin.dataset.id} = ${hi?'HIGH (5V)':'LOW (0V)'}`, hi?'success':'warning');
    const dot=pin.querySelector('.node-pin');
    if(dot){ dot.setAttribute('r',9); dot.style.fill=hi?'var(--success)':'var(--danger)';
      setTimeout(()=>{dot.setAttribute('r',3.6);dot.style.fill='';},450); }
  }
  _probeWire(wire){
    const state=this._getPinState(wire.startPin||wire.endPin,this.getWires(),window.__vio.sim.telemetry.digital_outputs);
    const hi=state===1;
    Toast.show(`PROBE · NET = ${hi?'HIGH (5V)':'LOW (0V)'}`, hi?'success':'warning');
    const p=wire.querySelector('.schematic-wire');
    if(p){ const ow=p.getAttribute('stroke-width'); p.setAttribute('stroke-width',5); p.style.stroke=hi?'var(--success)':'var(--danger)';
      setTimeout(()=>{p.setAttribute('stroke-width',ow);p.style.stroke='';},450); }
  }
  _getPinState(pin,wires,outputs,visited=new Set()){
    if(!pin||!pin.dataset||visited.has(pin)) return 0;
    visited.add(pin);
    const id=pin.dataset.id, node=pin.closest('.schematic-node'), type=node?.dataset.type;
    const m=id&&id.match(/^P([A-L])(\d)$/i);
    if(m&&outputs){ const idx=(m[1].toUpperCase().charCodeAt(0)-65)*8+parseInt(m[2]); return outputs[idx]?1:0; }
    if(['VCC','VDD','AVCC'].includes(id)) return 1;
    if(['GND','VSS','AGND'].includes(id)) return 0;
    if(type==='vsrc') return id==='+'?1:0;
    if(type==='gnd') return 0;
    if(type==='button'){
      const other=pin.dataset.id==='1'?node.querySelector('.pin-group[data-id="2"]'):node.querySelector('.pin-group[data-id="1"]');
      if(other) return this._getPinState(other,wires,outputs,visited);
    }
    for(const w of wires){
      let next=null;
      if(w.startPin===pin) next=w.endPin; else if(w.endPin===pin) next=w.startPin;
      if(next){ const s=this._getPinState(next,wires,outputs,visited); if(s) return s; }
    }
    return 0;
  }
  /* ── canvas events ── */
  _setupCanvas(){
    this.svg.addEventListener('mousedown',e=>{
      const pos=this.viewport.screenToWorld(e.clientX,e.clientY);
      this._down={x:e.clientX,y:e.clientY,mode:'bg'}; this._moved=false;
      if(e.button===2||e.button===1){ this._pan={lx:e.clientX,ly:e.clientY}; this.svg.classList.add('panning'); e.preventDefault(); return; }
      if(e.button!==0) return;
      const pin=e.target.closest?.('.pin-group');
      const wire=e.target.closest?.('.wire-group');
      const chip=e.target.closest?.('.schematic-node');
      if(pin){ this._down={...this._down,mode:'pin',pin}; return; }
      if(chip) return; // chip handler owns it
      if(wire){ this._down={...this._down,mode:'wire',wire}; return; }
      if(this.tool==='zoom-box'){
        this._box={start:pos,rect:svgEl('rect',{id:'selection-box',fill:'rgba(34,211,238,.08)',stroke:'var(--accent)','stroke-width':1,'stroke-dasharray':'4,3'},this.canvas)};
        return;
      }
      this._pan={lx:e.clientX,ly:e.clientY}; this.svg.classList.add('panning');
    });
    window.addEventListener('mousemove',e=>{
      const pos=this.viewport.screenToWorld(e.clientX,e.clientY);
      this._mouse=pos;
      if(this._down&&Math.hypot(e.clientX-this._down.x,e.clientY-this._down.y)>4) this._moved=true;
      if(this._pan){ this.viewport.pan(e.clientX-this._pan.lx,e.clientY-this._pan.ly); this._pan={lx:e.clientX,ly:e.clientY}; return; }
      if(this._drag){
        const nx=Math.round((pos.x-this._drag.ox)/GRID)*GRID, ny=Math.round((pos.y-this._drag.oy)/GRID)*GRID;
        const g=this._drag.target, r=parseInt(g.dataset.rotation)||0;
        g.dataset.x=nx; g.dataset.y=ny;
        const bw=(+g.dataset.bw)/2, bh=(+g.dataset.bh)/2;
        g.setAttribute('transform',`translate(${nx}, ${ny})${r?` rotate(${r}, ${bw}, ${bh})`:''}`);
        this._updateWiresFor(g); return;
      }
      if(this._box){
        const s=this._box.start;
        this._box.rect.setAttribute('x',Math.min(s.x,pos.x)); this._box.rect.setAttribute('y',Math.min(s.y,pos.y));
        this._box.rect.setAttribute('width',Math.abs(pos.x-s.x)); this._box.rect.setAttribute('height',Math.abs(pos.y-s.y));
        return;
      }
      if(this._wire) this._updateGhost(pos);
    });
    window.addEventListener('mouseup',e=>{
      if(this._pan){ this._pan=null; this.svg.classList.remove('panning'); }
      if(this._drag){
        const g=this._drag.target, nx=+g.dataset.x, ny=+g.dataset.y;
        if(nx!==this._drag.ix||ny!==this._drag.iy){
          const cmd=new MoveChipCommand(g,this._drag.ix,this._drag.iy,nx,ny,()=>this._updateWiresFor(g));
          this.history.stack.push(cmd); this.history.redoStack=[]; this.history.onChange&&this.history.onChange();
        }
        this._drag=null;
      }
      if(this._box){
        const pos=this.viewport.screenToWorld(e.clientX,e.clientY), s=this._box.start;
        const rect={x:Math.min(s.x,pos.x),y:Math.min(s.y,pos.y),width:Math.abs(pos.x-s.x),height:Math.abs(pos.y-s.y)};
        this._box.rect.remove(); this._box=null;
        if(rect.width>8&&rect.height>8){ this.viewport.zoomToRect(rect); this.setTool('select'); }
        return;
      }
      const d=this._down; this._down=null;
      if(!d||this._moved) return;
      /* click semantics */
      if(d.mode==='pin'){
        const pin=d.pin;
        if(this.tool==='probe'){ this._probePin(pin); return; }
        if(!this._wire){ if(this.tool==='select') this.setTool('wire'); this._startWire(pin); }
        else if(pin!==this._wire.start) this._finishWire(pin);
        return;
      }
      if(d.mode==='wire'){
        const wire=d.wire, pos=this.viewport.screenToWorld(e.clientX,e.clientY);
        if(this.tool==='probe'){ this._probeWire(wire); return; }
        if(this._wire){
          const snapped={x:Math.round(pos.x/GRID)*GRID,y:Math.round(pos.y/GRID)*GRID};
          const j=this._junctionFromWire(wire,snapped);
          this._finishWire(j);
        }else{
          const pts=JSON.parse(wire.dataset.points||'[]');
          const idx=getClosestSegmentIndex(pos.x,pos.y,pts);
          this._clearSelection();
          this._selectedWire=wire; wire.classList.add('wire-selected');
          wire.dataset.selectedSegment=idx===-1?0:idx; updateWirePath(wire);
        }
        return;
      }
      if(d.mode==='bg'){
        if(this.tool==='wire'&&this._wire){ this._addWaypoint(this.viewport.screenToWorld(e.clientX,e.clientY)); return; }
        if(this.tool==='probe') return;
        if(!this._wire) this._clearSelection();
      }
    });
    this.svg.addEventListener('wheel',e=>{
      e.preventDefault();
      this.viewport.zoomAt(e.deltaY<0?1.12:1/1.12,e.clientX,e.clientY);
    },{passive:false});
    this.svg.addEventListener('contextmenu',e=>{
      e.preventDefault();
      if(e.target.closest('.schematic-node')) return; // chip handler
      const vio=window.__vio;
      vio.contextMenu.show(e.clientX,e.clientY,[
        {label:'Add Component…',icon:'➕',action:()=>{ this._switchTab('library'); }},
        {label:'Zoom Fit',icon:'🔍',shortcut:'F',action:()=>this.viewport.zoomFit(this.getChips())},
        {label:'Export Netlist JSON',icon:'📄',action:()=>$('#btn-export').click()},
        {type:'divider'},
        {label:'Run ERC Check',icon:'🩺',action:()=>openERCDialog(ERCManager.check(this.getChips(),this.getWires()),()=>{})},
      ]);
    });
    this.svg.addEventListener('dragover',e=>e.preventDefault());
    this.svg.addEventListener('drop',e=>{
      e.preventDefault();
      const type=e.dataTransfer.getData('type'); if(!getComponent(type)) return;
      const pos=this.viewport.screenToWorld(e.clientX,e.clientY);
      this.placeComponent(type,Math.round(pos.x/GRID)*GRID,Math.round(pos.y/GRID)*GRID);
      Toast.show(`Placed ${getComponent(type).label}`,'success');
    });
  }
  _setupKeys(){
    window.addEventListener('keydown',e=>{
      if(['INPUT','TEXTAREA','SELECT'].includes(e.target.tagName)) return;
      const k=e.key.toLowerCase();
      if(e.ctrlKey&&k==='z'){e.preventDefault();this.history.undo();return;}
      if(e.ctrlKey&&(k==='y'||(e.shiftKey&&k==='z'))){e.preventDefault();this.history.redo();return;}
      if(e.ctrlKey&&k==='d'){e.preventDefault();this._duplicate();return;}
      if(e.ctrlKey&&k==='s'){e.preventDefault();this._autoSave();Toast.show('Schematic saved locally','success');return;}
      if(k==='s')this.setTool('select');
      if(k==='w')this.setTool('wire');
      if(k==='p')this.setTool('probe');
      if(k==='z')this.setTool('zoom-box');
      if(k==='r')this._rotateSelected();
      if(k==='f')this.viewport.zoomFit(this.getChips());
      if(k==='+'||k==='=')this.viewport.zoomCenter(1.2);
      if(k==='-')this.viewport.zoomCenter(1/1.2);
      if(e.key==='Escape'){this._cancelWire();this._clearSelection();}
      if(e.key===' '&&this._wire){this._wire.flip=!this._wire.flip;e.preventDefault();}
      if(e.key==='Delete'||e.key==='Backspace'){
        if(this._selectedWire){this.history.execute(new DeleteWireCommand(this._selectedWire,this.canvas));this._selectedWire=null;}
        else if(this._selectedChip) this._deleteChip(this._selectedChip);
      }
    });
  }
  _setupToolbar(){
    $('#tool-select').onclick=()=>this.setTool('select');
    $('#tool-wire').onclick=()=>this.setTool('wire');
    $('#tool-probe').onclick=()=>this.setTool('probe');
    $('#tool-zoom-box').onclick=()=>this.setTool('zoom-box');
    $('#zoom-in').onclick=()=>this.viewport.zoomCenter(1.25);
    $('#zoom-out').onclick=()=>this.viewport.zoomCenter(1/1.25);
    $('#zoom-fit').onclick=()=>this.viewport.zoomFit(this.getChips());
    $('#btn-undo').onclick=()=>this.history.undo();
    $('#btn-redo').onclick=()=>this.history.redo();
    $('#btn-clear').onclick=()=>confirmDialog('Clear the entire canvas? This cannot be undone via the UI once saved.',()=>{
      this.canvas.innerHTML=''; this.history=new HistoryManager();
      this.history.onChange=()=>{this._autoSave();this._refreshHistoryButtons();};
      this._clearSelection(); this._cancelWire(); this._autoSave();
      Toast.show('Canvas cleared','info');
    },'Clear Canvas');
    $('#btn-export').onclick=()=>{
      const blob=new Blob([this.serialize()],{type:'application/json'});
      const a=document.createElement('a');
      a.href=URL.createObjectURL(blob); a.download='viospice_schematic.json'; a.click();
      URL.revokeObjectURL(a.href);
      Toast.show('Netlist exported','success');
    };
    $('#btn-import').onclick=()=>{
      const input=document.createElement('input'); input.type='file'; input.accept='.json';
      input.onchange=e=>{
        const f=e.target.files[0]; if(!f) return;
        const r=new FileReader(); r.onload=re=>this.deserialize(re.target.result); r.readAsText(f);
      };
      input.click();
    };
    $$('.panel-tab').forEach(tab=>tab.onclick=()=>this._switchTab(tab.dataset.tab));
    $('#library-search').addEventListener('input',e=>{
      const q=e.target.value.toLowerCase();
      $$('.chip-item',$('#chip-list')).forEach(it=>{
        it.style.display=it.querySelector('.chip-name').textContent.toLowerCase().includes(q)?'flex':'none';
      });
      $$('.library-category',$('#chip-list')).forEach(cat=>{
        let vis=false,n=cat.nextElementSibling;
        while(n&&!n.classList.contains('library-category')){ if(n.style.display!=='none'){vis=true;break;} n=n.nextElementSibling; }
        cat.style.display=vis?'block':'none';
      });
    });
  }
  _switchTab(name){
    $$('.panel-tab').forEach(t=>t.classList.toggle('active',t.dataset.tab===name));
    $$('.panel-tab-content').forEach(c=>c.classList.toggle('active',c.id==='tab-'+name));
  }
  _refreshHistoryButtons(){
    $('#btn-undo').disabled=!this.history.stack.length;
    $('#btn-redo').disabled=!this.history.redoStack.length;
  }
  /* ── library & projects ── */
  _buildLibrary(){
    const list=$('#chip-list'); list.innerHTML='';
    const grouped={};
    getAllComponents().forEach(c=>(grouped[c.category]||=[]).push(c));
    Object.entries(grouped).forEach(([cat,comps])=>{
      const h=document.createElement('div'); h.className='library-category'; h.textContent=cat; list.appendChild(h);
      comps.forEach(comp=>{
        const item=document.createElement('div');
        item.className='chip-item'; item.draggable=true; item.style.setProperty('--chip-color',comp.color);
        item.innerHTML=`<div class="chip-icon"><span>${comp.label.replace(/[^A-Za-z0-9]/g,'').slice(0,3).toUpperCase()}</span></div>
          <div class="chip-info"><span class="chip-name">${comp.label}</span><span class="chip-pins">${comp.pins.length} pins</span></div>`;
        item.addEventListener('dragstart',e=>e.dataTransfer.setData('type',comp.type));
        item.addEventListener('dblclick',()=>{
          this.placeComponent(comp.type,Math.round(this._mouse.x/GRID)*GRID||200,Math.round(this._mouse.y/GRID)*GRID||200);
          Toast.show(`Placed ${comp.label}`,'success');
        });
        list.appendChild(item);
      });
    });
  }
  _buildProjects(){
    const PROJECTS=[
      {list:'#test-list',items:[
        {name:'External Interrupt Stress',badge:'test',tag:'HARDWARE FIDELITY',desc:'Verifies INT0 response time and vector jumping under load.',
         pins:['PD2 — External Interrupt 0 (input)','PB0 — Status LED (output)'],jobs:['Interrupt vector validation','Context switch timing','SREG preservation'],
         data:{chips:[{id:'mcu',type:'atmega328p',x:80,y:40,props:{binary:'tests/core/firmware/test.hex',name:'DUT'}},{id:'btn',type:'button',x:420,y:440,name:'INT0 BTN'},{id:'led',type:'led',x:420,y:120,name:'STATUS'}],
               wires:[{s:{c:'mcu',p:'PD2'},e:{c:'btn',p:'1'}},{s:{c:'mcu',p:'PB0'},e:{c:'led',p:'A'}}]}},
        {name:'UART Protocol Validation',badge:'test',tag:'PROTOCOL COMPLIANCE',desc:'Checks baud rate accuracy and frame parity handling.',
         pins:['PD1 — TX (output)','PD0 — RX (input)'],jobs:['Baud rate tolerance ±2%','Frame parity check','FIFO overflow guard'],
         data:{chips:[{id:'mcu',type:'atmega328p',x:80,y:40,props:{binary:'tests/firmware/uart_test_compare.hex',name:'UART DUT'}},{id:'term',type:'eusart_terminal',x:440,y:420,name:'UART MON'}],
               wires:[{s:{c:'mcu',p:'PD1'},e:{c:'term',p:'RX'}}]}},
      ]},
      {list:'#example-list',items:[
        {name:'Blinky (Hello World)',badge:'example',tag:'GETTING STARTED',desc:'The classic LED toggle on PB0 driven by a wait loop.',
         pins:['PB0 — Status LED (output)'],jobs:['Basic GPIO toggling','Wait-loop timing','Direction register setup'],
         data:{chips:[{id:'mcu',type:'atmega328p',x:80,y:40,props:{binary:'tests/blink.hex',name:'BLINKY'}},{id:'led',type:'led',x:430,y:120,name:'LED1'},{id:'gnd1',type:'gnd',x:450,y:240}],
               wires:[{s:{c:'mcu',p:'PB0'},e:{c:'led',p:'A'}},{s:{c:'led',p:'K'},e:{c:'gnd1',p:'GND'}}]}},
        {name:'7-Segment Counter',badge:'example',tag:'DISPLAY CONTROL',desc:'Timer1 CTC interrupt drives a counting display.',
         pins:['PB0–PB6 — Segment bus','PB7 — Decimal point'],jobs:['7-segment pattern translation','Timer1 CTC mode','ISR latency budget'],
         data:{chips:[{id:'mcu',type:'atmega328p',x:80,y:40,props:{binary:'tests/firmware/timer1_interrupt_compare.hex',name:'COUNTER'}},{id:'seg',type:'seven_seg',x:440,y:120,name:'DIGIT 1'}],
               wires:[{s:{c:'mcu',p:'PB0'},e:{c:'seg',p:'A'}},{s:{c:'mcu',p:'PB1'},e:{c:'seg',p:'B'}}]}},
        {name:'Instrumentation Cluster',badge:'example',tag:'MIXED SIGNAL',desc:'Logic analyzer + DAC monitor + EUSART tap on one MCU.',
         pins:['PB0–PB7 — Logic bus','PD1 — UART TX'],jobs:['ADC sampling jitter','DAC waveform shaping','EUSART RX decode'],
         data:{chips:[{id:'mcu',type:'atmega328p',x:60,y:20,props:{binary:'tests/firmware/adc_pwm.hex',name:'MIXED-SIG'}},{id:'la',type:'logic_analyzer',x:440,y:60,name:'LA-8'},{id:'dac',type:'dac_monitor',x:460,y:320,name:'DAC-V'},{id:'term',type:'eusart_terminal',x:460,y:480,name:'TTY0'}],
               wires:[{s:{c:'mcu',p:'PB0'},e:{c:'la',p:'CH0'}},{s:{c:'mcu',p:'PB1'},e:{c:'la',p:'CH1'}},{s:{c:'mcu',p:'PD1'},e:{c:'term',p:'RX'}}]}},
      ]},
    ];
    PROJECTS.forEach(sec=>{
      const el=$(sec.list); el.innerHTML='';
      sec.items.forEach(item=>{
        const div=document.createElement('div'); div.className='project-item';
        div.innerHTML=`<div class="project-item-header"><div class="name">${item.name}</div>
            <button class="info-btn" title="Details"><svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5"><circle cx="12" cy="12" r="10"/><path d="M12 16v-4M12 8h.01"/></svg></button></div>
          <div class="desc">${item.desc}</div><div class="badge badge-${item.badge}">${item.tag}</div>`;
        div.onclick=e=>{
          if(e.target.closest('.info-btn')){
            const m=openModal(`
              <div class="dialog-header"><h2>${item.name}</h2><button class="btn-close">×</button></div>
              <div class="dialog-body">
                <div class="details-section"><label>Pin Configuration</label><ul>${item.pins.map(p=>`<li>${p}</li>`).join('')}</ul></div>
                <div class="details-section"><label>Operational Jobs</label><ul>${item.jobs.map(j=>`<li>${j}</li>`).join('')}</ul></div>
              </div>
              <div class="dialog-footer"><button class="btn-primary load">Load Project</button></div>`);
            m.dialog.querySelector('.load').onclick=()=>{m.close();this._loadProject(item);};
            return;
          }
          confirmDialog(`Load “${item.name}”? The current schematic will be replaced.`,()=>this._loadProject(item),'Load Project');
        };
        el.appendChild(div);
      });
    });
  }
  _loadProject(item){
    this.deserialize(JSON.stringify(item.data));
    Toast.show(`Loaded: ${item.name}`,'success');
  }
  /* ── serialize ── */
  serialize(){
    const chips=this.getChips().map((g,i)=>{
      g.dataset.id=g.dataset.id||`chip_${i}`;
      const props={};
      Object.keys(g.dataset).forEach(k=>{ if(!['id','type','x','y','label','rotation','bw','bh'].includes(k)) props[k]=g.dataset[k]; });
      return {id:g.dataset.id,type:g.dataset.type,x:+g.dataset.x,y:+g.dataset.y,rotation:+g.dataset.rotation||0,properties:props};
    });
    const wires=this.getWires().map(w=>{
      const sc=w.startPin?.closest('.schematic-node'), ec=w.endPin?.closest('.schematic-node');
      return { start: sc?{chipId:sc.dataset.id,pinId:w.startPin.dataset.id}:null,
               end: ec?{chipId:ec.dataset.id,pinId:w.endPin.dataset.id}:null,
               points:JSON.parse(w.dataset.points) };
    });
    return JSON.stringify({chips,wires},null,2);
  }
  deserialize(json){
    try{
      const data=JSON.parse(json);
      this.canvas.innerHTML='';
      this.history=new HistoryManager();
      this.history.onChange=()=>{this._autoSave();this._refreshHistoryButtons();};
      this._selectedChip=null;this._selectedWire=null;this._cancelWire();
      const map={};
      (data.chips||[]).forEach(c=>{
        const def=getComponent(c.type); if(!def) return;
        const g=this._attachChipEvents(createChip(this.canvas,c.x,c.y,c.type,def));
        g.dataset.id=c.id;
        if(c.rotation){ g.dataset.rotation=c.rotation;
          g.setAttribute('transform',`translate(${c.x}, ${c.y}) rotate(${c.rotation}, ${(+g.dataset.bw)/2}, ${(+g.dataset.bh)/2})`); }
        if(c.properties) Object.entries(c.properties).forEach(([k,v])=>{ g.dataset[k]=v; });
        const lbl=g.querySelector('.node-label');
        if(lbl&&g.dataset.name) lbl.textContent=g.dataset.name.toUpperCase();
        map[c.id]=g;
      });
      (data.wires||[]).forEach(w=>{
        const sp=w.start?map[w.start.chipId]?.querySelector(`.pin-group[data-id="${w.start.pinId}"]`):null;
        const ep=w.end?map[w.end.chipId]?.querySelector(`.pin-group[data-id="${w.end.pinId}"]`):null;
        let pts=w.points;
        if(sp&&ep&&(!pts||pts.length<3)){
          const a=getAbsPinPos(sp),b=getAbsPinPos(ep),mx=(a.x+b.x)/2;
          pts=[a,{x:mx,y:a.y},{x:mx,y:b.y},b];
        }
        if(pts&&pts.length>=2) this.canvas.appendChild(createWireGroup(sp,ep,pts));
      });
      this._updatePropertyPanel(); this._refreshHistoryButtons();
      this.update(window.__vio.sim.telemetry);
    }catch(err){ console.error('[VioSpice] deserialize failed',err); Toast.show('Failed to load schematic','error'); }
  }
  _autoSave(){ try{ localStorage.setItem('viospice_schematic_v2',this.serialize()); }catch(_){} }
  _restore(){
    const saved=localStorage.getItem('viospice_schematic_v2');
    if(saved){ this.deserialize(saved); return; }
    this.deserialize(JSON.stringify({
      chips:[
        {id:'mcu_0',type:'atmega328p',x:80,y:40,rotation:0,properties:{name:'MAIN MCU',binary:'tests/blink.hex',f_cpu:16000000,vref:5}},
        {id:'led_0',type:'led',x:430,y:120,rotation:0,properties:{name:'STATUS LED'}},
        {id:'seg_0',type:'seven_seg',x:430,y:240,rotation:0,properties:{name:'DIGIT 1'}},
        {id:'btn_0',type:'button',x:430,y:440,rotation:0,properties:{name:'INT0 BTN'}},
        {id:'gnd_0',type:'gnd',x:455,y:330,rotation:0,properties:{}},
      ],
      wires:[
        {start:{chipId:'mcu_0',pinId:'PB0'},end:{chipId:'led_0',pinId:'A'}},
        {start:{chipId:'led_0',pinId:'K'},end:{chipId:'gnd_0',pinId:'GND'}},
        {start:{chipId:'mcu_0',pinId:'PB1'},end:{chipId:'seg_0',pinId:'A'}},
        {start:{chipId:'mcu_0',pinId:'PD2'},end:{chipId:'btn_0',pinId:'1'}},
      ]
    }));
    Toast.show('Welcome — demo project loaded','info');
  }
  /* ── property panel ── */
  _updatePropertyPanel(){
    const list=$('#property-list'), label=$('#node-type-label');
    if(!this._selectedChip){
      label.textContent='No selection';
      list.innerHTML='<div class="property-placeholder">Select a component to edit its properties.<br/><br/>Drag parts from the Library,<br/>click pins to draw wires.</div>';
      return;
    }
    const chip=this._selectedChip, def=getComponent(chip.dataset.type);
    label.textContent=def?def.label:chip.dataset.type;
    list.innerHTML='';
    const field=(id,lbl,val,extra='')=>{
      const g=document.createElement('div'); g.className='property-group';
      g.innerHTML=`<label>${lbl}</label><input type="text" value="${val??''}"/>${extra}`;
      const inp=g.querySelector('input');
      inp.addEventListener('change',()=>{
        chip.dataset[id]=inp.value;
        if(id==='name'){ const t=chip.querySelector('.node-label'); if(t)t.textContent=(inp.value||def.label).toUpperCase(); }
        this._autoSave();
      });
      list.appendChild(g); return g;
    };
    field('name','Instance Name',chip.dataset.name||def?.label||'');
    (def?.properties||[]).forEach(p=>{
      const g=field(p.id,p.label,chip.dataset[p.id]??p.default);
      if(p.id==='binary'){
        const actions=document.createElement('div'); actions.className='property-actions';
        actions.innerHTML=`<button class="btn-browse">📂 Browse</button><button class="btn-flash">⚡ Flash</button>`;
        g.appendChild(actions);
        const inp=g.querySelector('input');
        actions.querySelector('.btn-browse').onclick=async()=>{
          const files=await window.__vio.sim.listHexFiles();
          filePickerDialog('Select HEX file',files,f=>{ inp.value=f; chip.dataset.binary=f; });
        };
        actions.querySelector('.btn-flash').onclick=()=>{ if(inp.value) window.__vio.sim.loadHex(inp.value); };
      }
    });
  }
  /* ── live telemetry on canvas ── */
  update(t){
    if(!t||!t.digital_outputs) return;
    const wires=this.getWires(), outs=t.digital_outputs;
    let anyHigh=false;
    $$('.node-type-led',this.canvas).forEach(n=>{
      const s=this._getPinState(n.querySelector('.pin-group[data-id="A"]'),wires,outs)===1;
      n.classList.toggle('led-on',s); if(s)anyHigh=true;
    });
    $$('.node-type-seven_seg',this.canvas).forEach(n=>{
      ['A','B','C','D','E','F','G','DP'].forEach(id=>{
        const s=this._getPinState(n.querySelector(`.pin-group[data-id="${id}"]`),wires,outs)===1;
        n.querySelector(`.seg-${id}`)?.classList.toggle('seg-lit',s);
      });
    });
    $$('.node-type-motor_dc',this.canvas).forEach(n=>
      n.classList.toggle('motor-running',this._getPinState(n.querySelector('.pin-group[data-id="+"]'),wires,outs)===1));
    $$('.node-type-logic_analyzer',this.canvas).forEach(n=>{
      n._hist||=(n._hist=Array.from({length:8},()=>new Array(46).fill(0)));
      for(let i=0;i<8;i++){
        const s=this._getPinState(n.querySelector(`.pin-group[data-id="CH${i}"]`),wires,outs);
        n._hist[i].push(s); n._hist[i].shift();
        const tr=n.querySelector(`.logic-trace-${i}`); if(!tr) continue;
        const yB=30+i*17, step=192/46;
        let d=`M 48 ${yB-n._hist[i][0]*11}`;
        for(let sIdx=1;sIdx<46;sIdx++){
          const x=48+sIdx*step, y=yB-n._hist[i][sIdx]*11, py=yB-n._hist[i][sIdx-1]*11;
          if(y!==py) d+=` L ${x} ${py} L ${x} ${y}`; else d+=` L ${x} ${y}`;
        }
        tr.setAttribute('d',d);
      }
    });
    $$('.node-type-eusart_terminal',this.canvas).forEach(n=>{
      if(t.eusart&&t.eusart.data&&n._lastLine!==t.eusart.data){
        n._lastLine=t.eusart.data;
        n._lines=(n._lines||['> ready']).slice(-4); n._lines.push('> '+t.eusart.data);
        n._lines.forEach((l,i)=>{ const el=n.querySelector(`.term-line-${i}`); if(el)el.textContent=l; });
      }
    });
    $$('.node-type-dac_monitor',this.canvas).forEach(n=>{
      const v=t.dac?t.dac.voltage:.5;
      this._dacHist.push(v); this._dacHist.shift();
      const wave=n.querySelector('.dac-wave');
      if(wave){
        const step=140/46;
        wave.setAttribute('points',this._dacHist.map((val,i)=>`${10+i*step},${30+(1-val)*54}`).join(' '));
      }
      const val=n.querySelector('.dac-value'); if(val)val.textContent=(v*5).toFixed(2)+'V';
    });
    $$('.node-type-lcd_glass',this.canvas).forEach(n=>{
      const lcd=t.lcd;
      for(let d=0;d<4;d++) for(let i=0;i<8;i++){
        const seg=['A','B','C','D','E','F','G','DP'][i];
        const el=n.querySelector(`.digit-${d} .seg-${seg}`); if(!el) continue;
        const on=lcd&&lcd.enabled?((lcd.data[i%4]>>(d*2+(i>>2)))&1)===1:false;
        el.classList.toggle('active',on);
      }
    });
    this.getWires().forEach(w=>w.classList.toggle('wire-live',anyHigh));
  }
}