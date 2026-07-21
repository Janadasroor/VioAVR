const { spawn } = require('child_process');
const WebSocket = require('ws');
const net = require('net');
const fs = require('fs');
const path = require('path');

const BRIDGE_PATH = path.join(__dirname, 'shm_bridge');
const PORT_FILE = path.join(__dirname, 'port.json');
const START_PORT = 18081;

function findFreePort(start) {
  return new Promise((resolve) => {
    function tryPort(p) {
      if (p > start + 100) { resolve(null); return; }
      const srv = net.createServer();
      srv.on('error', () => { srv.close(); tryPort(p + 1); });
      srv.listen(p, '127.0.0.1', () => { const port = srv.address().port; srv.close(() => resolve(port)); });
    }
    tryPort(start);
  });
}

async function main() {
  const port = await findFreePort(START_PORT);
  if (!port) { console.error('No free port found'); process.exit(1); }

  const wss = new WebSocket.Server({ host: '127.0.0.1', port });
  fs.writeFileSync(PORT_FILE, JSON.stringify({ port }));
  console.log(`VioSpice Gateway starting on ws://127.0.0.1:${port}`);

  let bridgeAlive = true;
  const bridge = spawn(BRIDGE_PATH);

  bridge.stderr.on('data', (data) => {
    const errorMsg = data.toString().trim();
    console.error(`Bridge Error: ${errorMsg}`);
    wss.clients.forEach(client => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(JSON.stringify({ type: 'error', message: errorMsg }));
      }
    });
  });

  bridge.on('close', (code) => {
    bridgeAlive = false;
    console.log(`Bridge process exited with code ${code}`);
    wss.clients.forEach(client => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(JSON.stringify({ type: 'bridge_status', alive: false, message: 'Bridge process crashed/exited unexpectedly' }));
      }
    });
  });

  wss.on('connection', (ws) => {
    console.log('Client connected to dashboard gateway');
    ws.send(JSON.stringify({ type: 'bridge_status', alive: bridgeAlive, message: bridgeAlive ? 'Bridge connected' : 'Bridge unavailable — SHM daemon not running' }));

    const onBridgeData = (data) => {
      if (ws.readyState === WebSocket.OPEN) ws.send(data.toString().trim());
    };
    bridge.stdout.on('data', onBridgeData);

    const pollTimer = setInterval(() => {
      if (bridgeAlive) bridge.stdin.write('POLL\n');
    }, 50);

    let runInterval = null;

    ws.on('message', (message) => {
      const data = JSON.parse(message);
      if (data.type === 'run') {
        if (!bridgeAlive) { ws.send(JSON.stringify({ type: 'error', message: 'Cannot run: bridge daemon not running' })); return; }
        if (!runInterval) runInterval = setInterval(() => bridge.stdin.write('CMD 8\n'), 10);
      } else if (data.type === 'stop') {
        clearInterval(runInterval); runInterval = null;
      } else if (data.type === 'reset') {
        if (!bridgeAlive) { ws.send(JSON.stringify({ type: 'error', message: 'Cannot reset: bridge daemon not running' })); return; }
        bridge.stdin.write('CMD 1\n');
      } else if (data.type === 'load') {
        if (!bridgeAlive) { ws.send(JSON.stringify({ type: 'error', message: 'Cannot load hex: bridge daemon not running' })); return; }
        bridge.stdin.write(`CMD 2 ${data.path}\n`);
      } else if (data.type === 'vcd') {
        if (!bridgeAlive) { ws.send(JSON.stringify({ type: 'error', message: 'Cannot toggle VCD: bridge daemon not running' })); return; }
        bridge.stdin.write('CMD 16\n');
      } else if (data.type === 'list_hex') {
        const searchDirs = [
          process.cwd(),
          path.resolve(__dirname, '..', '..', 'build_release', 'tests'),
          path.resolve(__dirname, '..', '..', 'tests'),
          path.resolve(__dirname, '..', '..', 'external_projects'),
        ];
        const hexFiles = [];
        for (const dir of searchDirs) {
          try {
            if (fs.statSync(dir).isDirectory()) {
              for (const f of fs.readdirSync(dir)) if (f.endsWith('.hex')) hexFiles.push(path.join(dir, f));
            }
          } catch (_) {}
        }
        ws.send(JSON.stringify({ type: 'hex_list', files: hexFiles }));
      }
    });

    ws.on('close', () => {
      clearInterval(pollTimer);
      if (runInterval) clearInterval(runInterval);
      bridge.stdout.removeListener('data', onBridgeData);
      console.log('Client disconnected');
    });
  });
}

main();
