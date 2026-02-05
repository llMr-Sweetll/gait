#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
    <title>GaitOS V2.0</title>
    
    <!-- Chart.js CDN -->
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0"></script>
    <script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@2.0.1"></script>
    
    <style>
        :root {
            --bg: #0a0a0a;
            --card-bg: rgba(255, 255, 255, 0.05);
            --card-border: rgba(255, 255, 255, 0.1);
            --text: #ffffff;
            --text-muted: rgba(255, 255, 255, 0.6);
            --accent: #0A84FF;
            --success: #32D74B;
            --warning: #FF9F0A;
            --danger: #FF453A;
            --font: -apple-system, BlinkMacSystemFont, "SF Pro Text", "Segoe UI", Roboto, sans-serif;
        }

        * { box-sizing: border-box; }
        
        body {
            font-family: var(--font);
            background: var(--bg);
            color: var(--text);
            margin: 0;
            padding: 20px;
            padding-bottom: 100px;
            -webkit-font-smoothing: antialiased;
        }

        .container {
            max-width: 1200px;
            margin: 0 auto;
            display: flex;
            flex-direction: column;
            gap: 20px;
        }

        /* Header */
        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 10px;
        }

        h1 {
            font-size: 28px;
            font-weight: 700;
            margin: 0;
            letter-spacing: -0.5px;
        }

        .status-badge {
            display: flex;
            align-items: center;
            gap: 8px;
            padding: 8px 16px;
            background: rgba(50, 215, 75, 0.15);
            border-radius: 20px;
            font-size: 13px;
            font-weight: 600;
        }

        .live-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            background: var(--success);
            animation: pulse 2s infinite;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }

        /* Cards */
        .card {
            background: var(--card-bg);
            border: 1px solid var(--card-border);
            border-radius: 16px;
            padding: 20px;
            backdrop-filter: blur(10px);
        }

        .metric-card {
            background: var(--card-bg);
            border: 1px solid var(--card-border);
            border-left: 4px solid var(--text-muted);
            border-radius: 12px;
            padding: 16px;
            transition: all 0.3s ease;
        }

        .metric-card.status-normal { border-left-color: var(--success); }
        .metric-card.status-warning { border-left-color: var(--warning); }
        .metric-card.status-critical { border-left-color: var(--danger); }

        .metric-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 12px;
        }

        .metric-label {
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 1px;
            color: var(--text-muted);
            font-weight: 600;
        }

        .metric-info {
            cursor: help;
            opacity: 0.5;
            font-size: 14px;
        }

        .metric-value-container {
            display: flex;
            align-items: baseline;
            gap: 10px;
        }

        .metric-value {
            font-size: 36px;
            font-weight: 700;
            letter-spacing: -1px;
        }

        .metric-trend {
            font-size: 18px;
            color: var(--text-muted);
        }

        .metric-stats {
            display: flex;
            justify-content: space-between;
            margin-top: 12px;
            padding-top: 12px;
            border-top: 1px solid rgba(255,255,255,0.1);
            font-size: 11px;
            color: var(--text-muted);
        }

        /* Grid Layouts */
        .grid-2 {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 16px;
        }

        .grid-3 {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 12px;
        }

        .grid-4 {
            display: grid;
            grid-template-columns: repeat(4, 1fr);
            gap: 12px;
        }

        @media (max-width: 768px) {
            .grid-2, .grid-3, .grid-4 {
                grid-template-columns: 1fr;
            }
        }

        /* Chart Container */
        .chart-wrapper {
            position: relative;
            height: 250px;
            margin-top: 10px;
        }

        .chart-controls {
            position: absolute;
            top: 10px;
            right: 10px;
            display: flex;
            gap: 8px;
            z-index: 10;
        }

        .chart-btn {
            background: rgba(0,0,0,0.5);
            border: 1px solid rgba(255,255,255,0.2);
            color: white;
            padding: 6px 12px;
            border-radius: 6px;
            font-size: 11px;
            cursor: pointer;
            transition: all 0.2s;
        }

        .chart-btn:hover {
            background: rgba(0,0,0,0.7);
        }

        /* Buttons */
        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 10px;
            font-weight: 600;
            font-size: 14px;
            cursor: pointer;
            transition: all 0.2s;
            font-family: var(--font);
        }

        .btn-primary {
            background: var(--accent);
            color: white;
        }

        .btn-primary:hover {
            background: #0066CC;
            transform: translateY(-1px);
        }

        .btn-glass {
            background: var(--card-bg);
            border: 1px solid var(--card-border);
            color: var(--text);
        }

        .btn-glass:hover {
            background: rgba(255,255,255,0.1);
        }

        .btn-danger {
            background: var(--danger);
            color: white;
        }

        /* Action Bar */
        .action-bar {
            position: fixed;
            bottom: 0;
            left: 0;
            right: 0;
            background: rgba(10, 10, 10, 0.95);
            backdrop-filter: blur(20px);
            border-top: 1px solid var(--card-border);
            padding: 16px 20px;
            display: flex;
            gap: 12px;
            z-index: 100;
        }

        /* PHASE 3: Alert Banner */
        .alert-banner {
            position: fixed;
            top: 90px;
            left: 20px;
            right: 20px;
            background: linear-gradient(135deg, #FF453A, #FF6B5E);
            border-radius: 12px;
            padding: 16px;
            box-shadow: 0 4px 16px rgba(255, 69, 58, 0.5);
            z-index: 999;
            animation: slideDown 0.3s ease-out;
        }

        @keyframes slideDown {
            from { transform: translateY(-100%); opacity: 0; }
            to { transform: translateY(0); opacity: 1; }
        }

        .alert-content {
            display: flex;
            align-items: center;
            gap: 12px;
            color: white;
        }

        .alert-icon {
            font-size: 24px;
        }

        .alert-title {
            font-weight: 700;
            font-size: 14px;
        }

        .alert-message {
            font-size: 12px;
            opacity: 0.9;
            margin-top: 4px;
        }

        .alert-close {
            background: rgba(255,255,255,0.2);
            border: none;
            color: white;
            width: 28px;
            height: 28px;
            border-radius: 50%;
            cursor: pointer;
            margin-left: auto;
            font-size: 18px;
            font-weight: bold;
        }

        .alert-close:hover {
            background: rgba(255,255,255,0.3);
        }

        /* Modal */
        .modal {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0, 0, 0, 0.85);
            z-index: 1000;
            align-items: center;
            justify-content: center;
            backdrop-filter: blur(10px);
        }

        .modal-content {
            background: #1a1a1a;
            border: 1px solid var(--card-border);
            border-radius: 16px;
            padding: 30px;
            max-width: 500px;
            width: 90%;
            max-height: 80vh;
            overflow-y: auto;
        }

        .modal h2 {
            margin: 0 0 20px 0;
            font-size: 24px;
        }

        .form-group {
            margin-bottom: 20px;
        }

        .form-group label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            font-size: 13px;
            color: var(--text-muted);
        }

        .form-group input,
        .form-group select,
        .form-group textarea {
            width: 100%;
            padding: 12px;
            background: rgba(255, 255, 255, 0.05);
            border: 1px solid var(--card-border);
            border-radius: 8px;
            color: var(--text);
            font-size: 14px;
            font-family: var(--font);
        }

        .form-group input:focus,
        .form-group select:focus,
        .form-group textarea:focus {
            outline: none;
            border-color: var(--accent);
        }

        .form-actions {
            display: flex;
            gap: 12px;
            justify-content: flex-end;
            margin-top: 24px;
        }

        /* Export Options */
        .export-option {
            margin-bottom: 12px;
        }

        .export-desc {
            font-size: 11px;
            color: var(--text-muted);
            margin: 5px 0 0 0;
        }

        /* Collapsible */
        .collapsible-header {
            cursor: pointer;
            display: flex;
            justify-content: space-between;
            align-items: center;
            user-select: none;
        }

        .collapsible-header:hover {
            opacity: 0.8;
        }

        .collapsible-content {
            margin-top: 15px;
            padding-top: 15px;
            border-top: 1px solid var(--card-border);
        }

        /* Unit Label */
        .unit {
            font-size: 11px;
            color: var(--text-muted);
            margin-top: 4px;
        }

        /* Log List */
        .log-item {
            padding: 12px;
            background: rgba(255, 255, 255, 0.03);
            border-radius: 8px;
            margin-bottom: 8px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            cursor: pointer;
            transition: all 0.2s;
        }

        .log-item:hover {
            background: rgba(255, 255, 255, 0.08);
        }

        .log-item.selected {
            background: rgba(10, 132, 255, 0.2);
            border: 1px solid var(--accent);
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <div class="header">
            <h1>GaitOS V2.0</h1>
            <div class="status-badge">
                <div class="live-dot"></div>
                <span id="status-text">CONNECTED</span>
            </div>
        </div>

        <!-- Primary Metrics -->
        <div class="grid-2">
            <div class="metric-card" data-metric="stability">
                <div class="metric-header">
                    <span class="metric-label">Stability Index</span>
                    <span class="metric-info" title="Gait rhythmicity - 100% is perfect consistency">ⓘ</span>
                </div>
                <div class="metric-value-container">
                    <div class="metric-value" id="val-stab">--</div>
                    <span class="metric-trend" id="trend-stab"></span>
                </div>
                <div class="metric-stats">
                    <span>Min: <span id="min-stab">--</span></span>
                    <span>Avg: <span id="avg-stab">--</span></span>
                    <span>Max: <span id="max-stab">--</span></span>
                </div>
            </div>

            <div class="metric-card" data-metric="cadence">
                <div class="metric-header">
                    <span class="metric-label">Cadence</span>
                    <span class="metric-info" title="Steps per minute - Normal range: 90-130">ⓘ</span>
                </div>
                <div class="metric-value-container">
                    <div class="metric-value" id="val-cad">--</div>
                    <span class="metric-trend" id="trend-cad"></span>
                </div>
                <div class="metric-stats">
                    <span>Min: <span id="min-cad">--</span></span>
                    <span>Avg: <span id="avg-cad">--</span></span>
                    <span>Max: <span id="max-cad">--</span></span>
                </div>
            </div>
        </div>

        <!-- Secondary Metrics -->
        <div class="grid-4">
            <div class="card" style="padding:12px; text-align:center;">
                <div class="metric-label">Clearance</div>
                <div style="font-size:24px; font-weight:700; margin:8px 0;" id="val-clear">0.0</div>
                <div class="unit">cm</div>
            </div>
            <div class="card" style="padding:12px; text-align:center;">
                <div class="metric-label">Battery</div>
                <div style="font-size:24px; font-weight:700; margin:8px 0;" id="val-battery">100</div>
                <div class="unit">%</div>
            </div>
            <div class="card" style="padding:12px; text-align:center;">
                <div class="metric-label">Distance</div>
                <div style="font-size:24px; font-weight:700; margin:8px 0;" id="val-dist">0.0</div>
                <div class="unit">m</div>
            </div>
            <div class="card" style="padding:12px; text-align:center;">
                <div class="metric-label">Phase</div>
                <div style="font-size:16px; font-weight:700; margin:12px 0;" id="val-phase">STANCE</div>
            </div>
        </div>

        <!-- Real-Time Trajectory Chart -->
        <div class="card">
            <div class="metric-label" style="margin-bottom: 10px;">Real-Time Trajectory (Side View)</div>
            <div class="chart-wrapper">
                <div class="chart-controls">
                    <button class="chart-btn" onclick="resetZoom()">Reset Zoom</button>
                    <button class="chart-btn" onclick="clearTrajectory()">Clear</button>
                </div>
                <canvas id="trajectoryChart"></canvas>
            </div>
        </div>

        <!-- Cadence Time-Series Chart -->
        <div class="card">
            <div class="metric-label" style="margin-bottom: 10px;">Cadence Over Time</div>
            <div class="chart-wrapper" style="height: 180px;">
                <canvas id="cadenceChart"></canvas>
            </div>
        </div>

        <!-- Advanced Tuning -->
        <div class="card">
            <div class="collapsible-header" onclick="toggleSection('tuning')">
                <div class="metric-label">Advanced Tuning</div>
                <div class="metric-label" style="color:var(--accent)">▼</div>
            </div>
            <div id="tuning-panel" class="collapsible-content" style="display:none;">
                <div class="form-group">
                    <label>Min Step Duration (ms)</label>
                    <input type="number" id="cfg-step-time" value="280" step="10">
                </div>
                <div class="form-group">
                    <label>ZUPT Acceleration Threshold (g)</label>
                    <input type="number" id="cfg-zupt-acc" value="0.25" step="0.05">
                </div>
                <button class="btn btn-primary" onclick="saveConfig()" style="width:100%;">Apply Settings</button>
            </div>
        </div>

        <!-- Data Export -->
        <div class="card">
            <div class="metric-label" style="margin-bottom: 15px;">Data Export</div>
            
            <div class="export-option">
                <button class="btn btn-primary" onclick="exportCSV()" style="width:100%;">
                    📊 Download CSV (Full Data)
                </button>
                <p class="export-desc">23 columns, 100Hz raw data</p>
            </div>
            
            <div class="export-option">
                <button class="btn btn-glass" onclick="exportJSON()" style="width:100%;">
                    { } Download JSON
                </button>
                <p class="export-desc">Machine-readable format</p>
            </div>
            
            <div class="export-option">
                <button class="btn btn-glass" onclick="exportSummary()" style="width:100%;">
                    📄 Generate Summary Report
                </button>
                <p class="export-desc">HTML report with statistics</p>
            </div>
        </div>

        <!-- Session Logs -->
        <div class="card">
            <div class="collapsible-header" onclick="toggleSection('logs')">
                <div class="metric-label">Session History</div>
                <div class="metric-label" style="cursor:pointer; color:var(--accent)" onclick="event.stopPropagation(); fetchLogs()">REFRESH</div>
            </div>
            <div id="logs-panel" class="collapsible-content">
                <div id="log-list"></div>
            </div>
        </div>
    </div>

    <!-- Floating Action Bar -->
    <div class="action-bar">
        <button class="btn btn-glass" onclick="api('calibrate')" style="flex:1;">Zero Sensors</button>
        <button id="btn-toggle" class="btn btn-primary" onclick="toggleRecord()" style="flex:2;">Start Recording</button>
    </div>

    <!-- Session Modal -->
    <div id="sessionModal" class="modal">
        <div class="modal-content">
            <h2>Start New Session</h2>
            <form id="sessionForm" onsubmit="startSession(event)">
                <div class="form-group">
                    <label>Session Name</label>
                    <input type="text" id="sessionName" placeholder="Auto-generated or custom...">
                </div>
                
                <div class="form-group">
                    <label>Patient ID (Optional)</label>
                    <input type="text" id="patientId" placeholder="P-12345">
                </div>
                
                <div class="form-group">
                    <label>Session Type</label>
                    <select id="sessionType">
                        <option value="baseline">Baseline Assessment</option>
                        <option value="therapy">During Therapy</option>
                        <option value="post">Post-Treatment</option>
                        <option value="followup">Follow-up</option>
                        <option value="research">Research/Testing</option>
                    </select>
                </div>
                
                <div class="form-group">
                    <label>Notes</label>
                    <textarea id="sessionNotes" rows="3" placeholder="Pre-session observations, patient condition..."></textarea>
                </div>
                
                <div class="form-actions">
                    <button type="button" class="btn btn-glass" onclick="closeSessionModal()">Cancel</button>
                    <button type="submit" class="btn btn-primary">Start Recording</button>
                </div>
            </form>
        </div>
    </div>

    <script>
        // Global state
        let recording = false;
        let selectedLog = null;
        
        // Metric history for statistics and trends
        const metricHistory = {
            stability: [],
            cadence: [],
            clearance: []
        };

        // Chart.js instances
        let trajectoryChart, cadenceChart;

        // Initialize charts on page load
        window.onload = function() {
            initCharts();
            fetchLogs();
            setInterval(sync, 100);
        };

        function initCharts() {
            // Trajectory Chart
            const trajCtx = document.getElementById('trajectoryChart').getContext('2d');
            trajectoryChart = new Chart(trajCtx, {
                type: 'scatter',
                data: {
                    datasets: [{
                        label: 'Gait Path',
                        data: [],
                        borderColor: '#0A84FF',
                        backgroundColor: 'rgba(10, 132, 255, 0.2)',
                        showLine: true,
                        pointRadius: 2
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    animation: false,
                    scales: {
                        x: { 
                            title: { display: true, text: 'Forward (m)', color: '#fff' },
                            grid: { color: 'rgba(255,255,255,0.1)' },
                            ticks: { color: '#999' }
                        },
                        y: { 
                            title: { display: true, text: 'Height (m)', color: '#fff' },
                            grid: { color: 'rgba(255,255,255,0.1)' },
                            ticks: { color: '#999' },
                            min: 0
                        }
                    },
                    plugins: {
                        legend: { display: false },
                        zoom: {
                            zoom: {
                                wheel: { enabled: true },
                                pinch: { enabled: true },
                                mode: 'xy'
                            },
                            pan: {
                                enabled: true,
                                mode: 'xy'
                            }
                        }
                    }
                }
            });

            // Cadence Chart
            const cadCtx = document.getElementById('cadenceChart').getContext('2d');
            cadenceChart = new Chart(cadCtx, {
                type: 'line',
                data: {
                    labels: [],
                    datasets: [{
                        label: 'Cadence (spm)',
                        data: [],
                        borderColor: '#32D74B',
                        backgroundColor: 'rgba(50, 215, 75, 0.1)',
                        fill: true,
                        tension: 0.4
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false,
                    animation: false,
                    scales: {
                        x: { 
                            display: false
                        },
                        y: { 
                            title: { display: true, text: 'Steps/min', color: '#fff' },
                            grid: { color: 'rgba(255,255,255,0.1)' },
                            ticks: { color: '#999' },
                            min: 0,
                            max: 150
                        }
                    },
                    plugins: {
                        legend: { display: false }
                    }
                }
            });
        }

        // Sync with device
        async function sync() {
            try {
                const res = await fetch('/api/status');
                const d = await res.json();
                
                // Update metrics
                updateMetric('stability', d.stab);
                updateMetric('cadence', d.cad);
                
                document.getElementById('val-clear').innerText = (d.pz * 100).toFixed(1);
                document.getElementById('val-battery').innerText = d.battery_pct || 100;
                document.getElementById('val-dist').innerText = d.dist_m.toFixed(1);
                document.getElementById('val-phase').innerText = d.phase ? "SWING" : "STANCE";
                
                // Update charts
                trajectoryChart.data.datasets[0].data.push({x: d.px, y: d.pz});
                if (trajectoryChart.data.datasets[0].data.length > 300) {
                    trajectoryChart.data.datasets[0].data.shift();
                }
                trajectoryChart.update('none');
                
                cadenceChart.data.labels.push('');
                cadenceChart.data.datasets[0].data.push(d.cad);
                if (cadenceChart.data.datasets[0].data.length > 60) {
                    cadenceChart.data.labels.shift();
                    cadenceChart.data.datasets[0].data.shift();
                }
                cadenceChart.update('none');
                
                // Update recording state
                if (d.recording !== recording) {
                    recording = d.recording;
                    updateRecordButton();
                }
            } catch (err) {
                document.getElementById('status-text').innerText = 'DISCONNECTED';
            }
        }

        function updateMetric(metric, value) {
            // Update value
            document.getElementById('val-' + metric.substring(0, 4)).innerText = value.toFixed(metric === 'cadence' ? 0 : 1);
            
            // Track history
            metricHistory[metric].push(value);
            if (metricHistory[metric].length > 100) metricHistory[metric].shift();
            
            // Calculate statistics
            if (metricHistory[metric].length > 2) {
                const min = Math.min(...metricHistory[metric]).toFixed(1);
                const max = Math.max(...metricHistory[metric]).toFixed(1);
                const avg = (metricHistory[metric].reduce((a,b) => a+b, 0) / metricHistory[metric].length).toFixed(1);
                
                document.getElementById('min-' + metric.substring(0, 4)).innerText = min;
                document.getElementById('max-' + metric.substring(0, 4)).innerText = max;
                document.getElementById('avg-' + metric.substring(0, 4)).innerText = avg;
                
                // Calculate trend
                const trend = calculateTrend(metric, value);
                document.getElementById('trend-' + metric.substring(0, 4)).innerText = trend;
            }
            
            // Update color coding
            updateMetricColor(metric, value);
        }

        function calculateTrend(metric, currentValue) {
            if (metricHistory[metric].length < 5) return '';
            
            const recent = metricHistory[metric].slice(-5);
            const avg = recent.reduce((a, b) => a + b, 0) / recent.length;
            const change = ((currentValue - avg) / avg * 100);
            
            if (change > 5) return '↑ ' + change.toFixed(0) + '%';
            if (change < -5) return '↓ ' + Math.abs(change).toFixed(0) + '%';
            return '→';
        }

        function updateMetricColor(metric, value) {
            const card = document.querySelector(`[data-metric="${metric}"]`);
            if (!card) return;
            
            if (metric === 'stability') {
                if (value >= 80) card.className = 'metric-card status-normal';
                else if (value >= 60) card.className = 'metric-card status-warning';
                else card.className = 'metric-card status-critical';
            }
            
            if (metric === 'cadence') {
                if (value >= 90 && value <= 130) card.className = 'metric-card status-normal';
                else if (value >= 70 && value <= 150) card.className = 'metric-card status-warning';
                else card.className = 'metric-card status-critical';
            }
        }

        // API calls
        async function api(endpoint) {
            await fetch('/api/' + endpoint, { method: 'POST' });
        }

        // Recording with session management
        function toggleRecord() {
            if (!recording) {
                document.getElementById('sessionModal').style.display = 'flex';
            } else {
                api('record/stop');
                recording = false;
                updateRecordButton();
                fetchLogs();
            }
        }

        function closeSessionModal() {
            document.getElementById('sessionModal').style.display = 'none';
        }

        async function startSession(e) {
            e.preventDefault();
            
            const metadata = {
                name: document.getElementById('sessionName').value || generateSessionName(),
                patientId: document.getElementById('patientId').value,
                type: document.getElementById('sessionType').value,
                notes: document.getElementById('sessionNotes').value
            };
            
            await fetch('/api/record/start', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(metadata)
            });
            
            recording = true;
            updateRecordButton();
            closeSessionModal();
            
            // Reset form
            document.getElementById('sessionForm').reset();
        }

        function generateSessionName() {
            const d = new Date();
            return `Session_${d.getFullYear()}${(d.getMonth()+1).toString().padStart(2,'0')}${d.getDate().toString().padStart(2,'0')}_${d.getHours().toString().padStart(2,'0')}${d.getMinutes().toString().padStart(2,'0')}`;
        }

        function updateRecordButton() {
            const btn = document.getElementById('btn-toggle');
            if (recording) {
                btn.innerText = 'Stop Recording';
                btn.className = 'btn btn-danger';
            } else {
                btn.innerText = 'Start Recording';
                btn.className = 'btn btn-primary';
            }
        }

        // Configuration
        async function saveConfig() {
            const config = {
                step_time: parseFloat(document.getElementById('cfg-step-time').value),
                zupt_acc: parseFloat(document.getElementById('cfg-zupt-acc').value)
            };
            
            await fetch('/api/config', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(config)
            });
            
            alert('Settings saved!');
        }

        // Logs
        async function fetchLogs() {
            const res = await fetch('/api/logs');
            const logs = await res.json();
            
            const listEl = document.getElementById('log-list');
            if (logs.length === 0) {
                listEl.innerHTML = '<p style="color: var(--text-muted); font-size: 13px;">No recordings yet</p>';
                return;
            }
            
            listEl.innerHTML = logs.map(log => `
                <div class="log-item${selectedLog === log.name ? ' selected' : ''}" onclick="selectLog('${log.name}')">
                    <div>
                        <div style="font-size: 13px; font-weight: 600;">${log.name}</div>
                        <div style="font-size: 11px; color: var(--text-muted);">${(log.size / 1024).toFixed(1)} KB</div>
                    </div>
                    <button class="btn btn-glass" style="padding: 6px 12px; font-size: 11px;" onclick="event.stopPropagation(); downloadLog('${log.name}')">Download</button>
                </div>
            `).join('');
        }

        function selectLog(name) {
            selectedLog = name;
            fetchLogs();
        }

        function downloadLog(name) {
            window.open('/' + name, '_blank');
        }

        // Export functions
        function exportCSV() {
            if (!selectedLog) {
                alert('Please select a session from the history');
                return;
            }
            downloadLog(selectedLog);
        }

        async function exportJSON() {
            if (!selectedLog) {
                alert('Please select a session');
                return;
            }
            
            const response = await fetch('/' + selectedLog);
            const csvText = await response.text();
            
            const lines = csvText.split('\n').filter(l => l && !l.startsWith('#'));
            const headers = lines[0].split(',').map(h => h.trim());
            const data = [];
            
            for (let i = 1; i < lines.length; i++) {
                if (!lines[i].trim()) continue;
                const values = lines[i].split(',');
                const row = {};
                headers.forEach((h, idx) => {
                    const val = values[idx];
                    row[h] = isNaN(val) ? val : parseFloat(val);
                });
                data.push(row);
            }
            
            const json = JSON.stringify({
                session: selectedLog,
                format: 'GaitOS V2.0',
                recordCount: data.length,
                data: data
            }, null, 2);
            
            downloadFile(json, selectedLog.replace('.csv', '.json'), 'application/json');
        }

        async function exportSummary() {
            if (!selectedLog) {
                alert('Please select a session');
                return;
            }
            
            const response = await fetch('/' + selectedLog);
            const csvText = await response.text();
            const stats = calculateSessionStats(csvText);
            
            const html = `<!DOCTYPE html>
<html>
<head>
    <title>GaitOS Session Report</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 40px auto; background: #f5f5f5; }
        h1 { color: #333; }
        .stat { background: white; padding: 20px; margin: 15px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .label { font-weight: bold; color: #666; font-size: 14px; }
        .value { font-size: 32px; color: #0A84FF; margin-top: 8px; }
    </style>
</head>
<body>
    <h1>GaitOS Session Report</h1>
    <p><strong>Session:</strong> ${selectedLog}</p>
    <p><strong>Duration:</strong> ${stats.duration}s | <strong>Device:</strong> M5StickC Plus 2 (Ankle-mounted)</p>
    
    <div class="stat">
        <div class="label">Total Steps</div>
        <div class="value">${stats.totalSteps}</div>
    </div>
    
    <div class="stat">
        <div class="label">Average Cadence</div>
        <div class="value">${stats.avgCadence.toFixed(1)} <small style="font-size:16px;">steps/min</small></div>
    </div>
    
    <div class="stat">
        <div class="label">Average Stability</div>
        <div class="value">${stats.avgStability.toFixed(1)} <small style="font-size:16px;">%</small></div>
    </div>
    
    <div class="stat">
        <div class="label">Total Distance</div>
        <div class="value">${stats.totalDistance.toFixed(2)} <small style="font-size:16px;">m</small></div>
    </div>
    
    <p style="margin-top: 40px; color: #999; font-size: 12px;">Generated by GaitOS V2.0 | Data sampled at 100Hz</p>
</body>
</html>`;
            
            downloadFile(html, selectedLog.replace('.csv', '_report.html'), 'text/html');
        }

        function calculateSessionStats(csvText) {
            const lines = csvText.split('\n').filter(l => l && !l.startsWith('#'));
            if (lines.length < 2) return { duration: 0, totalSteps: 0, avgCadence: 0, avgStability: 0, totalDistance: 0 };
            
            const data = lines.slice(1).map(l => l.split(','));
            
            // Extract columns (assuming V2.0 format: 23 columns)
            const cadences = data.map(r => parseFloat(r[21])).filter(v => !isNaN(v) && v > 0);
            const stabilities = data.map(r => parseFloat(r[22])).filter(v => !isNaN(v));
            const positions = data.map(r => parseFloat(r[17])).filter(v => !isNaN(v));
            
            return {
                duration: (data.length / 100).toFixed(1),
                totalSteps: Math.round(Math.max(...data.map(r => parseFloat(r[20]) || 0), 0)),
                avgCadence: cadences.length > 0 ? cadences.reduce((a,b) => a+b, 0) / cadences.length : 0,
                avgStability: stabilities.length > 0 ? stabilities.reduce((a,b) => a+b, 0) / stabilities.length : 0,
                totalDistance: Math.max(...positions, 0)
            };
        }

        function downloadFile(content, filename, mimeType) {
            const blob = new Blob([content], { type: mimeType });
            const url = URL.createObjectURL(blob);
            const a = document.createElement('a');
            a.href = url;
            a.download = filename;
            a.click();
            URL.revokeObjectURL(url);
        }

        // Chart controls
        function resetZoom() {
            trajectoryChart.resetZoom();
        }

        function clearTrajectory() {
            trajectoryChart.data.datasets[0].data = [];
            trajectoryChart.update();
        }

        // Collapsible sections
        function toggleSection(id) {
            const panel = document.getElementById(id + '-panel');
            panel.style.display = panel.style.display === 'none' ? 'block' : 'none';
        }
        
        // PHASE 3.5: File Management Functions
        async function deleteLog(filename) {
            if (!confirm(`Delete ${filename}?\n\nThis action cannot be undone.`)) {
                return;
            }
            
            try {
                const res = await fetch('/api/delete/' + encodeURIComponent(filename), {
                    method: 'DELETE'
                });
                
                if (res.ok) {
                    showToast('✓ File deleted successfully');
                } else {
                    showToast('✗ Failed to delete file');
                }
            } catch (err) {
                console.error('Delete error:', err);
                showToast('✗ Error deleting file');
            }
        }
        
        function downloadLog(filename) {
            window.location.href = '/' + filename;
        }
        
        function showToast(msg) {
            const toast = document.createElement('div');
            toast.innerText = msg;
            toast.style.cssText = `
                position: fixed;
                bottom: 100px;
                left: 50%;
                transform: translateX(-50%);
                background: #32D74B;
                color: white;
                padding: 12px 24px;
                border-radius: 8px;
                z-index: 9999;
                font-weight: 600;
                box-shadow: 0 4px 12px rgba(0,0,0,0.3);
            `;
            document.body.appendChild(toast);
            
            setTimeout(() => {
                toast.style.opacity = '0';
                toast.style.transition = 'opacity 0.3s';
                setTimeout(() => toast.remove(), 300);
            }, 2000);
        }
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGE_H
