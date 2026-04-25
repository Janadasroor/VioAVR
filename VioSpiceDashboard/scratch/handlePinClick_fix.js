function handlePinClick(pin) {
  const pos = getAbsPinPos(pin);

  if (!drawingWire) {
    drawingWire = { startPin: pin };
    document.querySelectorAll('.pin-container').forEach(p => p !== pin && p.classList.add('pin-pulse'));

    const svg = document.getElementById('schematic-svg');
    const ghost = document.createElementNS('http://www.w3.org/2000/svg', 'path');
    ghost.setAttribute('id', 'ghost-wire');
    ghost.setAttribute('stroke', 'var(--primary)');
    ghost.setAttribute('stroke-width', '2');
    ghost.setAttribute('fill', 'none');
    ghost.setAttribute('stroke-dasharray', '4,2');
    ghost.style.pointerEvents = 'none';
    svg.appendChild(ghost);

    drawingWire.onMouseMove = (e) => {
      const rect = svg.getBoundingClientRect();
      const mx = e.clientX - rect.left;
      const my = e.clientY - rect.top;
      updateGhostPath(mx, my);
    };
    window.addEventListener('mousemove', drawingWire.onMouseMove);
  } else {
    window.removeEventListener('mousemove', drawingWire.onMouseMove);
    document.querySelectorAll('.pin-container').forEach(p => p.classList.remove('pin-pulse'));
    const ghost = document.getElementById('ghost-wire');
    if (ghost) ghost.remove();

    if (pin !== drawingWire.startPin) {
      const wire = createOrthogonalWire(drawingWire.startPin, pin);
      history.execute(new AddWireCommand(wire, document.getElementById('schematic-content')));
      addLog(`Wire connected. Drag segments to adjust.`, 'success');
    }
    drawingWire = null;
  }
}
