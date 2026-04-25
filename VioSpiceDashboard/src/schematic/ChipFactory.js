export function createChip(parent, x, y, type, name, pins) {
  const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
  g.setAttribute('class', `schematic-node chip-item node-type-${type}`);
  g.setAttribute('transform', `translate(${x}, ${y})`);
  g.dataset.x = x;
  g.dataset.y = y;
  g.dataset.type = type;
  g.dataset.label = name;

  let bodyWidth = 150;
  let bodyHeight = (pins.length * 20 + 40);

  if (type === 'led') {
    renderLED(g);
    bodyWidth = 60;
    bodyHeight = 60;
  } else if (type === 'seven_seg') {
    renderSevenSeg(g);
    bodyWidth = 80;
    bodyHeight = 120;
  } else {
    // Default block rendering
    const body = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    body.setAttribute('class', 'node-body');
    body.setAttribute('width', bodyWidth.toString());
    body.setAttribute('height', bodyHeight.toString());
    
    const header = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    header.setAttribute('class', 'node-header-bar');
    header.setAttribute('width', bodyWidth.toString());
    header.setAttribute('height', '30');
    header.setAttribute('rx', '4');

    const title = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    title.setAttribute('class', 'node-label');
    title.setAttribute('x', (bodyWidth/2).toString());
    title.setAttribute('y', '20');
    title.setAttribute('text-anchor', 'middle');
    title.textContent = name;

    g.appendChild(body);
    g.appendChild(header);
    g.appendChild(title);
  }

  // Render Pins
  pins.forEach((p, i) => {
    const pinG = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    let px, py;
    
    if (type === 'led') {
       px = p.id === 'A' ? 10 : 50;
       py = 60;
    } else if (type === 'seven_seg') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 80;
       py = 15 + (i % 5) * 22;
    } else {
       px = p.side === 'left' ? 0 : 150;
       py = 40 + i * 20;
    }
    
    pinG.setAttribute('transform', `translate(${px}, ${py})`);
    pinG.dataset.localX = px;
    pinG.dataset.localY = py;
    pinG.dataset.id = p.id;
    pinG.classList.add('pin-group');

    const hitArea = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    hitArea.setAttribute('class', 'pin-hit-area');
    hitArea.setAttribute('r', '12');
    hitArea.setAttribute('fill', 'transparent');
    hitArea.style.cursor = 'crosshair';

    const dot = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    dot.setAttribute('class', 'node-pin');
    dot.setAttribute('r', '4');
    
    const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    label.setAttribute('class', 'pin-label');
    label.setAttribute('x', p.side === 'left' ? 14 : -14);
    label.setAttribute('y', '4');
    label.setAttribute('text-anchor', p.side === 'left' ? 'start' : 'end');
    label.textContent = p.id;

    pinG.appendChild(hitArea);
    pinG.appendChild(dot);
    pinG.appendChild(label);
    g.appendChild(pinG);
  });

  parent.appendChild(g);
  return g;
}

function renderLED(g) {
  const outer = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
  outer.setAttribute('class', 'led-body');
  outer.setAttribute('cx', '30');
  outer.setAttribute('cy', '30');
  outer.setAttribute('r', '25');
  
  const inner = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
  inner.setAttribute('class', 'led-lens');
  inner.setAttribute('cx', '30');
  inner.setAttribute('cy', '30');
  inner.setAttribute('r', '18');
  inner.setAttribute('fill', 'url(#led-glow)');

  const flat = document.createElementNS('http://www.w3.org/2000/svg', 'path');
  flat.setAttribute('d', 'M 10 50 L 50 50');
  flat.setAttribute('stroke', 'rgba(255,255,255,0.2)');
  flat.setAttribute('stroke-width', '2');

  g.appendChild(outer);
  g.appendChild(inner);
  g.appendChild(flat);
}

function renderSevenSeg(g) {
  const body = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  body.setAttribute('class', 'node-body seven-seg-case');
  body.setAttribute('width', '80');
  body.setAttribute('height', '120');
  body.setAttribute('rx', '4');
  g.appendChild(body);

  const segments = {
    A: 'M 20 15 L 60 15 L 55 20 L 25 20 Z',
    B: 'M 62 18 L 62 58 L 57 53 L 57 23 Z',
    C: 'M 62 62 L 62 102 L 57 97 L 57 67 Z',
    D: 'M 20 105 L 60 105 L 55 100 L 25 100 Z',
    E: 'M 18 62 L 18 102 L 23 97 L 23 67 Z',
    F: 'M 18 18 L 18 58 L 23 53 L 23 23 Z',
    G: 'M 20 60 L 60 60 L 55 65 L 25 65 L 25 55 L 55 55 Z'
  };

  Object.entries(segments).forEach(([id, path]) => {
    const p = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    p.setAttribute('d', path);
    p.setAttribute('class', `seg seg-${id}`);
    p.dataset.segId = id;
    g.appendChild(p);
  });

  const dp = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
  dp.setAttribute('cx', '70');
  dp.setAttribute('cy', '105');
  dp.setAttribute('r', '4');
  dp.setAttribute('class', 'seg seg-dp');
  g.appendChild(dp);
}
