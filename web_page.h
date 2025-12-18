#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>GaitOS V13</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        :root {
            --bg: #000000;
            --card: #1c1c1e;
            --text: #f5f5f7;
            --text-sub: #86868b;
            --accent: #0a84ff;
            --danger: #ff453a;
            --struct: #32d74b;
            --warn: #ff9f0a;
            --shadow: 0 4px 20px rgba(0,0,0,0.5);
            --font: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        }

        body { font-family: var(--font); margin: 0; padding: 15px; background: var(--bg); color: var(--text); -webkit-font-smoothing: antialiased; }
        .container { max-width: 1000px; margin: 0 auto; display: flex; flex-direction: column; gap: 15px; }

        /* HERO SECTION */
        .hero { background: var(--card); border-radius: 18px; padding: 15px; box-shadow: var(--shadow); position: relative; height: 320px; display:flex; flex-direction:column;}
        .hero-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 5px; height: 30px;}
        .hero h2 { margin: 0; font-size: 1rem; font-weight: 600; color: var(--text-sub); text-transform: uppercase; letter-spacing: 0.5px; }
        
        .status-pill { background: #2c2c2e; padding: 4px 10px; border-radius: 20px; font-size: 0.75rem; font-weight: 600; display: flex; align-items: center; gap: 6px; }
        .dot { width: 8px; height: 8px; border-radius: 50%; background: var(--text-sub); transition: 0.3s; }
        .dot.active { background: var(--danger); box-shadow: 0 0 8px var(--danger); animation: pulse 2s infinite; }
        
        @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }

        /* METRICS STRIP */
        .metrics { display: grid; grid-template-columns: repeat(4, 1fr); gap: 10px; }
        @media(max-width: 600px) { .metrics { grid-template-columns: 1fr 1fr; } }
        
        .metric { background: var(--card); border-radius: 18px; padding: 15px; text-align: center; box-shadow: var(--shadow); }
        .metric-label { font-size: 0.7rem; color: var(--text-sub); text-transform: uppercase; font-weight: 600; margin-bottom: 4px; }
        .metric-val { font-size: 1.6rem; font-weight: 700; letter-spacing: -0.5px; }
        .metric-unit { font-size: 0.8rem; color: var(--text-sub); font-weight: 500; }

        /* CONTROLS */
        .controls { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 10px; }
        button { 
            background: var(--card); color: var(--text); border: none; padding: 14px; 
            border-radius: 14px; font-size: 0.95rem; font-weight: 600; cursor: pointer; 
            transition: all 0.2s; box-shadow: var(--shadow);
        }
        button:active { transform: scale(0.98); }
        .btn-main { background: var(--accent); color: white; }
        .btn-stop { background: var(--danger); color: white; opacity: 0.5; pointer-events: none; }
        .btn-stop.active { opacity: 1; pointer-events: auto; }

        /* LOGS */
        .logs-box { background: var(--card); border-radius: 18px; padding: 15px; box-shadow: var(--shadow); min-height: 100px; }
        .logs-header { display: flex; justify-content: space-between; margin-bottom: 10px; font-size: 0.8rem; font-weight: 600; color: var(--text-sub); }
        .log-item { display: flex; justify-content: space-between; padding: 10px 0; border-bottom: 1px solid #2c2c2e; font-size: 0.85rem; align-items: center; }
        .log-item:last-child { border: none; }
        .log-actions a { color: var(--accent); text-decoration: none; margin-left: 15px; font-weight: 500; }
        
    </style>
</head>
<body>

<div class="container">
    <!-- Row 1: Hero Trajectory -->
    <div class="hero">
        <div class="hero-header">
            <h2>Step Trajectory (Z vs X)</h2>
            <div class="status-pill"><div id="rec-dot" class="dot"></div> <span id="status-text">IDLE</span></div>
        </div>
        <div style="flex:1; position:relative; width:100%;">
            <canvas id="chart-main"></canvas>
        </div>
    </div>

    <!-- Row 2: Primary Metrics -->
    <div class="metrics">
        <div class="metric">
            <div class="metric-label">Cadence</div>
            <div class="metric-val" style="color:var(--accent)"><span id="m-cad">0</span> <span class="metric-unit">SPM</span></div>
        </div>
        <div class="metric">
            <div class="metric-label">Stability</div>
            <div class="metric-val" style="color:var(--struct)"><span id="m-stab">100</span><span class="metric-unit">%</span></div>
        </div>
        <div class="metric">
            <div class="metric-label">Distance</div>
            <div class="metric-val"><span id="m-dist">0.0</span> <span class="metric-unit">m</span></div>
        </div>
        <div class="metric">
            <div class="metric-label">Steps</div>
            <div class="metric-val"><span id="m-steps">0</span></div>
        </div>
    </div>

    <!-- Row 3: Secondary Metrics (Pitch / Clearance) -->
    <div class="metrics">
         <div class="metric">
            <div class="metric-label">Clearance</div>
            <div class="metric-val"><span id="m-clear">0.0</span> <span class="metric-unit">cm</span></div>
        </div>
        <div class="metric">
            <div class="metric-label">Pitch</div>
            <div class="metric-val"><span id="m-pitch">0</span><span class="metric-unit">°</span></div>
        </div>
         <div class="metric">
            <div class="metric-label">Phase</div>
            <div class="metric-val" id="m-phase" style="font-size:1.2rem; margin-top:5px;">STANCE</div>
        </div>
         <div class="metric">
            <div class="metric-label">Status</div>
            <div class="metric-val" id="m-move" style="font-size:1.2rem; margin-top:5px; color:var(--warn)">STAT</div>
        </div>
    </div>

    <div class="controls">
        <button id="btn-rec" class="btn-main" onclick="record(true)">Start Recording</button>
        <button id="btn-stop" class="btn-stop" onclick="record(false)">Stop</button>
        <button onclick="api('calibrate')">Zero Sensors</button>
    </div>

    <div class="logs-box">
        <div class="logs-header">
            <span>DATA RECORDINGS</span>
            <span style="font-size:0.8rem; cursor:pointer;" onclick="fetchLogs()">REFRESH LIST</span>
        </div>
        <div id="log-list"></div>
    </div>
</div>

<script>
    // --- V13 LOW LATENCY LOGIC ---
    let recording = false;
    const MAX_PTS = 150; // Reduced for performance

    // Chart Setup
    Chart.defaults.font.family = '-apple-system';
    Chart.defaults.color = '#86868b';
    const ctx = document.getElementById('chart-main').getContext('2d');
    const chart = new Chart(ctx, {
        type: 'scatter',
        data: { 
            datasets: [{ 
                data: [], 
                borderColor: '#0a84ff', 
                backgroundColor: 'rgba(10, 132, 255, 0.15)', 
                borderWidth: 3,
                pointRadius: 0,
                showLine: true,
                fill: true,
                tension: 0.4
            }] 
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            animation: false, // CRITICAL FOR LATENCY
            plugins: { legend: { display: false } },
            scales: {
                x: { grid: { color: '#2c2c2e' }, ticks:{display:false} }, 
                y: { grid: { color: '#2c2c2e' }, min: 0, max: 0.4 } // Fixed scale for stability
            }
        }
    });

    async function api(endpoint) { await fetch('/api/'+endpoint, {method:'POST'}); sync(); }
    
    async function record(start) {
        if(start) await api('record/start');
        else { await api('record/stop'); setTimeout(fetchLogs, 500); }
    }

    async function sync() {
        try {
            // Tight Polling Loop
            const r = await fetch('/api/status');
            const d = await r.json();
            
            // 1. Controls State
            recording = d.recording;
            document.getElementById('status-text').innerText = recording ? "RECORDING" : "IDLE";
            document.getElementById('rec-dot').className = recording ? "dot active" : "dot";
            
            document.getElementById('btn-rec').style.display = recording ? 'none' : 'block';
            const stopBtn = document.getElementById('btn-stop');
            if(recording) { stopBtn.classList.add('active'); stopBtn.classList.add('btn-danger'); }
            else { stopBtn.classList.remove('active'); stopBtn.classList.remove('btn-danger'); }

            // 2. Metrics (DOM updates are fast)
            document.getElementById('m-steps').innerText = d.step_count;
            document.getElementById('m-dist').innerText = d.dist_m.toFixed(1);
            document.getElementById('m-clear').innerText = (d.pz * 100).toFixed(1);
            document.getElementById('m-pitch').innerText = d.pitch.toFixed(0);
            document.getElementById('m-cad').innerText = d.cad.toFixed(0); // NEW
            document.getElementById('m-stab').innerText = d.stab.toFixed(0); // NEW
            
            // Color logic for Stability
            const stabEl = document.getElementById('m-stab').parentElement;
            if(d.stab > 80) stabEl.style.color = 'var(--struct)';
            else if(d.stab > 50) stabEl.style.color = 'var(--warn)';
            else stabEl.style.color = 'var(--danger)';

            document.getElementById('m-phase').innerText = d.phase ? "SWING" : "STANCE";
            document.getElementById('m-move').innerText = d.is_stat ? "STAT" : "MOVE";
            
            // 3. Chart (Only update if changed to save cycles?)
            // Actually, for trajectory, always push.
            chart.data.datasets[0].data.push({x: d.px, y: d.pz});
            if(chart.data.datasets[0].data.length > MAX_PTS) chart.data.datasets[0].data.shift();
            chart.update('none'); // 'none' mode = No Animation = Instant

        } catch(e) {}
    }

    async function fetchLogs() {
        const el = document.getElementById('log-list');
        el.innerHTML = '<div style="padding:10px; color:#555">Loading...</div>';
        try {
            const r = await fetch('/api/logs');
            const files = await r.json();
            el.innerHTML = '';
            // Show newest first
            files.reverse().slice(0, 5).forEach(f => {
                el.innerHTML += `
                    <div class="log-item">
                        <span>${f.name.substring(1)}</span>
                        <div class="log-actions">
                            <span style="color:#666">${(f.size/1024).toFixed(1)} KB</span>
                            <a href="${f.name}" download>DL</a>
                        </div>
                    </div>`;
            });
            if(files.length === 0) el.innerHTML = '<div style="padding:10px">No recordings yet.</div>';
        } catch(e) {}
    }

    setInterval(sync, 100); // 10Hz High Speed Sync
    fetchLogs();

</script>
</body>
</html>
)rawliteral";

#endif
