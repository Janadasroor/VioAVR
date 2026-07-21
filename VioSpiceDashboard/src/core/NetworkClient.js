import { Emitter } from "./Utils.js";

export class NetworkClient extends Emitter {
  constructor(url='ws://localhost:8080'){ super(); this.url=url; this.socket=null; this.isConnected=false; this._closedByUs=false; }
  connect(){
    if(this.socket && (this.socket.readyState===WebSocket.CONNECTING||this.socket.readyState===WebSocket.OPEN)) return;
    try{ this.socket=new WebSocket(this.url); }catch(err){ this.emit('status',{connected:false}); return; }
    this.socket.onopen=()=>{ this.isConnected=true; this.emit('status',{connected:true}); };
    this.socket.onmessage=ev=>{
      try{ this.emit('message',JSON.parse(ev.data)); }
      catch(err){ console.warn('[VioSpice] unparseable gateway frame', ev.data); }
    };
    this.socket.onclose=()=>{
      this.isConnected=false; this.socket=null;
      this.emit('status',{connected:false});
      setTimeout(()=>this.connect(),3000); // resilient reconnect
    };
    this.socket.onerror=()=>{ try{ this.socket.close(); }catch(_){} };
  }
  send(type,payload={}){
    if(this.socket && this.socket.readyState===WebSocket.OPEN)
      this.socket.send(JSON.stringify({type,...payload}));
  }
}