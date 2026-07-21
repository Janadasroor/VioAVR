import { $, Emitter } from "./Utils.js";

export class SimController extends Emitter {
  constructor(){ super();
    this.isRunning=false; this.mode='connecting';
    this._bridgeAlive=true;
    this.telemetry={ pc:0,sp:0x08ff,sreg:0,cycles:0,gprs:new Array(32).fill(0),
                     digital_outputs:new Array(128).fill(0),flags:0 };
    this._lcd=null; this._dac={voltage:0.5}; this._eu=null;
  }
  init(client,demo){
    this.client=client; this.demo=demo;
    client.on('status',({connected})=>{
      if(connected){ demo.disconnect(); this.mode='gateway'; }
      else{ demo.connect(); this.mode='demo'; }
      this.emit('connectionChanged',{connected,mode:this.mode});
    });
    client.on('message',d=>this._handle(d));
    demo.on('telemetry',d=>this._handle(d));
  }
  _handle(data){
    if(!data) return;
    if(data.type==='error'){ this.emit('toast',{message:data.message||'Bridge error',type:'error'}); return; }
    if(data.type==='bridge_status'){
      this._bridgeAlive=data.alive;
      if(!data.alive) this.emit('toast',{message:data.message||'Bridge unavailable — SHM daemon not running',type:'error'});
      return;
    }
    if(!data.cpu) return;
    const t=this.telemetry;
    t.pc=data.cpu.pc; t.sp=data.cpu.sp; t.sreg=data.cpu.sreg; t.cycles=data.cpu.cycles;
    t.gprs=data.cpu.gprs||t.gprs; t.flags=data.cpu.flags||0;
    t.digital_outputs=data.digital_outputs||t.digital_outputs;
    if(data.lcd) this._lcd=data.lcd;
    if(data.dac) this._dac=data.dac;
    if(data.eusart) this._eu=data.eusart;
    if((t.flags&0x02)&&this.isRunning){
      this.pause();
      this.emit('analysisTriggered',{type:'fault'});
      this.emit('toast',{message:'Hardware Event: Analysis Freeze Triggered',type:'error'});
    }
    this.emit('telemetry',{...t,lcd:this._lcd,dac:this._dac,eusart:this._eu,analog_outputs:data.analog_outputs});
  }
  toggle(){ this.isRunning?this.pause():this.run(); }
  run(){
    if(this.mode==='gateway'&&!this._bridgeAlive){
      this.emit('toast',{message:'Cannot run — bridge daemon not running. Start ./build/vioavr-bridge-daemon',type:'error'});
      return;
    }
    this.isRunning=true; this.demo.setRunning(true); this.client.send('run'); this.emit('stateChanged',{running:true});
  }
  pause(){ this.isRunning=false; this.demo.setRunning(false); this.client.send('stop'); this.emit('stateChanged',{running:false}); }
  reset(){
    if(this.mode==='gateway'&&!this._bridgeAlive){
      this.emit('toast',{message:'Cannot reset — bridge daemon not running. Start ./build/vioavr-bridge-daemon',type:'error'});
      return;
    }
    this.client.send('reset'); this.demo.cycles=0; this.emit('toast',{message:'CPU Reset Triggered',type:'warning'}); this.emit('reset');
  }
  loadHex(path){ this.client.send('load',{path}); this.emit('hexLoaded',{path}); this.emit('toast',{message:`Flashing ${path.split('/').pop()}…`,type:'info'}); }
  exportTrace(){ this.client.send('vcd'); this.emit('toast',{message:'VCD Tracing Toggled',type:'info'}); }
  injectFault(){ if(this.isRunning) this.pause(); this.emit('analysisTriggered',{type:'fault'}); this.emit('toast',{message:'Fault Injected: Analysis Freeze',type:'error'}); }
  async listHexFiles(){
    if(this.mode==='gateway'&&this.client.isConnected){
      return new Promise(res=>{
        const off=this.client.on('message',d=>{ if(d.type==='hex_list'){ off(); res(d.files||[]); } });
        this.client.send('list_hex');
        setTimeout(()=>{ off(); res([]); },1500);
      });
    }
    return ['tests/blink.hex','tests/core/firmware/test.hex','tests/firmware/uart_test_compare.hex','tests/firmware/timer1_interrupt_compare.hex','tests/firmware/adc_pwm.hex'];
  }
}