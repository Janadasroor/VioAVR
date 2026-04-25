/**
 * NetworkClient.js
 * Handles WebSocket communication with the VioSpice Gateway.
 */
export class NetworkClient {
  constructor(url = 'ws://localhost:8080') {
    this.url = url;
    this.socket = null;
    this.isConnected = false;
    this._listeners = [];
    this._reconnectTimeout = null;
  }

  connect() {
    console.log(`Connecting to VioSpice Gateway: ${this.url}`);
    this.socket = new WebSocket(this.url);

    this.socket.onopen = () => {
      this.isConnected = true;
      console.log('Connected to VioSpice Gateway');
      this._emit('status', { connected: true });
    };

    this.socket.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        this._emit('message', data);
      } catch (err) {
        console.warn('Failed to parse gateway message:', event.data);
      }
    };

    this.socket.onclose = () => {
      this.isConnected = false;
      console.warn('Disconnected from VioSpice Gateway');
      this._emit('status', { connected: false });
      
      // Attempt reconnect
      clearTimeout(this._reconnectTimeout);
      this._reconnectTimeout = setTimeout(() => this.connect(), 2000);
    };

    this.socket.onerror = (err) => {
      console.error('WebSocket Error:', err);
    };
  }

  send(type, payload = {}) {
    if (!this.isConnected) return;
    this.socket.send(JSON.stringify({ type, ...payload }));
  }

  on(event, callback) {
    this._listeners.push({ event, callback });
  }

  _emit(event, data) {
    this._listeners
      .filter(l => l.event === event)
      .forEach(l => l.callback(data));
  }
}
