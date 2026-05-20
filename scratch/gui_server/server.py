import os
import re
import sys
import json
import time
import subprocess
from http.server import BaseHTTPRequestHandler, HTTPServer

PORT = 8080
WORKSPACE_DIR = "/home/jnd/cpp_projects/VioAVR"
SCRATCH_DIR = os.path.join(WORKSPACE_DIR, "scratch")
DAEMON_PATH = os.path.join(WORKSPACE_DIR, "build/vioavr-bridge-daemon")
NGSPICE_PATH = "/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice"

class SimulationHTTPRequestHandler(BaseHTTPRequestHandler):
    def end_headers(self):
        # Enable CORS for local testing/development flexibility
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(200)
        self.end_headers()

    def do_GET(self):
        if self.path == '/api/cir-files':
            self.handle_get_cir_files()
        elif self.path == '/' or self.path == '/index.html':
            self.serve_static_file('index.html', 'text/html')
        elif self.path == '/style.css':
            self.serve_static_file('style.css', 'text/css')
        elif self.path == '/app.js':
            self.serve_static_file('app.js', 'application/javascript')
        else:
            self.send_error(404, "File not found")

    def do_POST(self):
        if self.path == '/api/run-simulation':
            self.handle_run_simulation()
        else:
            self.send_error(404, "Endpoint not found")

    def handle_get_cir_files(self):
        cir_files = []
        try:
            for root, dirs, files in os.walk(SCRATCH_DIR):
                # Skip the gui_server static directory to avoid self-scanning
                if "gui_server" in root:
                    continue
                for file in files:
                    if file.endswith('.cir'):
                        filepath = os.path.join(root, file)
                        mcu_type, hex_path = self.parse_cir_metadata(filepath)
                        cir_files.append({
                            "name": file,
                            "path": filepath,
                            "mcu_type": mcu_type,
                            "hex_path": hex_path,
                            "dir": root
                        })
            
            response_data = json.dumps(cir_files).encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Content-Length', len(response_data))
            self.end_headers()
            self.wfile.write(response_data)
        except Exception as e:
            self.send_json_error(500, str(e))

    def parse_cir_metadata(self, filepath):
        mcu_type = "unknown"
        hex_path = ""
        try:
            with open(filepath, 'r') as f:
                content = f.read()
                # Find mcu_type in .model definition
                mcu_match = re.search(r'mcu_type\s*=\s*["\']([^"\']+)["\']', content, re.IGNORECASE)
                if mcu_match:
                    mcu_type = mcu_match.group(1)
                # Find hex_file in .model definition
                hex_match = re.search(r'hex_file\s*=\s*["\']([^"\']+)["\']', content, re.IGNORECASE)
                if hex_match:
                    hex_path = hex_match.group(1)
        except Exception:
            pass
        return mcu_type, hex_path

    def handle_run_simulation(self):
        try:
            content_length = int(self.headers['Content-Length'])
            post_data = self.rfile.read(content_length)
            params = json.loads(post_data.decode('utf-8'))
            
            cir_path = params.get('path')
            if not cir_path or not os.path.exists(cir_path):
                self.send_json_error(400, "Invalid or missing circuit file path.")
                return

            cir_dir = os.path.dirname(cir_path)
            cir_filename = os.path.basename(cir_path)
            mcu_type, hex_path = self.parse_cir_metadata(cir_path)

            # Step 1: Force terminate any stale bridge daemon instances
            self.kill_stale_daemons()

            # Step 2: Start fresh vioavr-bridge-daemon in the background
            daemon_process = None
            if mcu_type != "unknown":
                daemon_cmd = [DAEMON_PATH, "--mcu", mcu_type, "--instance", mcu_type]
                daemon_process = subprocess.Popen(
                    daemon_cmd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    preexec_fn=os.setsid
                )
                # Brief sleep to allow SHM bridge to allocate and bind
                time.sleep(0.6)

            # Step 3: Launch ngspice simulation synchronously
            log_path = os.path.join(cir_dir, "sim_matrix.log")
            ngspice_cmd = [NGSPICE_PATH, "-b", cir_filename]
            
            # Start timer
            start_time = time.time()
            
            sim_result = subprocess.run(
                ngspice_cmd,
                cwd=cir_dir,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                timeout=30 # 30s timeout guard
            )
            
            elapsed_time = time.time() - start_time

            # Write standard output to sim_matrix.log for audit persistence
            with open(log_path, 'w') as lf:
                lf.write(sim_result.stdout)

            # Step 4: Gracefully terminate background daemon
            if daemon_process:
                try:
                    import signal
                    os.killpg(os.getpgid(daemon_process.pid), signal.SIGTERM)
                    daemon_process.wait(timeout=1.0)
                except Exception:
                    pass

            # Step 5: Parse simulation log
            parsed_data = self.parse_simulation_log(log_path)
            
            # Formulate robust JSON response
            response = {
                "success": sim_result.returncode == 0,
                "elapsed_s": elapsed_time,
                "log": sim_result.stdout,
                "data": parsed_data
            }
            
            response_data = json.dumps(response).encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'application/json')
            self.send_header('Content-Length', len(response_data))
            self.end_headers()
            self.wfile.write(response_data)

        except subprocess.TimeoutExpired:
            self.kill_stale_daemons()
            self.send_json_error(504, "Simulation execution timed out (limit: 30s).")
        except Exception as e:
            self.kill_stale_daemons()
            self.send_json_error(500, f"Internal Simulation Error: {str(e)}")

    def kill_stale_daemons(self):
        try:
            subprocess.run(["pkill", "-f", "vioavr-bridge-daemon"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            time.sleep(0.1)
        except Exception:
            pass

    def parse_simulation_log(self, log_path):
        time_data = {}
        current_columns = []
        
        if not os.path.exists(log_path):
            return {"times": [], "nodes": {}}

        with open(log_path, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or "------------------------" in line:
                    continue
                    
                parts = line.split()
                
                # Check for header block
                if "Index" in parts and "time" in parts:
                    current_columns = [p.replace("v(", "").replace(")", "") for p in parts[2:]]
                    continue
                    
                # Parse column data
                if current_columns and len(parts) >= 2:
                    try:
                        # parts[0] is Index, parts[1] is time
                        t = float(parts[1])
                        vals = [float(p) for p in parts[2:]]
                        
                        if t not in time_data:
                            time_data[t] = {}
                            
                        for col_name, val in zip(current_columns, vals):
                            time_data[t][col_name] = val
                    except ValueError:
                        pass

        if not time_data:
            return {"times": [], "nodes": {}}

        # Structure data for frontend graphing
        sorted_times = sorted(time_data.keys())
        
        # We limit the returned data to around 1000 points to keep Chart.js rendering fast and smooth
        step = max(1, len(sorted_times) // 1000)
        sampled_times = sorted_times[::step]
        
        nodes_dict = {}
        for t in sampled_times:
            for pin, val in time_data[t].items():
                if pin not in nodes_dict:
                    nodes_dict[pin] = []
                nodes_dict[pin].append(round(val, 4))
                
        return {
            "times": [round(t * 1000.0, 4) for t in sampled_times], # Convert time to ms
            "nodes": nodes_dict
        }

    def serve_static_file(self, filename, content_type):
        filepath = os.path.join(SCRATCH_DIR, "gui_server/static", filename)
        if not os.path.exists(filepath):
            self.send_error(404, f"Static file {filename} not found")
            return
            
        try:
            with open(filepath, 'rb') as f:
                data = f.read()
                self.send_response(200)
                self.send_header('Content-Type', content_type)
                self.send_header('Content-Length', len(data))
                self.end_headers()
                self.wfile.write(data)
        except Exception as e:
            self.send_error(500, f"Error reading file: {str(e)}")

    def send_json_error(self, code, message):
        response_data = json.dumps({"success": False, "error": message}).encode('utf-8')
        self.send_response(code)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', len(response_data))
        self.end_headers()
        self.wfile.write(response_data)

def run_server():
    server_address = ('', PORT)
    httpd = HTTPServer(server_address, SimulationHTTPRequestHandler)
    print(f"VioAVR GUI Web Dashboard Server running at http://localhost:{PORT}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping GUI Server...")
        httpd.server_close()

if __name__ == '__main__':
    run_server()
