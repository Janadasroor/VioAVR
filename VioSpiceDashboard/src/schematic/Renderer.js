import { $, $$, svgEl } from "../core/Utils.js";

export function getAbsPinPos(pin){
  const node=pin.closest('.schematic-node');
  if(!node) return {x:0,y:0};
  const nx=parseFloat(node.dataset.x)||0, ny=parseFloat(node.dataset.y)||0;
  const rot=parseInt(node.dataset.rotation)||0;
  const lx=parseFloat(pin.dataset.localX)||0, ly=parseFloat(pin.dataset.localY)||0;
  if(!rot) return {x:nx+lx,y:ny+ly};
  const bw=parseFloat(node.dataset.bw)/2, bh=parseFloat(node.dataset.bh)/2;
  const rad=rot*Math.PI/180, dx=lx-bw, dy=ly-bh;
  return { x:nx+bw+dx*Math.cos(rad)-dy*Math.sin(rad), y:ny+bh+dx*Math.sin(rad)+dy*Math.cos(rad) };
}
export function updateWirePath(group){
  const pts=JSON.parse(group.dataset.points);
  if(group.startPin){ const p=getAbsPinPos(group.startPin); pts[0]=p; }
  else if(group.startJunction){ pts[0]={...group.startJunction.pos}; }
  if(group.endPin){ const p=getAbsPinPos(group.endPin); pts[pts.length-1]=p; }
  else if(group.endJunction){ pts[pts.length-1]={...group.endJunction.pos}; }
  group.dataset.points=JSON.stringify(pts);
  const d=pts.map((p,i)=>`${i?'L':'M'} ${p.x} ${p.y}`).join(' ');
  $$('.schematic-wire, .schematic-wire-glow, .schematic-wire-flow, .wire-hit-area, .schematic-wire-segment-highlight',group)
    .forEach(p=>p.setAttribute('d',d));
  const seg=parseInt(group.dataset.selectedSegment);
  const hl=group.querySelector('.schematic-wire-segment-highlight');
  if(hl&&!isNaN(seg)&&seg>=0&&seg<pts.length-1)
    hl.setAttribute('d',`M ${pts[seg].x} ${pts[seg].y} L ${pts[seg+1].x} ${pts[seg+1].y}`);
}
export function createWireGroup(startRef,endRef,points){
  const g=svgEl('g',{class:'wire-group'});
  if(startRef&&startRef.nodeType) g.startPin=startRef; else if(startRef) g.startJunction=startRef;
  if(endRef&&endRef.nodeType) g.endPin=endRef; else if(endRef) g.endJunction=endRef;
  g.dataset.points=JSON.stringify(points);
  svgEl('path',{class:'schematic-wire-glow',fill:'none'},g);
  svgEl('path',{class:'schematic-wire-flow',fill:'none'},g);
  svgEl('path',{class:'schematic-wire',fill:'none'},g);
  svgEl('path',{class:'schematic-wire-segment-highlight',fill:'none'},g);
  svgEl('path',{class:'wire-hit-area',fill:'none',stroke:'transparent','stroke-width':13},g);
  if(g.startJunction) svgEl('circle',{class:'wire-junction-dot',r:3.6,cx:g.startJunction.pos.x,cy:g.startJunction.pos.y},g);
  if(g.endJunction) svgEl('circle',{class:'wire-junction-dot',r:3.6,cx:g.endJunction.pos.x,cy:g.endJunction.pos.y},g);
  updateWirePath(g);
  return g;
}
