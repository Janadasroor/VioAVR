export function fitCanvas(canvas){
  const dpr=window.devicePixelRatio||1, r=canvas.parentElement.getBoundingClientRect();
  canvas.width=Math.max(1,r.width*dpr); canvas.height=Math.max(1,r.height*dpr);
  const ctx=canvas.getContext('2d'); ctx.setTransform(dpr,0,0,dpr,0,0);
  return {ctx,w:r.width,h:r.height};
}
