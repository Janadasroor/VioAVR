import { clamp } from "./Utils.js";

export class Viewport {
  constructor(svg,viewport){ this.svg=svg; this.viewport=viewport; this.zoom=1; this.panX=60; this.panY=40; }
  update(){ this.viewport.setAttribute('transform',`translate(${this.panX}, ${this.panY}) scale(${this.zoom})`); }
  screenToWorld(cx,cy){ const r=this.svg.getBoundingClientRect();
    return { x:(cx-r.left-this.panX)/this.zoom, y:(cy-r.top-this.panY)/this.zoom }; }
  zoomAt(factor,cx,cy){
    const w=this.screenToWorld(cx,cy);
    this.zoom=clamp(this.zoom*factor,.12,6);
    const r=this.svg.getBoundingClientRect();
    this.panX=(cx-r.left)-w.x*this.zoom; this.panY=(cy-r.top)-w.y*this.zoom;
    this.update();
  }
  zoomCenter(factor){ const r=this.svg.getBoundingClientRect(); this.zoomAt(factor,r.left+r.width/2,r.top+r.height/2); }
  pan(dx,dy){ this.panX+=dx; this.panY+=dy; this.update(); }
  zoomFit(nodes){
    if(!nodes.length){ this.zoom=1; this.panX=60; this.panY=40; this.update(); return; }
    let minX=1e9,minY=1e9,maxX=-1e9,maxY=-1e9;
    nodes.forEach(n=>{ const x=+n.dataset.x,y=+n.dataset.y,s=Math.max(+n.dataset.bw,+n.dataset.bh);
      minX=Math.min(minX,x); minY=Math.min(minY,y); maxX=Math.max(maxX,x+s+60); maxY=Math.max(maxY,y+s+40); });
    const r=this.svg.getBoundingClientRect();
    this.zoom=clamp(Math.min((r.width-120)/(maxX-minX),(r.height-120)/(maxY-minY)),.12,1.6);
    this.panX=(r.width-(maxX-minX)*this.zoom)/2-minX*this.zoom;
    this.panY=(r.height-(maxY-minY)*this.zoom)/2-minY*this.zoom;
    this.update();
  }
  zoomToRect(rect){
    if(rect.width<=0||rect.height<=0) return;
    const r=this.svg.getBoundingClientRect();
    this.zoom=clamp(Math.min((r.width-80)/rect.width,(r.height-80)/rect.height),.12,4);
    this.panX=(r.width-rect.width*this.zoom)/2-rect.x*this.zoom;
    this.panY=(r.height-rect.height*this.zoom)/2-rect.y*this.zoom;
    this.update();
  }
}
