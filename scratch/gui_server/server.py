#!/usr/bin/env python3
"""
VioAVR Scratch GUI Server
Improved:
- Threaded HTTP server
- Safer path handling
- Configurable paths via CLI/env
- Shared simulation runner for /api/run-simulation and /api/launch-oscope
- Correct daemon handling for both simulation modes
- More robust log parsing and down-sampled waveform JSON
"""

import os
import re
import sys
import json
import time
import math
import signal
import shutil
import argparse
import subprocess
from pathlib import Path
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import urlparse

BASE_DIR = Path(__file__).resolve().parent

WORKSPACE_DIR = Path(os.environ.get("VIOAVR_WORKSPACE", str(BASE_DIR.parent.parent))).expanduser().resolve()
SCRATCH_DIR = WORKSPACE_DIR / "scratch"
BUILD_DIR = WORKSPACE_DIR / "build"

DAEMON_PATH = Path(os.environ.get("VIOAVR_DAEMON", str(BUILD_DIR / "vioavr-bridge-daemon"))).expanduser().resolve()
NGSPICE_PATH = Path(os.environ.get("NGSPICE_PATH", "/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice")).expanduser().resolve()
COSIM_SO = Path(os.environ.get("VIOAVR_COSIM_SO", str(BUILD_DIR / "cosim/libavr_cosim.so"))).expanduser().resolve()
OSCOPE_PATH = Path(os.environ.get("VIOAVR_OSCOPE", str(BUILD_DIR / "oscope/vioavr_oscope"))).expanduser().resolve()

STATIC_DIR = BASE_DIR / "static"
PORT = int(os.environ.get("VIOAVR_GUI_PORT", "8080"))

MAX_BODY_BYTES = 1 * 1024 * 1024
SIM_TIMEOUT_LIMIT = 300
DEFAULT_SIM_TIMEOUT = 30
MAX_CHART_POINTS = max(1, int(os.environ.get("VIOAVR_MAX_POINTS", "2000")))
MAX_LOG_LINES = max(100, int(os.environ.get("VIOAVR_MAX_LOG_LINES", "5000")))


def apply_config(workspace=None, ngspice=None, daemon=None, cosim=None, oscope=None, port=None):
    global WORKSPACE_DIR, SCRATCH_DIR, BUILD_DIR
    global DAEMON_PATH, NGSPICE_PATH, COSIM_SO, OSCOPE_PATH
    global STATIC_DIR, PORT

    if workspace:
        WORKSPACE_DIR = Path(workspace).expanduser().resolve()

    SCRATCH_DIR = WORKSPACE_DIR / "scratch"
    BUILD_DIR = WORKSPACE_DIR / "build"

    if daemon:
        DAEMON_PATH = Path(daemon).expanduser().resolve()
    else:
        DAEMON_PATH = Path(os.environ.get("VIOAVR_DAEMON", str(BUILD_DIR / "vioavr-bridge-daemon"))).expanduser().resolve()

    if ngspice:
        NGSPICE_PATH = Path(ngspice).expanduser().resolve()
    else:
        NGSPICE_PATH = Path(os.environ.get("NGSPICE_PATH", "/home/jnd/cpp_projects/VioMATRIXC/release/src/ngspice")).expanduser().resolve()

    if cosim:
        COSIM_SO = Path(cosim).expanduser().resolve()
    else:
        COSIM_SO = Path(os.environ.get("VIOAVR_COSIM_SO", str(BUILD_DIR / "cosim/libavr_cosim.so"))).expanduser().resolve()

    if oscope:
        OSCOPE_PATH = Path(oscope).expanduser().resolve()
    else:
        OSCOPE_PATH = Path(os.environ.get("VIOAVR_OSCOPE", str(BUILD_DIR / "oscope/vioavr_oscope"))).expanduser().resolve()

    STATIC_DIR = BASE_DIR / "static"

    if port:
        PORT = int(port)


def is_safe_path(base, target):
    try:
        base_real = os.path.realpath(str(base))
        target_real = os.path.realpath(str(target))
        return os.path.commonpath([base_real, target_real]) == base_real
    except Exception:
        return False


def resolve_hex_path(cir_dir, hex_path=""):
    cir_dir = Path(cir_dir)
    candidates = []

    if hex_path:
        p = Path(hex_path).expanduser()
        if p.is_absolute():
            candidates.append(p)
        else:
            candidates.append(cir_dir / p)
            candidates.append(WORKSPACE_DIR / p)
            candidates.append(cir_dir / p.name)

    try:
        if cir_dir.exists():
            hex_files = sorted(
                cir_dir.glob("*.hex"),
                key=lambda x: (x.name.lower() != "firmware.hex", x.name.lower())
            )
            candidates.extend(hex_files)
    except Exception:
        pass

    for candidate in candidates:
        try:
            candidate = Path(candidate)
            if candidate.exists() and candidate.is_file():
                return candidate.resolve()
        except Exception:
            pass

    return None


def parse_cir_metadata(filepath):
    filepath = Path(filepath)
    meta = {
        "mcu_type": "unknown",
        "hex_path": "",
        "hex_resolved": "",
        "model": "d_vioavr"
    }

    try:
        content = filepath.read_text(encoding="utf-8", errors="ignore")
    except Exception:
        return meta

    if re.search(r"\bd_cosim\b", content, re.IGNORECASE):
        meta["model"] = "d_cosim"
    elif re.search(r"\bd_vioavr\b", content, re.IGNORECASE):
        meta["model"] = "d_vioavr"

    mcu_match = re.search(r'''mcu_type\s*=\s*["']?([A-Za-z0-9_\-]+)''', content, re.IGNORECASE)
    if mcu_match:
        meta["mcu_type"] = mcu_match.group(1)
    else:
        sim_match = re.search(r'''sim_args\s*=\s*\[\s*["']([^"']+)["']''', content, re.IGNORECASE)
        if sim_match:
            first = sim_match.group(1).split(",")[0].strip()
            if first:
                meta["mcu_type"] = first

    hex_match = re.search(r'''hex_file\s*=\s*["']([^"']+)["']''', content, re.IGNORECASE)
    if hex_match:
        meta["hex_path"] = hex_match.group(1).strip()

    resolved = resolve_hex_path(filepath.parent, meta["hex_path"])
    if resolved:
        meta["hex_resolved"] = str(resolved)

    return meta


def ensure_symlink(target, link):
    target = Path(target)
    link = Path(link)

    try:
        if not target.exists():
            return False

        link.parent.mkdir(parents=True, exist_ok=True)

        if link.is_symlink():
            try:
                current = Path(os.readlink(link))
                if not current.is_absolute():
                    current = link.parent / current
                if current.resolve() == target.resolve():
                    return True
            except Exception:
                pass

            try:
                link.unlink()
            except Exception:
                return False

        elif link.exists():
            if link.is_dir():
                return False
            try:
                link.unlink()
            except Exception:
                return False

        os.symlink(target, link)
        return True

    except Exception:
        try:
            if not link.exists():
                shutil.copy2(target, link)
                return True
        except Exception:
            pass
        return False


def prepare_cosim(cir_dir, meta):
    warnings = []
    cir_dir = Path(cir_dir)

    cosim_link = cir_dir / "cosim" / "libavr_cosim.so"
    if COSIM_SO.exists():
        if not ensure_symlink(COSIM_SO, cosim_link):
            warnings.append(f"Could not create cosim symlink: {cosim_link}")
    else:
        warnings.append(f"cosim shared object not found: {COSIM_SO}")

    hex_target = None
    if meta.get("hex_resolved"):
        hex_target = Path(meta["hex_resolved"])

    if not hex_target or not hex_target.exists():
        hex_target = resolve_hex_path(cir_dir, meta.get("hex_path", ""))

    if hex_target:
        if not ensure_symlink(hex_target, cir_dir / "firmware.hex"):
            warnings.append(f"Could not create firmware.hex symlink from {hex_target}")
    else:
        warnings.append("No .hex firmware file could be resolved for this circuit.")

    return warnings


def kill_stale_daemons():
    if shutil.which("pkill"):
        try:
            subprocess.run(
                ["pkill", "-f", "vioavr-bridge-daemon"],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                check=False
            )
            time.sleep(0.1)
        except Exception:
            pass


def start_daemon(mcu_type):
    kill_stale_daemons()

    if not DAEMON_PATH.exists():
        raise FileNotFoundError(f"Daemon binary not found: {DAEMON_PATH}")

    cmd = [
        str(DAEMON_PATH),
        "--mcu", str(mcu_type),
        "--instance", str(mcu_type)
    ]

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        stdin=subprocess.DEVNULL,
        start_new_session=True
    )

    time.sleep(0.5)

    if proc.poll() is not None:
        raise RuntimeError(f"Daemon exited early with code {proc.returncode}")

    return proc


def stop_daemon(proc):
    if not proc:
        return

    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
        proc.wait(timeout=1.5)
    except Exception:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            proc.wait(timeout=0.5)
        except Exception:
            pass


def run_ngspice(cir_dir, cir_filename, timeout):
    if not NGSPICE_PATH.exists():
        raise FileNotFoundError(f"ngspice binary not found: {NGSPICE_PATH}")

    cir_dir = Path(cir_dir)
    log_path = cir_dir / "sim_matrix.log"

    cmd = [str(NGSPICE_PATH), "-b", str(cir_filename)]

    start = time.monotonic()
    result = subprocess.run(
        cmd,
        cwd=str(cir_dir),
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        stdin=subprocess.DEVNULL,
        encoding="utf-8",
        errors="replace",
        timeout=timeout,
        check=False
    )
    elapsed = time.monotonic() - start

    output = result.stdout or ""

    try:
        log_path.write_text(output, encoding="utf-8", errors="replace")
    except Exception:
        pass

    return result, elapsed, log_path, output


def truncate_output(output, max_lines=MAX_LOG_LINES):
    if not output:
        return ""

    lines = output.splitlines()
    if len(lines) <= max_lines:
        return output

    omitted = len(lines) - max_lines
    return f"[Truncated {omitted} log lines]\n" + "\n".join(lines[-max_lines:])


def normalize_node_name(name):
    name = name.strip()
    name = re.sub(r"^v\(", "", name, flags=re.IGNORECASE)
    name = re.sub(r"\)$", "", name)
    return name.strip('"\'')


def parse_simulation_log(log_path, max_points=MAX_CHART_POINTS):
    empty = {
        "times": [],
        "nodes": {},
        "points": 0,
        "total_points": 0,
        "columns": []
    }

    try:
        max_points = max(1, int(max_points))

        if not log_path or not Path(log_path).exists():
            return empty

        columns = []
        rows = []

        with open(log_path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.strip()
                if not line:
                    continue

                if set(line) <= {"-", "=", " "}:
                    continue

                parts = line.split()
                lower_parts = [p.lower() for p in parts]

                if "index" in lower_parts and "time" in lower_parts:
                    try:
                        time_idx = lower_parts.index("time")
                    except ValueError:
                        time_idx = 1

                    columns = [normalize_node_name(p) for p in parts[time_idx + 1:]]
                    continue

                if not columns:
                    continue

                if len(parts) < 2:
                    continue

                try:
                    t = float(parts[1])
                    raw_vals = parts[2:]
                except Exception:
                    try:
                        t = float(parts[0])
                        raw_vals = parts[1:]
                    except Exception:
                        continue

                vals = []
                for token in raw_vals:
                    try:
                        vals.append(float(token))
                    except ValueError:
                        vals.append(float("nan"))

                if not vals:
                    continue

                if len(vals) < len(columns):
                    vals.extend([float("nan")] * (len(columns) - len(vals)))
                elif len(vals) > len(columns):
                    vals = vals[:len(columns)]

                rows.append((t, vals))

        if not rows or not columns:
            return empty

        total = len(rows)

        if total <= max_points:
            sampled = rows
        else:
            step = max(1, math.ceil(total / max_points))
            sampled = rows[::step]
            if sampled[-1][0] != rows[-1][0]:
                sampled.append(rows[-1])

        times = []
        nodes = {col: [] for col in columns}

        for t, vals in sampled:
            times.append(round(t * 1000.0, 6))
            for i, col in enumerate(columns):
                val = vals[i]
                if val is None or math.isnan(val) or math.isinf(val):
                    nodes[col].append(None)
                else:
                    nodes[col].append(round(val, 6))

        return {
            "times": times,
            "nodes": nodes,
            "points": len(sampled),
            "total_points": total,
            "columns": columns
        }

    except Exception:
        return empty


def execute_simulation(params, launch_oscope=False):
    if not isinstance(params, dict):
        raise ValueError("Invalid JSON body.")

    cir_raw = params.get("path")
    if not cir_raw:
        raise ValueError("Missing circuit file path.")

    cir_path = Path(cir_raw).expanduser()

    if not is_safe_path(SCRATCH_DIR, cir_path):
        raise PermissionError("Circuit path is outside the scratch workspace.")

    cir_path = Path(os.path.realpath(str(cir_path)))

    if not cir_path.exists() or not cir_path.is_file():
        raise FileNotFoundError(f"Circuit file not found: {cir_path}")

    try:
        timeout = float(params.get("timeout", DEFAULT_SIM_TIMEOUT))
    except Exception:
        timeout = DEFAULT_SIM_TIMEOUT

    timeout = max(1.0, min(timeout, float(SIM_TIMEOUT_LIMIT)))

    cir_dir = cir_path.parent
    meta = parse_cir_metadata(cir_path)
    warnings = []
    daemon_proc = None

    try:
        if meta.get("model") == "d_cosim":
            warnings.extend(prepare_cosim(cir_dir, meta))
        else:
            if meta.get("mcu_type", "unknown") != "unknown":
                try:
                    daemon_proc = start_daemon(meta["mcu_type"])
                except Exception as exc:
                    warnings.append(f"Daemon start failed: {exc}")
            else:
                warnings.append("MCU type could not be detected; daemon was not started.")

        result, elapsed, log_path, output = run_ngspice(cir_dir, cir_path.name, timeout)

    finally:
        stop_daemon(daemon_proc)

    parsed = parse_simulation_log(log_path)

    response = {
        "success": result.returncode == 0,
        "elapsed_s": round(elapsed, 6),
        "returncode": result.returncode,
        "log": truncate_output(output),
        "data": parsed,
        "meta": meta,
        "warnings": warnings,
        "oscope_launched": False
    }

    if launch_oscope:
        if result.returncode == 0 and OSCOPE_PATH.exists():
            try:
                subprocess.Popen(
                    [str(OSCOPE_PATH), str(log_path)],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                    stdin=subprocess.DEVNULL,
                    start_new_session=True
                )
                response["oscope_launched"] = True
            except Exception as exc:
                warnings.append(f"Failed to launch oscilloscope: {exc}")
        elif result.returncode == 0:
            warnings.append(f"Oscilloscope binary not found: {OSCOPE_PATH}")

    return response


class SimulationHTTPRequestHandler(BaseHTTPRequestHandler):
    server_version = "VioAVRGui/2.0"
    protocol_version = "HTTP/1.1"

    def log_message(self, format, *args):
        return

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        super().end_headers()

    def do_OPTIONS(self):
        self.send_response(204)
        self.send_header("Content-Length", "0")
        self.end_headers()

    def _send_json(self, payload, status=200):
        body = json.dumps(payload, default=str).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _send_error(self, code, message):
        self._send_json({"success": False, "error": message}, status=code)

    def _read_json_body(self):
        length_header = self.headers.get("Content-Length")
        if length_header is None:
            return {}

        try:
            length = int(length_header)
        except Exception:
            raise ValueError("Invalid Content-Length header.")

        if length < 0 or length > MAX_BODY_BYTES:
            raise ValueError("Request body too large.")

        raw = self.rfile.read(length) if length else b"{}"
        if not raw:
            return {}

        return json.loads(raw.decode("utf-8"))

    def do_GET(self):
        route = urlparse(self.path).path

        if route == "/api/cir-files" or route == "/api/netlists":
            self.handle_get_cir_files()
        elif route == "/api/health":
            self._send_json({
                "success": True,
                "service": "vioavr-gui",
                "workspace": str(WORKSPACE_DIR),
                "scratch_exists": SCRATCH_DIR.exists(),
                "ngspice_exists": NGSPICE_PATH.exists(),
                "daemon_exists": DAEMON_PATH.exists(),
                "cosim_exists": COSIM_SO.exists(),
                "oscope_exists": OSCOPE_PATH.exists()
            })
        elif route == "/" or route == "/index.html":
            self.serve_static_file("index.html", "text/html; charset=utf-8")
        elif route == "/style.css":
            self.serve_static_file("style.css", "text/css; charset=utf-8")
        elif route == "/app.js":
            self.serve_static_file("app.js", "application/javascript; charset=utf-8")
        elif route == "/favicon.ico":
            self.send_response(204)
            self.send_header("Content-Length", "0")
            self.end_headers()
        else:
            self._send_error(404, "File not found")

    def do_POST(self):
        route = urlparse(self.path).path

        if route == "/api/run-simulation":
            self.handle_run_simulation()
        elif route == "/api/launch-oscope":
            self.handle_launch_oscope()
        else:
            self._send_error(404, "Endpoint not found")

    def handle_get_cir_files(self):
        files = []

        try:
            if SCRATCH_DIR.exists():
                for root, dirs, filenames in os.walk(SCRATCH_DIR):
                    root_path = Path(root)

                    dirs[:] = [
                        d for d in dirs
                        if not d.startswith(".")
                        and d not in {"gui_server", "node_modules", "__pycache__", "build", ".git"}
                    ]

                    if "gui_server" in root_path.parts:
                        continue

                    for filename in sorted(filenames):
                        if not filename.lower().endswith(".cir"):
                            continue

                        filepath = root_path / filename

                        try:
                            st = filepath.stat()
                        except OSError:
                            continue

                        meta = parse_cir_metadata(filepath)

                        files.append({
                            "name": filename,
                            "path": str(filepath),
                            "dir": str(root_path),
                            "mcu_type": meta.get("mcu_type", "unknown"),
                            "hex_path": meta.get("hex_path", ""),
                            "hex_resolved": meta.get("hex_resolved", ""),
                            "has_hex": bool(meta.get("hex_resolved")),
                            "model": meta.get("model", "d_vioavr"),
                            "size": st.st_size,
                            "modified": st.st_mtime
                        })

            files.sort(key=lambda item: (item["dir"], item["name"]))
            self._send_json(files)

        except Exception as exc:
            self._send_error(500, str(exc))

    def handle_run_simulation(self):
        try:
            params = self._read_json_body()
            response = execute_simulation(params, launch_oscope=False)
            self._send_json(response)

        except subprocess.TimeoutExpired:
            kill_stale_daemons()
            self._send_error(504, "Simulation execution timed out.")
        except (ValueError, FileNotFoundError, PermissionError) as exc:
            self._send_error(400, str(exc))
        except Exception as exc:
            kill_stale_daemons()
            self._send_error(500, f"Internal simulation error: {exc}")

    def handle_launch_oscope(self):
        try:
            params = self._read_json_body()
            response = execute_simulation(params, launch_oscope=True)
            self._send_json(response)

        except subprocess.TimeoutExpired:
            kill_stale_daemons()
            self._send_error(504, "Simulation execution timed out.")
        except (ValueError, FileNotFoundError, PermissionError) as exc:
            self._send_error(400, str(exc))
        except Exception as exc:
            kill_stale_daemons()
            self._send_error(500, f"Internal simulation error: {exc}")

    def serve_static_file(self, filename, content_type):
        filepath = STATIC_DIR / Path(filename).name

        if not is_safe_path(STATIC_DIR, filepath) or not filepath.is_file():
            self._send_error(404, f"Static file not found: {filename}")
            return

        try:
            data = filepath.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", content_type)
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-cache")
            self.end_headers()
            self.wfile.write(data)
        except Exception as exc:
            self._send_error(500, str(exc))


def run_server(argv=None):
    parser = argparse.ArgumentParser(description="VioAVR GUI dashboard and oscilloscope launcher")
    parser.add_argument("--port", "-p", type=int, default=PORT, help="HTTP port")
    parser.add_argument("--workspace", "-w", default=None, help="VioAVR workspace root")
    parser.add_argument("--ngspice", default=None, help="Path to ngspice binary")
    parser.add_argument("--daemon", default=None, help="Path to vioavr-bridge-daemon")
    parser.add_argument("--cosim", default=None, help="Path to libavr_cosim.so")
    parser.add_argument("--oscope", default=None, help="Path to vioavr_oscope")
    parser.add_argument("--view", "-v", default=None, help="View a simulation log file with the native oscilloscope")

    args = parser.parse_args(argv)

    apply_config(
        workspace=args.workspace,
        ngspice=args.ngspice,
        daemon=args.daemon,
        cosim=args.cosim,
        oscope=args.oscope,
        port=args.port
    )

    if args.view:
        log_path = Path(args.view).expanduser().resolve()

        if not log_path.exists():
            print(f"Log file not found: {log_path}")
            sys.exit(1)

        if not OSCOPE_PATH.exists():
            print(f"Oscilloscope binary not found: {OSCOPE_PATH}")
            print("Build the oscope tool first.")
            sys.exit(1)

        print(f"Launching oscilloscope: {log_path}")
        subprocess.run([str(OSCOPE_PATH), str(log_path)])
        return

    if not SCRATCH_DIR.exists():
        print(f"Warning: scratch directory not found: {SCRATCH_DIR}")

    try:
        httpd = ThreadingHTTPServer(("0.0.0.0", PORT), SimulationHTTPRequestHandler)
    except OSError as exc:
        print(f"Failed to bind port {PORT}: {exc}")
        sys.exit(1)

    httpd.daemon_threads = True

    print(f"VioAVR GUI Web Dashboard Server running at http://localhost:{PORT}")
    print(f"  Workspace: {WORKSPACE_DIR}")
    print(f"  Scratch:   {SCRATCH_DIR}")
    print(f"  ngspice:   {NGSPICE_PATH}")
    print(f"  daemon:    {DAEMON_PATH}")
    print(f"  cosim:     {COSIM_SO}")
    print(f"  oscope:    {OSCOPE_PATH}")
    print("  GET  /api/cir-files")
    print("  POST /api/run-simulation")
    print("  POST /api/launch-oscope")
    print("  --view <logfile>  view a saved log in the oscilloscope")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nStopping GUI Server...")
    finally:
        httpd.server_close()


if __name__ == "__main__":
    run_server()
