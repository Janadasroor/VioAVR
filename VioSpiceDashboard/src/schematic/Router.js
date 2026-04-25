export function calculateOrthogonalPoints(p1, p2, orientation = 'h', type = 'hv') {
  const points = [p1];
  
  if (type === 'hv') {
    if (orientation === 'h') {
      points.push({ x: p2.x, y: p1.y });
    } else {
      points.push({ x: p1.x, y: p2.y });
    }
  } else if (type === 'hvh') {
    const midX = (p1.x + p2.x) / 2;
    points.push({ x: midX, y: p1.y });
    points.push({ x: midX, y: p2.y });
  } else if (type === 'vhv') {
    const midY = (p1.y + p2.y) / 2;
    points.push({ x: p1.x, y: midY });
    points.push({ x: p2.x, y: midY });
  }

  points.push(p2);
  return points;
}

export function simplifyPath(points) {
  if (points.length < 3) return points;
  const result = [points[0]];
  
  for (let i = 1; i < points.length - 1; i++) {
    const prev = result[result.length - 1];
    const curr = points[i];
    const next = points[i + 1];
    
    // Remove collinear points
    const isCollinearX = (Math.abs(prev.x - curr.x) < 0.1 && Math.abs(curr.x - next.x) < 0.1);
    const isCollinearY = (Math.abs(prev.y - curr.y) < 0.1 && Math.abs(curr.y - next.y) < 0.1);
    
    // Remove coincident points
    const isCoincident = (Math.abs(prev.x - curr.x) < 0.1 && Math.abs(prev.y - curr.y) < 0.1);

    if (!isCollinearX && !isCollinearY && !isCoincident) {
      result.push(curr);
    }
  }
  
  result.push(points[points.length - 1]);
  return result;
}

export function getClosestSegmentIndex(mx, my, points) {
  let closestIdx = -1;
  let minDistance = Infinity;

  for (let i = 0; i < points.length - 1; i++) {
    const p1 = points[i];
    const p2 = points[i + 1];
    
    const xMin = Math.min(p1.x, p2.x) - 10;
    const xMax = Math.max(p1.x, p2.x) + 10;
    const yMin = Math.min(p1.y, p2.y) - 10;
    const yMax = Math.max(p1.y, p2.y) + 10;
    
    if (mx >= xMin && mx <= xMax && my >= yMin && my <= yMax) {
      let dist;
      if (Math.abs(p1.x - p2.x) < 0.1) {
        dist = Math.abs(mx - p1.x);
      } else {
        dist = Math.abs(my - p1.y);
      }
      if (dist < minDistance && dist < 15) {
        minDistance = dist;
        closestIdx = i;
      }
    }
  }
  return closestIdx;
}
