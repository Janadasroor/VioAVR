import { getClosestSegmentIndex } from './Router.js';

export function getAbsPinPos(pin) {
  const node = pin.closest('.schematic-node');
  const dot = pin.querySelector('.node-pin');
  if (!node || !dot) return { x: 0, y: 0 };
  
  const nx = parseFloat(node.dataset.x) || 0;
  const ny = parseFloat(node.dataset.y) || 0;
  const rot = parseInt(node.dataset.rotation) || 0;
  
  const lx = (parseFloat(pin.dataset.localX) || 0) + (parseFloat(dot.getAttribute('cx')) || 0);
  const ly = (parseFloat(pin.dataset.localY) || 0) + (parseFloat(dot.getAttribute('cy')) || 0);
  
  if (rot === 0) return { x: nx + lx, y: ny + ly };
  
  const body = node.querySelector('.node-body');
  const cx = parseFloat(body.getAttribute('width')) / 2;
  const cy = parseFloat(body.getAttribute('height')) / 2;
  
  const rad = (rot * Math.PI) / 180;
  const dx = lx - cx;
  const dy = ly - cy;
  
  const rx = cx + dx * Math.cos(rad) - dy * Math.sin(rad);
  const ry = cy + dx * Math.sin(rad) + dy * Math.cos(rad);
  
  return { x: nx + rx, y: ny + ry };
}

export function updateWirePath(group) {
  const start = group.startPin ? getAbsPinPos(group.startPin) : null;
  const end = group.endPin ? getAbsPinPos(group.endPin) : null;
  let points = JSON.parse(group.dataset.points);
  
  if (start) points[0] = start;
  if (end) points[points.length - 1] = end;
  
  // Re-orthogonalize terminal segments
  if (points.length >= 2) {
     if (start) {
       const p1 = points[1];
       if (Math.abs(points[0].y - p1.y) < 2) p1.y = points[0].y;
       else p1.x = points[0].x;
     }
     if (end) {
       const pN_1 = points[points.length - 2];
       if (Math.abs(points[points.length - 1].y - pN_1.y) < 2) pN_1.y = points[points.length - 1].y;
       else pN_1.x = points[points.length - 1].x;
     }
  }
  
  group.dataset.points = JSON.stringify(points);
  
  let d = `M ${points[0].x} ${points[0].y}`;
  for (let i = 1; i < points.length; i++) d += ` L ${points[i].x} ${points[i].y}`;
  
  group.querySelectorAll('.schematic-wire, .schematic-wire-glow, .schematic-wire-flow, .wire-hit-area').forEach(p => p.setAttribute('d', d));
  
  // Update junctions
  const jStart = group.querySelector('.wire-junction-dot:first-of-type');
  const jEnd = group.querySelector('.wire-junction-dot:last-of-type');
  if (jStart) { jStart.setAttribute('cx', points[0].x); jStart.setAttribute('cy', points[0].y); }
  if (jEnd) { jEnd.setAttribute('cx', points[points.length - 1].x); jEnd.setAttribute('cy', points[points.length - 1].y); }

  const highlight = group.querySelector('.schematic-wire-segment-highlight');
  const segIndex = parseInt(group.dataset.selectedSegment);
  if (highlight && !isNaN(segIndex) && segIndex >= 0 && segIndex < points.length - 1) {
    highlight.setAttribute('d', `M ${points[segIndex].x} ${points[segIndex].y} L ${points[segIndex+1].x} ${points[segIndex+1].y}`);
  }
}

export function createWaypointWire(startPin, endPin, points) {
  const group = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  group.setAttribute('class', 'wire-group');
  group.startPin = startPin;
  group.endPin = endPin;
  group.dataset.points = JSON.stringify(points);

  const elements = [
    { class: 'schematic-wire-glow' },
    { class: 'schematic-wire-flow' },
    { class: 'schematic-wire' },
    { class: 'schematic-wire-segment-highlight' },
    { class: 'wire-hit-area', strokeWidth: '15' }
  ];

  elements.forEach(el => {
    const p = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    p.setAttribute('class', el.class);
    p.setAttribute('fill', 'none');
    if (el.class === 'wire-hit-area') p.setAttribute('stroke', 'transparent');
    if (el.strokeWidth) p.setAttribute('stroke-width', el.strokeWidth);
    group.appendChild(p);
  });

  const junctionStart = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
  junctionStart.setAttribute('class', 'wire-junction-dot');
  junctionStart.setAttribute('r', '3');
  junctionStart.style.display = startPin ? 'none' : 'block';

  const junctionEnd = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
  junctionEnd.setAttribute('class', 'wire-junction-dot');
  junctionEnd.setAttribute('r', '3');
  junctionEnd.style.display = endPin ? 'none' : 'block';

  group.appendChild(junctionStart);
  group.appendChild(junctionEnd);

  updateWirePath(group);
  return group;
}
