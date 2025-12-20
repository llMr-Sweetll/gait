#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
    <title>GaitOS Pro</title>
    <style>
        :root {
            --bg-grad: radial-gradient(circle at top left, #1a2a6c, #b21f1f, #fdbb2d);
            --bg-solid: #000;
            --glass: rgba(255, 255, 255, 0.08);
            --glass-border: rgba(255, 255, 255, 0.1);
            --text-main: #ffffff;
            --text-muted: rgba(255, 255, 255, 0.6);
            --accent: #0A84FF;
            --success: #32D74B;
            --warn: #FF9F0A;
            --danger: #FF453A;
            --font-stack: -apple-system, BlinkMacSystemFont, "SF Pro Text", "Segoe UI", Roboto, Helvetica, sans-serif;
        }

        body {
            font-family: var(--font-stack);
            background: #000;
            color: var(--text-main);
            margin: 0;
            padding: 20px;
            padding-bottom: 80px; /* Space for fab */
            -webkit-font-smoothing: antialiased;
        }

        /* Container */
        .app-container {
            max-width: 600px;
            margin: 0 auto;
            display: flex;
            flex-direction: column;
            gap: 24px;
        }

        /* Typography */
        h1, h2, h3 { margin: 0; font-weight: 600; letter-spacing: -0.5px; }
        .label { font-size: 11px; text-transform: uppercase; letter-spacing: 0.8px; color: var(--text-muted); font-weight: 600; }
        .value { font-size: 28px; font-weight: 700; letter-spacing: -1px; }
        .unit { font-size: 14px; font-weight: 500; color: var(--text-muted); margin-left: 2px; }

        /* Components */
        .card {
            background: var(--glass);
            backdrop-filter: blur(20px);
            -webkit-backdrop-filter: blur(20px);
            border: 1px solid var(--glass-border);
            border-radius: 24px;
            padding: 20px;
            position: relative;
            overflow: hidden;
            box-shadow: 0 8px 32px rgba(0,0,0,0.3);
        }

        /* Header */
        .header { display: flex; justify-content: space-between; align-items: center; }
        .status-badge {
            font-size: 12px; font-weight: 600; padding: 6px 12px; border-radius: 20px;
            background: rgba(255,255,255,0.1); color: var(--text-muted);
            display: flex; align-items: center; gap: 6px;
        }
        .live-dot { width: 8px; height: 8px; border-radius: 50%; background: var(--text-muted); }
        .live-dot.recording { background: var(--danger); box-shadow: 0 0 10px var(--danger); animation: pulse 2s infinite; }
        
        @keyframes pulse { 0% { opacity: 1; } 50% { opacity: 0.5; } 100% { opacity: 1; } }

        /* Chart */
        .chart-container { height: 260px; width: 100%; position: relative; }
        canvas { width: 100%; height: 100%; display: block; }
        
        /* Grid */
        .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; }
        .grid-4 { display: grid; grid-template-columns: 1fr 1fr 1fr 1fr; gap: 12px; }

        /* Buttons */
        .btn {
            width: 100%; padding: 16px; border: none; border-radius: 18px;
            font-size: 16px; font-weight: 600; cursor: pointer;
            transition: transform 0.1s;
        }
        .btn:active { transform: scale(0.96); }
        .btn-primary { background: var(--accent); color: white; }
        .btn-danger { background: var(--danger); color: white; }
        .btn-glass { background: rgba(255,255,255,0.1); color: white; }
        
        /* Interactive Controls Area */
        .action-bar {
            position: fixed; bottom: 20px; left: 50%; transform: translateX(-50%);
            width: 90%; max-width: 580px;
            background: rgba(28, 28, 30, 0.9); backdrop-filter: blur(20px);
            padding: 10px; border-radius: 24px; border: 1px solid rgba(255,255,255,0.1);
            display: flex; gap: 10px; z-index: 100;
            box-shadow: 0 10px 40px rgba(0,0,0,0.5);
        }

        /* Logs List */
        .log-item {
            display: flex; justify-content: space-between; align-items: center;
            padding: 12px 0; border-bottom: 1px solid rgba(255,255,255,0.1);
        }
        .log-item:last-child { border: none; }
        .log-link { color: var(--accent); text-decoration: none; font-weight: 600; font-size: 14px; }
    </style>
</head>
<body>

<div class="app-container">
    
    <!-- Title Area -->
    <div class="header">
        <h1>GaitOS Pro</h1>
        <div class="status-badge">
            <div id="live-dot" class="live-dot"></div>
            <span id="status-text">DISCONNECTED</span>
        </div>
    </div>

    <!-- Main Vis -->
    <div class="card" style="padding: 0;">
        <div style="padding: 20px 20px 0 20px;">
            <div class="label">Real-Time Trajectory (Side View)</div>
        </div>
        <div class="chart-container">
            <canvas id="main-canvas"></canvas>
        </div>
        <div style="position: absolute; bottom: 15px; left: 20px; font-size: 12px; color: var(--text-muted);">
            Z-Height (cm) vs Step Progress
        </div>
    </div>

    <!-- Primary Metrics -->
    <div class="grid-2">
        <div class="card">
            <div class="label">Stability Index</div>
            <div class="value" id="val-stab" style="color: var(--success)">--</div>
            <div class="unit">Healthy Rhythm</div>
        </div>
        <div class="card">
            <div class="label">Cadence</div>
            <div class="value" id="val-cad">--</div>
            <div class="unit">Steps / Min</div>
        </div>
    </div>

    <!-- Secondary Metrics -->
    <div class="grid-4">
        <div class="card" style="padding:12px; text-align:center;">
            <div class="label">Clearance</div>
            <div style="font-size:18px; font-weight:700; margin-top:4px;" id="val-clear">0.0</div>
            <div class="unit">cm</div>
        </div>
        <div class="card" style="padding:12px; text-align:center;">
            <div class="label">HFC</div>
            <div style="font-size:18px; font-weight:700; margin-top:4px;" id="val-hfc">0</div>
            <div class="unit">idx</div>
        </div>
        <div class="card" style="padding:12px; text-align:center;">
            <div class="label">Dist</div>
            <div style="font-size:18px; font-weight:700; margin-top:4px;" id="val-dist">0.0</div>
            <div class="unit">m</div>
        </div>
        <div class="card" style="padding:12px; text-align:center;">
            <div class="label">Phase</div>
            <div style="font-size:14px; font-weight:700; margin-top:8px; display: block;" id="val-phase">STANCE</div>
        </div>
    </div>

    <!-- Logs -->
    <div class="card">
        <div class="header">
            <div class="label">Session History</div>
            <div class="label" style="cursor: pointer; color: var(--accent)" onclick="fetchLogs()">REFRESH</div>
        </div>
        <div id="log-list" style="margin-top: 10px; min-height: 50px;">
            <div style="font-size:13px; color:#555">Loading records...</div>
        </div>
    </div>
    
    <div style="height: 40px;"></div>
</div>

<!-- Floating Controls (Apple Style) -->
<div class="action-bar">
    <button class="btn btn-glass" onclick="api('calibrate')" style="flex: 1;">Zero Sensors</button>
    <button id="btn-toggle" class="btn btn-primary" onclick="toggleRecord()" style="flex: 2;">Start Recording</button>
</div>

<script>
    // --- APP LOGIC ---
    let recording = false;
    let trajectory = [];
    const MAX_PTS = 300; // Increased buffer
    
    // Auto-Scaling Canvas Logic
    const canvas = document.getElementById('main-canvas');
    const ctx = canvas.getContext('2d');
    
    function resize() {
        // High-DPI support
        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.parentElement.getBoundingClientRect();
        canvas.width = rect.width * dpr;
        canvas.height = rect.height * dpr;
        ctx.scale(dpr, dpr);
        canvas.style.width = `${rect.width}px`;
        canvas.style.height = `${rect.height}px`;
    }
    window.addEventListener('resize', resize);
    setTimeout(resize, 100); // Init
    
    function draw() {
        const w = canvas.parentElement.clientWidth;
        const h = canvas.parentElement.clientHeight;
        
        ctx.clearRect(0,0,w,h);
        
        // Dynamic Grid
        ctx.strokeStyle = "rgba(255,255,255,0.05)";
        ctx.lineWidth = 1;
        ctx.beginPath();
        ctx.moveTo(0, h*0.25); ctx.lineTo(w, h*0.25);
        ctx.moveTo(0, h*0.50); ctx.lineTo(w, h*0.50);
        ctx.moveTo(0, h*0.75); ctx.lineTo(w, h*0.75);
        ctx.stroke();
        
        if(trajectory.length < 2) return;
        
        // Auto-Scale Y Axis (Z-Height)
        // Find Max Z in buffer to keep graph centered
        let maxZ = 0.2; // Min 20cm range
        for(let p of trajectory) if(p.y > maxZ) maxZ = p.y;
        maxZ = maxZ * 1.2; // 20% headroom
        
        // Draw Line
        ctx.beginPath();
        const stepX = w / MAX_PTS; 
        
        // Draw from Right to Left (History)
        // Head is at index length-1
        for(let i=0; i<trajectory.length; i++) {
            let pt = trajectory[trajectory.length - 1 - i]; // Reverse iter
            
            let x = w - (i * stepX); // Latest at Right Edge
            let y = h - ((pt.y / maxZ) * h); // Scale Z to Height
            
            // Curve smoothing
            if(i===0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        }
        
        ctx.lineJoin = "round";
        ctx.lineWidth = 3;
        ctx.strokeStyle = getComputedStyle(document.documentElement).getPropertyValue('--accent');
        ctx.stroke();
        
        // Gradient Fill
        ctx.lineTo(0, h);
        ctx.lineTo(w, h);
        ctx.closePath();
        const grad = ctx.createLinearGradient(0, 0, 0, h);
        grad.addColorStop(0, "rgba(10, 132, 255, 0.2)");
        grad.addColorStop(1, "rgba(10, 132, 255, 0)");
        ctx.fillStyle = grad;
        ctx.fill();
    }

    // Sync Loop
    async function sync() {
        try {
            const res = await fetch('/api/status');
            const d = await res.json();
            
            // State
            recording = (d.recording == 1 || d.recording === true);
            document.getElementById('status-text').innerText = recording ? "RECORDING" : "READY";
            document.getElementById('status-text').style.color = recording ? "var(--danger)" : "var(--text-muted)";
            const dot = document.getElementById('live-dot');
            dot.className = recording ? "live-dot recording" : "live-dot";
            
            // Button Logic
            const btn = document.getElementById('btn-toggle');
            if(recording) {
                btn.innerText = "Stop Recording";
                btn.className = "btn btn-danger";
            } else {
                btn.innerText = "Start Recording";
                btn.className = "btn btn-primary";
            }
            
            // Metrics
            document.getElementById('val-cad').innerText = d.cad.toFixed(0);
            document.getElementById('val-dist').innerText = d.dist_m.toFixed(1);
            document.getElementById('val-clear').innerText = (d.pz * 100).toFixed(1);
            document.getElementById('val-hfc').innerText = d.hfc.toFixed(0);
            document.getElementById('val-phase').innerText = d.phase ? "SWING" : "STANCE";
            
            // Stability Color
            const stabVal = d.stab.toFixed(0);
            const stabEl = document.getElementById('val-stab');
            stabEl.innerText = stabVal + "%";
            if(d.stab > 80) stabEl.style.color = "var(--success)";
            else if(d.stab > 50) stabEl.style.color = "var(--warn)";
            else stabEl.style.color = "var(--danger)";
            
            // Trajectory Push
            trajectory.push({x: d.px, y: d.pz}); 
            // We ignore X for plotting, treating it as a time-strip chart of Z-height
            // This is cleaner for "Gait Verification"
            if(trajectory.length > MAX_PTS) trajectory.shift();
            
            draw();
            
        } catch(e) {
            document.getElementById('status-text').innerText = "CONNECTING...";
        }
    }
    
    async function api(ep) { await fetch('/api/' + ep, { method: 'POST' }); }
    function toggleRecord() {
        if(recording) { api('record/stop'); setTimeout(fetchLogs, 1000); }
        else api('record/start');
    }
    
    async function fetchLogs() {
        const el = document.getElementById('log-list');
        el.innerHTML = '<div style="padding:10px; font-size:13px; color:#666">Refreshing...</div>';
        try {
            const r = await fetch('/api/logs');
            const json = await r.json();
            el.innerHTML = "";
            if(json.length === 0) {
                 el.innerHTML = '<div style="padding:10px; font-size:13px; color:#666">No recordings found.</div>';
                 return;
            }
            // Reverse to show new first
            json.reverse().forEach(f => {
                const name = f.name.replace('/', '');
                const kb = (f.size / 1024).toFixed(1);
                el.innerHTML += `
                    <div class="log-item">
                        <div>
                            <div style="font-weight:600; font-size:14px;">${name}</div>
                            <div style="font-size:11px; color:var(--text-muted)">${kb} KB</div>
                        </div>
                        <a href="${name}" class="log-link" download>DOWNLOAD</a>
                    </div>
                `;
            });
        } catch(e) {
            el.innerHTML = '<div style="padding:10px; font-size:13px; color:#666">Error loading logs.</div>';
        }
    }
    
    // Init
    setInterval(sync, 100);
    fetchLogs(); // Load initially
</script>
</body>
</html>
)rawliteral";

#endif
