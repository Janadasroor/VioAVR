document.addEventListener('DOMContentLoaded', () => {
    const API_CIR_FILES = '/api/cir-files';
    const API_RUN_SIMULATION = '/api/run-simulation';

    const netlistContainer = document.getElementById('netlist-container');
    const refreshNetlistsBtn = document.getElementById('refresh-netlists');
    const netlistSearch = document.getElementById('netlist-search');

    const runBtn = document.getElementById('run-btn');

    const statusDot = document.getElementById('status-dot');
    const statusText = document.getElementById('status-text');

    const metricMcu = document.getElementById('metric-mcu');
    const metricModel = document.getElementById('metric-model');
    const metricTime = document.getElementById('metric-time');
    const metricPoints = document.getElementById('metric-points');

    const terminal = document.getElementById('console-terminal');
    const copyLogBtn = document.getElementById('copy-log-btn');
    const clearLogBtn = document.getElementById('clear-log-btn');

    const chartPlaceholder = document.getElementById('chart-placeholder');
    const toggleGridBtn = document.getElementById('toggle-grid');
    const resetZoomBtn = document.getElementById('reset-zoom');
    const exportCsvBtn = document.getElementById('export-csv');

    let netlists = [];
    let selectedNetlist = null;
    let chartInstance = null;
    let gridVisible = true;
    let latestData = null;
    let isRunning = false;
    let statusTimer = null;
    let fallbackColorIndex = 0;

    const pinColors = {
        'pb0_an': '#ff4757',
        'pb1_an': '#ff6b81',
        'pd0_an': '#ffa502',
        'pd1_an': '#eccc68',
        'pd2_an': '#2ed573',
        'pd3_an': '#7bed9f',
        'pd4_an': '#1e90ff',
        'pd5_an': '#70a1ff',
        'pd6_an': '#9b59b6',
        'pd7_an': '#ffa502',
        'adc_in_an': '#00d2ff',
        'v_dac': '#39ff14',
        'pa0_an': '#00d2ff',
        'pa4_an': '#ff4757',
        'pc5_an': '#39ff14',
        'out_plus': '#00ff88',
        'out_minus': '#ff4488',
        'v_diff': '#fee440'
    };

    const fallbackPalette = [
        '#f97316',
        '#22c55e',
        '#3b82f6',
        '#a855f7',
        '#ec4899',
        '#eab308',
        '#14b8a6',
        '#ef4444',
        '#84cc16',
        '#06b6d4'
    ];

    const colorCache = new Map();

    try {
        if (typeof Chart !== 'undefined' && window.ChartZoom) {
            const alreadyRegistered = Chart.registry &&
                Chart.registry.plugins &&
                typeof Chart.registry.plugins.get === 'function' &&
                Chart.registry.plugins.get('zoom');

            if (!alreadyRegistered) {
                Chart.register(window.ChartZoom);
            }
        }
    } catch (e) {
        console.warn('Zoom plugin registration failed:', e);
    }

    fetchNetlists();

    refreshNetlistsBtn.addEventListener('click', () => {
        if (!isRunning) fetchNetlists();
    });

    netlistSearch.addEventListener('input', () => {
        renderNetlists(netlistSearch.value);
    });

    runBtn.addEventListener('click', runSimulation);

    toggleGridBtn.addEventListener('click', () => {
        gridVisible = !gridVisible;

        if (chartInstance) {
            chartInstance.options.scales.x.grid.display = gridVisible;
            chartInstance.options.scales.y.grid.display = gridVisible;
            chartInstance.update();
            logTerminal('system', `[SYSTEM] Grid lines ${gridVisible ? 'enabled' : 'disabled'}.`);
        }
    });

    resetZoomBtn.addEventListener('click', resetView);

    exportCsvBtn.addEventListener('click', exportCsv);

    copyLogBtn.addEventListener('click', async () => {
        const ok = await copyText(terminal.innerText);
        if (ok) {
            logTerminal('system', '[SYSTEM] Log copied to clipboard.');
        } else {
            logTerminal('error', '[ERROR] Clipboard copy failed.');
        }
    });

    clearLogBtn.addEventListener('click', () => {
        terminal.innerHTML = '';
        logTerminal('system', '[SYSTEM] Log cleared.');
    });

    document.addEventListener('keydown', (e) => {
        if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') {
            e.preventDefault();
            runSimulation();
        }

        if (e.key === 'Escape') {
            resetView();
        }
    });

    async function fetchNetlists() {
        try {
            netlistContainer.innerHTML = '<div class="loading-placeholder">Loading SPICE netlists...</div>';

            const response = await fetch(API_CIR_FILES, { cache: 'no-store' });
            if (!response.ok) throw new Error(`HTTP ${response.status}`);

            netlists = await response.json();
            renderNetlists(netlistSearch.value);
            logTerminal('system', `[SYSTEM] Loaded ${netlists.length} co-simulation netlist(s).`);
        } catch (err) {
            netlistContainer.innerHTML = '<div class="loading-placeholder" style="color:#ff4757">Failed to load netlists. Make sure server.py is running.</div>';
            logTerminal('error', `[ERROR] Fetching netlists: ${err.message}`);
        }
    }

    function renderNetlists(filterText) {
        const q = (filterText || '').trim().toLowerCase();

        const filtered = netlists.filter(nl => {
            if (!q) return true;

            return (
                (nl.name || '').toLowerCase().includes(q) ||
                (nl.path || '').toLowerCase().includes(q) ||
                (nl.dir || '').toLowerCase().includes(q) ||
                (nl.mcu_type || '').toLowerCase().includes(q) ||
                (nl.model || '').toLowerCase().includes(q)
            );
        });

        if (!filtered.length) {
            netlistContainer.innerHTML = `<div class="loading-placeholder">${netlists.length ? 'No netlists match filter.' : 'No .cir files found in scratch/'}</div>`;
            return;
        }

        netlistContainer.innerHTML = '';

        filtered.forEach(nl => {
            const item = document.createElement('div');
            item.className = 'netlist-item';

            if (selectedNetlist && nl.path === selectedNetlist.path) {
                item.classList.add('selected');
                selectedNetlist = nl;
            }

            if (isRunning) {
                item.style.pointerEvents = 'none';
            }

            const shortPath = (nl.dir || nl.path || '').split('/').slice(-2).join('/');
            const model = nl.model || 'd_vioavr';
            const mcu = (nl.mcu_type || 'unknown').toUpperCase();

            item.innerHTML = `
                <h4>${escapeHtml(nl.name)}</h4>
                <p>.../${escapeHtml(shortPath)}</p>
                <div class="badges">
                    <span class="mcu-badge">${escapeHtml(mcu)}</span>
                    <span class="model-badge ${escapeHtml(model)}">${escapeHtml(model)}</span>
                    ${nl.has_hex ? '<span class="hex-badge">HEX</span>' : '<span class="hex-badge missing">NO HEX</span>'}
                </div>
            `;

            item.addEventListener('click', () => selectNetlist(nl, item));
            netlistContainer.appendChild(item);
        });

        if (selectedNetlist) {
            runBtn.disabled = false;
        }
    }

    function selectNetlist(nl, itemElement) {
        document.querySelectorAll('.netlist-item').forEach(el => el.classList.remove('selected'));
        itemElement.classList.add('selected');

        selectedNetlist = nl;

        runBtn.disabled = false;

        metricMcu.textContent = (nl.mcu_type || 'unknown').toUpperCase();
        metricModel.textContent = nl.model || 'd_vioavr';
        metricTime.textContent = '-';
        metricPoints.textContent = '-';

        logTerminal('system', `[SYSTEM] Selected netlist: ${nl.name} (MCU: ${metricMcu.textContent}, Model: ${metricModel.textContent})`);
    }

    async function runSimulation() {
        if (!selectedNetlist || isRunning) return;

        setRunning(true);

        if (chartInstance) {
            chartInstance.destroy();
            chartInstance = null;
        }

        chartPlaceholder.style.display = 'none';
        latestData = null;

        logTerminal('system', `[SYSTEM] Starting co-simulation: ${selectedNetlist.name}`);

        try {
            const response = await fetch(API_RUN_SIMULATION, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({
                    path: selectedNetlist.path,
                    timeout: 60
                })
            });

            const result = await response.json().catch(() => null);

            if (!response.ok) {
                throw new Error((result && result.error) || `HTTP ${response.status}`);
            }

            if (result.log) {
                logTerminal('stdout', result.log);
            }

            if (Array.isArray(result.warnings)) {
                result.warnings.forEach(w => logTerminal('warning', `[WARN] ${w}`));
            }

            if (result.success) {
                metricTime.textContent = `${(result.elapsed_s || 0).toFixed(2)}s`;

                const points = result.data && result.data.points ? result.data.points : 0;
                const total = result.data && result.data.total_points ? result.data.total_points : 0;
                metricPoints.textContent = total ? `${points}/${total}` : `${points}`;

                latestData = result.data;
                renderWaveforms(result.data);

                flashStatus('success', 'Completed');
                logTerminal('success', `[SUCCESS] Co-simulation completed in ${(result.elapsed_s || 0).toFixed(2)} seconds.`);
            } else {
                chartPlaceholder.style.display = 'flex';
                flashStatus('error', 'Failed');
                logTerminal('error', `[ERROR] Simulation returned code ${result.returncode ?? 'unknown'}.`);
            }
        } catch (err) {
            chartPlaceholder.style.display = 'flex';
            flashStatus('error', 'Error');
            logTerminal('error', `[ERROR] Simulation run: ${err.message}`);
        } finally {
            setRunning(false);
        }
    }

    function setRunning(running) {
        isRunning = running;

        if (running) {
            runBtn.disabled = true;
            runBtn.textContent = '⏳';
            setStatus('running', 'Running');
            document.querySelectorAll('.netlist-item').forEach(el => el.style.pointerEvents = 'none');
        } else {
            runBtn.disabled = !selectedNetlist;
            runBtn.textContent = '▶ Run';
            if (!statusTimer) {
                setStatus('idle', 'Idle');
            }
            document.querySelectorAll('.netlist-item').forEach(el => el.style.pointerEvents = 'auto');
        }
    }

    function setStatus(kind, text) {
        statusDot.className = `status-indicator ${kind}`;
        statusText.textContent = `Status: ${text}`;
    }

    function flashStatus(kind, text) {
        clearTimeout(statusTimer);
        setStatus(kind, text);

        statusTimer = setTimeout(() => {
            setStatus('idle', 'Idle');
            statusTimer = null;
        }, 5000);
    }

    function renderWaveforms(data) {
        if (typeof Chart === 'undefined') {
            logTerminal('error', '[ERROR] Chart.js failed to load from CDN. Waveform rendering disabled.');
            chartPlaceholder.style.display = 'flex';
            return;
        }

        if (!data || !Array.isArray(data.times) || data.times.length === 0 || !data.nodes || !Object.keys(data.nodes).length) {
            logTerminal('error', '[ERROR] No data points parsed from output log.');
            chartPlaceholder.style.display = 'flex';
            return;
        }

        const columns = Object.keys(data.nodes);

        let min = Number.POSITIVE_INFINITY;
        let max = Number.NEGATIVE_INFINITY;
        let finiteCount = 0;

        columns.forEach(col => {
            const values = data.nodes[col] || [];
            values.forEach(v => {
                if (Number.isFinite(v)) {
                    finiteCount++;
                    if (v < min) min = v;
                    if (v > max) max = v;
                }
            });
        });

        if (!finiteCount) {
            logTerminal('error', '[ERROR] Waveform data contains no finite values.');
            chartPlaceholder.style.display = 'flex';
            return;
        }

        let yMin;
        let yMax;

        if (max - min < 1e-9) {
            yMin = min - 1;
            yMax = max + 1;
        } else {
            const pad = (max - min) * 0.1;
            yMin = min - pad;
            yMax = max + pad;
        }

        if (min >= 0 && yMin > -0.5) yMin = -0.5;
        if (max <= 5 && yMax < 5.5) yMax = 5.5;

        const ctx = document.getElementById('waveform-chart').getContext('2d');

        const datasets = columns.map(pin => {
            const color = getColorForPin(pin);
            const values = data.nodes[pin] || [];

            const points = data.times.map((t, i) => ({
                x: t,
                y: values[i] === undefined ? null : values[i]
            }));

            return {
                label: pin.toUpperCase(),
                data: points,
                borderColor: color,
                backgroundColor: color + '18',
                borderWidth: 1.8,
                pointRadius: 0,
                pointHoverRadius: 4,
                tension: 0.1,
                spanGaps: true,
                fill: false,
                normalized: true
            };
        });

        chartInstance = new Chart(ctx, {
            type: 'line',
            data: { datasets },
            options: {
                responsive: true,
                maintainAspectRatio: false,
                animation: false,
                normalized: true,
                interaction: {
                    mode: 'nearest',
                    axis: 'x',
                    intersect: false
                },
                plugins: {
                    legend: {
                        position: 'top',
                        labels: {
                            color: '#cbd5e1',
                            font: { family: 'Outfit', size: 11, weight: 'bold' },
                            boxWidth: 14
                        }
                    },
                    tooltip: {
                        backgroundColor: 'rgba(15, 23, 42, 0.9)',
                        titleColor: '#00f2fe',
                        titleFont: { family: 'Outfit', size: 12, weight: 'bold' },
                        bodyFont: { family: 'Outfit', size: 11 },
                        borderColor: 'rgba(255, 255, 255, 0.1)',
                        borderWidth: 1,
                        padding: 10,
                        callbacks: {
                            title: (items) => {
                                if (items && items.length && items[0].parsed) {
                                    return `Time: ${items[0].parsed.x.toFixed(4)} ms`;
                                }
                                return '';
                            }
                        }
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
                            onZoomComplete: function () {
                                logTerminal('system', '[ZOOM] Zoomed to selection area.');
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
                        type: 'linear',
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
                        ticks: {
                            color: '#94a3b8',
                            font: { family: 'Outfit' },
                            maxTicksLimit: 12
                        }
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
                        ticks: {
                            color: '#94a3b8',
                            font: { family: 'Outfit' }
                        },
                        suggestedMin: yMin,
                        suggestedMax: yMax
                    }
                }
            }
        });

        chartPlaceholder.style.display = 'none';
    }

    function resetView() {
        if (!chartInstance) return;

        if (typeof chartInstance.resetZoom === 'function') {
            chartInstance.resetZoom();
        } else if (typeof chartInstance.reset === 'function') {
            chartInstance.reset();
        }

        chartInstance.update();
        logTerminal('system', '[SYSTEM] View reset to full range.');
    }

    function exportCsv() {
        if (!latestData || !Array.isArray(latestData.times) || !latestData.nodes) {
            logTerminal('warning', '[WARN] No waveform data to export.');
            return;
        }

        const cols = Object.keys(latestData.nodes);
        const rows = [['time_ms', ...cols]];

        latestData.times.forEach((t, i) => {
            const row = [t];

            cols.forEach(col => {
                const arr = latestData.nodes[col] || [];
                row.push(arr[i] === undefined || arr[i] === null ? '' : arr[i]);
            });

            rows.push(row);
        });

        const csv = rows.map(row => {
            return row.map(cell => {
                const s = String(cell);
                return /[",\n]/.test(s) ? '"' + s.replace(/"/g, '""') + '"' : s;
            }).join(',');
        }).join('\n');

        const baseName = ((selectedNetlist && selectedNetlist.name) || 'simulation').replace(/\.cir$/i, '');
        downloadBlob(new Blob([csv], { type: 'text/csv;charset=utf-8' }), `${baseName}_waveforms.csv`);

        logTerminal('system', '[SYSTEM] CSV exported.');
    }

    function logTerminal(type, text) {
        const nearBottom = terminal.scrollHeight - terminal.scrollTop - terminal.clientHeight < 60;

        const line = document.createElement('div');
        line.className = `term-line ${type}`;
        line.textContent = String(text ?? '');

        terminal.appendChild(line);

        while (terminal.children.length > 800) {
            terminal.removeChild(terminal.firstChild);
        }

        if (nearBottom) {
            terminal.scrollTop = terminal.scrollHeight;
        }
    }

    async function copyText(text) {
        try {
            await navigator.clipboard.writeText(text);
            return true;
        } catch (err) {
            const ta = document.createElement('textarea');
            ta.value = text;
            ta.style.position = 'fixed';
            ta.style.opacity = '0';
            document.body.appendChild(ta);
            ta.select();

            let ok = false;
            try {
                ok = document.execCommand('copy');
            } catch (e) {
                ok = false;
            }

            ta.remove();
            return ok;
        }
    }

    function downloadBlob(blob, filename) {
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = filename;
        document.body.appendChild(a);
        a.click();
        a.remove();
        setTimeout(() => URL.revokeObjectURL(url), 1000);
    }

    function getColorForPin(pin) {
        if (pinColors[pin]) return pinColors[pin];

        if (colorCache.has(pin)) {
            return colorCache.get(pin);
        }

        const color = fallbackPalette[fallbackColorIndex % fallbackPalette.length];
        fallbackColorIndex++;
        colorCache.set(pin, color);
        return color;
    }

    function escapeHtml(value) {
        return String(value ?? '').replace(/[&<>"']/g, c => ({
            '&': '&amp;',
            '<': '&lt;',
            '>': '&gt;',
            '"': '&quot;',
            "'": '&#39;'
        }[c]));
    }
});
