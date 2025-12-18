#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>GaitOS V11</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        :root {
            --bg: #000000;
            --card: #1c1c1e; /* Apple Dark Gray */
            --text: #f5f5f7;
            --text-sub: #86868b;
            --accent: #0a84ff;   /* iOS Blue */
            --danger: #ff453a;   /* iOS Red */
            --struct: #32d74b;   /* iOS Green */
            --shadow: 0 4px 20px rgba(0,0,0,0.5);
            --font: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        }

        body { font-family: var(--font); margin: 0; padding: 20px; background: var(--bg); color: var(--text); -webkit-font-smoothing: antialiased; }
        .container { max-width: 1000px; margin: 0 auto; display: flex; flex-direction: column; gap: 20px; }

        /* HERO SECTION */
        .hero { background: var(--card); border-radius: 18px; padding: 20px; box-shadow: var(--shadow); position: relative; overflow: hidden; height: 380px; }
        .hero-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px; }
        .hero h2 { margin: 0; font-size: 1.2rem; font-weight: 600; color: var(--text-sub); letter-spacing: 0.5px; text-transform: uppercase; }
        
        .status-pill { background: #2c2c2e; padding: 4px 12px; border-radius: 20px; font-size: 0.8rem; font-weight: 600; display: flex; align-items: center; gap: 6px; }
        .dot { width: 8px; height: 8px; border-radius: 50%; background: var(--text-sub); transition: 0.3s; }
        .dot.active { background: var(--danger); box-shadow: 0 0 8px var(--danger); animation: pulse 2s infinite; }
        
        @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }

        /* METRICS STRIP */
        .metrics { display: grid; grid-template-columns: repeat(4, 1fr); gap: 15px; }
        @media(max-width: 600px) { .metrics { grid-template-columns: 1fr 1fr; } }
        
        .metric { background: var(--card); border-radius: 18px; padding: 15px; text-align: center; box-shadow: var(--shadow); }
        .metric-label { font-size: 0.75rem; color: var(--text-sub); text-transform: uppercase; font-weight: 600; margin-bottom: 4px; }
        .metric-val { font-size: 1.8rem; font-weight: 700; letter-spacing: -0.5px; }
        .metric-unit { font-size: 0.9rem; color: var(--text-sub); font-weight: 500; }

        /* CONTROLS */
        .controls { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 15px; }
        button { 
            background: var(--card); color: var(--text); border: none; padding: 16px; 
            border-radius: 14px; font-size: 1rem; font-weight: 600; cursor: pointer; 
            transition: all 0.2s; box-shadow: var(--shadow);
        }
        button:active { transform: scale(0.98); }
        .btn-main { background: var(--accent); color: white; }
        .btn-stop { background: var(--danger); color: white; opacity: 0.5; pointer-events: none; }
        .btn-stop.active { opacity: 1; pointer-events: auto; }

        /* LOGS */
        .logs-box { background: var(--card); border-radius: 18px; padding: 20px; box-shadow: var(--shadow); min-height: 150px; }
        .logs-header { display: flex; justify-content: space-between; margin-bottom: 15px; font-weight: 600; color: var(--text-sub); }
        .log-item { display: flex; justify-content: space-between; padding: 12px 0; border-bottom: 1px solid #2c2c2e; font-size: 0.9rem; align-items: center; }
        .log-item:last-child { border: none; }
        .log-actions a { color: var(--accent); text-decoration: none; margin-left: 15px; font-weight: 500; }
        
    </style>
</head>
<body>

<div class="container">
    <div class="hero">
        <div class="hero-header">
            <h2>Gait Trajectory (Z vs X)</h2>
            <div class="status-pill"><div id="rec-dot" class="dot"></div> <span id="status-text">IDLE</span></div>
        </div>
        <canvas id="chart-main"></canvas>
    </div>

    <div class="metrics">
        <div class="metric">
            <div class="metric-label">Steps</div>
            <div class="metric-val" id="m-steps">0</div>
        </div>
        <div class="metric">
            <div class="metric-label">Distance</div>
            <div class="metric-val"><span id="m-dist">0.0</span> <span class="metric-unit">m</span></div>
        </div>
        <div class="metric">
            <div class="metric-label">Clearance</div>
            <div class="metric-val" style="color:var(--struct)"><span id="m-clear">0.0</span> <span class="metric-unit">cm</span></div>
        </div>
        <div class="metric">
            <div class="metric-label">Pitch</div>
            <div class="metric-val" style="color:var(--accent)"><span id="m-pitch">0</span><span class="metric-unit">°</span></div>
        </div>
    </div>

    <div class="controls">
        <button id="btn-rec" class="btn-main" onclick="record(true)">Record</button>
        <button id="btn-stop" class="btn-stop" onclick="record(false)">Stop</button>
        <button onclick="api('calibrate')">Zero Sensors</button>
    </div>

    <!-- Hidden Logs, toggle visibility? No, just list last 3 -->
    <div class="logs-box">
        <div class="logs-header">
            <span>RECENT SESSIONS</span>
            <span style="font-size:0.8rem; cursor:pointer;" onclick="fetchLogs()">REFRESH</span>
        </div>
        <div id="log-list"></div>
    </div>
</div>

<script>
    // --- APP LOGIC ---
    let recording = false;
    const MAX_PTS = 200;

    // Chart Setup (Minimalist)
    Chart.defaults.font.family = '-apple-system';
    Chart.defaults.color = '#86868b';
    const ctx = document.getElementById('chart-main').getContext('2d');
    const chart = new Chart(ctx, {
        type: 'scatter',
        data: { 
            datasets: [{ 
                data: [], 
                borderColor: '#0a84ff', 
                backgroundColor: 'rgba(10, 132, 255, 0.1)', // Fill
                borderWidth: 3,
                pointRadius: 0,
                showLine: true,
                fill: true,
                tension: 0.4 // Organic curves (Catmull-Rom)
            }] 
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false,
            plugins: { legend: { display: false } },
            scales: {
                x: { grid: { color: '#2c2c2e' }, title: { display: false } }, 
                y: { grid: { color: '#2c2c2e' }, min: 0, max: 0.5 }
            }
        }
    });

    // API Wrapper
    async function api(endpoint) { await fetch('/api/'+endpoint, {method:'POST'}); sync(); }
    
    async function record(start) {
        if(start) await api('record/start');
        else { await api('record/stop'); fetchLogs(); }
    }

    async function sync() {
        try {
            const r = await fetch('/api/status');
            const d = await r.json();
            
            // State
            recording = d.recording;
            document.getElementById('status-text').innerText = recording ? "RECORDING" : "IDLE";
            document.getElementById('rec-dot').className = recording ? "dot active" : "dot";
            
            document.getElementById('btn-rec').style.display = recording ? 'none' : 'block';
            document.getElementById('btn-rec').disabled = recording; // Just hide/disable logic
            
            const stopBtn = document.getElementById('btn-stop');
            if(recording) { stopBtn.classList.add('active'); stopBtn.classList.add('btn-danger'); }
            else { stopBtn.classList.remove('active'); stopBtn.classList.remove('btn-danger'); }

            // Metrics
            document.getElementById('m-steps').innerText = d.step_count;
            document.getElementById('m-dist').innerText = d.dist_m.toFixed(1);
            document.getElementById('m-clear').innerText = (d.pz * 100).toFixed(1);
            document.getElementById('m-pitch').innerText = d.pitch.toFixed(0);

            // Chart (Organic Push)
            chart.data.datasets[0].data.push({x: d.px, y: d.pz});
            if(chart.data.datasets[0].data.length > MAX_PTS) chart.data.datasets[0].data.shift();
            chart.update('none');

        } catch(e) {}
    }

    async function fetchLogs() {
        const el = document.getElementById('log-list');
        el.innerHTML = '<div style="padding:10px; color:#555">Loading...</div>';
        try {
            const r = await fetch('/api/logs');
            const files = await r.json();
            el.innerHTML = '';
            files.slice(0, 5).forEach(f => {
                el.innerHTML += `
                    <div class="log-item">
                        <span>${f.name.substring(1)}</span>
                        <div class="log-actions">
                            <span style="color:#666">${(f.size/1024).toFixed(1)} KB</span>
                            <a href="/logs${f.name}" download>DL</a>
                        </div>
                    </div>`;
            });
            if(files.length === 0) el.innerHTML = '<div style="padding:10px">No recordings yet.</div>';
        } catch(e) {}
    }

    setInterval(sync, 100); // 10Hz Sync
    fetchLogs();

</script>
</body>
</html>
)rawliteral";

#endif
