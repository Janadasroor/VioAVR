export class Viewport {
  constructor(svgId, viewportId) {
    this.svg = document.getElementById(svgId);
    this.viewport = document.getElementById(viewportId);
    this.zoomLevel = 1.0;
    this.panX = 0;
    this.panY = 0;
  }

  update() {
    if (this.viewport) {
      this.viewport.setAttribute('transform', `translate(${this.panX}, ${this.panY}) scale(${this.zoomLevel})`);
    }
  }

  zoom(delta, centerX = window.innerWidth / 2, centerY = window.innerHeight / 2) {
    const oldZoom = this.zoomLevel;
    this.zoomLevel = Math.max(0.1, Math.min(10, this.zoomLevel + delta));
    
    const ratio = this.zoomLevel / oldZoom;
    this.panX = centerX - (centerX - this.panX) * ratio;
    this.panY = centerY - (centerY - this.panY) * ratio;
    
    this.update();
  }

  screenToWorld(clientX, clientY) {
    const rect = this.svg.getBoundingClientRect();
    return {
      x: (clientX - rect.left - this.panX) / this.zoomLevel,
      y: (clientY - rect.top - this.panY) / this.zoomLevel
    };
  }

  zoomFit(entities) {
    if (entities.length === 0) {
      this.zoomLevel = 1.0; this.panX = 0; this.panY = 0;
      this.update();
      return;
    }
    
    let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
    entities.forEach(entity => {
      const x = parseFloat(entity.dataset.x);
      const y = parseFloat(entity.dataset.y);
      minX = Math.min(minX, x);
      minY = Math.min(minY, y);
      // Rough bounds estimation for chips
      maxX = Math.max(maxX, x + 150);
      maxY = Math.max(maxY, y + 200);
    });
    
    const w = maxX - minX;
    const h = maxY - minY;
    const container = this.svg.parentElement;
    const cw = container.clientWidth - 100;
    const ch = container.clientHeight - 100;
    
    this.zoomLevel = Math.min(cw / w, ch / h, 1.5);
    this.panX = (cw - w * this.zoomLevel) / 2 - minX * this.zoomLevel;
    this.panY = (ch - h * this.zoomLevel) / 2 - minY * this.zoomLevel;
    
    this.update();
  }

  zoomToRect(worldRect) {
    const { x, y, width, height } = worldRect;
    if (width <= 0 || height <= 0) return;

    const container = this.svg.parentElement;
    const cw = container.clientWidth - 40;
    const ch = container.clientHeight - 40;

    this.zoomLevel = Math.min(cw / width, ch / height, 5.0);
    this.panX = (container.clientWidth - width * this.zoomLevel) / 2 - x * this.zoomLevel;
    this.panY = (container.clientHeight - height * this.zoomLevel) / 2 - y * this.zoomLevel;
    
    this.update();
  }
}
