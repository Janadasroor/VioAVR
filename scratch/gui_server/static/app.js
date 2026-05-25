document.addEventListener('DOMContentLoaded', () => {
    // API Endpoints
    const API_CIR_FILES = '/api/cir-files';
    const API_RUN_SIMULATION = '/api/run-simulation';

    // DOM Elements
    const netlistContainer = document.getElementById('netlist-container');
    const runBtn = document.getElementById('run-btn');
    const statusDot = document.getElementById('status-dot');
    const statusText = document.getElementById('status-text');
    const metricMcu = document.getElementById('metric-mcu');
    const metricTime = document.getElementById('metric-time');
    const terminal = document.getElementById('console-terminal');
    const copyLogBtn = document.getElementById('copy-log-btn');
    const chartPlaceholder = document.getElementById('chart-placeholder');
    const toggleGridBtn = document.getElementById('toggle-grid');
    const resetZoomBtn = document.getElementById('reset-zoom');

    // State Variables
    let netlists = [];
    let selectedNetlist = null;
    let chartInstance = null;
    let gridVisible = true;
    let zoomMode = false;

    // Register Chart.js Zoom plugin
    if (typeof ChartZoom !== 'undefined') {
        Chart.register(ChartZoom);
    }

    // Harmonious neon colors for the 10-LED matrix pins and ADC/DAC nodes
    const pinColors = {
        'pb0_an': '#ff4757', // Red
        'pb1_an': '#ff6b81', // Coral pink
        'pd0_an': '#ffa502', // Orange
        'pd1_an': '#eccc68', // Light Gold
        'pd2_an': '#2ed573', // Green
        'pd3_an': '#7bed9f', // Light Green
        'pd4_an': '#1e90ff', // Blue
        'pd5_an': '#70a1ff', // Light Blue
        'pd6_an': '#9b59b6', // Amethyst Purple
        'pd7_an': '#e84393', // Magenta
        'adc_in_an': '#00d2ff', // Neon Cyan (Analog Input)
        'v_dac': '#39ff14',    // Bright Neon Green (Staircase DAC Output)
        'pa0_an': '#00d2ff',   // Neon Electric Cyan (COM0)
        'pa4_an': '#ff4757',   // Neon Coral Red (SEG0)
        'pc5_an': '#39ff14',   // Neon Electric Green (SEG11)
        'pd7_an': '#ffa502',   // Neon Orange (SEG19)
        'out_plus': '#00ff88',  // Neon Green (inverter positive output)
        'out_minus': '#ff4488'  // Hot Pink (inverter negative output)
    };

    // Load available netlists on startup
    fetchNetlists();

    async function fetchNetlists() {
        try {
            logTerminal("system", "[SYSTEM] Fetching available netlist files...");
            const response = await fetch(API_CIR_FILES);
            if (!response.ok) throw new Error("Failed to load netlists");
            netlists = await response.json();
            renderNetlists();
        } catch (err) {
            logTerminal("error", `[ERROR] Fetching netlists: ${err.message}`);
            netlistContainer.innerHTML = `<div class="loading-placeholder" style="color: #ff4757">Failed to load netlists. Make sure server.py is running.</div>`;
        }
    }

    function renderNetlists() {
        if (netlists.length === 0) {
            netlistContainer.innerHTML = `<div class="loading-placeholder">No .cir files found in scratch/</div>`;
            return;
        }

        netlistContainer.innerHTML = '';
        netlists.forEach(nl => {
            const item = document.createElement('div');
            item.className = 'netlist-item';
            item.innerHTML = `
                <h4>${nl.name}</h4>
                <p>Path: .../${nl.path.split('/').slice(-2).join('/')}</p>
                <div class="mcu-badge">${nl.mcu_type.toUpperCase()}</div>
            `;
            item.addEventListener('click', () => selectNetlist(nl, item));
            netlistContainer.appendChild(item);
        });
        logTerminal("system", `[SYSTEM] Loaded ${netlists.length} co-simulation netlist(s).`);
    }

    function selectNetlist(nl, itemElement) {
        // Toggle selected state in UI
        document.querySelectorAll('.netlist-item').forEach(el => el.classList.remove('selected'));
        itemElement.classList.add('selected');

        selectedNetlist = nl;
        runBtn.disabled = false;
        metricMcu.textContent = nl.mcu_type.toUpperCase();
        metricTime.textContent = "-";
        
        logTerminal("system", `[SYSTEM] Selected netlist: ${nl.name} (MCU: ${nl.mcu_type.toUpperCase()})`);
    }

    // Handle running the co-simulation
    runBtn.addEventListener('click', async () => {
        if (!selectedNetlist) return;

        // Set UI State to Running
        setRunningState(true);
        logTerminal("system", `[SYSTEM] Initializing co-simulation bridge for ${selectedNetlist.mcu_type.toUpperCase()}...`);
        logTerminal("system", `[SYSTEM] Launching daemon and ngspice mixed-signal engine...`);
        
        // Hide placeholder or clear old graph
        chartPlaceholder.style.display = 'none';
        if (chartInstance) {
            chartInstance.destroy();
            chartInstance = null;
        }

        try {
            const response = await fetch(API_RUN_SIMULATION, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ path: selectedNetlist.path })
            });

            if (!response.ok) {
                const errData = await response.json();
                throw new Error(errData.error || "Simulation run failed.");
            }

            const result = await response.json();
            
            // Print execution log to terminal
            if (result.log) {
                logTerminal("stdout", result.log);
            }

            if (result.success) {
                metricTime.textContent = `${(result.elapsed_s || 0).toFixed(2)}s`;
                logTerminal("success", `[SUCCESS] Co-simulation completed in ${result.elapsed_s.toFixed(2)} seconds!`);
                renderWaveforms(result.data);
            } else {
                logTerminal("error", "[ERROR] NgSpice reported failure during transient execution.");
                chartPlaceholder.style.display = 'flex';
            }
        } catch (err) {
            logTerminal("error", `[ERROR] Simulation run: ${err.message}`);
            chartPlaceholder.style.display = 'flex';
        } finally {
            setRunningState(false);
        }
    });

    function setRunningState(running) {
        if (running) {
            runBtn.disabled = true;
            runBtn.querySelector('.btn-text').textContent = 'Simulating...';
            statusDot.className = 'status-indicator running';
            statusText.textContent = 'Status: Running';
            document.querySelectorAll('.netlist-item').forEach(el => el.style.pointerEvents = 'none');
        } else {
            runBtn.disabled = false;
            runBtn.querySelector('.btn-text').textContent = 'Run Co-Simulation';
            statusDot.className = 'status-indicator idle';
            statusText.textContent = 'Status: Idle';
            document.querySelectorAll('.netlist-item').forEach(el => el.style.pointerEvents = 'auto');
        }
    }

    // Render multi-pin voltages in Chart.js
    function renderWaveforms(data) {
        if (!data || !data.times || data.times.length === 0) {
            logTerminal("error", "[ERROR] No data points parsed from output log.");
            chartPlaceholder.style.display = 'flex';
            return;
        }

        // Auto-scale y-axis based on data range
        const allVals = Object.values(data.nodes).flat();
        const dMin = Math.min(...allVals);
        const dMax = Math.max(...allVals);
        let yMin = -0.5, yMax = 5.5;
        if (dMax > 5.5 || dMin < -0.5) {
            const pad = (dMax - dMin) * 0.1;
            yMin = Math.floor((dMin - pad) * 2) / 2;
            yMax = Math.ceil((dMax + pad) * 2) / 2;
        }

        const ctx = document.getElementById('waveform-chart').getContext('2d');
        const datasets = [];

        // Build dataset for each parsed pin node
        Object.keys(data.nodes).forEach(pin => {
            const color = pinColors[pin] || getRandomColor();
            datasets.push({
                label: pin.toUpperCase(),
                data: data.nodes[pin],
                borderColor: color,
                backgroundColor: color + '08', // very subtle fill opacity
                borderWidth: 2,
                pointRadius: 0, // hide dot points to speed up render and look clean
                pointHoverRadius: 4,
                tension: 0.1,
                fill: true
            });
        });

        chartInstance = new Chart(ctx, {
            type: 'line',
            data: {
                labels: data.times,
                datasets: datasets
            },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                interaction: {
                    mode: 'index',
                    intersect: false,
                },
                plugins: {
                    legend: {
                        position: 'top',
                        labels: {
                            color: '#cbd5e1',
                            font: { family: 'Outfit', size: 11, weight: 'bold' }
                        }
                    },
                    tooltip: {
                        backgroundColor: 'rgba(15, 23, 42, 0.9)',
                        titleColor: '#00f2fe',
                        titleFont: { family: 'Outfit', size: 12, weight: 'bold' },
                        bodyFont: { family: 'Outfit', size: 11 },
                        borderColor: 'rgba(255, 255, 255, 0.1)',
                        borderWidth: 1,
                        padding: 10
                    },
                    zoom: {
                        zoom: {
                            drag: {
                                enabled: true,
                                backgroundColor: 'rgba(0, 242, 254, 0.12)',
                                borderColor: '#00f2fe',
                                borderWidth: 1,
                                threshold: 8
                            },
                            mode: 'xy',
                            onZoomComplete: function() {
                                logTerminal("system", "[ZOOM] Zoomed to selection area.");
                            }
                        },
                        pan: {
                            enabled: true,
                            mode: 'xy',
                            modifierKey: 'shift'
                        }
                    }
                },
                scales: {
                    x: {
                        title: {
                            display: true,
                            text: 'Time (ms)',
                            color: '#cbd5e1',
                            font: { family: 'Outfit', size: 12, weight: 'bold' }
                        },
                        grid: {
                            color: 'rgba(255, 255, 255, 0.04)',
                            display: gridVisible
                        },
                        ticks: { color: '#94a3b8', font: { family: 'Outfit' } }
                    },
                    y: {
                        title: {
                            display: true,
                            text: 'Voltage (V)',
                            color: '#cbd5e1',
                            font: { family: 'Outfit', size: 12, weight: 'bold' }
                        },
                        grid: {
                            color: 'rgba(255, 255, 255, 0.04)',
                            display: gridVisible
                        },
                        ticks: { color: '#94a3b8', font: { family: 'Outfit' } },
                        min: yMin,
                        max: yMax
                    }
                }
            }
        });
    }

    // Toggle grid visibility
    toggleGridBtn.addEventListener('click', () => {
        gridVisible = !gridVisible;
        if (chartInstance) {
            chartInstance.options.scales.x.grid.display = gridVisible;
            chartInstance.options.scales.y.grid.display = gridVisible;
            chartInstance.update();
            logTerminal("system", `[SYSTEM] Grid lines ${gridVisible ? 'enabled' : 'disabled'}.`);
        }
    });

    // Reset Zoom / view
    resetZoomBtn.addEventListener('click', () => {
        if (chartInstance) {
            if (typeof chartInstance.resetZoom === 'function') {
                chartInstance.resetZoom('default');
            } else {
                chartInstance.reset();
            }
            chartInstance.update();
            logTerminal("system", "[SYSTEM] View reset to full range.");
        }
    });

    // Terminal Logging Helper
    function logTerminal(type, text) {
        const line = document.createElement('div');
        line.className = `term-line ${type}`;
        line.textContent = text;
        terminal.appendChild(line);
        terminal.scrollTop = terminal.scrollHeight;
    }

    // Copy terminal logs to clipboard
    copyLogBtn.addEventListener('click', () => {
        const text = terminal.innerText;
        navigator.clipboard.writeText(text).then(() => {
            logTerminal("system", "[SYSTEM] Log copied to clipboard.");
        }).catch(err => {
            logTerminal("error", `[ERROR] Clipboard copy: ${err.message}`);
        });
    });

    function getRandomColor() {
        const letters = '0123456789ABCDEF';
        let color = '#';
        for (let i = 0; i < 6; i++) {
            color += letters[Math.floor(Math.random() * 16)];
        }
        return color;
    }
});
