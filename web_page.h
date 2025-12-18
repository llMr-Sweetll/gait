#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>M5 Gait Lab V3</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        :root {
            --bg-color: #121212;
            --card-bg: #1e1e1e;
            --text-main: #e0e0e0;
            --text-muted: #a0a0a0;
            --accent: #00e5ff;
            --accent-hover: #00b8d4;
            --danger: #cf6679;
            --success: #03dac6;
            --font-stack: 'Inter', -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        }
        body {
            font-family: var(--font-stack);
            margin: 0;
            padding: 20px;
            background-color: var(--bg-color);
            color: var(--text-main);
            line-height: 1.6;
        }
        .container { max-width: 1000px; margin: 0 auto; }
        
        /* Header */
        header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 30px;
            padding-bottom: 20px;
            border-bottom: 1px solid #333;
        }
        h1 { margin: 0; font-weight: 300; letter-spacing: 1px; font-size: 1.8rem; }
        .header-info { text-align: right; font-size: 0.9rem; color: var(--text-muted); }
        .status-badge {
            display: inline-block;
            padding: 5px 12px;
            border-radius: 20px;
            font-size: 0.85rem;
            font-weight: 600;
            background: #333;
            color: #fff;
            margin-top: 5px;
        }
        .status-badge.recording { background: var(--danger); color: #000; }
        
        /* Controls */
        .controls {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
            gap: 15px;
            margin-bottom: 30px;
        }
        button {
            padding: 15px;
            border: none;
            border-radius: 8px;
            font-size: 1rem;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
            text-transform: uppercase;
            letter-spacing: 1px;
            color: #fff;
        }
        .btn-primary { background: var(--accent); color: #000; }
        .btn-primary:hover { background: var(--accent-hover); }
        .btn-danger { background: var(--danger); color: #000; }
        .btn-danger:hover { background: #b00020; }
        .btn-secondary { background: #333; }
        .btn-secondary:hover { background: #444; }
        .btn-sm { padding: 5px 10px; font-size: 0.8rem; margin-left: 10px; }
        button:disabled { opacity: 0.5; cursor: not-allowed; }

        /* Dashboard */
        .dashboard {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        .card {
            background: var(--card-bg);
            border-radius: 12px;
            padding: 20px;
            box-shadow: 0 4px 6px rgba(0,0,0,0.3);
            border: 1px solid #333;
            text-align: center;
        }
        .card h3 { margin: 0 0 10px; font-size: 0.85rem; color: var(--text-muted); text-transform: uppercase; }
        .metric-value { font-size: 2.5rem; font-weight: 700; color: var(--accent); }
        .metric-unit { font-size: 1rem; color: var(--text-muted); }

        /* Charts */
        .charts-container {
            display: grid;
            grid-template-columns: 1fr;
            gap: 20px;
            margin-bottom: 30px;
        }
        @media (min-width: 768px) { .charts-container { grid-template-columns: 1fr 1fr; } }
        .chart-wrapper {
            background: var(--card-bg);
            padding: 15px;
            border-radius: 12px;
            height: 300px;
            border: 1px solid #333;
        }

        /* Logs */
        .logs-section {
            background: var(--card-bg);
            padding: 20px;
            border-radius: 12px;
            border: 1px solid #333;
        }
        .log-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px; }
        .log-list { list-style: none; padding: 0; max-height: 300px; overflow-y: auto; }
        .log-item {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 12px;
            border-bottom: 1px solid #333;
            color: var(--text-muted);
        }
        .log-item:last-child { border-bottom: none; }
        .log-item a { color: var(--accent); text-decoration: none; font-weight: 500; }
        .log-item a:hover { text-decoration: underline; }
        .log-actions { display: flex; gap: 10px; align-items: center; }
    </style>
</head>
<body>
    <div class="container">
        <header>
            <h1>M5 Gait Lab V3</h1>
            <div class="header-info">
                <div>Bat: <span id="bat-level">--</span>%</div>
                <div id="status-badge" class="status-badge">Idle</div>
            </div>
        </header>

        <div class="controls">
            <button id="btn-start" class="btn-primary" onclick="startRecording()">Start Rec</button>
            <button id="btn-stop" class="btn-danger" onclick="stopRecording()" disabled>Stop Rec</button>
            <button class="btn-secondary" onclick="calibrate()">Calibrate</button>
            <button class="btn-secondary" onclick="fetchLogs()">Refresh Logs</button>
        </div>

        <div class="dashboard">
            <div class="card">
                <h3>Cadence</h3>
                <div><span class="metric-value" id="val-cad">0.0</span> <span class="metric-unit">spm</span></div>
            </div>
            <div class="card">
                <h3>Speed</h3>
                <div><span class="metric-value" id="val-spd">0.00</span> <span class="metric-unit">m/s</span></div>
            </div>
            <div class="card">
                <h3>Limp Index</h3>
                <div><span class="metric-value" id="val-limp">0.000</span> <span class="metric-unit">idx</span></div>
            </div>
            <div class="card">
                <h3>Steps</h3>
                <div><span class="metric-value" id="val-steps">0</span></div>
            </div>
        </div>

        <div class="charts-container">
            <div class="chart-wrapper"><canvas id="chart-pitch"></canvas></div>
            <div class="chart-wrapper"><canvas id="chart-accel"></canvas></div>
        </div>

        <div class="logs-section">
            <div class="log-header">
                <h3 style="margin:0; color:var(--text-muted);">Data Logs</h3>
                <button class="btn-danger btn-sm" onclick="formatStorage()">Format Storage</button>
            </div>
            <ul id="log-list" class="log-list"><li style="padding:10px;">Loading...</li></ul>
        </div>
    </div>

    <script>
        const MAX_POINTS = 100;
        let isRecording = false;

        // Chart Config
        Chart.defaults.color = '#a0a0a0';
        Chart.defaults.borderColor = '#333';
        const commonOptions = {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            interaction: { mode: 'none' },
            elements: { point: { radius: 0 }, line: { tension: 0.1 } },
            scales: { x: { display: false }, y: { grid: { color: '#333' } } }
        };

        const chartPitch = new Chart(document.getElementById('chart-pitch').getContext('2d'), {
            type: 'line',
            data: { labels: [], datasets: [{ label: 'Pitch (deg)', data: [], borderColor: '#00e5ff', borderWidth: 2, fill: true, backgroundColor: 'rgba(0, 229, 255, 0.1)' }] },
            options: { ...commonOptions, plugins: { title: { display: true, text: 'Pitch Angle' } } }
        });

        const chartAccel = new Chart(document.getElementById('chart-accel').getContext('2d'), {
            type: 'line',
            data: { labels: [], datasets: [{ label: 'Vertical Accel (g)', data: [], borderColor: '#cf6679', borderWidth: 2, fill: true, backgroundColor: 'rgba(207, 102, 121, 0.1)' }] },
            options: { ...commonOptions, plugins: { title: { display: true, text: 'Vertical Acceleration' } } }
        });

        function addData(chart, label, data) {
            chart.data.labels.push(label);
            chart.data.datasets.forEach((dataset) => { dataset.data.push(data); });
            if (chart.data.labels.length > MAX_POINTS) {
                chart.data.labels.shift();
                chart.data.datasets.forEach((dataset) => { dataset.data.shift(); });
            }
            chart.update();
        }

        async function updateStatus() {
            try {
                const res = await fetch('/api/status');
                const data = await res.json();
                
                isRecording = data.recording;
                updateRecordingUI();

                if(data.step_count !== undefined) document.getElementById('val-steps').innerText = data.step_count;
                if(data.cadence_spm !== undefined) document.getElementById('val-cad').innerText = data.cadence_spm.toFixed(1);
                if(data.speed_mps !== undefined) document.getElementById('val-spd').innerText = data.speed_mps.toFixed(2);
                if(data.limping_index !== undefined) document.getElementById('val-limp').innerText = data.limping_index.toFixed(3);
                if(data.battery !== undefined) document.getElementById('bat-level').innerText = data.battery;

                const now = new Date().toLocaleTimeString();
                if(data.pitch !== undefined) addData(chartPitch, now, data.pitch);
                if(data.az !== undefined) addData(chartAccel, now, data.az - 1.0);

            } catch (e) { console.error("Status fetch failed", e); }
        }

        function updateRecordingUI() {
            const badge = document.getElementById('status-badge');
            const btnStart = document.getElementById('btn-start');
            const btnStop = document.getElementById('btn-stop');
            if (isRecording) {
                badge.innerText = "Recording";
                badge.classList.add('recording');
                btnStart.disabled = true;
                btnStop.disabled = false;
            } else {
                badge.innerText = "Idle";
                badge.classList.remove('recording');
                btnStart.disabled = false;
                btnStop.disabled = true;
            }
        }

        async function startRecording() { try { await fetch('/api/record/start', { method: 'POST' }); updateStatus(); } catch(e) { alert("Error starting"); } }
        async function stopRecording() { try { await fetch('/api/record/stop', { method: 'POST' }); updateStatus(); fetchLogs(); } catch(e) { alert("Error stopping"); } }
        async function calibrate() { if(confirm("Calibrate? Stand still.")) { try { await fetch('/api/calibrate', { method: 'POST' }); alert("Done!"); } catch(e) { alert("Error"); } } }
        
        async function formatStorage() {
            if(confirm("WARNING: This will delete ALL logs. Continue?")) {
                try { 
                    await fetch('/api/format', { method: 'POST' }); 
                    alert("Formatted! Device may restart."); 
                    fetchLogs();
                } catch(e) { alert("Error formatting"); }
            }
        }

        async function deleteLog(filename) {
            if(confirm(`Delete ${filename}?`)) {
                try {
                    await fetch(`/api/delete?file=${filename}`, { method: 'POST' });
                    fetchLogs();
                } catch(e) { alert("Error deleting"); }
            }
        }

        async function fetchLogs() {
            const list = document.getElementById('log-list');
            list.innerHTML = '<li style="padding:10px;">Loading...</li>';
            try {
                const res = await fetch('/api/logs');
                const files = await res.json();
                list.innerHTML = '';
                if (files.length === 0) { list.innerHTML = '<li style="padding:10px;">No logs found.</li>'; return; }
                files.forEach(f => {
                    const li = document.createElement('li');
                    li.className = 'log-item';
                    const displayName = f.name.substring(1);
                    const sizeKB = (f.size / 1024).toFixed(1);
                    li.innerHTML = `
                        <div>
                            <a href="/logs${f.name}" download="${displayName}">${displayName}</a>
                            <span style="margin-left:10px; font-size:0.8rem;">${sizeKB} KB</span>
                        </div>
                        <div class="log-actions">
                            <button class="btn-danger btn-sm" onclick="deleteLog('${f.name}')">Del</button>
                        </div>
                    `;
                    list.appendChild(li);
                });
            } catch (e) { list.innerHTML = '<li style="padding:10px; color:var(--danger);">Error fetching logs.</li>'; }
        }

        setInterval(updateStatus, 100);
        fetchLogs();
    </script>
</body>
</html>
)rawliteral";

#endif
