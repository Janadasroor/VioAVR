import { svgEl } from "../core/Utils.js";

export const SEG_PATHS = {
  A:'M 10 5 L 40 5 L 35 10 L 15 10 Z', B:'M 42 8 L 42 38 L 37 33 L 37 13 Z',
  C:'M 42 42 L 42 72 L 37 67 L 37 47 Z', D:'M 10 75 L 40 75 L 35 70 L 15 70 Z',
  E:'M 8 42 L 8 72 L 13 67 L 13 47 Z',  F:'M 8 8 L 8 38 L 13 33 L 13 13 Z',
  G:'M 10 40 L 40 40 L 35 45 L 15 45 L 15 35 L 35 35 Z' };
export function digitGroup(parent,cls,scale=1){
  const g=svgEl('g',{class:cls,transform:`scale(${scale})`},parent);
  Object.entries(SEG_PATHS).forEach(([id,d])=>svgEl('path',{d,class:`lcd-seg seg-${id}`,'data-seg':id},g));
  svgEl('circle',{cx:52,cy:75,r:3.5,class:'lcd-seg seg-DP','data-seg':'DP'},g);
  return g;
}
export function createChip(parent,x,y,type,def){
  const g=svgEl('g',{class:`schematic-node node-type-${type}`,transform:`translate(${x}, ${y})`},parent);
  g.dataset.x=x; g.dataset.y=y; g.dataset.type=type; g.dataset.label=def.label; g.dataset.rotation=0;
  let bw=150,bh=120;
  const left=def.pins.filter(p=>p.side==='left'), right=def.pins.filter(p=>p.side==='right');
  if(type==='led'){ bw=60;bh=60;
    svgEl('circle',{class:'led-body',cx:30,cy:28,r:24},g);
    svgEl('circle',{class:'led-lens',cx:30,cy:28,r:17},g);
    svgEl('path',{d:'M 12 52 L 48 52',stroke:'rgba(255,255,255,.2)','stroke-width':2},g);
  }else if(type==='seven_seg'){ bw=80;bh=120;
    svgEl('rect',{class:'node-body',width:bw,height:bh,rx:6},g);
    Object.entries(SEG_PATHS).forEach(([id,d])=>svgEl('path',{d:d.replace(/(\d+\.?\d*)\s(\d+\.?\d*)/g,(m,a,b)=>`${+a+10} ${+b+15}`),class:`seg seg-${id}`},g));
    svgEl('circle',{cx:66,cy:104,r:4,class:'seg seg-DP'},g);
  }else if(type==='lcd_glass'){ bw=240;bh=120;
    svgEl('rect',{class:'node-body',width:bw,height:bh,rx:12,fill:'#26262b'},g);
    svgEl('rect',{x:10,y:10,width:bw-20,height:bh-20,rx:6,fill:'url(#lcd-glass-gradient)',stroke:'rgba(0,0,0,.5)'},g);
    svgEl('rect',{x:10,y:10,width:bw-20,height:bh-20,rx:6,fill:'url(#pixel-mesh)','pointer-events':'none'},g);
    for(let d=0;d<4;d++){ const dg=svgEl('g',{class:`lcd-digit digit-${d}`,transform:`translate(${26+d*52},26) scale(0.88)`},g);
      Object.entries(SEG_PATHS).forEach(([id,p])=>svgEl('path',{d:p,class:'lcd-svg-seg seg-'+id,'data-seg':id},dg)); }
  }else if(type==='button'){ bw=64;bh=40;
    svgEl('rect',{class:'node-body',width:bw,height:bh,rx:8},g);
    svgEl('circle',{cx:32,cy:20,r:11,fill:'#334155',stroke:'rgba(255,255,255,.25)'},g);
    svgEl('circle',{cx:32,cy:20,r:7,fill:'#475569'},g);
  }else if(type==='res'){ bw=64;bh=22;
    svgEl('rect',{class:'node-body',width:bw,height:bh,rx:5,fill:'#2b2317'},g);
    [[8,'#ef4444'],[18,'#a855f7'],[28,'#f97316'],[38,'#eab308']].forEach(([bx,c])=>svgEl('rect',{x:bx,y:3,width:5,height:16,fill:c,opacity:.85,rx:1},g));
  }else if(type==='vsrc'){ bw=40;bh=44;
    svgEl('circle',{class:'node-body',cx:20,cy:22,r:16,fill:'#122117',stroke:'#10b981','stroke-width':1.6},g);
    svgEl('text',{x:20,y:19,'text-anchor':'middle',fill:'#4ade80','font-size':11,'font-weight':800},g).textContent='+';
    svgEl('text',{x:20,y:33,'text-anchor':'middle',fill:'#4ade80','font-size':11,'font-weight':800},g).textContent='−';
  }else if(type==='gnd'){ bw=40;bh=20;
    svgEl('path',{d:'M 4 4 L 36 4 M 10 11 L 30 11 M 16 18 L 24 18',stroke:'#94a3b8','stroke-width':2.4,fill:'none','stroke-linecap':'round'},g);
  }else if(type==='motor_dc'){ bw=100;bh=100;
    svgEl('circle',{class:'node-body',cx:50,cy:50,r:42,fill:'#2c2c34'},g);
    svgEl('circle',{cx:50,cy:50,r:9,fill:'#52525b',stroke:'#71717a'},g);
    svgEl('rect',{class:'motor-shaft',x:49,y:16,width:2.4,height:34,fill:'#facc15',rx:1},g);
    svgEl('text',{x:50,y:88,'text-anchor':'middle',fill:'rgba(255,255,255,.35)','font-size':9,'font-weight':700},g).textContent='MOTOR';
  }else if(type==='eusart_terminal'){ bw=240;bh=140;
    svgEl('rect',{class:'node-body',width:bw,height:bh,rx:6,fill:'#0a1020',stroke:'#0ea5e9','stroke-width':1.6},g);
    svgEl('rect',{width:bw,height:24,rx:6,fill:'#0c4a6e'},g);
    svgEl('text',{x:10,y:16,fill:'#e0f2fe','font-size':10,'font-weight':700},g).textContent='EUSART TERMINAL · 9600 8N1';
    for(let i=0;i<5;i++) svgEl('text',{x:10,y:44+i*19,class:`term-line term-line-${i}`},g).textContent = i===0?'> ready_':'';
  }else if(type==='logic_analyzer'){ bw=260;bh=180;
    svgEl('rect',{class:'node-body',width:bw,height:bh,rx:8,fill:'#101c14',stroke:'#22c55e','stroke-width':1.4},g);
    svgEl('rect',{x:44,y:16,width:200,height:148,rx:3,fill:'#020803',stroke:'#14532d'},g);
    for(let i=0;i<8;i++){
      svgEl('text',{x:38,y:34+i*17,'text-anchor':'end',fill:'#22c55e','font-size':8,'font-family':'monospace'},g).textContent='CH'+i;
      svgEl('path',{d:`M 48 ${30+i*17} L 240 ${30+i*17}`,class:`logic-trace logic-trace-${i}`},g); }
  }else if(type==='dac_monitor'){ bw=160;bh=120;
    svgEl('rect',{class:'node-body',width:bw,height:bh,rx:6,fill:'#0d1a12',stroke:'#4ade80','stroke-width':1.4},g);
    svgEl('text',{x:10,y:18,fill:'#bbf7d0','font-size':9,'font-weight':700},g).textContent='DAC OUTPUT (V)';
    svgEl('rect',{x:10,y:28,width:140,height:58,fill:'#020803',rx:2},g);
    svgEl('polyline',{points:'10,60 40,50 70,70 100,45 130,62 150,55',class:'dac-wave'},g);
    svgEl('text',{x:10,y:108,class:'dac-value'},g).textContent='2.50V';
  }else{ /* default MCU block */
    bh = 40 + Math.max(left.length,right.length)*20 + 10;
    svgEl('rect',{class:'node-body',width:bw,height:bh,rx:8},g);
    svgEl('rect',{class:'node-header-bar',width:bw,height:30,rx:8},g);
    svgEl('circle',{cx:12,cy:42,r:3,fill:'rgba(255,255,255,.25)'},g);
    const title=svgEl('text',{class:'node-label',x:bw/2,y:20,'text-anchor':'middle'},g);
    title.textContent=def.label.toUpperCase();
  }
  /* pins */
  const counters={left:0,right:0};
  const pinPos=p=>{
    const i=counters[p.side]++;
    switch(type){
      case 'led': return p.id==='A'?[15,60]:[45,60];
      case 'res': return p.side==='left'?[0,11]:[64,11];
      case 'button': return p.side==='left'?[0,20]:[64,20];
      case 'vsrc': return p.id==='+'?[20,2]:[20,42];
      case 'gnd': return [20,0];
      case 'motor_dc': return p.side==='left'?[0,50]:[100,50];
      case 'eusart_terminal': return p.side==='left'?[0,70]:[240,70];
      case 'logic_analyzer': return p.side==='left'?[0,30+i*17]:[260,i===0?60:100];
      case 'dac_monitor': return [0,i===0?50:85];
      case 'lcd_glass': return p.side==='left'?[0,12+i*6.4]:[240,25+i*24];
      case 'seven_seg': return p.side==='left'?[0,20+i*22]:[80,20+i*24];
      default: return p.side==='left'?[0,40+i*20]:[bw,40+i*20];
    }
  };
  def.pins.forEach(p=>{
    const [px,py]=pinPos(p);
    const pg=svgEl('g',{class:'pin-group',transform:`translate(${px}, ${py})`},g);
    pg.dataset.id=p.id; pg.dataset.localX=px; pg.dataset.localY=py; pg.dataset.ptype=p.type;
    svgEl('circle',{class:'pin-hit-area',r:11,fill:'transparent'},pg);
    svgEl('circle',{class:'node-pin',r:3.6},pg);
    const lbl=svgEl('text',{class:'pin-label',x:p.side==='left'?-8:8,y:3,'text-anchor':p.side==='left'?'end':'start'},pg);
    lbl.textContent=p.id;
  });
  g.dataset.bw=bw; g.dataset.bh=bh;
  return g;
}
