const { spawn } = require('child_process');
const WebSocket = require('ws');
const path = require('path');

const wss = new WebSocket.Server({ port: 8080 });
const BRIDGE_PATH = path.join(__dirname, 'shm_bridge');

// Start the bridge process once
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
    console.log(`Bridge process exited with code ${code}`);
    wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
            client.send(JSON.stringify({ type: 'error', message: 'Bridge process crashed/exited unexpectedly' }));
        }
    });
});

console.log('VioSpice Gateway starting on ws://localhost:8080');

wss.on('connection', (ws) => {
  console.log('Client connected to dashboard gateway');
  
  const onBridgeData = (data) => {
      if (ws.readyState === WebSocket.OPEN) {
          ws.send(data.toString().trim());
      }
  };

  bridge.stdout.on('data', onBridgeData);

  const pollTimer = setInterval(() => {
    bridge.stdin.write('POLL\n');
  }, 50);

  let runInterval = null;

  ws.on('message', (message) => {
    const data = JSON.parse(message);
    if (data.type === 'run') {
        if (!runInterval) {
            runInterval = setInterval(() => bridge.stdin.write('CMD 8\n'), 10);
        }
    } else if (data.type === 'stop') {
        clearInterval(runInterval);
        runInterval = null;
    } else if (data.type === 'reset') {
        bridge.stdin.write('CMD 1\n');
    } else if (data.type === 'load') {
        bridge.stdin.write(`CMD 2 ${data.path}\n`);
    } else if (data.type === 'vcd') {
        bridge.stdin.write('CMD 16\n');
    } else if (data.type === 'list_hex') {
        const fs = require('fs');
        fs.readdir(process.cwd(), (err, files) => {
            const hexFiles = files ? files.filter(f => f.endsWith('.hex')) : [];
            ws.send(JSON.stringify({ type: 'hex_list', files: hexFiles }));
        });
    }
  });

  ws.on('close', () => {
    clearInterval(pollTimer);
    if (runInterval) clearInterval(runInterval);
    bridge.stdout.removeListener('data', onBridgeData);
    console.log('Client disconnected');
  });
});
