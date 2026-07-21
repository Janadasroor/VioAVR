export function calculateOrthogonalPoints(p1,p2,orientation='h'){
  const pts=[p1];
  if(orientation==='h') pts.push({x:p2.x,y:p1.y}); else pts.push({x:p1.x,y:p2.y});
  pts.push(p2); return pts;
}
export function simplifyPath(points){
  if(points.length<3) return points;
  const out=[points[0]];
  for(let i=1;i<points.length-1;i++){
    const a=out[out.length-1],b=points[i],c=points[i+1];
    const colX=Math.abs(a.x-b.x)<.1&&Math.abs(b.x-c.x)<.1;
    const colY=Math.abs(a.y-b.y)<.1&&Math.abs(b.y-c.y)<.1;
    const same=Math.abs(a.x-b.x)<.1&&Math.abs(a.y-b.y)<.1;
    if(!colX&&!colY&&!same) out.push(b);
  }
  out.push(points[points.length-1]); return out;
}
export function getClosestSegmentIndex(mx,my,points){
  let idx=-1,best=Infinity;
  for(let i=0;i<points.length-1;i++){
    const p1=points[i],p2=points[i+1];
    if(mx<Math.min(p1.x,p2.x)-10||mx>Math.max(p1.x,p2.x)+10||my<Math.min(p1.y,p2.y)-10||my>Math.max(p1.y,p2.y)+10) continue;
    const d=Math.abs(p1.x-p2.x)<.1?Math.abs(mx-p1.x):Math.abs(my-p1.y);
    if(d<best&&d<15){best=d;idx=i;}
  }
  return idx;
}
