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
  } else if (type === 'lcd') {
    renderCharLcd(g);
    bodyWidth = 280;
    bodyHeight = 160;
  } else if (type === 'glcd_128x64') {
    renderGLCD(g);
    bodyWidth = 300;
    bodyHeight = 200;
  } else if (type === 'keypad_4x4') {
    renderKeypad(g);
    bodyWidth = 160;
    bodyHeight = 160;
  } else if (type === 'motor_dc' || type === 'motor_stepper') {
    renderMotor(g, type);
    bodyWidth = 100;
    bodyHeight = 100;
  } else if (type === 'eeprom_i2c' || type === 'flash_spi' || type === 'can_transceiver') {
    bodyWidth = 60;
    bodyHeight = 80;
  } else if (type === 'logic_analyzer') {
    renderLogicAnalyzer(g);
    bodyWidth = 260;
    bodyHeight = 180;
  } else if (type === 'can_debugger') {
    renderCanDebugger(g);
    bodyWidth = 200;
    bodyHeight = 240;
  } else if (type === 'eusart_terminal') {
    renderEusartTerminal(g);
    bodyWidth = 240;
    bodyHeight = 180;
  } else if (type === 'dac_monitor') {
    renderDacMonitor(g);
    bodyWidth = 160;
    bodyHeight = 120;
  } else if (type === 'seven_seg') {
    renderSevenSeg(g);
    bodyWidth = 80;
    bodyHeight = 120;
  } else if (type === 'lcd_glass') {
    renderLcdGlass(g);
    bodyWidth = 240;
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
    } else if (type === 'lcd') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 280;
       py = 35 + (i % 10) * 15;
    } else if (type === 'glcd_128x64') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 300;
       py = 40 + (i % 12) * 15;
    } else if (type === 'keypad_4x4') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 160;
       py = 40 + (i % 4) * 25;
    } else if (type === 'motor_dc' || type === 'motor_stepper') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 100;
       py = 50 + (i % 2) * 20;
    } else if (type === 'eeprom_i2c' || type === 'flash_spi' || type === 'can_transceiver') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 60;
       py = 15 + (i % 4) * 15;
    } else if (type === 'logic_analyzer') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 260;
       py = 30 + (i % 8) * 18;
    } else if (type === 'can_debugger') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 200;
       py = 40 + (i % 2) * 20;
    } else if (type === 'eusart_terminal') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 240;
       py = 40 + (i % 2) * 20;
    } else if (type === 'dac_monitor') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 160;
       py = 40 + (i % 2) * 20;
    } else if (type === 'seven_seg') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 80;
       py = 15 + (i % 5) * 22;
    } else if (type === 'lcd_glass') {
       const isLeft = p.side === 'left';
       px = isLeft ? 0 : 240;
       py = 15 + (i % 18) * 15;
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

function renderLcdGlass(g) {
  const bodyWidth = 240;
  const bodyHeight = 120;

  // 1. External Bezel (Plastic Frame)
  const bezel = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  bezel.setAttribute('width', bodyWidth.toString());
  bezel.setAttribute('height', bodyHeight.toString());
  bezel.setAttribute('rx', '12');
  bezel.setAttribute('fill', '#2a2a2e');
  bezel.setAttribute('stroke', 'rgba(255,255,255,0.1)');
  bezel.setAttribute('stroke-width', '1');
  g.appendChild(bezel);

  // 2. Glass Panel (The actual LCD surface)
  const glass = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  glass.setAttribute('x', '10');
  glass.setAttribute('y', '10');
  glass.setAttribute('width', (bodyWidth - 20).toString());
  glass.setAttribute('height', (bodyHeight - 20).toString());
  glass.setAttribute('rx', '6');
  glass.setAttribute('fill', 'url(#lcd-glass-gradient)');
  glass.setAttribute('stroke', 'rgba(0,0,0,0.5)');
  g.appendChild(glass);

  const mesh = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  mesh.setAttribute('x', '10');
  mesh.setAttribute('y', '10');
  mesh.setAttribute('width', (bodyWidth - 20).toString());
  mesh.setAttribute('height', (bodyHeight - 20).toString());
  mesh.setAttribute('rx', '6');
  mesh.setAttribute('fill', 'url(#pixel-mesh)');
  mesh.setAttribute('pointer-events', 'none');
  g.appendChild(mesh);

  // 3. Inner Shadow / Depth Effect
  const innerShadow = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  innerShadow.setAttribute('x', '12');
  innerShadow.setAttribute('y', '12');
  innerShadow.setAttribute('width', (bodyWidth - 24).toString());
  innerShadow.setAttribute('height', (bodyHeight - 24).toString());
  innerShadow.setAttribute('rx', '4');
  innerShadow.setAttribute('fill', 'none');
  innerShadow.setAttribute('stroke', 'rgba(0,0,0,0.2)');
  innerShadow.setAttribute('stroke-width', '2');
  g.appendChild(innerShadow);

  // 4. Subtle Reflections (Glass Effect)
  const shine = document.createElementNS('http://www.w3.org/2000/svg', 'path');
  shine.setAttribute('d', `M 15 15 L ${bodyWidth - 40} 15 L ${bodyWidth - 80} 40 L 15 40 Z`);
  shine.setAttribute('fill', 'rgba(255,255,255,0.05)');
  g.appendChild(shine);

  // 5. 4 Digits
  for (let d = 0; d < 4; d++) {
    const dx = 25 + d * 52;
    const dy = 28;
    
    const digitG = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    digitG.setAttribute('transform', `translate(${dx}, ${dy}) scale(0.85)`);
    digitG.setAttribute('class', `lcd-digit digit-${d}`);
    
    // Ghost segments (faintly visible when off)
    const segments = {
      A: 'M 10 5 L 40 5 L 35 10 L 15 10 Z',
      B: 'M 42 8 L 42 38 L 37 33 L 37 13 Z',
      C: 'M 42 42 L 42 72 L 37 67 L 37 47 Z',
      D: 'M 10 75 L 40 75 L 35 70 L 15 70 Z',
      E: 'M 8 42 L 8 72 L 13 67 L 13 47 Z',
      F: 'M 8 8 L 8 38 L 13 33 L 13 13 Z',
      G: 'M 10 40 L 40 40 L 35 45 L 15 45 L 15 35 L 35 35 Z'
    };

    Object.entries(segments).forEach(([id, path]) => {
      const p = document.createElementNS('http://www.w3.org/2000/svg', 'path');
      p.setAttribute('d', path);
      p.setAttribute('class', `lcd-seg seg-${id}`);
      p.setAttribute('fill', 'rgba(0,0,0,0.03)'); // Ghost state
      p.dataset.segId = id;
      p.dataset.digitId = d;
      digitG.appendChild(p);
    });
    
    const dp = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    dp.setAttribute('cx', '52');
    dp.setAttribute('cy', '75');
    dp.setAttribute('r', '3.5');
    dp.setAttribute('class', 'lcd-seg seg-dp');
    dp.setAttribute('fill', 'rgba(0,0,0,0.03)');
    dp.dataset.segId = 'DP';
    dp.dataset.digitId = d;
    digitG.appendChild(dp);

    g.appendChild(digitG);
  }
}

function renderCharLcd(g) {
  const bodyWidth = 280;
  const bodyHeight = 160;

  // 1. PCB (Green Board)
  const pcb = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  pcb.setAttribute('width', bodyWidth.toString());
  pcb.setAttribute('height', bodyHeight.toString());
  pcb.setAttribute('rx', '4');
  pcb.setAttribute('fill', '#1a4a1a'); // Dark green PCB
  pcb.setAttribute('stroke', 'rgba(0,0,0,0.3)');
  g.appendChild(pcb);

  // 2. Mounting Holes
  const holes = [[10,10], [270,10], [10,150], [270,150]];
  holes.forEach(([hx, hy]) => {
    const hole = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
    hole.setAttribute('cx', hx.toString());
    hole.setAttribute('cy', hy.toString());
    hole.setAttribute('r', '4');
    hole.setAttribute('fill', 'rgba(0,0,0,0.4)');
    hole.setAttribute('stroke', 'rgba(255,255,255,0.1)');
    g.appendChild(hole);
  });

  // 3. Metal Frame
  const frame = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  frame.setAttribute('x', '20');
  frame.setAttribute('y', '30');
  frame.setAttribute('width', '240');
  frame.setAttribute('height', '100');
  frame.setAttribute('rx', '2');
  frame.setAttribute('fill', '#111');
  frame.setAttribute('stroke', '#333');
  frame.setAttribute('stroke-width', '2');
  g.appendChild(frame);

  // 4. Blue Backlit Screen
  const screen = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  screen.setAttribute('x', '30');
  screen.setAttribute('y', '40');
  screen.setAttribute('width', '220');
  screen.setAttribute('height', '80');
  screen.setAttribute('rx', '1');
  screen.setAttribute('fill', 'url(#lcd-blue-gradient)');
  g.appendChild(screen);

  // 5. Pixel Mesh for Character Blocks
  const mesh = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  mesh.setAttribute('x', '30');
  mesh.setAttribute('y', '40');
  mesh.setAttribute('width', '220');
  mesh.setAttribute('height', '80');
  mesh.setAttribute('fill', 'url(#pixel-mesh)');
  mesh.setAttribute('opacity', '0.2');
  g.appendChild(mesh);

  // 6. Default Text Placeholders (Ghostly)
  for (let r = 0; r < 2; r++) {
    const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    text.setAttribute('x', '40');
    text.setAttribute('y', (65 + r * 35).toString());
    text.setAttribute('fill', 'rgba(255,255,255,0.05)');
    text.setAttribute('font-family', 'monospace');
    text.setAttribute('font-size', '20px');
    text.setAttribute('font-weight', 'bold');
    text.textContent = 'VioSpice Sim...';
    g.appendChild(text);
  }
}

function renderGLCD(g) {
  const bodyWidth = 300;
  const bodyHeight = 200;

  // 1. PCB (Blue-Green Board)
  const pcb = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  pcb.setAttribute('width', bodyWidth.toString());
  pcb.setAttribute('height', bodyHeight.toString());
  pcb.setAttribute('rx', '6');
  pcb.setAttribute('fill', '#1a3a4a'); // Blue-green PCB
  pcb.setAttribute('stroke', 'rgba(0,0,0,0.4)');
  g.appendChild(pcb);

  // 2. Bezel
  const bezel = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  bezel.setAttribute('x', '20');
  bezel.setAttribute('y', '20');
  bezel.setAttribute('width', '260');
  bezel.setAttribute('height', '160');
  bezel.setAttribute('rx', '4');
  bezel.setAttribute('fill', '#111');
  bezel.setAttribute('stroke', '#333');
  g.appendChild(bezel);

  // 3. Screen (128x64 aspect ratio is roughly 2:1)
  const screen = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  screen.setAttribute('x', '30');
  screen.setAttribute('y', '40');
  screen.setAttribute('width', '240');
  screen.setAttribute('height', '120');
  screen.setAttribute('rx', '2');
  screen.setAttribute('fill', 'url(#lcd-blue-gradient)');
  g.appendChild(screen);

  // 4. Pixel Canvas (Using ForeignObject)
  const fo = document.createElementNS('http://www.w3.org/2000/svg', 'foreignObject');
  fo.setAttribute('x', '30');
  fo.setAttribute('y', '40');
  fo.setAttribute('width', '240');
  fo.setAttribute('height', '120');
  
  const canvas = document.createElement('canvas');
  canvas.width = 128;
  canvas.height = 64;
  canvas.style.width = '100%';
  canvas.style.height = '100%';
  canvas.style.imageRendering = 'pixelated';
  canvas.id = 'glcd-canvas';
  
  // Initial clear
  const ctx = canvas.getContext('2d');
  ctx.fillStyle = 'rgba(0,0,0,0.1)';
  ctx.fillRect(0, 0, 128, 64);
  
  fo.appendChild(canvas);
  g.appendChild(fo);

  // 5. Decorative "128x64" Label
  const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
  label.setAttribute('x', '250');
  label.setAttribute('y', '175');
  label.setAttribute('fill', 'rgba(255,255,255,0.2)');
  label.setAttribute('font-size', '10px');
  label.setAttribute('text-anchor', 'end');
  label.textContent = '128x64 GRAPHICS';
  g.appendChild(label);
}

function renderKeypad(g) {
  const bodyWidth = 160;
  const bodyHeight = 160;

  // 1. Keypad Body
  const body = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  body.setAttribute('width', bodyWidth.toString());
  body.setAttribute('height', bodyHeight.toString());
  body.setAttribute('rx', '8');
  body.setAttribute('fill', '#222');
  body.setAttribute('stroke', 'rgba(255,255,255,0.1)');
  g.appendChild(body);

  // 2. Buttons
  const labels = [
    '1','2','3','A',
    '4','5','6','B',
    '7','8','9','C',
    '*','0','#','D'
  ];

  for (let r = 0; r < 4; r++) {
    for (let c = 0; c < 4; c++) {
      const bx = 20 + c * 32;
      const by = 20 + r * 32;
      
      const btnG = document.createElementNS('http://www.w3.org/2000/svg', 'g');
      btnG.setAttribute('transform', `translate(${bx}, ${by})`);
      btnG.style.cursor = 'pointer';
      
      const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      rect.setAttribute('width', '28');
      rect.setAttribute('height', '28');
      rect.setAttribute('rx', '4');
      rect.setAttribute('fill', '#333');
      rect.setAttribute('stroke', '#444');
      rect.setAttribute('class', 'keypad-btn');
      
      const txt = document.createElementNS('http://www.w3.org/2000/svg', 'text');
      txt.setAttribute('x', '14');
      txt.setAttribute('y', '20');
      txt.setAttribute('text-anchor', 'middle');
      txt.setAttribute('fill', '#888');
      txt.setAttribute('font-size', '14px');
      txt.setAttribute('font-weight', 'bold');
      txt.setAttribute('pointer-events', 'none');
      txt.textContent = labels[r * 4 + c];
      
      btnG.appendChild(rect);
      btnG.appendChild(txt);
      g.appendChild(btnG);
    }
  }
}

function renderMotor(g, type) {
  const bodyWidth = 100;
  const bodyHeight = 100;
  const centerX = bodyWidth / 2;
  const centerY = bodyHeight / 2;

  // 1. Motor Housing
  const housing = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
  housing.setAttribute('cx', centerX.toString());
  housing.setAttribute('cy', centerY.toString());
  housing.setAttribute('r', '45');
  housing.setAttribute('fill', type === 'motor_dc' ? '#555' : '#444');
  housing.setAttribute('stroke', 'rgba(255,255,255,0.2)');
  g.appendChild(housing);

  // 2. Central Shaft
  const shaft = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
  shaft.setAttribute('cx', centerX.toString());
  shaft.setAttribute('cy', centerY.toString());
  shaft.setAttribute('r', '8');
  shaft.setAttribute('fill', '#888');
  shaft.setAttribute('stroke', '#aaa');
  g.appendChild(shaft);

  // 3. Rotation Marker (to see it spin later)
  const marker = document.createElementNS('http://www.w3.org/2000/svg', 'line');
  marker.setAttribute('x1', centerX.toString());
  marker.setAttribute('y1', centerY.toString());
  marker.setAttribute('x2', centerX.toString());
  marker.setAttribute('y2', (centerY - 35).toString());
  marker.setAttribute('stroke', '#ffea00');
  marker.setAttribute('stroke-width', '2');
  marker.setAttribute('class', 'motor-shaft');
  g.appendChild(marker);

  // 4. Terminals
  const term1 = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  term1.setAttribute('x', '0');
  term1.setAttribute('y', '45');
  term1.setAttribute('width', '10');
  term1.setAttribute('height', '10');
  term1.setAttribute('fill', '#777');
  g.appendChild(term1);

  const term2 = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  term2.setAttribute('x', '90');
  term2.setAttribute('y', '45');
  term2.setAttribute('width', '10');
  term2.setAttribute('height', '10');
  term2.setAttribute('fill', '#777');
  g.appendChild(term2);

  if (type === 'motor_stepper') {
    // Add 2 more terminals for stepper
    const term3 = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    term3.setAttribute('x', '0');
    term3.setAttribute('y', '65');
    term3.setAttribute('width', '10');
    term3.setAttribute('height', '10');
    term3.setAttribute('fill', '#777');
    g.appendChild(term3);

    const term4 = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    term4.setAttribute('x', '90');
    term4.setAttribute('y', '65');
    term4.setAttribute('width', '10');
    term4.setAttribute('height', '10');
    term4.setAttribute('fill', '#777');
    g.appendChild(term4);
  }
}

function renderLogicAnalyzer(g) {
  const bodyWidth = 260;
  const bodyHeight = 180;

  // 1. Body (Aluminum Case)
  const body = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  body.setAttribute('width', bodyWidth.toString());
  body.setAttribute('height', bodyHeight.toString());
  body.setAttribute('rx', '6');
  body.setAttribute('fill', '#2c2c2c');
  body.setAttribute('stroke', '#555');
  g.appendChild(body);

  // 2. Waveform Screen
  const screen = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  screen.setAttribute('x', '40');
  screen.setAttribute('y', '20');
  screen.setAttribute('width', '180');
  screen.setAttribute('height', '140');
  screen.setAttribute('rx', '2');
  screen.setAttribute('fill', '#000');
  screen.setAttribute('stroke', '#444');
  g.appendChild(screen);

  // 3. Channel Labels & Waveforms (Decorative)
  for (let i = 0; i < 8; i++) {
    const y = 35 + i * 15;
    
    // Channel Label
    const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    label.setAttribute('x', '35');
    label.setAttribute('y', y.toString());
    label.setAttribute('fill', '#00b140');
    label.setAttribute('font-size', '8px');
    label.setAttribute('text-anchor', 'end');
    label.textContent = `CH${i}`;
    g.appendChild(label);

    // Waveform Trace (Real-time dynamic path)
    const trace = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    trace.setAttribute('d', `M 45 ${y} L 220 ${y}`); // Flat line initially
    trace.setAttribute('fill', 'none');
    trace.setAttribute('stroke', '#00b140');
    trace.setAttribute('stroke-width', '1');
    trace.setAttribute('opacity', '0.8');
    trace.setAttribute('class', `logic-trace logic-trace-${i}`);
    g.appendChild(trace);
  }

  // 4. Logo
  const logo = document.createElementNS('http://www.w3.org/2000/svg', 'text');
  logo.setAttribute('x', '225');
  logo.setAttribute('y', '170');
  logo.setAttribute('fill', 'rgba(255,255,255,0.1)');
  logo.setAttribute('font-size', '10px');
  logo.setAttribute('font-style', 'italic');
  logo.setAttribute('text-anchor', 'end');
  logo.textContent = 'VioLOGIC Pro';
  g.appendChild(logo);
}

function renderCanDebugger(g) {
  const bodyWidth = 200;
  const bodyHeight = 240;
  const body = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  body.setAttribute('width', bodyWidth.toString());
  body.setAttribute('height', bodyHeight.toString());
  body.setAttribute('fill', '#1a1a1a');
  body.setAttribute('stroke', '#d13438');
  body.setAttribute('stroke-width', '2');
  g.appendChild(body);

  const title = document.createElementNS('http://www.w3.org/2000/svg', 'text');
  title.setAttribute('x', '10');
  title.setAttribute('y', '20');
  title.setAttribute('fill', '#fff');
  title.setAttribute('font-size', '12px');
  title.textContent = 'CAN Bus Monitor';
  g.appendChild(title);
}

function renderEusartTerminal(g) {
  const bodyWidth = 240;
  const bodyHeight = 180;
  
  const body = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  body.setAttribute('width', bodyWidth.toString());
  body.setAttribute('height', bodyHeight.toString());
  body.setAttribute('fill', '#0c0c0c');
  body.setAttribute('stroke', '#0078d4');
  body.setAttribute('stroke-width', '2');
  g.appendChild(body);

  const header = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  header.setAttribute('width', bodyWidth.toString());
  header.setAttribute('height', '24');
  header.setAttribute('fill', '#0078d4');
  g.appendChild(header);

  const title = document.createElementNS('http://www.w3.org/2000/svg', 'text');
  title.setAttribute('x', '10');
  title.setAttribute('y', '16');
  title.setAttribute('fill', '#fff');
  title.setAttribute('font-size', '11px');
  title.textContent = 'EUSART Terminal';
  g.appendChild(title);

  // Terminal Lines
  for (let i = 0; i < 6; i++) {
    const txt = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    txt.setAttribute('x', '10');
    txt.setAttribute('y', (45 + i * 18).toString());
    txt.setAttribute('fill', '#00ff00');
    txt.setAttribute('font-size', '10px');
    txt.setAttribute('font-family', 'monospace');
    txt.setAttribute('class', `terminal-line terminal-line-${i}`);
    txt.textContent = i === 0 ? '> Initializing EUSART...' : '';
    g.appendChild(txt);
  }
}

function renderDacMonitor(g) {
  const bodyWidth = 160;
  const bodyHeight = 120;

  const body = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  body.setAttribute('width', bodyWidth.toString());
  body.setAttribute('height', bodyHeight.toString());
  body.setAttribute('fill', '#1e1e1e');
  body.setAttribute('stroke', '#107c10');
  body.setAttribute('stroke-width', '2');
  g.appendChild(body);

  const title = document.createElementNS('http://www.w3.org/2000/svg', 'text');
  title.setAttribute('x', '10');
  title.setAttribute('y', '18');
  title.setAttribute('fill', '#fff');
  title.setAttribute('font-size', '10px');
  title.textContent = 'DAC Output (V)';
  g.appendChild(title);

  // Waveform Area
  const waveArea = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
  waveArea.setAttribute('x', '10');
  waveArea.setAttribute('y', '30');
  waveArea.setAttribute('width', '140');
  waveArea.setAttribute('height', '60');
  waveArea.setAttribute('fill', '#000');
  g.appendChild(waveArea);

  const wave = document.createElementNS('http://www.w3.org/2000/svg', 'polyline');
  wave.setAttribute('points', '10,60 30,50 50,70 70,40 90,60 110,50 130,80 150,60');
  wave.setAttribute('fill', 'none');
  wave.setAttribute('stroke', '#00ff00');
  wave.setAttribute('stroke-width', '1');
  wave.setAttribute('class', 'dac-wave');
  g.appendChild(wave);

  const value = document.createElementNS('http://www.w3.org/2000/svg', 'text');
  value.setAttribute('x', '10');
  value.setAttribute('y', '110');
  value.setAttribute('fill', '#00ff00');
  value.setAttribute('font-size', '14px');
  value.setAttribute('font-weight', 'bold');
  value.textContent = '2.50V';
  value.setAttribute('class', 'dac-value');
  g.appendChild(value);
}
