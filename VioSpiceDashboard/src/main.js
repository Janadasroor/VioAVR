import './style.css';
import { NetworkClient } from './core/NetworkClient.js';
import { AppShell } from './core/AppShell.js';
import { SimController } from './core/SimController.js';
import { ContextMenu } from './core/ContextMenu.js';
import { SchematicEditor } from './schematic/SchematicEditor.js';
import { DemoSimulator } from './core/DemoSimulator.js';
import { ERCManager } from './core/ERCManager.js';
import { Toast, openModal, confirmDialog, filePickerDialog, $, $$, Emitter } from './core/Utils.js';
import { RegisterView } from './views/RegisterView.js';
import { PinGridView } from './views/PinGridView.js';
import { LogView } from './views/LogView.js';
import { DashboardScope } from './views/DashboardScope.js';
import { LcdDashView } from './views/LcdDashView.js';
import { LogicAnalyzerView } from './views/LogicPageView.js';
import { AnalogScopeView } from './views/AnalogScopeView.js';
import { LcdGlassView } from './views/LcdGlassView.js';
import { NvmView } from './views/NvmView.js';
import { ControlView } from './views/ControlView.js';
import { openERCDialog } from './core/ERCDialog.js';

async function discoverGatewayPort() {
  try {
    const res = await fetch('/gateway/port.json');
    if (res.ok) return (await res.json()).port;
  } catch (_) {}
  return null;
}

const shell=new AppShell();
const log=new LogView();
const demo=new DemoSimulator();
const sim=new SimController();
const editor=new SchematicEditor();
const contextMenu=new ContextMenu();
const registers=new RegisterView();
const pins=new PinGridView();
const scope=new DashboardScope();
const lcdDash=new LcdDashView();
const logicView=new LogicAnalyzerView();
const analogView=new AnalogScopeView();
const lcdView=new LcdGlassView();
const nvm=new NvmView();
const control=new ControlView();

window.__vio={shell,log,client:null,sim,editor,contextMenu};

shell.init();
registers.init(); pins.init(); scope.init(); lcdDash.init();
logicView.init(); analogView.init(); lcdView.init();
editor.init();

let client;
discoverGatewayPort().then(gwPort => {
  const url = gwPort ? `ws://localhost:${gwPort}` : 'ws://localhost:18081';
  client = new NetworkClient(url);
  window.__vio.client = client;
  sim.init(client,demo);
  client.connect();

  const gwLabel = gwPort ? `ws://localhost:${gwPort}` : 'ws://localhost:18081';
  log.add(gwPort ? `Gateway discovered on ${gwLabel}` : 'Gateway port file not found — trying default', 'system');
  log.add('VioSpice Control Center initialized','system');
  log.add('Dashboard services online — awaiting telemetry','system');
  Toast.show('Simulator Environment Ready','success');
});

/* telemetry fan-out */
let tickCount=0,lastEu=null;
sim.on('telemetry',d=>{
  registers.update(d); pins.update(d); scope.update(d);
  lcdDash.update(d.lcd); lcdView.update(d.lcd);
  logicView.update(d.digital_outputs); analogView.update(d.dac);
  editor.update(d);
  if(d.eusart&&d.eusart.data&&d.eusart.data!==lastEu){ lastEu=d.eusart.data; log.add(`EUSART RX: ${d.eusart.data}`,'info'); }
  if(++tickCount%5===0){
    $('#res-sim-time').textContent=(d.cycles/16e6).toFixed(6)+'s';
    $('#res-cpu-cycles').textContent=d.cycles.toLocaleString();
    const rows=[];
    ['B','C','D'].forEach(port=>{
      const base=(port.charCodeAt(0)-65)*8;
      for(let b=0;b<8;b++){
        const v=d.digital_outputs[base+b]===1;
        rows.push(`<tr><td>PORT${port}${b}</td><td class="${v?'val-high':'val-low'}">${v?'HIGH':'LOW'}</td></tr>`);
      }
    });
    $('#res-signal-list').innerHTML=rows.join('');
  }
});
sim.on('stateChanged',({running})=>{
  control.updateState(running);
  if(running){ document.body.classList.remove('fault-active'); logicView.unfreeze(); analogView.unfreeze(); }
  log.add(running?'Simulation RUN issued — core executing':'Simulation paused',running?'success':'info');
});
sim.on('connectionChanged',({connected,mode})=>{
  control.updateMode(mode);
  log.add(connected?'Gateway link established':'Gateway unavailable — running on internal demo telemetry',connected?'success':'warning');
});
sim.on('toast',t=>Toast.show(t.message,t.type));
sim.on('analysisTriggered',()=>{
  document.body.classList.add('fault-active');
  logicView.freeze(); analogView.freeze();
  log.add('FAULT: analysis freeze triggered — traces held for inspection','error');
});
sim.on('reset',()=>log.add('CPU reset vector executed','warning'));
sim.on('hexLoaded',({path})=>log.add(`NVM: programming ${path}`,'system'));

/* top-bar actions */
$('#btn-play').addEventListener('click',()=>{
  if(!sim.isRunning){
    const issues=ERCManager.check(editor.getChips(),editor.getWires());
    const errors=issues.filter(i=>i.level==='error');
    if(errors.length){ openERCDialog(issues,()=>sim.toggle()); return; }
    if(issues.length&&tickCount===0){ openERCDialog(issues,()=>sim.toggle()); return; }
    sim.toggle();
  }else sim.toggle();
});
$('#btn-reset').addEventListener('click',()=>sim.reset());
$('#btn-load').addEventListener('click',async()=>{
  const files=await sim.listHexFiles();
  filePickerDialog('Load HEX Firmware',files,f=>{ sim.loadHex(f); });
});
$('#mode-badge').addEventListener('click',()=>{
  Toast.show('Reconnecting to gateway…','info');
  if(client) client.connect(); else log.add('Gateway not yet initialized','error');
});

/* view lifecycle */
shell.on('viewChanged',({viewId})=>{
  if(viewId==='view-dashboard') scope.onShow();
  if(viewId==='view-logic') logicView.onShow();
  if(viewId==='view-analog') analogView.onShow();
  if(viewId==='view-schematic') editor.viewport.update();
});

/* global context menu */
window.addEventListener('contextmenu',e=>{
  if(['INPUT','TEXTAREA'].includes(e.target.tagName)) return;
  if(e.target.closest('.schematic-node')) return; // editor handles chips
  e.preventDefault();
  const items=[];
  if(e.target.closest('.state-card')||e.target.closest('#view-dashboard')){
    items.push({label:'Dashboard',icon:'📊',className:'header'},{type:'divider'});
    items.push({label:'Reset Registers',icon:'🧹',action:()=>sim.reset()});
    items.push({label:'Export VCD Trace',icon:'📈',action:()=>sim.exportTrace()});
    items.push({label:'Inject Fault (Freeze)',icon:'💥',className:'danger',action:()=>sim.injectFault()});
  }else{
    items.push({label:'VioSpice',icon:'⚡',className:'header'},{type:'divider'});
    items.push({label:'Open Schematic',icon:'🧩',action:()=>shell.switchView('view-schematic')});
    items.push({label:'Export VCD Trace',icon:'📈',action:()=>sim.exportTrace()});
  }
  contextMenu.show(e.clientX,e.clientY,items);
});

/* go — gateway discovery and client.connect handled above */
