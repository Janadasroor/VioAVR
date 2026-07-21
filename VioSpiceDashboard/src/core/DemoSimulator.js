import { Emitter } from "./Utils.js";

const SEG_PATTERNS={0:0b0111111,1:0b0000110,2:0b1011011,3:0b1001111,4:0b1100110,5:0b1101101,6:0b1111101,7:0b0000111,8:0b1111111,9:0b1101111};
export class DemoSimulator extends Emitter {
  constructor(){ super(); this.running=false; this.cycles=0; this.t=0; this._timer=null; this._euIdx=0;
    this.euLines=['VioAVR core online @ 16 MHz','PORTB direction register set','Timer1 CTC mode armed','ADC sample: 512 (2.50V)','EUSART TX buffer flushed','Watchdog reset avoided','I2C bus scan complete','PWM duty updated: 62%']; }
  setRunning(r){ this.running=r; }
  connect(){ if(this._timer) return; this._timer=setInterval(()=>this._tick(),50); }
  disconnect(){ clearInterval(this._timer); this._timer=null; }
  _tick(){
    this.t++;
    const t=this.t, run=this.running;
    if(run) this.cycles+=16000;
    const blink = run ? Math.floor(t/10)%2 : 0;
    const dout=new Array(128).fill(0);
    dout[8]=blink; dout[9]=run?Math.floor(t/5)%2:0; dout[10]=run?(t%3===0?1:0):0;
    dout[11]=run?(t%7<3?1:0):0; dout[13]=run?((t>>2)&1):0; dout[16]=run?(t%6<3?1:0):0;
    dout[25]=run?(t%4<2?1:0):0; dout[26]=run?((t*5)%11<5?1:0):0;
    let eu=null;
    if(run && t%40===0){ this._euIdx=(this._euIdx+1)%this.euLines.length; eu={data:this.euLines[this._euIdx]}; }
    const gprs=new Array(32).fill(0);
    if(run){ gprs[0]=t&0xff; gprs[16]=t&0xff; gprs[17]=(t>>8)&0xff;
      gprs[18]=(Math.sin(t/10)*127+128)|0; gprs[19]=(Math.cos(t/10)*127+128)|0;
      gprs[24]=blink; gprs[26]=(t*13)&0xff; gprs[30]=(t*3)&0xff; gprs[31]=(t*7)&0xff; }
    const dacV = run ? 0.5+0.45*Math.sin(t/8) : 0.5;
    const digits=String(run? t%10000 : 0).padStart(4,'0');
    const lcdData=[0,0,0,0];
    for(let d=0;d<4;d++){ const pat=SEG_PATTERNS[+digits[d]];
      for(let i=0;i<8;i++) if(pat&(1<<i)){ const com=i%4, sIdx=d*2+(i>>2); lcdData[com]|=(1<<sIdx); } }
    this.emit('telemetry',{
      status: run?1:0, sync_counter:t,
      cpu:{ pc: run? 0x0100+((t*2)%0x2f0) : 0x0000, sp:0x08ff-(t%6),
            sreg:0x03|(blink?0x02:0)|(t%2), gprs, cycles:this.cycles,
            core_state: run?1:0, flags:0 },
      digital_outputs:dout,
      analog_outputs:Array.from({length:32},(_,i)=> i===0? dacV*5 : (i===1? 2.47+Math.sin(t/3)*.03 : 0)),
      lcd:{enabled:1,duty:2,segments:8,data:lcdData},
      dac:{voltage:dacV}, eusart:eu
    });
  }
}
