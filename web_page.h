#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <meta name="theme-color" content="#0a0a0a">
    <title>GaitOS V2.0</title>
    
    <!-- Chart.js: loaded dynamically to avoid blocking page on AP-only networks -->
    <script>
        // Non-blocking CDN loader — page works even if CDN is unreachable
        (function() {
            function loadScript(url, cb) {
                var s = document.createElement('script');
                s.src = url; s.async = true;
                s.onload = function() { cb && cb(true); };
                s.onerror = function() { console.warn('CDN unreachable: ' + url); cb && cb(false); };
                document.head.appendChild(s);
            }
            loadScript('https://cdn.jsdelivr.net/npm/chart.js@4.4.0', function(ok) {
                if (ok) loadScript('https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@2.0.1');
            });
        })();
    </script>
    
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
            padding-bottom: 80px; /* matches action-bar height (~56px) + 20px gap */
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
            opacity: 0.8;
            font-size: 14px;
            position: relative;
        }

        .metric-info:hover::after {
            content: attr(title);
            position: absolute;
            right: 0;
            top: 22px;
            background: #333;
            color: var(--text);
            font-size: 11px;
            padding: 6px 10px;
            border-radius: 6px;
            white-space: nowrap;
            z-index: 50;
            border: 1px solid var(--card-border);
            max-width: 220px;
            white-space: normal;
            line-height: 1.4;
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
            .grid-2 { grid-template-columns: 1fr; }
            .grid-3 { grid-template-columns: 1fr; }
            .grid-4 { grid-template-columns: repeat(2, 1fr); }
            body { padding: 12px; padding-bottom: 80px; }
            .header h1 { font-size: 22px; }
            .metric-value { font-size: 28px; }
            .action-bar { padding: 12px 16px; }
        }

        /* Toggle Switch */
        .toggle-row {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px 0;
            border-bottom: 1px solid rgba(255,255,255,0.06);
        }
        .toggle-row:last-child { border-bottom: none; }
        .toggle-label {
            font-size: 13px;
            color: var(--text);
        }
        .toggle-desc {
            font-size: 11px;
            color: var(--text-muted);
            margin-top: 2px;
        }
        .toggle {
            position: relative;
            width: 44px;
            height: 24px;
            flex-shrink: 0;
        }
        .toggle input {
            opacity: 0;
            width: 0;
            height: 0;
        }
        .toggle .slider {
            position: absolute;
            cursor: pointer;
            inset: 0;
            background: rgba(255,255,255,0.15);
            border-radius: 24px;
            transition: 0.25s;
        }
        .toggle .slider::before {
            content: '';
            position: absolute;
            height: 18px;
            width: 18px;
            left: 3px;
            bottom: 3px;
            background: white;
            border-radius: 50%;
            transition: 0.25s;
        }
        .toggle input:checked + .slider {
            background: var(--success);
        }
        .toggle input:checked + .slider::before {
            transform: translateX(20px);
        }

        /* Live alert indicator */
        .alert-dot {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            display: inline-block;
            margin-right: 6px;
        }
        .alert-dot.green { background: var(--success); }
        .alert-dot.red { background: var(--danger); animation: pulse 0.6s infinite; }
        .alert-dot.off { background: rgba(255,255,255,0.2); }

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
            background: linear-gradient(135deg, var(--danger), #FF6B5E);
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

        /* Traffic Light Indicator */
        .traffic-light {
            display: flex;
            align-items: center;
            gap: 14px;
            padding: 14px 18px;
            border-radius: 14px;
            background: var(--card-bg);
            border: 1px solid var(--card-border);
            transition: all 0.4s ease;
        }
        .traffic-light.zone-ok { border-color: var(--success); }
        .traffic-light.zone-warning { border-color: var(--warning); }
        .traffic-light.zone-critical { border-color: var(--danger); box-shadow: 0 0 20px rgba(255,69,58,0.3); }

        .traffic-dot {
            width: 28px; height: 28px; border-radius: 50%;
            background: rgba(255,255,255,0.15);
            transition: all 0.3s;
            flex-shrink: 0;
        }
        .traffic-dot.green { background: var(--success); box-shadow: 0 0 12px rgba(50,215,75,0.5); }
        .traffic-dot.amber { background: var(--warning); box-shadow: 0 0 12px rgba(255,159,10,0.5); animation: pulse 1s infinite; }
        .traffic-dot.red { background: var(--danger); box-shadow: 0 0 16px rgba(255,69,58,0.6); animation: pulse 0.5s infinite; }

        .traffic-text { flex: 1; }
        .traffic-title { font-size: 14px; font-weight: 700; }
        .traffic-reason { font-size: 12px; color: var(--text-muted); margin-top: 2px; }

        /* Bottom Tab Navigation */
        .bottom-tabs {
            position: fixed; bottom: 0; left: 0; right: 0;
            background: rgba(10, 10, 10, 0.97);
            backdrop-filter: blur(20px);
            border-top: 1px solid var(--card-border);
            display: flex;
            z-index: 200;
            padding-bottom: env(safe-area-inset-bottom, 0);
        }
        .tab-btn {
            flex: 1; display: flex; flex-direction: column; align-items: center;
            gap: 3px; padding: 8px 4px 6px;
            background: none; border: none; color: var(--text-muted);
            font-size: 10px; font-family: var(--font); cursor: pointer;
            transition: color 0.2s;
        }
        .tab-btn.active { color: var(--accent); }
        .tab-btn .tab-icon { font-size: 20px; line-height: 1; }

        /* Tab Sections */
        .tab-section { display: none; }
        .tab-section.active { display: flex; flex-direction: column; gap: 20px; }

        /* Floating Record Button */
        .record-fab {
            position: fixed; bottom: 70px; right: 20px;
            width: 56px; height: 56px;
            border-radius: 50%; border: none;
            background: var(--accent); color: white;
            font-size: 24px; cursor: pointer;
            box-shadow: 0 4px 16px rgba(10,132,255,0.4);
            z-index: 201; display: flex; align-items: center; justify-content: center;
            transition: all 0.3s;
        }
        .record-fab.recording { background: var(--danger); box-shadow: 0 4px 16px rgba(255,69,58,0.4); animation: pulse 1.5s infinite; }

        /* Web Audio Mute Toggle */
        .mute-btn {
            background: none; border: 1px solid var(--card-border);
            color: var(--text-muted); padding: 4px 10px; border-radius: 6px;
            font-size: 12px; cursor: pointer; font-family: var(--font);
        }
        .mute-btn.muted { color: var(--danger); border-color: var(--danger); }

        /* Storage Bar */
        .storage-bar-container {
            background: rgba(255,255,255,0.05); border-radius: 6px;
            padding: 10px 14px; margin-bottom: 12px;
        }
        .storage-bar {
            height: 6px; background: rgba(255,255,255,0.1);
            border-radius: 3px; overflow: hidden; margin-top: 6px;
        }
        .storage-bar-fill {
            height: 100%; border-radius: 3px;
            background: var(--accent); transition: width 0.5s;
        }
        .storage-bar-fill.warn { background: var(--warning); }
        .storage-bar-fill.crit { background: var(--danger); }

        /* Metric card glow on alert */
        .metric-card.glow-warning { box-shadow: 0 0 12px rgba(255,159,10,0.3); }
        .metric-card.glow-critical { box-shadow: 0 0 16px rgba(255,69,58,0.4); }

        /* Mobile improvements */
        @media (max-width: 768px) {
            .grid-2 { grid-template-columns: 1fr; }
            .grid-3 { grid-template-columns: 1fr; }
            .grid-4 { grid-template-columns: repeat(2, 1fr); }
            body { padding: 12px; padding-bottom: 120px; }
            .header h1 { font-size: 22px; }
            .metric-value { font-size: 28px; }
            .bottom-tabs { padding-bottom: env(safe-area-inset-bottom, 8px); }
            .record-fab { bottom: 74px; right: 16px; }
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <div class="header">
            <h1>GaitOS V2.0</h1>
            <div style="display:flex; gap:8px; align-items:center;">
                <button id="mute-web-audio" class="mute-btn" onclick="toggleWebMute()" title="Mute/unmute browser beeps">🔊</button>
                <div class="status-badge">
                    <div class="live-dot"></div>
                    <span id="status-text">CONNECTED</span>
                </div>
            </div>
        </div>

        <!-- Traffic Light Indicator -->
        <div id="traffic-light" class="traffic-light zone-ok">
            <div id="traffic-dot" class="traffic-dot"></div>
            <div class="traffic-text">
                <div id="traffic-title" class="traffic-title">Waiting for data...</div>
                <div id="traffic-reason" class="traffic-reason">Calibrate device to begin</div>
            </div>
        </div>

        <!-- ============ TAB: LIVE ============ -->
        <div id="section-live" class="tab-section active">

        <!-- Primary Metrics -->
        <div class="grid-2">
            <div class="metric-card" data-metric="stability">
                <div class="metric-header">
                    <span class="metric-label">Stability Index</span>
                    <span class="metric-info" title="Gait rhythmicity - 100% is perfect consistency. >85%=excellent, 50-85%=monitor, <50%=high fall risk">ⓘ</span>
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
                    <span class="metric-info" title="Steps per minute. 90-130=OK zone, 70-89/131-150=warning, <70/>150=critical">ⓘ</span>
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
                <div class="metric-label">Gait Phase</div>
                <div style="font-size:16px; font-weight:700; margin:12px 0;" id="val-phase">STANCE</div>
            </div>
        </div>

        </div><!-- /section-live -->

        <!-- ============ TAB: CHARTS ============ -->
        <div id="section-charts" class="tab-section">

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

        </div><!-- /section-charts -->

        <!-- ============ TAB: ALERTS ============ -->
        <div id="section-alerts" class="tab-section">

        <div class="card">
            <div class="collapsible-header" onclick="toggleSection('alerts')">
                <div class="metric-label"><span id="alert-indicator" class="alert-dot off"></span>Patient Alerts</div>
                <div class="metric-label" style="color:var(--accent)">▼</div>
            </div>
            <div id="alerts-panel" class="collapsible-content">
                <div class="toggle-row">
                    <div>
                        <div class="toggle-label">Beep Alerts</div>
                        <div class="toggle-desc">Audio warning when out of range</div>
                    </div>
                    <label class="toggle">
                        <input type="checkbox" id="tog-beep" checked onchange="updateAlerts()">
                        <span class="slider"></span>
                    </label>
                </div>
                <div class="toggle-row">
                    <div>
                        <div class="toggle-label">LED Indicator</div>
                        <div class="toggle-desc">OK=1Hz pulse, Warn=2Hz, Critical=4Hz blink</div>
                    </div>
                    <label class="toggle">
                        <input type="checkbox" id="tog-led" checked onchange="updateAlerts()">
                        <span class="slider"></span>
                    </label>
                </div>
                <div class="toggle-row">
                    <div>
                        <div class="toggle-label">Range Monitoring</div>
                        <div class="toggle-desc">Continuous gait range checking</div>
                    </div>
                    <label class="toggle">
                        <input type="checkbox" id="tog-range" checked onchange="updateAlerts()">
                        <span class="slider"></span>
                    </label>
                </div>
                <div style="margin-top:12px; padding-top:12px; border-top:1px solid rgba(255,255,255,0.1);">
                    <div class="metric-label" style="margin-bottom:10px;">Safe Ranges</div>
                    <div style="display:grid; grid-template-columns:1fr 1fr; gap:8px;">
                        <div class="form-group" style="margin-bottom:8px;">
                            <label style="font-size:11px;">Cadence Min (spm)</label>
                            <input type="number" id="alert-cad-min" value="70" min="30" max="120" style="padding:8px;">
                        </div>
                        <div class="form-group" style="margin-bottom:8px;">
                            <label style="font-size:11px;">Cadence Max (spm)</label>
                            <input type="number" id="alert-cad-max" value="150" min="80" max="200" style="padding:8px;">
                        </div>
                        <div class="form-group" style="margin-bottom:8px;">
                            <label style="font-size:11px;">Min Stability (%)</label>
                            <input type="number" id="alert-stab-min" value="50" min="10" max="90" style="padding:8px;">
                        </div>
                        <div class="form-group" style="margin-bottom:8px;">
                            <label style="font-size:11px;">Min Clearance (cm)</label>
                            <input type="number" id="alert-clear-min" value="2" min="0" max="10" step="0.5" style="padding:8px;">
                        </div>
                    </div>
                    <button class="btn btn-primary" onclick="saveAlertConfig()" style="width:100%; margin-top:4px;">Apply Alert Settings</button>
                </div>
                <div id="alert-status" style="margin-top:10px; padding:8px; border-radius:8px; font-size:12px; display:none;"></div>
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
                    <label>Min Step Duration (ms) — valid range: 150–600</label>
                    <input type="number" id="cfg-step-time" value="280" step="10" min="150" max="600" placeholder="280">
                </div>
                <div class="form-group">
                    <label>ZUPT Acceleration Threshold (g) — fraction of gravity, 0.05–1.0</label>
                    <input type="number" id="cfg-zupt-acc" value="0.25" step="0.05" min="0.05" max="1.0" placeholder="0.25">
                </div>
                <button class="btn btn-primary" onclick="saveConfig()" style="width:100%;">Apply Settings</button>
            </div>
        </div>

        </div><!-- /section-alerts -->

        <!-- ============ TAB: DATA ============ -->
        <div id="section-data" class="tab-section">

        <!-- Storage Usage -->
        <div class="storage-bar-container">
            <div style="display:flex; justify-content:space-between; font-size:12px;">
                <span>Storage</span>
                <span id="storage-text">Loading...</span>
            </div>
            <div class="storage-bar">
                <div id="storage-fill" class="storage-bar-fill" style="width:0%"></div>
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
                <div style="display:flex; gap:8px;" onclick="event.stopPropagation()">
                    <button class="btn btn-glass" style="padding:4px 10px; font-size:11px; color:var(--warning); border-color:var(--warning);" onclick="formatStorage()">Format</button>
                    <button class="btn btn-danger"  style="padding:4px 10px; font-size:11px;" onclick="deleteAllLogs()">Delete All</button>
                    <button class="btn btn-glass"   style="padding:4px 10px; font-size:11px;" onclick="fetchLogs()">Refresh</button>
                </div>
            </div>
            <div id="logs-panel" class="collapsible-content">
                <div id="log-list"></div>
            </div>
        </div><!-- /session logs card -->

        <!-- PHASE 4: Session Comparison (inside container for max-width constraint) -->
        <div class="card">
            <div class="metric-label" style="margin-bottom: 15px;">Session Comparison</div>
            <div style="display:flex; gap:10px; margin-bottom:15px;">
                <select id="compareSession1" style="flex:1; padding:8px; border-radius:8px; border:1px solid var(--card-border); background:var(--card-bg); color:var(--text); font-family:var(--font);">
                    <option value="">Select Session 1...</option>
                </select>
                <select id="compareSession2" style="flex:1; padding:8px; border-radius:8px; border:1px solid var(--card-border); background:var(--card-bg); color:var(--text); font-family:var(--font);">
                    <option value="">Select Session 2...</option>
                </select>
            </div>
            <button class="btn btn-primary" onclick="compareSessions()" style="width:100%; margin-bottom:15px;">
                Compare Trajectories
            </button>
            <div id="comparisonResults" style="display:none;">
                <div class="chart-wrapper" style="height:250px; margin-bottom:15px;">
                    <canvas id="comparisonChart"></canvas>
                </div>
                <div id="comparisonStats"></div>
            </div>
        </div><!-- /session comparison -->

        </div><!-- /section-data -->

    </div><!-- /container -->

    <!-- Bottom Tab Navigation -->
    <nav class="bottom-tabs">
        <button class="tab-btn active" data-tab="live" onclick="switchTab('live')">
            <span class="tab-icon">📊</span>Live
        </button>
        <button class="tab-btn" data-tab="charts" onclick="switchTab('charts')">
            <span class="tab-icon">📈</span>Charts
        </button>
        <button class="tab-btn" data-tab="alerts" onclick="switchTab('alerts')">
            <span class="tab-icon">⚠️</span>Alerts
        </button>
        <button class="tab-btn" data-tab="data" onclick="switchTab('data')">
            <span class="tab-icon">💾</span>Data
        </button>
    </nav>

    <!-- Floating Record Button -->
    <button id="record-fab" class="record-fab" onclick="toggleRecord()" title="Start / Stop Recording">⏺</button>

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
        let recordingActionPending = false; // lock to prevent sync() overriding in-flight actions
        let alertsInitialized = false;
        let webAudioMuted = false;
        let lastWebBeepTime = 0;

        // Web Audio for browser beeps
        let audioCtx = null;
        function getAudioCtx() {
            if (!audioCtx) audioCtx = new (window.AudioContext || window.webkitAudioContext)();
            return audioCtx;
        }

        // --- Tab Routing ---
        let chartsInitialized = false;

        function switchTab(tabName) {
            ['live','charts','alerts','data'].forEach(t => {
                const section = document.getElementById('section-' + t);
                const btn = document.querySelector('.tab-btn[data-tab=\"' + t + '\"]');
                if (section) section.classList.toggle('active', t === tabName);
                if (btn) btn.classList.toggle('active', t === tabName);
            });
            if (location.hash !== '#' + tabName) {
                history.pushState(null, '', '#' + tabName);
            }
            // Lazy-init charts on first visit (Chart.js needs visible canvas)
            if (tabName === 'charts' && !chartsInitialized) {
                setTimeout(function() { initCharts(); chartsInitialized = true; }, 50);
            }
            // Resize charts when re-visiting charts tab
            if (tabName === 'charts' && chartsInitialized) {
                if (typeof trajectoryChart !== 'undefined' && trajectoryChart) trajectoryChart.resize();
                if (typeof cadenceChart !== 'undefined' && cadenceChart) cadenceChart.resize();
            }
            // Fetch storage when switching to data tab
            if (tabName === 'data') fetchStorageUsage();
        }

        window.addEventListener('hashchange', () => {
            const hash = location.hash.replace('#', '') || 'live';
            switchTab(hash);
        });

        // Init from hash
        if (location.hash) switchTab(location.hash.replace('#', ''));

        // --- Traffic Light Updater ---
        function updateTrafficLight(zone, reason) {
            const el = document.getElementById('traffic-light');
            const dot = document.getElementById('traffic-dot');
            const title = document.getElementById('traffic-title');
            const reasonEl = document.getElementById('traffic-reason');

            el.className = 'traffic-light zone-' + zone;
            dot.className = 'traffic-dot ' + (zone === 'ok' ? 'green' : zone === 'warning' ? 'amber' : 'red');
            
            if (zone === 'ok') {
                title.textContent = 'All metrics in range';
                reasonEl.textContent = 'Patient gait is normal';
            } else if (zone === 'warning') {
                title.textContent = '⚠ Warning';
                reasonEl.textContent = reason || 'Borderline metric detected';
            } else {
                title.textContent = '🔴 Critical Alert';
                reasonEl.textContent = reason || 'Out of safe range';
            }
        }

        // --- Web Audio Beep ---
        function playWebBeep(freq, duration) {
            if (webAudioMuted) return;
            if (Date.now() - lastWebBeepTime < 2000) return; // Cooldown
            lastWebBeepTime = Date.now();
            try {
                const ctx = getAudioCtx();
                const osc = ctx.createOscillator();
                const gain = ctx.createGain();
                osc.connect(gain);
                gain.connect(ctx.destination);
                osc.frequency.value = freq || 800;
                gain.gain.value = 0.3;
                osc.start();
                osc.stop(ctx.currentTime + (duration || 200) / 1000);
            } catch(e) { /* Web Audio not supported */ }
        }

        function toggleWebMute() {
            webAudioMuted = !webAudioMuted;
            const btn = document.getElementById('mute-web-audio');
            btn.textContent = webAudioMuted ? '🔇' : '🔊';
            btn.classList.toggle('muted', webAudioMuted);
        }

        // --- Storage Usage ---
        async function fetchStorageUsage() {
            try {
                const res = await fetch('/api/storage');
                const d = await res.json();
                const pct = d.percent || 0;
                const usedKB = (d.used / 1024).toFixed(0);
                const totalKB = (d.total / 1024).toFixed(0);
                document.getElementById('storage-text').textContent = usedKB + ' / ' + totalKB + ' KB (' + pct + '%)';
                const fill = document.getElementById('storage-fill');
                fill.style.width = pct + '%';
                fill.className = 'storage-bar-fill' + (pct > 90 ? ' crit' : pct > 70 ? ' warn' : '');
            } catch(e) {}
        }

        // --- Record FAB ---
        function updateRecordFab() {
            const fab = document.getElementById('record-fab');
            if (fab) {
                fab.textContent = recording ? '⏹' : '⏺';
                fab.classList.toggle('recording', recording);
                fab.title = recording ? 'Stop Recording' : 'Start Recording';
            }
        }

        // Metric history for statistics and trends (per session — reset on each recording)
        const metricHistory = {
            stability: [],
            cadence: [],
            clearance: []
        };

        function resetSession() {
            metricHistory.stability = [];
            metricHistory.cadence = [];
            metricHistory.clearance = [];
            // Reset stat displays
            ['stab', 'cad'].forEach(k => {
                ['min','max','avg'].forEach(s => {
                    const el = document.getElementById(s + '-' + k);
                    if (el) el.innerText = '--';
                });
                const t = document.getElementById('trend-' + k);
                if (t) t.innerText = '';
            });
            // Clear live charts
            if (trajectoryChart) {
                trajectoryChart.data.datasets[0].data = [];
                trajectoryChart.update('none');
            }
            if (cadenceChart) {
                cadenceChart.data.labels = [];
                cadenceChart.data.datasets[0].data = [];
                cadenceChart.update('none');
            }
        }

        // Chart.js instances
        let trajectoryChart, cadenceChart;

        // Initialize charts on page load
        window.onload = function() {
            // Charts initialized lazily on first tab visit (needs visible canvas)
            fetchLogs();
            fetchStorageUsage();
            setInterval(sync, 500); // 500ms: fast enough for live display, won't flood device
        };

        function initCharts() {
            // Guard: Chart.js may not be available on AP-only networks (no CDN)
            if (typeof Chart === 'undefined') {
                console.warn('Chart.js not loaded — charts unavailable in AP mode');
                return;
            }
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
                
                // Update charts (guarded — charts are lazy-initialized)
                if (trajectoryChart) {
                    trajectoryChart.data.datasets[0].data.push({x: d.px, y: d.pz});
                    if (trajectoryChart.data.datasets[0].data.length > 300) {
                        trajectoryChart.data.datasets[0].data.shift();
                    }
                    trajectoryChart.update('none');
                }
                
                if (cadenceChart) {
                    cadenceChart.data.labels.push('');
                    cadenceChart.data.datasets[0].data.push(d.cad);
                    if (cadenceChart.data.datasets[0].data.length > 60) {
                        cadenceChart.data.labels.shift();
                        cadenceChart.data.datasets[0].data.shift();
                    }
                    cadenceChart.update('none');
                }
                
                // Sync recording state from device — skip if a local action is in-flight
                if (!recordingActionPending && d.recording !== recording) {
                    recording = d.recording;
                    updateRecordButton();
                    updateRecordFab();
                }

                // Update alert indicator
                const alertDot = document.getElementById('alert-indicator');
                const alertStatus = document.getElementById('alert-status');

                // Traffic light + zone updates
                const zone = d.zone || 'ok';
                const reason = d.range_reason || d.abnormal_reason || '';
                updateTrafficLight(zone, reason);

                if (d.range_alert || d.abnormal) {
                    alertDot.className = 'alert-dot red';
                    if (alertStatus) {
                        alertStatus.style.display = 'block';
                        alertStatus.style.background = 'rgba(255,69,58,0.15)';
                        alertStatus.style.color = 'var(--danger)';
                        alertStatus.innerText = 'OUT OF RANGE: ' + (reason || 'Alert');
                    }
                    // Web audio beep on critical
                    if (zone === 'critical') {
                        playWebBeep(800, 200);
                        // Mobile vibration
                        if (navigator.vibrate) navigator.vibrate([100, 50, 100]);
                    }
                } else if (d.calibrated && d.step_count > 0) {
                    alertDot.className = 'alert-dot green';
                    if (alertStatus) {
                        alertStatus.style.display = 'block';
                        alertStatus.style.background = 'rgba(50,215,75,0.1)';
                        alertStatus.style.color = 'var(--success)';
                        alertStatus.innerText = 'All metrics in range';
                    }
                } else {
                    alertDot.className = 'alert-dot off';
                    if (alertStatus) alertStatus.style.display = 'none';
                }

                // Metric card glow based on zone
                document.querySelectorAll('.metric-card').forEach(card => {
                    card.classList.remove('glow-warning', 'glow-critical');
                    if (zone === 'critical') card.classList.add('glow-critical');
                    else if (zone === 'warning') card.classList.add('glow-warning');
                });

                // Sync toggle states from device (initial load)
                if (!alertsInitialized) {
                    document.getElementById('tog-beep').checked = d.beep_on;
                    document.getElementById('tog-led').checked = d.led_on;
                    document.getElementById('tog-range').checked = d.range_on;
                    alertsInitialized = true;
                }

                // Update connection badge (connected)
                const badge = document.querySelector('.status-badge');
                const dot   = document.querySelector('.live-dot');
                document.getElementById('status-text').innerText = 'CONNECTED';
                badge.style.background = 'rgba(50, 215, 75, 0.15)';
                dot.style.background   = 'var(--success)';
            } catch (err) {
                document.getElementById('status-text').innerText = 'DISCONNECTED';
                const badge = document.querySelector('.status-badge');
                const dot   = document.querySelector('.live-dot');
                badge.style.background = 'rgba(255, 69, 58, 0.15)';
                dot.style.background   = 'var(--danger)';
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
            try {
                await fetch('/api/' + endpoint, { method: 'POST' });
            } catch (err) {
                console.error('API error (' + endpoint + '):', err);
                showToast('✗ Device not reachable');
            }
        }

        // Recording with session management
        function toggleRecord() {
            if (!recording) {
                document.getElementById('sessionModal').style.display = 'flex';
            } else {
                recordingActionPending = true;
                api('record/stop').then(() => {
                    recording = false;
                    recordingActionPending = false;
                    updateRecordButton();
                    updateRecordFab();
                    fetchLogs();
                    fetchStorageUsage();
                }).catch(err => {
                    console.error('Stop recording error:', err);
                    recordingActionPending = false;
                    showToast('✗ Failed to stop recording');
                });
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

            try {
                recordingActionPending = true;
                await fetch('/api/record/start', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify(metadata)
                });

                recording = true;
                recordingActionPending = false;
                updateRecordButton();
                updateRecordFab();
                closeSessionModal();
                resetSession(); // clear charts and history for the new session
                document.getElementById('sessionForm').reset();
            } catch (err) {
                console.error('Start session error:', err);
                recordingActionPending = false;
                showToast('✗ Failed to start recording');
            }
        }

        function generateSessionName() {
            const d = new Date();
            return `Session_${d.getFullYear()}${(d.getMonth()+1).toString().padStart(2,'0')}${d.getDate().toString().padStart(2,'0')}_${d.getHours().toString().padStart(2,'0')}${d.getMinutes().toString().padStart(2,'0')}`;
        }

        function updateRecordButton() {
            const btn = document.getElementById('btn-toggle');
            if (!btn) return; // btn-toggle may not exist with new bottom tab layout
            if (recording) {
                btn.innerText = 'Stop Recording';
                btn.className = 'btn btn-danger';
            } else {
                btn.innerText = 'Start Recording';
                btn.className = 'btn btn-primary';
            }
            updateRecordFab();
        }

        // Configuration
        async function saveConfig() {
            const config = {
                step_time: parseFloat(document.getElementById('cfg-step-time').value),
                zupt_acc: parseFloat(document.getElementById('cfg-zupt-acc').value)
            };

            try {
                const res = await fetch('/api/config', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify(config)
                });
                if (res.ok) {
                    showToast('✓ Settings applied to device');
                } else {
                    showToast('✗ Failed to apply settings');
                }
            } catch (err) {
                showToast('✗ Device not reachable');
            }
        }

        // --- Alert Controls ---
        async function updateAlerts() {
            try {
                await fetch('/api/alerts', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        beep: document.getElementById('tog-beep').checked,
                        led: document.getElementById('tog-led').checked,
                        range: document.getElementById('tog-range').checked
                    })
                });
            } catch (err) {
                console.error('Alert toggle error:', err);
                showToast('✗ Failed to update alerts');
            }
        }

        async function saveAlertConfig() {
            try {
                const res = await fetch('/api/alerts', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/json'},
                    body: JSON.stringify({
                        beep: document.getElementById('tog-beep').checked,
                        led: document.getElementById('tog-led').checked,
                        range: document.getElementById('tog-range').checked,
                        cad_min: parseFloat(document.getElementById('alert-cad-min').value),
                        cad_max: parseFloat(document.getElementById('alert-cad-max').value),
                        stab_min: parseFloat(document.getElementById('alert-stab-min').value),
                        clear_min: parseFloat(document.getElementById('alert-clear-min').value) / 100 // cm to meters
                    })
                });
                if (res.ok) {
                    showToast('✓ Alert settings applied');
                } else {
                    showToast('✗ Failed to apply alert settings');
                }
            } catch (err) {
                console.error('Save alert config error:', err);
                showToast('✗ Device not reachable');
            }
        }

        // Logs
        async function fetchLogs() {
            let res, logs;
            try {
                res = await fetch('/api/logs');
                logs = await res.json();
            } catch (err) {
                console.error('fetchLogs error:', err);
                return;
            }

            const listEl = document.getElementById('log-list');
            if (logs.length === 0) {
                listEl.innerHTML = '<p style="color: var(--text-muted); font-size: 13px;">No recordings yet</p>';
            } else {
                listEl.innerHTML = logs.map(log => `
                    <div class="log-item${selectedLog === log.name ? ' selected' : ''}" onclick="selectLog('${log.name}')">
                        <div>
                            <div style="font-size: 13px; font-weight: 600;">${log.name}</div>
                            <div style="font-size: 11px; color: var(--text-muted);">${(log.size / 1024).toFixed(1)} KB</div>
                        </div>
                        <div style="display:flex; gap:6px;">
                            <button class="btn btn-danger" style="padding: 6px 12px; font-size: 11px;" onclick="event.stopPropagation(); deleteLog('${log.name}')">Delete</button>
                            <button class="btn btn-glass" style="padding: 6px 12px; font-size: 11px;" onclick="event.stopPropagation(); downloadLog('${log.name}')">Download</button>
                        </div>
                    </div>
                `).join('');
            }

            // Keep comparison selectors in sync with log list
            populateSessionSelectors(logs);
        }

        function selectLog(name) {
            selectedLog = name;
            fetchLogs();
        }

        // downloadLog is defined below with proper fetch+blob handling

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

            try {
                const path = selectedLog.startsWith('/') ? selectedLog : '/' + selectedLog;
                const response = await fetch(path);
                if (!response.ok) throw new Error('Failed to fetch session');
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
            } catch (err) {
                console.error('Export JSON error:', err);
                showToast('✗ Export failed');
            }
        }

        async function exportSummary() {
            if (!selectedLog) {
                alert('Please select a session');
                return;
            }

            try {
            const path2 = selectedLog.startsWith('/') ? selectedLog : '/' + selectedLog;
            const response = await fetch(path2);
            if (!response.ok) throw new Error('Failed to fetch session');
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
            } catch (err) {
                console.error('Export summary error:', err);
                showToast('✗ Export failed');
            }
        }

        function calculateSessionStats(csvText) {
            const lines = csvText.split('\n').filter(l => l && !l.startsWith('#'));
            if (lines.length < 2) return { duration: 0, totalSteps: 0, avgCadence: 0, avgStability: 0, totalDistance: 0 };

            const data = lines.slice(1).map(l => l.split(','));

            // Extract columns (V2.0 format: t,ax,ay,az,gx,gy,gz,q0,q1,q2,q3,roll,pitch,yaw,vx,vy,vz,px,py,pz,phase,cadence,stability,abnormal)
            const cadences    = data.map(r => parseFloat(r[21])).filter(v => isFinite(v) && v > 0);
            const stabilities = data.map(r => parseFloat(r[22])).filter(v => isFinite(v));
            const positions   = data.map(r => parseFloat(r[17])).filter(v => isFinite(v));
            const stepCounts  = data.map(r => parseFloat(r[20])).filter(v => isFinite(v));

            return {
                duration:      (data.length / 100).toFixed(1),
                totalSteps:    stepCounts.length  > 0 ? Math.round(Math.max(...stepCounts))  : 0,
                avgCadence:    cadences.length    > 0 ? cadences.reduce((a,b) => a+b, 0) / cadences.length : 0,
                avgStability:  stabilities.length > 0 ? stabilities.reduce((a,b) => a+b, 0) / stabilities.length : 0,
                totalDistance: positions.length   > 0 ? Math.max(...positions) : 0
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
                // Strip leading slash if present to avoid encoding issues
                let cleanName = filename.startsWith('/') ? filename.substring(1) : filename;
                const res = await fetch('/api/delete/' + encodeURIComponent(cleanName), {
                    method: 'DELETE'
                });
                
                if (res.ok) {
                    showToast('✓ File deleted successfully');
                    await fetchLogs(); // Refresh the file list
                } else {
                    const errText = await res.text();
                    showToast('✗ ' + errText);
                }
            } catch (err) {
                console.error('Delete error:', err);
                showToast('✗ Error deleting file');
            }
        }
        
        async function downloadLog(filename) {
            try {
                showToast('⏳ Downloading...');
                // Normalize path - avoid double slashes
                const path = filename.startsWith('/') ? filename : '/' + filename;
                const res = await fetch(path);
                if (!res.ok) throw new Error('Download failed');
                const blob = await res.blob();
                const url = URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.href = url;
                // Use clean filename without leading slash
                a.download = filename.startsWith('/') ? filename.substring(1) : filename;
                document.body.appendChild(a);
                a.click();
                document.body.removeChild(a);
                URL.revokeObjectURL(url);
                showToast('✓ Downloaded: ' + a.download);
            } catch (err) {
                console.error('Download error:', err);
                showToast('✗ Download failed');
            }
        }
        
        async function deleteAllLogs() {
            if (!confirm('Delete ALL session files?\n\nThis cannot be undone!')) {
                return;
            }
            
            try {
                showToast('⏳ Deleting all files...');
                const res = await fetch('/api/deleteall', { method: 'POST' });
                const text = await res.text();
                
                if (res.ok) {
                    showToast('✓ ' + text);
                } else {
                    showToast('✗ ' + text);
                }
                await fetchLogs();
            } catch (err) {
                console.error('Delete all error:', err);
                showToast('✗ Error deleting files');
            }
        }
        
        async function formatStorage() {
            if (!confirm('FORMAT STORAGE?\n\nThis will PERMANENTLY delete ALL files including non-CSV files!\n\nContinue?')) {
                return;
            }
            
            try {
                showToast('⏳ Formatting storage...');
                const res = await fetch('/api/format', { method: 'POST' });
                const text = await res.text();
                
                if (res.ok) {
                    showToast('✓ ' + text);
                } else {
                    showToast('✗ ' + text);
                }
                await fetchLogs();
            } catch (err) {
                console.error('Format error:', err);
                showToast('✗ Error formatting storage');
            }
        }
        
        let activeToast = null;
        let activeToastTimer = null;
        function showToast(msg) {
            // Remove previous toast immediately
            if (activeToast) {
                clearTimeout(activeToastTimer);
                activeToast.remove();
                activeToast = null;
            }

            // Pick color by message prefix
            let bg = '#32D74B'; // success green
            if (msg.startsWith('✗'))  bg = '#FF453A'; // error red
            if (msg.startsWith('⏳')) bg = '#0A84FF'; // in-progress blue

            const toast = document.createElement('div');
            toast.innerText = msg;
            toast.style.cssText = `
                position: fixed;
                bottom: 80px;
                left: 50%;
                transform: translateX(-50%);
                background: ${bg};
                color: white;
                padding: 12px 24px;
                border-radius: 8px;
                z-index: 9999;
                font-weight: 600;
                box-shadow: 0 4px 12px rgba(0,0,0,0.3);
                max-width: 90vw;
                text-align: center;
                word-break: break-word;
            `;
            document.body.appendChild(toast);
            activeToast = toast;

            activeToastTimer = setTimeout(() => {
                toast.style.opacity = '0';
                toast.style.transition = 'opacity 0.3s';
                setTimeout(() => {
                    toast.remove();
                    if (activeToast === toast) activeToast = null;
                }, 300);
            }, 2500);
        }
        
        // PHASE 4: Session Comparison Functions
        // Called with the already-fetched logs array from fetchLogs()
        function populateSessionSelectors(logs) {
            const opts1 = document.getElementById('compareSession1');
            const opts2 = document.getElementById('compareSession2');

            opts1.innerHTML = '<option value="">Select Session 1...</option>';
            opts2.innerHTML = '<option value="">Select Session 2...</option>';

            logs.forEach(log => {
                const name = log.name; // log is {name, size} — use .name not the object itself
                opts1.innerHTML += `<option value="${name}">${name}</option>`;
                opts2.innerHTML += `<option value="${name}">${name}</option>`;
            });
        }
        
        async function loadCSV(filename) {
            // Normalize path — avoid double-slash if filename already starts with /
            const path = filename.startsWith('/') ? filename : '/' + filename;
            const response = await fetch(path);
            if (!response.ok) throw new Error(`Failed to load ${filename}: ${response.status}`);
            const csvText = await response.text();

            // split('\n') — a single newline character, not the two-char literal \n
            const lines = csvText.split('\n').filter(l => l && !l.startsWith('#'));
            if (lines.length < 2) return [];

            const headers = lines[0].split(',').map(h => h.trim());
            const data = [];

            for (let i = 1; i < lines.length; i++) {
                if (!lines[i].trim()) continue;
                const values = lines[i].split(',');
                const row = {};
                headers.forEach((h, idx) => {
                    const val = values[idx];
                    row[h] = (val !== undefined && !isNaN(val)) ? parseFloat(val) : val;
                });
                data.push(row);
            }

            return data;
        }
        
        let comparisonChartInstance = null;
        
        async function compareSessions() {
            const s1 = document.getElementById('compareSession1').value;
            const s2 = document.getElementById('compareSession2').value;
            
            if (!s1 || !s2) {
                alert('Please select two sessions to compare');
                return;
            }
            
            if (s1 === s2) {
                alert('Please select two different sessions');
                return;
            }
            
            try {
                const data1 = await loadCSV(s1);
                const data2 = await loadCSV(s2);
                
                // Extract trajectories (px vs pz)
                const traj1 = data1.map(d => ({x: d.px || 0, y: d.pz || 0}));
                const traj2 = data2.map(d => ({x: d.px || 0, y: d.pz || 0}));
                
                // Show results
                document.getElementById('comparisonResults').style.display = 'block';
                
                // Destroy old chart if exists
                if (comparisonChartInstance) {
                    comparisonChartInstance.destroy();
                }
                
                // Create overlay chart
                const ctx = document.getElementById('comparisonChart').getContext('2d');
                comparisonChartInstance = new Chart(ctx, {
                    type: 'scatter',
                    data: {
                        datasets: [
                            {
                                label: s1,
                                data: traj1,
                                borderColor: '#00D9FF',
                                backgroundColor: 'transparent',
                                showLine: true,
                                pointRadius: 1,
                                borderWidth: 2
                            },
                            {
                                label: s2,
                                data: traj2,
                                borderColor: '#FF9500',
                                backgroundColor: 'transparent',
                                showLine: true,
                                pointRadius: 1,
                                borderWidth: 2
                            }
                        ]
                    },
                    options: {
                        responsive: true,
                        maintainAspectRatio: false,
                        scales: {
                            x: {
                                type: 'linear',
                                position: 'bottom',
                                title: {
                                    display: true,
                                    text: 'Forward (m)',
                                    color: '#999'
                                },
                                grid: { color: 'rgba(255,255,255,0.1)' },
                                ticks: { color: '#999' }
                            },
                            y: {
                                title: {
                                    display: true,
                                    text: 'Height / Clearance (m)',
                                    color: '#999'
                                },
                                grid: { color: 'rgba(255,255,255,0.1)' },
                                ticks: { color: '#888' }
                            }
                        },
                        plugins: {
                            legend: {
                                labels: { color: '#fff' }
                            }
                        }
                    }
                });
                
                // Calculate and show statistics comparison
                showComparisonStats(data1, data2, s1, s2);
                
                showToast('✓ Sessions compared successfully');
                
            } catch (err) {
                console.error('Comparison error:', err);
                alert('Error loading sessions: ' + err.message);
            }
        }
        
        function showComparisonStats(d1, d2, name1, name2) {
            const avg = (arr) => arr.reduce((a, b) => a + b, 0) / arr.length;
            
            const cadence1 = avg(d1.map(r => r.cadence || 0).filter(v => v > 0));
            const cadence2 = avg(d2.map(r => r.cadence || 0).filter(v => v > 0));
            
            const stab1 = avg(d1.map(r => r.stability || 0).filter(v => v > 0));
            const stab2 = avg(d2.map(r => r.stability || 0).filter(v => v > 0));
            
            const dist1 = Math.max(...d1.map(r => r.dist_m || 0));
            const dist2 = Math.max(...d2.map(r => r.dist_m || 0));
            
            const html = `
                <table style="width:100%; border-collapse: collapse; color:#fff;">
                    <tr style="border-bottom:1px solid var(--border);">
                        <th style="padding:8px; text-align:left; color:#888;">Metric</th>
                        <th style="padding:8px; text-align:right;">${name1.substring(0, 20)}</th>
                        <th style="padding:8px; text-align:right;">${name2.substring(0, 20)}</th>
                        <th style="padding:8px; text-align:right;">Δ</th>
                    </tr>
                    <tr style="border-bottom:1px solid var(--border);">
                        <td style="padding:8px;">Avg Cadence (spm)</td>
                        <td style="padding:8px; text-align:right;">${cadence1.toFixed(1)}</td>
                        <td style="padding:8px; text-align:right;">${cadence2.toFixed(1)}</td>
                        <td style="padding:8px; text-align:right; color:${cadence2 > cadence1 ? '#32D74B' : '#FF453A'};">
                            ${(cadence2 - cadence1).toFixed(1)}
                        </td>
                    </tr>
                    <tr style="border-bottom:1px solid var(--border);">
                        <td style="padding:8px;">Avg Stability (%)</td>
                        <td style="padding:8px; text-align:right;">${stab1.toFixed(1)}</td>
                        <td style="padding:8px; text-align:right;">${stab2.toFixed(1)}</td>
                        <td style="padding:8px; text-align:right; color:${stab2 > stab1 ? '#32D74B' : '#FF453A'};">
                            ${(stab2 - stab1).toFixed(1)}
                        </td>
                    </tr>
                    <tr>
                        <td style="padding:8px;">Distance (m)</td>
                        <td style="padding:8px; text-align:right;">${dist1.toFixed(2)}</td>
                        <td style="padding:8px; text-align:right;">${dist2.toFixed(2)}</td>
                        <td style="padding:8px; text-align:right; color:#888;">
                            ${(dist2 - dist1).toFixed(2)}
                        </td>
                    </tr>
                </table>
            `;
            
            document.getElementById('comparisonStats').innerHTML = html;
        }
        
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_PAGE_H
