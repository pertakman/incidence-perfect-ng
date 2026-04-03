#include "remote_control_page.h"

const char PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Incidence Perfect NG</title>
  <style>
    :root {
      --bg: #0b1020;
      --text: #eef4ff;
      --muted: #9fb3d9;
      --card: #141c34;
      --card-2: #101933;
      --border: #223056;
      --input-bg: #0d1733;
      --input-border: #31406f;
      --button-bg: #2a7fff;
      --button-text: #ffffff;
      --code: #8ec7ff;
      --bar-bg: #2e354f;
      --bar-fill: #6fd3ff;
      --pill-bg: #1c2748;
      --pill-text: #c9d8ff;
      --pill-ok-bg: #193a2a;
      --pill-ok-text: #8df1c7;
      --pill-warn-bg: #4a2d2d;
      --pill-warn-text: #ffd1d1;
      --metric-ok: #8df1c7;
      --metric-warn: #ffd782;
    }
    body[data-theme="day"] {
      --bg: #eef3fb;
      --text: #11203c;
      --muted: #516684;
      --card: #ffffff;
      --card-2: #f4f7fc;
      --border: #c6d3ea;
      --input-bg: #ffffff;
      --input-border: #9fb3d9;
      --button-bg: #245fd1;
      --button-text: #ffffff;
      --code: #1f58b8;
      --bar-bg: #dbe4f2;
      --bar-fill: #2f7de1;
      --pill-bg: #dde6f6;
      --pill-text: #32496d;
      --pill-ok-bg: #d8efe4;
      --pill-ok-text: #1d6b49;
      --pill-warn-bg: #f6dedc;
      --pill-warn-text: #8b433c;
      --metric-ok: #1f8c60;
      --metric-warn: #b06d00;
    }
    body { font-family: Arial, sans-serif; margin: 16px; background: var(--bg); color: var(--text); }
    h1 { margin: 0 0 8px 0; font-size: 24px; }
    .muted { color: var(--muted); margin-bottom: 12px; }
    .card { background: var(--card); border-radius: 10px; padding: 14px; margin-bottom: 12px; }
    .card summary { cursor: pointer; font-size: 18px; font-weight: 700; }
    .row { display: flex; gap: 10px; flex-wrap: wrap; }
    #metricsRow { justify-content: center; gap: 28px; }
    .metric-card { flex: 0 1 280px; min-width: 200px; max-width: 320px; text-align: center; }
    #metricsRow.single-axis { justify-content: center; }
    #metricsRow.single-axis > .metric-card { flex: 0 1 300px; max-width: 300px; }
    .slider-control { min-width: 220px; max-width: 420px; }
    .slider-control > .row { gap: 10px; align-items: center; flex-wrap: nowrap; }
    .slider-control input[type="range"] { flex: 1; min-width: 150px; }
    .slider-control code { flex: 0 0 auto; min-width: 42px; text-align: right; }
    .value { font-size: 36px; font-weight: bold; margin-top: 4px; transition: color .12s linear; }
    .value.warn { color: #FFBF00; }
    .value.crit { color: #DC143C; }
    button {
      border: 0; border-radius: 8px; padding: 12px 14px; min-width: 90px;
      background: var(--button-bg); color: var(--button-text); font-weight: 700; cursor: pointer;
    }
    button:disabled { opacity: .55; cursor: not-allowed; }
    button:active { transform: scale(0.98); }
    input, select {
      width: 100%; box-sizing: border-box;
      border: 1px solid var(--input-border); border-radius: 8px;
      background: var(--input-bg); color: var(--text); padding: 10px;
    }
    input[type="checkbox"] { width: auto; padding: 0; }
    input[type="range"] { accent-color: var(--button-bg); }
    label { display: block; }
    #msg { min-height: 18px; color: #8cf0c6; margin-top: 8px; }
    .hidden { display: none; }
    .bar { height: 8px; background: var(--bar-bg); border-radius: 99px; overflow: hidden; margin-top: 10px; }
    .bar > div { height: 100%; width: 0%; background: var(--bar-fill); transition: width .12s linear; }
    code { color: var(--code); }
    .diag-layout { margin-top: 10px; display: grid; gap: 10px; }
    .diag-pills { display: flex; gap: 8px; flex-wrap: wrap; }
    .pill {
      display: inline-flex; align-items: center; border-radius: 999px; padding: 4px 10px;
      font-size: 11px; font-weight: 700; letter-spacing: .03em; text-transform: uppercase;
      background: var(--pill-bg); color: var(--pill-text);
    }
    .pill.ok { background: var(--pill-ok-bg); color: var(--pill-ok-text); }
    .pill.warn { background: var(--pill-warn-bg); color: var(--pill-warn-text); }
    .diag-grid { display: grid; gap: 8px; grid-template-columns: 1fr; }
    @media (min-width: 640px) { .diag-grid { grid-template-columns: 1fr 1fr; } }
    .diag-block { background: var(--card-2); border: 1px solid var(--border); border-radius: 8px; padding: 8px 10px; }
    .diag-block h3 {
      margin: 0 0 6px 0; font-size: 12px; letter-spacing: .04em;
      text-transform: uppercase; color: var(--muted);
    }
    .diag-row {
      display: grid; grid-template-columns: 68px minmax(0, 1fr);
      gap: 10px; margin: 3px 0; align-items: baseline;
    }
    .diag-row span { color: var(--muted); font-size: 12px; }
    .diag-row code {
      color: var(--text); font-size: 12px; text-align: right;
      white-space: normal; word-break: break-word; overflow-wrap: anywhere;
    }
    @media (max-width: 460px) {
      .diag-row { grid-template-columns: 1fr; gap: 2px; }
      .diag-row code { text-align: left; }
    }
    .diag-note { color: var(--muted); font-size: 11px; margin: 0; }
    .calc-grid { display: grid; gap: 10px; grid-template-columns: 1fr; }
    @media (min-width: 700px) { .calc-grid { grid-template-columns: 1fr 1fr; } }
    @media (min-width: 1020px) { .calc-grid.calc-grid-3 { grid-template-columns: 1fr 1fr 1fr; } }
    @media (min-width: 1020px) {
      .calc-grid.calc-grid-compact {
        grid-template-columns: repeat(3, minmax(0, 320px));
        justify-content: start;
      }
    }
    .calc-block { background: var(--card-2); border: 1px solid var(--border); border-radius: 8px; padding: 10px 12px; }
    .calc-block h3 { margin: 0 0 8px 0; font-size: 13px; color: var(--text); }
    .calc-metric { font-size: 28px; font-weight: 700; margin-top: 6px; }
    .calc-caption { color: var(--muted); font-size: 12px; margin-top: 4px; }
    .calc-metric.ok { color: var(--metric-ok); }
    .calc-metric.warn { color: var(--metric-warn); }
    .preset-row { display: flex; gap: 8px; flex-wrap: wrap; align-items: flex-end; }
    .linearity-canvas {
      width: 100%;
      height: 280px;
      display: block;
      border-radius: 8px;
      background: var(--card-2);
      border: 1px solid var(--border);
      margin-top: 10px;
    }
  </style>
</head>
<body>
  <h1>Incidence Perfect NG</h1>
  <div class="muted">FW <span id="fw"></span></div>

  <div class="card">
    <div id="metricsRow" class="row">
      <div id="rollCard" class="metric-card">
        <div>ROLL</div>
        <div id="roll" class="value">--</div>
      </div>
      <div id="pitchCard" class="metric-card">
        <div>PITCH</div>
        <div id="pitch" class="value">--</div>
      </div>
    </div>
    <div style="margin-top:8px;">
      <code id="status">Loading...</code>
    </div>
    <div id="condHint" class="muted" style="margin-top:6px;"></div>
    <div id="batHint" class="muted" style="margin-top:4px;"></div>
  </div>

  <div class="card">
    <div id="normalControls" class="row">
      <button onclick="sendCmd('freeze')">FREEZE</button>
      <button onclick="sendCmd('zero')">ZERO</button>
      <button onclick="sendCmd('offset_cal')">OFFSET CAL</button>
      <button onclick="sendCmd('axis')">AXIS</button>
      <button onclick="sendCmd('mode_toggle')">MODE</button>
      <button onclick="sendCmd('align_start')">ALIGN</button>
      <button onclick="sendCmd('rotate')">ROTATE</button>
      <button onclick="sendCmd('sleep')">SLEEP</button>
    </div>
    <div id="modeControls" class="row hidden" style="margin-top:10px;">
      <button onclick="sendCmd('cancel')">CANCEL</button>
      <button onclick="sendCmd('confirm')">CONFIRM</button>
    </div>
    <div id="alignControls" class="row hidden" style="margin-top:10px;">
      <button onclick="sendCmd('cancel')">CANCEL</button>
      <button onclick="sendCmd('capture')">CAPTURE</button>
    </div>
    <div id="offsetCalControls" class="row hidden" style="margin-top:10px;">
      <button onclick="sendCmd('cancel')">CANCEL</button>
      <button onclick="sendCmd('confirm')">CONFIRM</button>
    </div>
    <div id="progressWrap" class="bar hidden"><div id="progressBar"></div></div>
    <div id="msg"></div>
  </div>

  <details id="surfaceDisplacementCard" class="card">
    <summary>Surface Displacement</summary>
    <p class="diag-note">Estimate trailing-edge displacement from live angle using `displacement = depth * sin(angle)` and optionally guide setup toward separate up/down targets.</p>
    <div class="preset-row" style="margin-top:8px;">
      <label style="min-width:180px;">
        <div class="muted" style="margin:0 0 4px 0;">Preset</div>
        <select id="dispPreset">
          <option value="custom">Custom</option>
          <option value="aileron">Aileron</option>
          <option value="elevator">Elevator</option>
          <option value="rudder">Rudder</option>
        </select>
      </label>
      <button id="dispPresetApplyBtn" type="button">Apply Preset</button>
      <div id="dispPresetHint" class="muted" style="min-width:240px;">Choose a surface preset to prefill common values.</div>
    </div>
    <div class="row" style="margin-top:8px;">
      <label style="min-width:180px; max-width:260px;">
        <div class="muted" style="margin:0 0 4px 0;">Surface label</div>
        <input id="dispLabel" type="text" placeholder="Aileron" maxlength="24">
      </label>
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Angle source</div>
        <select id="dispAxis">
          <option value="auto">AUTO</option>
          <option value="roll">ROLL</option>
          <option value="pitch">PITCH</option>
        </select>
      </label>
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Depth</div>
        <input id="dispDepth" type="number" min="0" step="0.1" placeholder="30.0">
      </label>
      <label style="min-width:120px;">
        <div class="muted" style="margin:0 0 4px 0;">Units</div>
        <input id="dispUnit" type="text" value="mm" maxlength="8">
      </label>
    </div>
    <div class="row" style="margin-top:8px;">
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Target mode</div>
        <select id="dispTargetMode">
          <option value="displacement">Displacement</option>
          <option value="angle">Angle</option>
        </select>
      </label>
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Up target</div>
        <input id="dispTargetUp" type="number" step="0.1" placeholder="12.0">
      </label>
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Down target</div>
        <input id="dispTargetDown" type="number" step="0.1" placeholder="-8.0">
      </label>
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Tolerance</div>
        <input id="dispTolerance" type="number" min="0.01" step="0.01" value="0.5">
      </label>
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Approach window</div>
        <input id="dispApproachWindow" type="number" min="0.01" step="0.01" placeholder="Auto">
      </label>
    </div>
    <div class="row" style="margin-top:8px; align-items:flex-end;">
      <label style="display:flex; align-items:center; gap:8px; min-width:170px;">
        <input id="dispAudioEnabled" type="checkbox">
        <span>Audio cues</span>
      </label>
      <button id="dispAudioArmBtn" type="button">Enable Audio</button>
      <div id="dispAudioStatus" class="muted" style="min-width:220px;">Audio optional. Enable if you want audible guidance.</div>
    </div>
    <div class="calc-grid calc-grid-3 calc-grid-compact" style="margin-top:10px;">
      <div class="calc-block">
        <h3 id="dispHeading">Live Surface</h3>
        <div id="dispValue" class="calc-metric">--</div>
        <div id="dispCaption" class="calc-caption">Enter depth to calculate displacement.</div>
      </div>
      <div class="calc-block">
        <h3>Reference</h3>
        <div id="dispAngle" class="calc-metric">--</div>
        <div id="dispFormula" class="calc-caption">Using live angle from the selected axis.</div>
      </div>
      <div class="calc-block">
        <h3 id="dispTargetHeading">Setup Target</h3>
        <div id="dispTargetValue" class="calc-metric">--</div>
        <div id="dispTargetCaption" class="calc-caption">Configure targets to guide setup.</div>
      </div>
    </div>
  </details>

  <details id="linearityCard" class="card">
    <summary>Linearity</summary>
    <p class="diag-note">Start a continuous endpoint-to-endpoint sweep on the radio first, then record here. The web UI treats time within each sweep as servo travel.</p>
    <div class="row" style="margin-top:8px;">
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Source</div>
        <select id="linearitySource">
          <option value="displacement">Displacement</option>
          <option value="pitch">PITCH</option>
          <option value="roll">ROLL</option>
        </select>
      </label>
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Record seconds</div>
        <input id="linearityDuration" type="number" min="4" max="20" step="1" value="8">
      </label>
      <div class="row" style="align-items:flex-end;">
        <button id="linearityRecordBtn" type="button">Record</button>
        <button id="linearityStopBtn" type="button" disabled>Stop</button>
        <button id="linearityClearBtn" type="button">Clear</button>
      </div>
    </div>
    <div id="linearityStatus" class="muted" style="margin-top:8px;">Ready. Start the servo sweep, then press Record.</div>
    <canvas id="linearityCanvas" class="linearity-canvas" width="900" height="280"></canvas>
    <div class="calc-grid calc-grid-3" style="margin-top:10px;">
      <div class="calc-block">
        <h3>Capture</h3>
        <div id="linearityCaptureMetric" class="calc-metric">--</div>
        <div id="linearityCaptureCaption" class="calc-caption">No sweep recorded yet.</div>
      </div>
      <div class="calc-block">
        <h3>Deviation</h3>
        <div id="linearityDeviationMetric" class="calc-metric">--</div>
        <div id="linearityDeviationCaption" class="calc-caption">Deviation from ideal straight-line motion will appear here.</div>
      </div>
      <div class="calc-block">
        <h3>Hysteresis</h3>
        <div id="linearityHysteresisMetric" class="calc-metric">--</div>
        <div id="linearityHysteresisCaption" class="calc-caption">Difference between averaged up/down sweeps will appear here.</div>
      </div>
    </div>
  </details>

  <details id="diagnosticsCard" class="card">
    <summary>Device Settings</summary>
    <div id="deviceStatus" class="muted">Loading device settings...</div>
    <div class="row" style="margin-top:8px;">
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Battery</div>
        <select id="deviceBatteryMode">
          <option value="auto">Auto detect</option>
          <option value="present">Installed</option>
          <option value="absent">No battery installed</option>
        </select>
      </label>
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Startup ZERO</div>
        <select id="deviceZeroOnBoot">
          <option value="on">Enabled</option>
          <option value="off">Disabled</option>
        </select>
      </label>
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Readout decimals</div>
        <select id="deviceDisplayPrecision">
          <option value="1">1</option>
          <option value="2">2</option>
          <option value="3">3</option>
        </select>
      </label>
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Touch input</div>
        <select id="deviceTouchEnabled">
          <option value="on">Enabled</option>
          <option value="off">Disabled</option>
        </select>
      </label>
      <label class="slider-control">
        <div class="muted" style="margin:0 0 4px 0;">Brightness</div>
        <div class="row">
          <input id="deviceDisplayBrightness" type="range" min="10" max="100" step="5">
          <code id="deviceBrightnessValue">100%</code>
        </div>
      </label>
    </div>
    <label class="muted" style="display:flex; align-items:center; gap:8px; margin-top:8px;">
      <input id="deviceTouchPersist" type="checkbox">
      Persist touch lock across reboot
    </label>
    <div class="row" style="margin-top:10px;">
      <button id="deviceSaveBtn" onclick="saveDeviceSettings()">Save Device Settings</button>
    </div>
    <div id="deviceMsg" class="muted" style="margin-top:8px;"></div>
  </details>

  <details class="card">
    <summary>Network</summary>
    <div id="netStatus" class="muted">Loading network state...</div>
    <div id="netAddr" class="muted"></div>
    <div class="row" style="margin-top:8px;">
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Mode</div>
        <select id="netMode">
          <option value="ap">AP only</option>
          <option value="sta">STA with AP fallback</option>
        </select>
      </label>
      <label style="flex:1; min-width:180px;">
        <div class="muted" style="margin:0 0 4px 0;">Hostname</div>
        <input id="netHostname" type="text" placeholder="incidence-perfect-ng" maxlength="32">
      </label>
    </div>
    <div class="row" style="margin-top:8px;">
      <label style="flex:1; min-width:180px;">
        <div class="muted" style="margin:0 0 4px 0;">STA SSID</div>
        <input id="netSsid" type="text" placeholder="Wi-Fi network name" maxlength="32">
      </label>
      <label style="flex:1; min-width:180px;">
        <div class="muted" style="margin:0 0 4px 0;">STA Password</div>
        <input id="netPassword" type="password" placeholder="Leave blank to keep existing" maxlength="64">
      </label>
    </div>
    <div class="row" style="margin-top:10px;">
      <button id="netSaveBtn" onclick="saveNetwork()">Save</button>
      <button id="netRecoverBtn" onclick="recoverNetwork()">Recover AP Mode</button>
    </div>
    <div id="netMsg" class="muted" style="margin-top:8px;"></div>
  </details>

  <details class="card">
    <summary>Web UI Appearance</summary>
    <div class="row" style="margin-top:8px;">
      <label style="min-width:180px;">
        <div class="muted" style="margin:0 0 4px 0;">Theme</div>
        <select id="webTheme">
          <option value="dark">Dark</option>
          <option value="day">Day</option>
        </select>
      </label>
    </div>
    <div id="webThemeMsg" class="muted" style="margin-top:8px;">Theme preference is saved in this browser.</div>
  </details>

  <details class="card">
    <summary>OTA Update</summary>
    <p class="diag-note">Upload a firmware `.bin` built for this board/environment. Version and SHA-256 are required.</p>
    <input id="otaFile" type="file" accept=".bin">
    <div class="row" style="margin-top:8px;">
      <label style="flex:1; min-width:180px;">
        <div class="muted" style="margin:0 0 4px 0;">Target version</div>
        <input id="otaVersion" type="text" placeholder="2026.3.8">
      </label>
    </div>
    <div class="row" style="margin-top:8px;">
      <label style="flex:1; min-width:180px;">
        <div class="muted" style="margin:0 0 4px 0;">SHA-256 (auto-calculated)</div>
        <input id="otaSha256" type="text" placeholder="64 hex chars" maxlength="64">
      </label>
    </div>
    <label class="muted" style="display:flex; align-items:center; gap:8px; margin-top:6px;">
      <input id="otaForce" type="checkbox">
      Allow reinstall/downgrade for service scenarios
    </label>
    <div class="bar"><div id="otaProgressBar"></div></div>
    <div class="row" style="margin-top:10px;">
      <button id="otaBtn" onclick="uploadOta()">Upload & Install</button>
    </div>
    <div id="otaMsg" class="muted" style="margin-top:8px;"></div>
  </details>

  <details class="card">
    <summary>Diagnostics</summary>
    <div class="diag-layout">
      <div class="diag-pills">
        <span id="diagValid" class="pill">diag --</span>
        <span id="diagWorkflows" class="pill">workflows: --</span>
      </div>
      <div class="diag-grid">
        <div class="diag-block">
          <h3>Raw Sensor</h3>
          <div class="diag-row"><span>Accel</span><code id="diagRawA">--</code></div>
          <div class="diag-row"><span>Gyro</span><code id="diagRawG">--</code></div>
        </div>
        <div class="diag-block">
          <h3>Mapped</h3>
          <div class="diag-row"><span>Accel</span><code id="diagMapA">--</code></div>
          <div class="diag-row"><span>Gyro</span><code id="diagMapG">--</code></div>
        </div>
        <div class="diag-block">
          <h3>Corrected</h3>
          <div class="diag-row"><span>Accel</span><code id="diagCorrA">--</code></div>
          <div class="diag-row"><span>Gyro</span><code id="diagCorrG">--</code></div>
        </div>
        <div class="diag-block">
          <h3>Angles</h3>
          <div class="diag-row"><span>Physics</span><code id="diagPhys">--</code></div>
          <div class="diag-row"><span>Conditioning</span><code id="diagCond">--</code></div>
        </div>
        <div class="diag-block">
          <h3>Calibration Refs</h3>
          <div class="diag-row"><span>Bias A</span><code id="diagBiasA">--</code></div>
          <div class="diag-row"><span>Bias G</span><code id="diagBiasG">--</code></div>
          <div class="diag-row"><span>Zero</span><code id="diagZero">--</code></div>
          <div class="diag-row"><span>Align</span><code id="diagAlign">--</code></div>
        </div>
      </div>
      <p class="diag-note">Live angles refresh ~120 ms; diagnostics refresh ~600 ms.</p>
    </div>
  </details>

  <script>
    const statusEl = document.getElementById('status');
    const msgEl = document.getElementById('msg');
    const fwEl = document.getElementById('fw');
    const webThemeEl = document.getElementById('webTheme');
    const webThemeMsgEl = document.getElementById('webThemeMsg');
    const rollEl = document.getElementById('roll');
    const pitchEl = document.getElementById('pitch');
    const rollCard = document.getElementById('rollCard');
    const pitchCard = document.getElementById('pitchCard');
    const metricsRow = document.getElementById('metricsRow');
    const condEl = document.getElementById('condHint');
    const batEl = document.getElementById('batHint');
    const normalControls = document.getElementById('normalControls');
    const modeControls = document.getElementById('modeControls');
    const alignControls = document.getElementById('alignControls');
    const offsetCalControls = document.getElementById('offsetCalControls');
    const progressWrap = document.getElementById('progressWrap');
    const progressBar = document.getElementById('progressBar');
    const dispLabelEl = document.getElementById('dispLabel');
    const dispPresetEl = document.getElementById('dispPreset');
    const dispPresetApplyBtn = document.getElementById('dispPresetApplyBtn');
    const dispPresetHintEl = document.getElementById('dispPresetHint');
    const dispAxisEl = document.getElementById('dispAxis');
    const dispDepthEl = document.getElementById('dispDepth');
    const dispUnitEl = document.getElementById('dispUnit');
    const dispTargetModeEl = document.getElementById('dispTargetMode');
    const dispTargetUpEl = document.getElementById('dispTargetUp');
    const dispTargetDownEl = document.getElementById('dispTargetDown');
    const dispToleranceEl = document.getElementById('dispTolerance');
    const dispApproachWindowEl = document.getElementById('dispApproachWindow');
    const dispAudioEnabledEl = document.getElementById('dispAudioEnabled');
    const dispAudioArmBtn = document.getElementById('dispAudioArmBtn');
    const dispAudioStatusEl = document.getElementById('dispAudioStatus');
    const dispHeadingEl = document.getElementById('dispHeading');
    const dispValueEl = document.getElementById('dispValue');
    const dispCaptionEl = document.getElementById('dispCaption');
    const dispAngleEl = document.getElementById('dispAngle');
    const dispFormulaEl = document.getElementById('dispFormula');
    const dispTargetHeadingEl = document.getElementById('dispTargetHeading');
    const dispTargetValueEl = document.getElementById('dispTargetValue');
    const dispTargetCaptionEl = document.getElementById('dispTargetCaption');
    const surfaceDisplacementCardEl = document.getElementById('surfaceDisplacementCard');
    const linearityCardEl = document.getElementById('linearityCard');
    const linearitySourceEl = document.getElementById('linearitySource');
    const linearityDurationEl = document.getElementById('linearityDuration');
    const linearityRecordBtn = document.getElementById('linearityRecordBtn');
    const linearityStopBtn = document.getElementById('linearityStopBtn');
    const linearityClearBtn = document.getElementById('linearityClearBtn');
    const linearityStatusEl = document.getElementById('linearityStatus');
    const linearityCanvasEl = document.getElementById('linearityCanvas');
    const linearityCaptureMetricEl = document.getElementById('linearityCaptureMetric');
    const linearityCaptureCaptionEl = document.getElementById('linearityCaptureCaption');
    const linearityDeviationMetricEl = document.getElementById('linearityDeviationMetric');
    const linearityDeviationCaptionEl = document.getElementById('linearityDeviationCaption');
    const linearityHysteresisMetricEl = document.getElementById('linearityHysteresisMetric');
    const linearityHysteresisCaptionEl = document.getElementById('linearityHysteresisCaption');
    const deviceStatusEl = document.getElementById('deviceStatus');
    const deviceBatteryModeEl = document.getElementById('deviceBatteryMode');
    const deviceZeroOnBootEl = document.getElementById('deviceZeroOnBoot');
    const deviceDisplayPrecisionEl = document.getElementById('deviceDisplayPrecision');
    const deviceTouchEnabledEl = document.getElementById('deviceTouchEnabled');
    const deviceTouchPersistEl = document.getElementById('deviceTouchPersist');
    const deviceDisplayBrightnessEl = document.getElementById('deviceDisplayBrightness');
    const deviceBrightnessValueEl = document.getElementById('deviceBrightnessValue');
    const deviceSaveBtn = document.getElementById('deviceSaveBtn');
    const deviceMsgEl = document.getElementById('deviceMsg');
    const netStatusEl = document.getElementById('netStatus');
    const netAddrEl = document.getElementById('netAddr');
    const netModeEl = document.getElementById('netMode');
    const netHostnameEl = document.getElementById('netHostname');
    const netSsidEl = document.getElementById('netSsid');
    const netPasswordEl = document.getElementById('netPassword');
    const netSaveBtn = document.getElementById('netSaveBtn');
    const netRecoverBtn = document.getElementById('netRecoverBtn');
    const netMsgEl = document.getElementById('netMsg');
    const otaFileEl = document.getElementById('otaFile');
    const otaVersionEl = document.getElementById('otaVersion');
    const otaSha256El = document.getElementById('otaSha256');
    const otaForceEl = document.getElementById('otaForce');
    const otaBtnEl = document.getElementById('otaBtn');
    const otaProgressBarEl = document.getElementById('otaProgressBar');
    const otaMsgEl = document.getElementById('otaMsg');
    const diagnosticsCardEl = document.getElementById('diagnosticsCard');
    const diagValidEl = document.getElementById('diagValid');
    const diagWorkflowsEl = document.getElementById('diagWorkflows');
    const diagRawAEl = document.getElementById('diagRawA');
    const diagRawGEl = document.getElementById('diagRawG');
    const diagMapAEl = document.getElementById('diagMapA');
    const diagMapGEl = document.getElementById('diagMapG');
    const diagCorrAEl = document.getElementById('diagCorrA');
    const diagCorrGEl = document.getElementById('diagCorrG');
    const diagPhysEl = document.getElementById('diagPhys');
    const diagCondEl = document.getElementById('diagCond');
    const diagBiasAEl = document.getElementById('diagBiasA');
    const diagBiasGEl = document.getElementById('diagBiasG');
    const diagZeroEl = document.getElementById('diagZero');
    const diagAlignEl = document.getElementById('diagAlign');
    const WARN_LIMIT = 30.0;
    const CRIT_LIMIT = 45.0;
    const DISPLACEMENT_PREFS_KEY = 'ipng_displacement_v2';
    const WEB_THEME_PREFS_KEY = 'ipng_web_theme_v1';
    const LINEARITY_PREFS_KEY = 'ipng_linearity_v1';
    let networkFormDirty = false;
    let deviceFormDirty = false;
    let otaUploadInFlight = false;
    let currentDisplayDecimals = 2;
    let lastLiveState = null;
    let liveRefreshInFlight = false;
    let stateRefreshInFlight = false;
    let networkRefreshInFlight = false;
    let guideAudioCtx = null;
    let guideAudioOsc = null;
    let guideAudioGain = null;
    let guideAudioUnlocked = false;
    let guideAudioPulseUntilMs = 0;
    let guideAudioLastPulseMs = 0;
    const LIVE_FETCH_TIMEOUT_MS = 1200;
    const STATE_FETCH_TIMEOUT_MS = 1800;
    const NETWORK_FETCH_TIMEOUT_MS = 2500;
    const LIVE_POLL_ACTIVE_MS = 120;
    const LIVE_POLL_HIDDEN_MS = 700;
    const STATE_POLL_ACTIVE_MS = 600;
    const STATE_POLL_IDLE_MS = 1200;
    const STATE_POLL_HIDDEN_MS = 1800;
    const NETWORK_POLL_ACTIVE_MS = 3000;
    const NETWORK_POLL_HIDDEN_MS = 6000;
    const DISPLACEMENT_RENDER_MIN_INTERVAL_MS = 240;
    let linearityCapture = {
      recording: false,
      startedAtMs: 0,
      durationMs: 8000,
      samples: [],
      result: null
    };
    let livePollTimer = null;
    let statePollTimer = null;
    let networkPollTimer = null;
    let liveFailureCount = 0;
    let stateFailureCount = 0;
    let networkFailureCount = 0;
    let lastDisplacementRenderMs = 0;

    [deviceBatteryModeEl, deviceZeroOnBootEl, deviceDisplayPrecisionEl, deviceTouchEnabledEl, deviceTouchPersistEl, deviceDisplayBrightnessEl].forEach((el) => {
      el.addEventListener('input', () => { deviceFormDirty = true; syncBrightnessLabel(); });
    });
    [netModeEl, netHostnameEl, netSsidEl, netPasswordEl].forEach((el) => {
      el.addEventListener('input', () => { networkFormDirty = true; });
    });
    otaFileEl.addEventListener('change', onOtaFileSelected);
    webThemeEl.addEventListener('input', () => {
      applyWebTheme(webThemeEl.value);
      saveWebTheme();
    });
    [dispLabelEl, dispPresetEl, dispAxisEl, dispDepthEl, dispUnitEl, dispTargetModeEl, dispTargetUpEl, dispTargetDownEl, dispToleranceEl, dispApproachWindowEl, dispAudioEnabledEl].forEach((el) => {
      el.addEventListener('input', () => {
        saveDisplacementPrefs();
        renderDisplacement(lastLiveState, true);
      });
    });
    dispAudioArmBtn.addEventListener('click', armGuideAudio);
    dispPresetApplyBtn.addEventListener('click', applyDisplacementPreset);
    [linearitySourceEl, linearityDurationEl].forEach((el) => {
      el.addEventListener('input', () => {
        saveLinearityPrefs();
        drawLinearityChart(linearityCapture.result);
      });
    });
    linearityRecordBtn.addEventListener('click', startLinearityCapture);
    linearityStopBtn.addEventListener('click', () => stopLinearityCapture(false));
    linearityClearBtn.addEventListener('click', clearLinearityCapture);
    window.addEventListener('resize', () => {
      if (linearityCardEl && linearityCardEl.open) drawLinearityChart(linearityCapture.result);
    });
    document.addEventListener('visibilitychange', () => {
      scheduleLivePoll(0);
      scheduleStatePoll(0);
      scheduleNetworkPoll(0);
    });
    if (surfaceDisplacementCardEl) {
      surfaceDisplacementCardEl.addEventListener('toggle', () => {
        if (surfaceDisplacementCardEl.open) renderDisplacement(lastLiveState, true);
      });
    }
    if (linearityCardEl) {
      linearityCardEl.addEventListener('toggle', () => {
        if (linearityCardEl.open) drawLinearityChart(linearityCapture.result);
      });
    }
    if (diagnosticsCardEl) {
      diagnosticsCardEl.addEventListener('toggle', () => {
        if (diagnosticsCardEl.open) scheduleStatePoll(0);
      });
    }

    function fmt(v) {
      const n = Number(v);
      if (Number.isNaN(n)) return '--';
      return (n >= 0 ? '+' : '') + n.toFixed(currentDisplayDecimals) + '\u00B0';
    }

    async function fetchJsonWithTimeout(url, timeoutMs) {
      let timeoutHandle = null;
      let controller = null;
      try {
        if (typeof AbortController !== 'undefined') {
          controller = new AbortController();
          timeoutHandle = setTimeout(() => controller.abort(), timeoutMs);
          const response = await fetch(url, {
            cache: 'no-store',
            signal: controller.signal
          });
          return await response.json();
        }

        const timeoutPromise = new Promise((_, reject) => {
          timeoutHandle = setTimeout(() => reject(new Error('request timeout')), timeoutMs);
        });
        const response = await Promise.race([
          fetch(url, { cache: 'no-store' }),
          timeoutPromise
        ]);
        return await response.json();
      } finally {
        if (timeoutHandle) clearTimeout(timeoutHandle);
      }
    }

    function scheduleLivePoll(delayMs = LIVE_POLL_ACTIVE_MS) {
      if (livePollTimer) clearTimeout(livePollTimer);
      livePollTimer = setTimeout(refreshLive, Math.max(0, delayMs));
    }

    function scheduleStatePoll(delayMs = STATE_POLL_ACTIVE_MS) {
      if (statePollTimer) clearTimeout(statePollTimer);
      statePollTimer = setTimeout(refreshState, Math.max(0, delayMs));
    }

    function scheduleNetworkPoll(delayMs = NETWORK_POLL_ACTIVE_MS) {
      if (networkPollTimer) clearTimeout(networkPollTimer);
      networkPollTimer = setTimeout(refreshNetwork, Math.max(0, delayMs));
    }

    function nextLivePollDelay() {
      return document.hidden ? LIVE_POLL_HIDDEN_MS : LIVE_POLL_ACTIVE_MS;
    }

    function nextStatePollDelay(s) {
      if (document.hidden) return STATE_POLL_HIDDEN_MS;
      const workflowActive = !!(s && (s.mode_active || s.align_active || s.offset_cal_active || s.zero_active));
      const diagnosticsOpen = diagnosticsCardEl && diagnosticsCardEl.open;
      return (workflowActive || diagnosticsOpen) ? STATE_POLL_ACTIVE_MS : STATE_POLL_IDLE_MS;
    }

    function nextNetworkPollDelay() {
      return document.hidden ? NETWORK_POLL_HIDDEN_MS : NETWORK_POLL_ACTIVE_MS;
    }

    function applyWebTheme(theme) {
      const next = theme === 'day' ? 'day' : 'dark';
      document.body.setAttribute('data-theme', next);
      webThemeEl.value = next;
      webThemeMsgEl.textContent = `${next === 'day' ? 'Day' : 'Dark'} theme active in this browser.`;
      if (linearityCardEl && linearityCardEl.open) drawLinearityChart(linearityCapture.result);
    }

    function saveWebTheme() {
      try {
        localStorage.setItem(WEB_THEME_PREFS_KEY, webThemeEl.value === 'day' ? 'day' : 'dark');
      } catch (e) {
      }
    }

    function loadWebTheme() {
      let theme = 'dark';
      try {
        const raw = localStorage.getItem(WEB_THEME_PREFS_KEY);
        if (raw === 'day' || raw === 'dark') theme = raw;
      } catch (e) {
      }
      applyWebTheme(theme);
    }

    function setAngleClass(el, v) {
      el.classList.remove('warn', 'crit');
      const n = Number(v);
      if (Number.isNaN(n)) return;
      const a = Math.abs(n);
      if (a >= CRIT_LIMIT) el.classList.add('crit');
      else if (a >= WARN_LIMIT) el.classList.add('warn');
    }

    function setAxisLayout(axis) {
      const isRoll = axis === 'ROLL';
      const isPitch = axis === 'PITCH';
      if (isRoll || isPitch) {
        metricsRow.classList.add('single-axis');
        metricsRow.style.justifyContent = 'center';
        const active = isRoll ? rollCard : pitchCard;
        const inactive = isRoll ? pitchCard : rollCard;
        active.classList.remove('hidden');
        inactive.classList.add('hidden');
        active.style.flex = '0 1 320px';
        active.style.maxWidth = '320px';
      } else {
        metricsRow.classList.remove('single-axis');
        metricsRow.style.justifyContent = '';
        rollCard.classList.remove('hidden');
        pitchCard.classList.remove('hidden');
        rollCard.style.flex = '';
        pitchCard.style.flex = '';
        rollCard.style.maxWidth = '';
        pitchCard.style.maxWidth = '';
      }
    }

    function f(v, d = 3) {
      const n = Number(v);
      return Number.isFinite(n) ? n.toFixed(d) : '--';
    }

    function sanitizeUnitText(raw) {
      const cleaned = String(raw || '').trim();
      return cleaned ? cleaned : 'mm';
    }

    function sanitizeTolerance(raw) {
      const value = Number(raw);
      return Number.isFinite(value) && value > 0 ? value : 0.5;
    }

    function sanitizeDisplayPrecision(raw) {
      const value = Number(raw);
      if (value === 1 || value === 3) return value;
      return 2;
    }

    function sanitizeDisplayBrightness(raw) {
      const value = Number(raw);
      if (!Number.isFinite(value)) return 100;
      const rounded = Math.round(value / 5) * 5;
      return Math.max(10, Math.min(100, rounded));
    }

    function syncBrightnessLabel() {
      deviceBrightnessValueEl.textContent = `${sanitizeDisplayBrightness(deviceDisplayBrightnessEl.value)}%`;
    }

    function sanitizePositiveOptional(raw) {
      const value = Number(raw);
      return Number.isFinite(value) && value > 0 ? value : NaN;
    }

    function sanitizeLinearityDuration(raw) {
      const value = Math.round(Number(raw));
      if (!Number.isFinite(value)) return 8;
      return Math.max(4, Math.min(20, value));
    }

    function displacementLabelText() {
      const label = String(dispLabelEl.value || '').trim();
      return label ? label : 'Surface';
    }

    function signedNumberText(value, digits = currentDisplayDecimals, suffix = '') {
      const n = Number(value);
      if (!Number.isFinite(n)) return '--';
      return `${n >= 0 ? '+' : ''}${n.toFixed(digits)}${suffix}`;
    }

    function setupMetricMeta(mode, unit) {
      return mode === 'angle'
        ? { label: 'angle', suffix: '\u00B0', window: 2.0 }
        : { label: 'displacement', suffix: ` ${unit}`, window: 3.0 };
    }

    function presetDefinition(name) {
      switch (name) {
        case 'aileron':
          return {
            label: 'Aileron',
            axis: 'pitch',
            targetMode: 'displacement',
            unit: 'mm',
            hint: 'Aileron preset suggests PITCH and displacement targets for differential setup.'
          };
        case 'elevator':
          return {
            label: 'Elevator',
            axis: 'pitch',
            targetMode: 'displacement',
            unit: 'mm',
            hint: 'Elevator preset suggests PITCH and displacement targets.'
          };
        case 'rudder':
          return {
            label: 'Rudder',
            axis: 'auto',
            targetMode: 'displacement',
            unit: 'mm',
            hint: 'Rudder preset keeps AUTO source so you can verify which measured axis matches your fixture.'
          };
        default:
          return {
            label: '',
            axis: 'auto',
            targetMode: 'displacement',
            unit: 'mm',
            hint: 'Custom leaves you in full manual control.'
          };
      }
    }

    function resolveAutoAxis(s) {
      const preset = presetDefinition(dispPresetEl.value);
      const stateAxis = String((s && s.axis) || '').toUpperCase();
      if (stateAxis === 'ROLL') return 'roll';
      if (stateAxis === 'PITCH') return 'pitch';
      if (preset.axis === 'roll' || preset.axis === 'pitch') return preset.axis;
      return 'roll';
    }

    function axisChoiceText(rawAxis, resolvedAxis, s) {
      if (rawAxis === 'auto') {
        const stateAxis = String((s && s.axis) || '').toUpperCase();
        if (stateAxis === 'ROLL' || stateAxis === 'PITCH') {
          return `AUTO -> ${stateAxis}`;
        }
        return `AUTO -> ${resolvedAxis === 'pitch' ? 'PITCH' : 'ROLL'}`;
      }
      return resolvedAxis === 'pitch' ? 'PITCH' : 'ROLL';
    }

    function applyDisplacementPreset() {
      const preset = presetDefinition(dispPresetEl.value);
      dispLabelEl.value = preset.label;
      dispAxisEl.value = preset.axis;
      dispTargetModeEl.value = preset.targetMode;
      dispUnitEl.value = preset.unit;
      if (!String(dispToleranceEl.value || '').trim()) {
        dispToleranceEl.value = '0.5';
      }
      dispPresetHintEl.textContent = preset.hint;
      saveDisplacementPrefs();
      renderDisplacement(lastLiveState);
    }

    function loadDisplacementPrefs() {
      try {
        const raw = localStorage.getItem(DISPLACEMENT_PREFS_KEY);
        if (!raw) return;
        const prefs = JSON.parse(raw);
        if (prefs && typeof prefs === 'object') {
          if (typeof prefs.label === 'string') dispLabelEl.value = prefs.label;
          if (prefs.preset === 'aileron' || prefs.preset === 'elevator' || prefs.preset === 'rudder') dispPresetEl.value = prefs.preset;
          if (prefs.axis === 'pitch') dispAxisEl.value = 'pitch';
          if (prefs.axis === 'roll') dispAxisEl.value = 'roll';
          if (prefs.axis === 'auto') dispAxisEl.value = 'auto';
          if (prefs.depth !== undefined && prefs.depth !== null && prefs.depth !== '') dispDepthEl.value = String(prefs.depth);
          if (typeof prefs.unit === 'string') dispUnitEl.value = prefs.unit;
          if (prefs.targetMode === 'angle') dispTargetModeEl.value = 'angle';
          if (prefs.targetMode === 'displacement') dispTargetModeEl.value = 'displacement';
          if (prefs.targetUp !== undefined && prefs.targetUp !== null && prefs.targetUp !== '') dispTargetUpEl.value = String(prefs.targetUp);
          if (prefs.targetDown !== undefined && prefs.targetDown !== null && prefs.targetDown !== '') dispTargetDownEl.value = String(prefs.targetDown);
          if (prefs.tolerance !== undefined && prefs.tolerance !== null && prefs.tolerance !== '') dispToleranceEl.value = String(prefs.tolerance);
          if (prefs.approachWindow !== undefined && prefs.approachWindow !== null && prefs.approachWindow !== '') dispApproachWindowEl.value = String(prefs.approachWindow);
          dispAudioEnabledEl.checked = !!prefs.audioEnabled;
        }
      } catch (e) {
      }
    }

    function saveDisplacementPrefs() {
      try {
        const prefs = {
          label: String(dispLabelEl.value || '').trim(),
          preset: dispPresetEl.value === 'custom' ? 'custom' : dispPresetEl.value,
          axis: dispAxisEl.value === 'pitch' ? 'pitch' : 'roll',
          depth: String(dispDepthEl.value || '').trim(),
          unit: sanitizeUnitText(dispUnitEl.value),
          targetMode: dispTargetModeEl.value === 'angle' ? 'angle' : 'displacement',
          targetUp: String(dispTargetUpEl.value || '').trim(),
          targetDown: String(dispTargetDownEl.value || '').trim(),
          tolerance: String(dispToleranceEl.value || '').trim(),
          approachWindow: String(dispApproachWindowEl.value || '').trim(),
          audioEnabled: !!dispAudioEnabledEl.checked
        };
        if (dispAxisEl.value === 'auto') prefs.axis = 'auto';
        localStorage.setItem(DISPLACEMENT_PREFS_KEY, JSON.stringify(prefs));
      } catch (e) {
      }
    }

    function linearySourceMetaName(source) {
      if (source === 'roll') return 'ROLL';
      if (source === 'pitch') return 'PITCH';
      return 'Displacement';
    }

    function loadLinearityPrefs() {
      try {
        const raw = localStorage.getItem(LINEARITY_PREFS_KEY);
        if (!raw) return;
        const prefs = JSON.parse(raw);
        if (prefs && typeof prefs === 'object') {
          if (prefs.source === 'roll' || prefs.source === 'pitch' || prefs.source === 'displacement') {
            linearitySourceEl.value = prefs.source;
          }
          if (prefs.duration !== undefined && prefs.duration !== null && prefs.duration !== '') {
            linearityDurationEl.value = String(sanitizeLinearityDuration(prefs.duration));
          }
        }
      } catch (e) {
      }
    }

    function saveLinearityPrefs() {
      try {
        localStorage.setItem(LINEARITY_PREFS_KEY, JSON.stringify({
          source: linearitySourceEl.value === 'roll' ? 'roll' : (linearitySourceEl.value === 'pitch' ? 'pitch' : 'displacement'),
          duration: sanitizeLinearityDuration(linearityDurationEl.value)
        }));
      } catch (e) {
      }
    }

    function setLinearityStatus(text) {
      linearityStatusEl.textContent = text;
    }

    function resetLinearityMetrics() {
      linearityCaptureMetricEl.textContent = '--';
      linearityCaptureCaptionEl.textContent = 'No sweep recorded yet.';
      linearityDeviationMetricEl.textContent = '--';
      linearityDeviationCaptionEl.textContent = 'Deviation from ideal straight-line motion will appear here.';
      linearityHysteresisMetricEl.textContent = '--';
      linearityHysteresisCaptionEl.textContent = 'Difference between averaged up/down sweeps will appear here.';
      linearityCaptureMetricEl.classList.remove('ok', 'warn');
      linearityDeviationMetricEl.classList.remove('ok', 'warn');
      linearityHysteresisMetricEl.classList.remove('ok', 'warn');
    }

    function getLinearityMeasurement(s) {
      if (!s) return { ok: false, reason: 'Waiting for live data.' };
      const source = linearitySourceEl.value === 'roll'
        ? 'roll'
        : (linearitySourceEl.value === 'pitch' ? 'pitch' : 'displacement');
      if (source === 'roll' || source === 'pitch') {
        const value = Number(source === 'pitch' ? s.pitch : s.roll);
        if (!Number.isFinite(value)) return { ok: false, reason: `${linearySourceMetaName(source)} is unavailable.` };
        return {
          ok: true,
          source,
          value,
          unit: '\u00B0',
          label: linearySourceMetaName(source)
        };
      }
      const axis = dispAxisEl.value === 'auto' ? resolveAutoAxis(s) : dispAxisEl.value;
      const angle = Number(axis === 'pitch' ? s.pitch : s.roll);
      const depth = Number(dispDepthEl.value);
      const unit = sanitizeUnitText(dispUnitEl.value);
      if (!Number.isFinite(depth) || depth <= 0) {
        return { ok: false, reason: 'Enter surface depth in Surface Displacement before recording displacement linearity.' };
      }
      if (!Number.isFinite(angle)) {
        return { ok: false, reason: 'Selected displacement source angle is unavailable.' };
      }
      return {
        ok: true,
        source,
        value: depth * Math.sin(angle * Math.PI / 180.0),
        unit: ` ${unit}`,
        label: `${displacementLabelText()} displacement`
      };
    }

    function drawLinearityChart(result) {
      if (linearityCardEl && !linearityCardEl.open && !result) return;
      const canvas = linearityCanvasEl;
      const ctx = canvas.getContext('2d');
      const ratio = Math.max(1, window.devicePixelRatio || 1);
      const width = Math.max(320, canvas.clientWidth || canvas.width || 900);
      const height = Math.max(220, canvas.clientHeight || canvas.height || 280);
      if (canvas.width !== Math.round(width * ratio) || canvas.height !== Math.round(height * ratio)) {
        canvas.width = Math.round(width * ratio);
        canvas.height = Math.round(height * ratio);
      }
      ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
      const styles = getComputedStyle(document.body);
      const bg = styles.getPropertyValue('--card-2').trim() || '#101933';
      const border = styles.getPropertyValue('--border').trim() || '#223056';
      const text = styles.getPropertyValue('--text').trim() || '#eef4ff';
      const muted = styles.getPropertyValue('--muted').trim() || '#9fb3d9';
      const upColor = styles.getPropertyValue('--metric-ok').trim() || '#8df1c7';
      const downColor = styles.getPropertyValue('--metric-warn').trim() || '#ffd782';

      ctx.clearRect(0, 0, width, height);
      ctx.fillStyle = bg;
      ctx.fillRect(0, 0, width, height);

      const pad = { left: 54, right: 18, top: 18, bottom: 34 };
      const plotW = width - pad.left - pad.right;
      const plotH = height - pad.top - pad.bottom;
      const xAt = (pct) => pad.left + (plotW * pct / 100.0);
      const yAt = (pct) => pad.top + plotH - (plotH * pct / 100.0);

      ctx.strokeStyle = border;
      ctx.lineWidth = 1;
      for (let pct = 0; pct <= 100; pct += 25) {
        ctx.beginPath();
        ctx.moveTo(xAt(pct), pad.top);
        ctx.lineTo(xAt(pct), pad.top + plotH);
        ctx.stroke();
        ctx.beginPath();
        ctx.moveTo(pad.left, yAt(pct));
        ctx.lineTo(pad.left + plotW, yAt(pct));
        ctx.stroke();
      }

      ctx.strokeStyle = muted;
      ctx.setLineDash([6, 5]);
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      ctx.moveTo(xAt(0), yAt(0));
      ctx.lineTo(xAt(100), yAt(100));
      ctx.stroke();
      ctx.setLineDash([]);

      const drawCurve = (curve, color) => {
        if (!curve || !Array.isArray(curve.points) || curve.points.length < 2) return;
        ctx.strokeStyle = color;
        ctx.lineWidth = 2.5;
        ctx.beginPath();
        curve.points.forEach((pt, idx) => {
          const x = xAt(pt.x);
          const y = yAt(Math.max(-10, Math.min(110, pt.y)));
          if (idx === 0) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        });
        ctx.stroke();
      };

      drawCurve(result && result.upCurve, upColor);
      drawCurve(result && result.downCurve, downColor);

      ctx.fillStyle = text;
      ctx.font = '12px Arial, sans-serif';
      ctx.fillText('Output %', 10, 18);
      ctx.fillText('Servo travel %', width - 94, height - 10);
      ctx.fillStyle = muted;
      ctx.fillText('Ideal', xAt(100) - 34, yAt(100) - 8);
      ctx.fillStyle = upColor;
      ctx.fillText('Up', xAt(100) - 34, yAt(100) + 12);
      ctx.fillStyle = downColor;
      ctx.fillText('Down', xAt(100) - 34, yAt(100) + 30);

      if (!result) {
        ctx.fillStyle = muted;
        ctx.font = '14px Arial, sans-serif';
        ctx.fillText('Record a sweep to plot output % versus time-normalized servo travel.', pad.left, pad.top + 28);
      }
    }

    function detectLinearityTurningPoints(samples) {
      if (!Array.isArray(samples) || samples.length < 7) return [];
      const values = samples.map((sample) => sample.smooth);
      const minValue = Math.min(...values);
      const maxValue = Math.max(...values);
      const range = maxValue - minValue;
      if (!(range > 0.001)) return [];
      const slopeEps = Math.max(range * 0.015, 0.02);
      const swingMin = Math.max(range * 0.12, 0.15);
      const raw = [];
      for (let i = 1; i < samples.length - 1; i += 1) {
        const prev = samples[i].smooth - samples[i - 1].smooth;
        const next = samples[i + 1].smooth - samples[i].smooth;
        if (prev > slopeEps && next < -slopeEps) {
          raw.push({ idx: i, type: 'max', value: samples[i].smooth });
        } else if (prev < -slopeEps && next > slopeEps) {
          raw.push({ idx: i, type: 'min', value: samples[i].smooth });
        }
      }
      const compressed = [];
      raw.forEach((point) => {
        const last = compressed[compressed.length - 1];
        if (last && last.type === point.type) {
          const better = point.type === 'max' ? point.value > last.value : point.value < last.value;
          if (better) compressed[compressed.length - 1] = point;
          return;
        }
        compressed.push(point);
      });
      const filtered = [];
      compressed.forEach((point) => {
        const last = filtered[filtered.length - 1];
        if (!last) {
          filtered.push(point);
          return;
        }
        if (point.idx - last.idx < 4) return;
        if (Math.abs(point.value - last.value) < swingMin) return;
        filtered.push(point);
      });
      return filtered;
    }

    function buildLinearitySegments(samples, turningPoints) {
      if (!Array.isArray(turningPoints) || turningPoints.length < 2) return [];
      const values = samples.map((sample) => sample.smooth);
      const fullRange = Math.max(...values) - Math.min(...values);
      const minAmplitude = Math.max(fullRange * 0.35, 0.4);
      const segments = [];
      for (let i = 0; i < turningPoints.length - 1; i += 1) {
        const start = turningPoints[i];
        const end = turningPoints[i + 1];
        if (end.idx <= start.idx + 4) continue;
        const startSample = samples[start.idx];
        const endSample = samples[end.idx];
        const durationMs = endSample.t - startSample.t;
        const amplitude = Math.abs(endSample.smooth - startSample.smooth);
        if (durationMs < 700 || amplitude < minAmplitude) continue;
        const direction = endSample.smooth > startSample.smooth ? 'up' : 'down';
        segments.push({
          direction,
          startIndex: start.idx,
          endIndex: end.idx,
          startValue: startSample.smooth,
          endValue: endSample.smooth,
          durationMs,
          amplitude
        });
      }
      return segments;
    }

    function resampleLinearitySegment(samples, segment, pointCount = 51) {
      const startSample = samples[segment.startIndex];
      const endSample = samples[segment.endIndex];
      const range = endSample.smooth - startSample.smooth;
      if (!Number.isFinite(range) || Math.abs(range) < 1e-6) return null;
      const out = [];
      let cursor = segment.startIndex;
      for (let i = 0; i < pointCount; i += 1) {
        const ratio = i / (pointCount - 1);
        const targetT = startSample.t + ((endSample.t - startSample.t) * ratio);
        while (cursor < segment.endIndex - 1 && samples[cursor + 1].t < targetT) cursor += 1;
        const a = samples[cursor];
        const b = samples[Math.min(segment.endIndex, cursor + 1)];
        const dt = Math.max(1, b.t - a.t);
        const local = Math.max(0, Math.min(1, (targetT - a.t) / dt));
        const value = a.smooth + ((b.smooth - a.smooth) * local);
        out.push({
          x: ratio * 100,
          y: ((value - startSample.smooth) / range) * 100
        });
      }
      return {
        direction: segment.direction,
        points: out,
        amplitude: segment.amplitude,
        durationMs: segment.durationMs
      };
    }

    function averageLinearityCurves(curves) {
      if (!Array.isArray(curves) || !curves.length) return null;
      const pointCount = curves[0].points.length;
      const points = [];
      for (let i = 0; i < pointCount; i += 1) {
        const x = curves[0].points[i].x;
        let total = 0;
        curves.forEach((curve) => { total += curve.points[i].y; });
        points.push({ x, y: total / curves.length });
      }
      return { points };
    }

    function summarizeLinearityCurve(curve) {
      if (!curve || !curve.points || !curve.points.length) return null;
      let maxDev = 0;
      let sumSq = 0;
      curve.points.forEach((pt) => {
        const dev = pt.y - pt.x;
        maxDev = Math.max(maxDev, Math.abs(dev));
        sumSq += dev * dev;
      });
      return {
        maxDev,
        rmsDev: Math.sqrt(sumSq / curve.points.length)
      };
    }

    function analyzeLinearitySamples(samples) {
      if (!Array.isArray(samples) || samples.length < 12) {
        return { ok: false, error: 'Need more live samples. Try a longer recording.' };
      }
      const smoothed = samples.map((sample, index) => {
        let total = 0;
        let count = 0;
        for (let i = Math.max(0, index - 2); i <= Math.min(samples.length - 1, index + 2); i += 1) {
          total += samples[i].value;
          count += 1;
        }
        return {
          ...sample,
          smooth: total / Math.max(1, count)
        };
      });
      const values = smoothed.map((sample) => sample.smooth);
      const minValue = Math.min(...values);
      const maxValue = Math.max(...values);
      const fullRange = maxValue - minValue;
      if (fullRange < 1.0e-3) {
        return { ok: false, error: 'Sweep amplitude was too small to analyze.' };
      }
      const turningPoints = detectLinearityTurningPoints(smoothed);
      const segments = buildLinearitySegments(smoothed, turningPoints);
      const curves = segments
        .map((segment) => resampleLinearitySegment(smoothed, segment))
        .filter((curve) => !!curve);
      const upCurves = curves.filter((curve) => curve.direction === 'up');
      const downCurves = curves.filter((curve) => curve.direction === 'down');
      if (!upCurves.length && !downCurves.length) {
        return { ok: false, error: 'Could not detect clean endpoint-to-endpoint sweeps. Try a longer or steadier recording.' };
      }
      const upCurve = averageLinearityCurves(upCurves);
      const downCurve = averageLinearityCurves(downCurves);
      const upSummary = summarizeLinearityCurve(upCurve);
      const downSummary = summarizeLinearityCurve(downCurve);
      let hysteresisMax = 0;
      if (upCurve && downCurve) {
        for (let i = 0; i < upCurve.points.length; i += 1) {
          hysteresisMax = Math.max(hysteresisMax, Math.abs(upCurve.points[i].y - downCurve.points[i].y));
        }
      }
      return {
        ok: true,
        sampleCount: samples.length,
        minValue,
        maxValue,
        range: fullRange,
        segmentCount: segments.length,
        upCount: upCurves.length,
        downCount: downCurves.length,
        upCurve,
        downCurve,
        upSummary,
        downSummary,
        hysteresisMax
      };
    }

    function renderLinearityResult(result) {
      resetLinearityMetrics();
      if (!result || !result.ok) {
        drawLinearityChart(null);
        if (result && result.error) setLinearityStatus(result.error);
        return;
      }
      const measurement = getLinearityMeasurement(lastLiveState);
      const unit = (measurement && measurement.ok) ? measurement.unit : '';
      const label = (measurement && measurement.ok) ? measurement.label : linearySourceMetaName(linearitySourceEl.value);
      linearityCaptureMetricEl.textContent = `${result.segmentCount} sweeps`;
      linearityCaptureMetricEl.classList.add('ok');
      linearityCaptureCaptionEl.textContent = `${label} range ${result.minValue.toFixed(currentDisplayDecimals)} to ${result.maxValue.toFixed(currentDisplayDecimals)}${unit}. Averaged ${result.upCount} up and ${result.downCount} down sweeps from ${result.sampleCount} samples.`;

      const maxDev = Math.max(result.upSummary ? result.upSummary.maxDev : 0, result.downSummary ? result.downSummary.maxDev : 0);
      const rmsParts = [];
      if (result.upSummary) rmsParts.push(`UP RMS ${result.upSummary.rmsDev.toFixed(1)}%`);
      if (result.downSummary) rmsParts.push(`DOWN RMS ${result.downSummary.rmsDev.toFixed(1)}%`);
      linearityDeviationMetricEl.textContent = `${maxDev.toFixed(1)}%`;
      linearityDeviationMetricEl.classList.add(maxDev <= 5 ? 'ok' : 'warn');
      linearityDeviationCaptionEl.textContent = `Peak deviation from ideal straight-line response. ${rmsParts.join(' | ') || 'Need at least one valid sweep direction.'}`;

      linearityHysteresisMetricEl.textContent = `${result.hysteresisMax.toFixed(1)}%`;
      linearityHysteresisMetricEl.classList.add(result.hysteresisMax <= 4 ? 'ok' : 'warn');
      linearityHysteresisCaptionEl.textContent = result.upCurve && result.downCurve
        ? 'Maximum separation between averaged up and down sweeps.'
        : 'Record both directions to quantify hysteresis.';
      setLinearityStatus(`Analysis complete. ${result.segmentCount} usable sweeps detected from the recorded trace.`);
      drawLinearityChart(result);
    }

    function clearLinearityCapture() {
      linearityCapture.recording = false;
      linearityCapture.samples = [];
      linearityCapture.result = null;
      linearityRecordBtn.disabled = false;
      linearityStopBtn.disabled = true;
      setLinearityStatus('Ready. Start the servo sweep, then press Record.');
      resetLinearityMetrics();
      drawLinearityChart(null);
    }

    function stopLinearityCapture(autoStop = false) {
      if (!linearityCapture.recording) return;
      linearityCapture.recording = false;
      linearityRecordBtn.disabled = false;
      linearityStopBtn.disabled = true;
      const result = analyzeLinearitySamples(linearityCapture.samples);
      linearityCapture.result = result.ok ? result : null;
      if (!result.ok) {
        setLinearityStatus(result.error);
        resetLinearityMetrics();
        drawLinearityChart(null);
        return;
      }
      renderLinearityResult(result);
      if (autoStop) {
        setLinearityStatus(`Recording stopped after ${Math.round(linearityCapture.durationMs / 1000)} s. ${result.segmentCount} usable sweeps detected.`);
      }
    }

    function startLinearityCapture() {
      const measurement = getLinearityMeasurement(lastLiveState);
      if (!measurement.ok) {
        setLinearityStatus(measurement.reason);
        return;
      }
      linearityCapture.recording = true;
      linearityCapture.startedAtMs = Date.now();
      linearityCapture.durationMs = sanitizeLinearityDuration(linearityDurationEl.value) * 1000;
      linearityCapture.samples = [];
      linearityCapture.result = null;
      linearityDurationEl.value = String(sanitizeLinearityDuration(linearityDurationEl.value));
      linearityRecordBtn.disabled = true;
      linearityStopBtn.disabled = false;
      saveLinearityPrefs();
      resetLinearityMetrics();
      drawLinearityChart(null);
      setLinearityStatus(`Recording ${measurement.label} for ${Math.round(linearityCapture.durationMs / 1000)} s. Keep the servo sweeping continuously.`);
    }

    function sampleLinearity(s) {
      if (!linearityCapture.recording) return;
      const measurement = getLinearityMeasurement(s);
      if (!measurement.ok) {
        stopLinearityCapture(false);
        setLinearityStatus(measurement.reason);
        return;
      }
      const elapsedMs = Date.now() - linearityCapture.startedAtMs;
      linearityCapture.samples.push({
        t: elapsedMs,
        value: measurement.value
      });
      linearityCaptureMetricEl.textContent = `${linearityCapture.samples.length} samples`;
      linearityCaptureCaptionEl.textContent = `${measurement.label} live: ${measurement.value.toFixed(currentDisplayDecimals)}${measurement.unit}.`;
      if (elapsedMs >= linearityCapture.durationMs) {
        stopLinearityCapture(true);
      } else {
        setLinearityStatus(`Recording... ${(elapsedMs / 1000).toFixed(1)} / ${(linearityCapture.durationMs / 1000).toFixed(0)} s`);
      }
    }

    function stopGuideAudio() {
      try {
        if (guideAudioGain && guideAudioCtx && guideAudioCtx.state !== 'closed') {
          guideAudioGain.gain.setTargetAtTime(0.0001, guideAudioCtx.currentTime, 0.02);
        }
      } catch (e) {
      }
    }

    function setGuideAudioMessage(text) {
      dispAudioStatusEl.textContent = text;
    }

    async function ensureGuideAudio() {
      if (guideAudioCtx) return guideAudioCtx;
      const Ctor = window.AudioContext || window.webkitAudioContext;
      if (!Ctor) return null;
      guideAudioCtx = new Ctor();
      guideAudioOsc = guideAudioCtx.createOscillator();
      guideAudioGain = guideAudioCtx.createGain();
      guideAudioOsc.type = 'sine';
      guideAudioOsc.frequency.value = 660;
      guideAudioGain.gain.value = 0.0001;
      guideAudioOsc.connect(guideAudioGain);
      guideAudioGain.connect(guideAudioCtx.destination);
      guideAudioOsc.start();
      return guideAudioCtx;
    }

    async function armGuideAudio() {
      const ctx = await ensureGuideAudio();
      if (!ctx) {
        setGuideAudioMessage('Audio unavailable in this browser.');
        return;
      }
      try {
        await ctx.resume();
        guideAudioUnlocked = true;
        dispAudioArmBtn.textContent = 'Audio Ready';
        renderDisplacement(lastLiveState);
      } catch (e) {
        setGuideAudioMessage('Audio could not be enabled.');
      }
    }

    function disableGuideAudioWithMessage(text) {
      stopGuideAudio();
      guideAudioUnlocked = false;
      dispAudioEnabledEl.checked = false;
      dispAudioArmBtn.textContent = 'Enable Audio';
      setGuideAudioMessage(text);
      saveDisplacementPrefs();
    }

    function updateGuideAudio(guide) {
      try {
        if (!dispAudioEnabledEl.checked) {
          stopGuideAudio();
          dispAudioArmBtn.textContent = guideAudioUnlocked ? 'Audio Ready' : 'Enable Audio';
          setGuideAudioMessage('Audio cues are off.');
          return;
        }
        if (!guideAudioUnlocked) {
          stopGuideAudio();
          dispAudioArmBtn.textContent = 'Enable Audio';
          setGuideAudioMessage('Tap Enable Audio to allow browser beeps.');
          return;
        }
        if (!guide || !guide.hasGuide) {
          stopGuideAudio();
          setGuideAudioMessage('Enter targets to use audio cues.');
          return;
        }
        if (!guideAudioCtx || !guideAudioOsc || !guideAudioGain || guideAudioCtx.state === 'closed') {
          disableGuideAudioWithMessage('Audio became unavailable. Re-enable it if you want browser beeps.');
          return;
        }

        const now = Date.now();
        if (guide.withinTolerance) {
          guideAudioOsc.frequency.setTargetAtTime(880, guideAudioCtx.currentTime, 0.01);
          guideAudioGain.gain.setTargetAtTime(0.035, guideAudioCtx.currentTime, 0.02);
          setGuideAudioMessage('Solid tone: within tolerance.');
          return;
        }

        if (guide.deltaAbs <= guide.approachWindow) {
          const ratio = Math.max(0, Math.min(1, (guide.approachWindow - guide.deltaAbs) / Math.max(guide.approachWindow, 0.001)));
          const intervalMs = 850 - (ratio * 620);
          guideAudioOsc.frequency.setTargetAtTime(580 + (ratio * 140), guideAudioCtx.currentTime, 0.01);
          if ((now - guideAudioLastPulseMs) >= intervalMs) {
            guideAudioLastPulseMs = now;
            guideAudioPulseUntilMs = now + 120;
          }
          const pulseActive = now < guideAudioPulseUntilMs;
          guideAudioGain.gain.setTargetAtTime(pulseActive ? 0.035 : 0.0001, guideAudioCtx.currentTime, 0.02);
          setGuideAudioMessage('Beeping faster as you approach target.');
          return;
        }

        stopGuideAudio();
        setGuideAudioMessage('Outside approach window.');
      } catch (e) {
        disableGuideAudioWithMessage('Audio cue control failed. Audio was turned off to keep the UI responsive.');
      }
    }

    function renderDisplacement(s, force = false) {
      const nowMs = Date.now();
      if (!force && (nowMs - lastDisplacementRenderMs) < DISPLACEMENT_RENDER_MIN_INTERVAL_MS) {
        return;
      }
      lastDisplacementRenderMs = nowMs;
      const label = displacementLabelText();
      const rawAxis = (dispAxisEl.value === 'pitch' || dispAxisEl.value === 'roll') ? dispAxisEl.value : 'auto';
      const axis = rawAxis === 'auto' ? resolveAutoAxis(s) : rawAxis;
      const axisText = axis === 'pitch' ? 'PITCH' : 'ROLL';
      const sourceText = axisChoiceText(rawAxis, axis, s);
      const unit = sanitizeUnitText(dispUnitEl.value);
      const depth = Number(dispDepthEl.value);
      const decimals = currentDisplayDecimals;
      const targetMode = dispTargetModeEl.value === 'angle' ? 'angle' : 'displacement';
      const targetUp = Number(dispTargetUpEl.value);
      const targetDown = Number(dispTargetDownEl.value);
      const tolerance = sanitizeTolerance(dispToleranceEl.value);
      const manualApproachWindow = sanitizePositiveOptional(dispApproachWindowEl.value);
      const metricMeta = setupMetricMeta(targetMode, unit);

      dispHeadingEl.textContent = `Live ${label}`;
      dispTargetHeadingEl.textContent = `${targetMode === 'angle' ? 'Angle' : 'Displacement'} Target`;
      dispTargetValueEl.classList.remove('ok', 'warn');

      if (!s) {
        dispValueEl.textContent = '--';
        dispAngleEl.textContent = '--';
        dispCaptionEl.textContent = 'Waiting for live angle data.';
        dispFormulaEl.textContent = 'Using live angle from the selected axis.';
        dispTargetValueEl.textContent = '--';
        dispTargetCaptionEl.textContent = 'Waiting for live angle data.';
        dispPresetHintEl.textContent = presetDefinition(dispPresetEl.value).hint;
        updateGuideAudio({ hasGuide: false });
        return;
      }

      const angle = Number(axis === 'pitch' ? s.pitch : s.roll);
      if (!Number.isFinite(angle)) {
        dispValueEl.textContent = '--';
        dispAngleEl.textContent = '--';
        dispCaptionEl.textContent = 'Selected angle is unavailable.';
        dispFormulaEl.textContent = `Source axis: ${sourceText}.`;
        dispTargetValueEl.textContent = '--';
        dispTargetCaptionEl.textContent = 'Selected angle is unavailable.';
        dispPresetHintEl.textContent = presetDefinition(dispPresetEl.value).hint;
        updateGuideAudio({ hasGuide: false });
        return;
      }

      dispAngleEl.textContent = `${angle >= 0 ? '+' : ''}${angle.toFixed(decimals)}\u00B0`;

      if (!Number.isFinite(depth) || depth <= 0) {
        dispValueEl.textContent = '--';
        dispCaptionEl.textContent = `Enter ${label.toLowerCase()} depth to calculate displacement.`;
        dispFormulaEl.textContent = `Source axis: ${sourceText}. Formula: depth \u00D7 sin(angle).`;
        if (targetMode === 'displacement') {
          dispTargetValueEl.textContent = '--';
          dispTargetCaptionEl.textContent = `Enter ${label.toLowerCase()} depth before using displacement targets.`;
          dispPresetHintEl.textContent = presetDefinition(dispPresetEl.value).hint;
          updateGuideAudio({ hasGuide: false });
          return;
        }
      }

      const displacement = Number.isFinite(depth) && depth > 0
        ? depth * Math.sin(angle * Math.PI / 180.0)
        : NaN;
      const magnitude = Number.isFinite(displacement) ? Math.abs(displacement) : NaN;
      const direction = displacement > 0 ? 'positive' : (displacement < 0 ? 'negative' : 'neutral');

      if (Number.isFinite(displacement)) {
        dispValueEl.textContent = `${displacement >= 0 ? '+' : ''}${displacement.toFixed(decimals)} ${unit}`;
        dispCaptionEl.textContent = `${label} displacement from ${axisText}; magnitude ${magnitude.toFixed(decimals)} ${unit} (${direction}).`;
        dispFormulaEl.textContent = `Source axis: ${sourceText}. Formula: ${depth.toFixed(decimals)} ${unit} \u00D7 sin(${angle.toFixed(decimals)}\u00B0).`;
      } else {
        dispValueEl.textContent = '--';
      }

      const currentValue = targetMode === 'angle' ? angle : displacement;
      const currentDirection = Number.isFinite(currentValue) && currentValue < 0 ? 'down' : 'up';
      const targetValue = currentDirection === 'down' ? targetDown : targetUp;
      const hasTargetValue = Number.isFinite(targetValue);
      const hasGuide = Number.isFinite(currentValue) && hasTargetValue;

      if (!hasTargetValue) {
        dispTargetValueEl.textContent = '--';
        dispTargetCaptionEl.textContent = `Enter ${currentDirection} target in ${metricMeta.label} mode.`;
        updateGuideAudio({ hasGuide: false });
        return;
      }

      const delta = targetValue - currentValue;
      const deltaAbs = Math.abs(delta);
      const withinTolerance = deltaAbs <= tolerance;
      const targetLabel = currentDirection === 'down' ? 'DOWN' : 'UP';
      const autoApproachWindow = Math.max(tolerance * 4.0, metricMeta.window);
      const approachWindow = Number.isFinite(manualApproachWindow) ? manualApproachWindow : autoApproachWindow;
      const deltaText = signedNumberText(delta, decimals, metricMeta.suffix);

      dispTargetValueEl.textContent = `${targetLabel} ${signedNumberText(targetValue, decimals, metricMeta.suffix)}`;
      dispTargetValueEl.classList.add(withinTolerance ? 'ok' : 'warn');
      if (withinTolerance) {
        dispTargetCaptionEl.textContent = `On target. Current ${metricMeta.label}: ${signedNumberText(currentValue, decimals, metricMeta.suffix)} within \u00B1${tolerance.toFixed(decimals)}${metricMeta.suffix}. Audio window ${approachWindow.toFixed(decimals)}${metricMeta.suffix}.`;
      } else {
        dispTargetCaptionEl.textContent = `${targetLabel} target active. Current ${metricMeta.label}: ${signedNumberText(currentValue, decimals, metricMeta.suffix)}. Delta ${deltaText}. Tolerance \u00B1${tolerance.toFixed(decimals)}${metricMeta.suffix}. Audio window ${approachWindow.toFixed(decimals)}${metricMeta.suffix}.`;
      }

      updateGuideAudio({
        hasGuide,
        withinTolerance,
        deltaAbs,
        approachWindow
      });
      dispPresetHintEl.textContent = `${presetDefinition(dispPresetEl.value).hint} Current source: ${sourceText}.`;
    }

    function vec2(x, y, d = 3) {
      return `(${f(x, d)},${f(y, d)})`;
    }

    function vec3(x, y, z, d = 3) {
      return `(${f(x, d)},${f(y, d)},${f(z, d)})`;
    }

    function workflowText(s) {
      const active = [];
      if (s.zero_active) active.push('ZERO');
      if (s.offset_cal_active) active.push('OFFSET CAL');
      if (s.mode_active) active.push('MODE');
      if (s.align_active) active.push('ALIGN');
      return active.length ? active.join(', ') : 'none';
    }

    function renderDiag(s) {
      const valid = !!s.diag_valid;
      diagValidEl.classList.remove('ok', 'warn');
      diagValidEl.classList.add(valid ? 'ok' : 'warn');
      diagValidEl.textContent = valid ? 'diag valid' : 'diag missing';
      diagWorkflowsEl.textContent = `workflows: ${workflowText(s)}`;
      diagRawAEl.textContent = vec3(s.sens_ax, s.sens_ay, s.sens_az, 3);
      diagRawGEl.textContent = vec3(s.sens_gx, s.sens_gy, s.sens_gz, 3);
      diagMapAEl.textContent = vec3(s.map_ax, s.map_ay, s.map_az, 3);
      diagMapGEl.textContent = vec2(s.map_gx, s.map_gy, 3);
      diagCorrAEl.textContent = vec3(s.corr_ax, s.corr_ay, s.corr_az, 3);
      diagCorrGEl.textContent = vec2(s.corr_gx, s.corr_gy, 3);
      diagPhysEl.textContent = `r=${f(s.phys_roll, 2)}\u00B0  p=${f(s.phys_pitch, 2)}\u00B0`;
      diagCondEl.textContent = `${f(s.roll_cond_pct, 0)}%${s.roll_cond_low ? ' (low)' : ''}`;
      diagBiasAEl.textContent = vec3(s.bias_ax, s.bias_ay, s.bias_az, 4);
      diagBiasGEl.textContent = vec2(s.bias_gx, s.bias_gy, 4);
      diagZeroEl.textContent = vec2(s.zero_roll, s.zero_pitch, 3);
      diagAlignEl.textContent = vec2(s.align_roll, s.align_pitch, 3);
    }

    function clearDiag(disconnected = false) {
      diagValidEl.classList.remove('ok', 'warn');
      diagValidEl.classList.add('warn');
      diagValidEl.textContent = disconnected ? 'diag disconnected' : 'diag missing';
      diagWorkflowsEl.textContent = 'workflows: --';
      diagRawAEl.textContent = '--';
      diagRawGEl.textContent = '--';
      diagMapAEl.textContent = '--';
      diagMapGEl.textContent = '--';
      diagCorrAEl.textContent = '--';
      diagCorrGEl.textContent = '--';
      diagPhysEl.textContent = '--';
      diagCondEl.textContent = '--';
      diagBiasAEl.textContent = '--';
      diagBiasGEl.textContent = '--';
      diagZeroEl.textContent = '--';
      diagAlignEl.textContent = '--';
    }

    function renderNetwork(s) {
      currentDisplayDecimals = sanitizeDisplayPrecision(s.display_precision);
      const brightnessPct = sanitizeDisplayBrightness(s.display_brightness_pct);
      const batteryModeText = String(s.battery_mode || 'auto').toUpperCase();
      const zeroOnBoot = (s.zero_on_boot !== false);
      const touchEnabled = (s.touch_enabled !== false);
      const touchPersist = !!s.touch_persist;
      const deviceBits = [`Battery ${batteryModeText}`, `Startup ZERO ${zeroOnBoot ? 'ON' : 'OFF'}`];
      deviceBits.push(`Readout ${currentDisplayDecimals}dp`);
      deviceBits.push(`Brightness ${brightnessPct}%`);
      deviceBits.push(`Touch ${touchEnabled ? 'ON' : 'OFF'}${touchPersist ? ' (persist)' : ''}`);
      deviceStatusEl.textContent = deviceBits.join(' | ');

      const mode = String(s.net_mode || 'AP');
      const pref = String(s.net_pref || 'ap').toUpperCase();
      const statusBits = [`Mode ${mode}`, `Preference ${pref}`];
      if (s.sta_connected) statusBits.push('STA connected');
      if (s.ap_active) statusBits.push('AP active');
      netStatusEl.textContent = statusBits.join(' | ');

      const addrBits = [];
      if (s.sta_ip) addrBits.push(`STA ${s.sta_ip}`);
      if (s.ap_ip) addrBits.push(`AP ${s.ap_ip} (${s.ap_ssid || 'AP'})`);
      if (s.hostname_local) addrBits.push(`Host ${s.hostname_local}`);
      netAddrEl.textContent = addrBits.join(' | ');

      if (!deviceFormDirty) {
        const batteryMode = String(s.battery_mode || 'auto').toLowerCase();
        deviceBatteryModeEl.value = (batteryMode === 'present' || batteryMode === 'absent') ? batteryMode : 'auto';
        deviceZeroOnBootEl.value = (s.zero_on_boot === false) ? 'off' : 'on';
        deviceDisplayPrecisionEl.value = String(currentDisplayDecimals);
        deviceTouchEnabledEl.value = touchEnabled ? 'on' : 'off';
        deviceTouchPersistEl.checked = touchPersist;
        deviceDisplayBrightnessEl.value = String(brightnessPct);
        syncBrightnessLabel();
      }

      if (!networkFormDirty) {
        netModeEl.value = (String(s.net_pref || 'ap').toLowerCase() === 'sta') ? 'sta' : 'ap';
        netHostnameEl.value = s.hostname || '';
        netSsidEl.value = s.sta_ssid || '';
      }
    }

    function applyLiveState(s) {
      currentDisplayDecimals = sanitizeDisplayPrecision(s.display_precision);
      lastLiveState = s;
      rollEl.textContent = fmt(s.roll);
      pitchEl.textContent = fmt(s.pitch);
      setAngleClass(rollEl, s.roll);
      setAngleClass(pitchEl, s.pitch);
      if (s.fw) {
        fwEl.textContent = s.fw;
        if (!otaVersionEl.value.trim()) {
          otaVersionEl.value = String(s.fw);
        }
      }
      const batteryKnown = !!s.battery_valid;
      const batteryPresent = (s.battery_present !== false);
      const batteryPresentInferred = !!s.battery_present_inferred;
      const batteryConfiguredAbsent = (!batteryPresent && !batteryPresentInferred);
      const batteryPct = Number(s.battery_soc_pct || 0).toFixed(0);
      const batteryVolts = Number(s.battery_voltage_v || 0).toFixed(1);
      const batteryState = ((!batteryPresent && batteryPresentInferred) ? 'BAT?' : (s.battery_charging ? 'CHG' : 'BAT'));
      const batteryStatus = batteryConfiguredAbsent
        ? ''
        : (batteryKnown ? ` | ${batteryState} ${batteryPct}% ${batteryVolts} V` : ' | BAT --');
      statusEl.textContent =
        `${s.orientation} | ${s.axis} | ROT ${s.rotation} | ${s.live}${batteryStatus}`;
      const axisText = String(s.axis || '').toUpperCase();
      const axis = (axisText === 'ROLL' || s.axis_id === 1) ? 'ROLL'
                 : (axisText === 'PITCH' || s.axis_id === 2) ? 'PITCH'
                 : 'BOTH';
      setAxisLayout(axis);
      if (s.roll_cond_low) {
        condEl.textContent = `ROLL conditioning reduced (${Number(s.roll_cond_pct || 0).toFixed(0)}%) near high pitch`;
      } else {
        condEl.textContent = '';
      }
      if (batteryKnown) {
        if (batteryConfiguredAbsent) {
          batEl.textContent = '';
        } else if (!batteryPresent && batteryPresentInferred) {
          batEl.textContent = `Battery likely not connected (inferred; rail ${batteryVolts} V)`;
        } else {
          batEl.textContent = s.battery_charging
            ? `Battery charging (${batteryVolts} V, ${batteryPct}%)`
            : `Battery not charging (${batteryVolts} V, ${batteryPct}%)`;
        }
      } else {
        batEl.textContent = 'Battery telemetry unavailable';
      }
      if (surfaceDisplacementCardEl && surfaceDisplacementCardEl.open) renderDisplacement(s);
      sampleLinearity(s);
    }

    async function refreshLive() {
      if (otaUploadInFlight || liveRefreshInFlight) return;
      liveRefreshInFlight = true;
      try {
        const s = await fetchJsonWithTimeout('/api/live', LIVE_FETCH_TIMEOUT_MS);
        liveFailureCount = 0;
        applyLiveState(s);
      } catch (e) {
        liveFailureCount += 1;
        if (liveFailureCount >= 3) {
          statusEl.textContent = 'Disconnected';
          batEl.textContent = 'Battery telemetry unavailable';
          lastLiveState = null;
          if (surfaceDisplacementCardEl && surfaceDisplacementCardEl.open) renderDisplacement(null, true);
        }
      } finally {
        liveRefreshInFlight = false;
        scheduleLivePoll(nextLivePollDelay());
      }
    }

    async function refreshState() {
      if (otaUploadInFlight || stateRefreshInFlight) return;
      stateRefreshInFlight = true;
      try {
        const s = await fetchJsonWithTimeout('/api/state', STATE_FETCH_TIMEOUT_MS);
        stateFailureCount = 0;
        currentDisplayDecimals = sanitizeDisplayPrecision(s.display_precision);
        if (s.fw) {
          fwEl.textContent = s.fw;
          if (!otaVersionEl.value.trim()) {
            otaVersionEl.value = String(s.fw);
          }
        }
        if (diagnosticsCardEl && diagnosticsCardEl.open) renderDiag(s);
        normalControls.classList.toggle('hidden', s.mode_active || s.align_active || s.offset_cal_active || s.zero_active);
        modeControls.classList.toggle('hidden', !s.mode_active || s.align_active || s.offset_cal_active || s.zero_active);
        alignControls.classList.toggle('hidden', !s.align_active || s.offset_cal_active || s.zero_active);
        offsetCalControls.classList.toggle('hidden', !(s.offset_cal_active || s.zero_active));
        progressWrap.classList.add('hidden');
        progressBar.style.width = '0%';
        if (s.align_active) {
          msgEl.textContent = s.align_instruction || 'ALIGN active';
          if (s.align_capture_active) {
            progressWrap.classList.remove('hidden');
            progressBar.style.width = `${Math.max(0, Math.min(100, s.align_capture_pct || 0))}%`;
          }
        } else if (s.mode_active) {
          if (s.mode_confirmed) {
            msgEl.textContent = `MODE: hold still ${s.mode_rem_s.toFixed(1)} s`;
            progressWrap.classList.remove('hidden');
            progressBar.style.width = `${Math.max(0, Math.min(100, s.mode_progress_pct || 0))}%`;
          } else {
            msgEl.textContent = `MODE target: ${s.mode_target}`;
          }
        } else if (s.zero_active) {
          if (s.zero_confirmed) {
            msgEl.textContent = `ZERO: hold still ${s.zero_rem_s.toFixed(1)} s`;
            progressWrap.classList.remove('hidden');
            progressBar.style.width = `${Math.max(0, Math.min(100, s.zero_progress_pct || 0))}%`;
          } else {
            msgEl.textContent = 'ZERO: press CONFIRM and hold still';
          }
        } else if (s.offset_cal_active) {
          if (s.offset_cal_confirmed) {
            msgEl.textContent = `OFFSET CAL: hold still ${s.offset_cal_rem_s.toFixed(1)} s`;
            progressWrap.classList.remove('hidden');
            progressBar.style.width = `${Math.max(0, Math.min(100, s.offset_cal_progress_pct || 0))}%`;
          } else {
            msgEl.textContent = `OFFSET CAL target: ${s.offset_cal_target}`;
          }
        } else {
          msgEl.textContent = '';
        }
      } catch (e) {
        stateFailureCount += 1;
        if (stateFailureCount >= 2 && diagnosticsCardEl && diagnosticsCardEl.open) clearDiag(true);
      } finally {
        stateRefreshInFlight = false;
        scheduleStatePoll(nextStatePollDelay(lastLiveState));
      }
    }

    async function refreshNetwork() {
      if (otaUploadInFlight || networkRefreshInFlight) return;
      networkRefreshInFlight = true;
      try {
        const s = await fetchJsonWithTimeout('/api/network', NETWORK_FETCH_TIMEOUT_MS);
        networkFailureCount = 0;
        renderNetwork(s);
      } catch (e) {
        networkFailureCount += 1;
        if (networkFailureCount >= 2) netStatusEl.textContent = 'Network status unavailable';
      } finally {
        networkRefreshInFlight = false;
        scheduleNetworkPoll(nextNetworkPollDelay());
      }
    }

    async function saveNetwork() {
      const mode = netModeEl.value === 'sta' ? 'sta' : 'ap';
      const ssid = netSsidEl.value.trim();
      const hostname = netHostnameEl.value.trim();
      const password = netPasswordEl.value;

      if (mode === 'sta' && !ssid) {
        netMsgEl.textContent = 'STA mode requires an SSID.';
        return;
      }

      netSaveBtn.disabled = true;
      netMsgEl.textContent = 'Saving network...';
      try {
        const body = new URLSearchParams();
        body.set('mode', mode);
        body.set('ssid', ssid);
        body.set('hostname', hostname);
        if (password.length > 0) body.set('password', password);

        const r = await fetch('/api/network', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body.toString()
        });
        const out = await r.json();
        if (out.ok) {
          netMsgEl.textContent = 'Network settings applied.';
          netPasswordEl.value = '';
          networkFormDirty = false;
        } else {
          netMsgEl.textContent = `Network save failed: ${out.error || 'unknown error'}`;
        }
      } catch (e) {
        netMsgEl.textContent = 'Network save failed.';
      } finally {
        netSaveBtn.disabled = false;
      }
      scheduleNetworkPoll(120);
      scheduleStatePoll(120);
      scheduleLivePoll(120);
    }

    async function saveDeviceSettings() {
      const batteryMode = deviceBatteryModeEl.value === 'present'
        ? 'present'
        : (deviceBatteryModeEl.value === 'absent' ? 'absent' : 'auto');
      const zeroOnBoot = (deviceZeroOnBootEl.value === 'off') ? 'off' : 'on';
      const displayPrecision = String(sanitizeDisplayPrecision(deviceDisplayPrecisionEl.value));
      const touchEnabled = deviceTouchEnabledEl.value === 'off' ? 'off' : 'on';
      const touchPersist = deviceTouchPersistEl.checked ? 'on' : 'off';
      const displayBrightness = String(sanitizeDisplayBrightness(deviceDisplayBrightnessEl.value));

      deviceSaveBtn.disabled = true;
      deviceMsgEl.textContent = 'Saving device settings...';
      try {
        const body = new URLSearchParams();
        body.set('battery_mode', batteryMode);
        body.set('zero_on_boot', zeroOnBoot);
        body.set('display_precision', displayPrecision);
        body.set('touch_enabled', touchEnabled);
        body.set('touch_persist', touchPersist);
        body.set('display_brightness', displayBrightness);

        const r = await fetch('/api/network', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body.toString()
        });
        const out = await r.json();
        if (out.ok) {
          deviceMsgEl.textContent = 'Device settings applied.';
          deviceFormDirty = false;
        } else {
          deviceMsgEl.textContent = `Device save failed: ${out.error || 'unknown error'}`;
        }
      } catch (e) {
        deviceMsgEl.textContent = 'Device save failed.';
      } finally {
        deviceSaveBtn.disabled = false;
      }
      scheduleNetworkPoll(120);
      scheduleStatePoll(120);
      scheduleLivePoll(120);
    }

    async function recoverNetwork() {
      if (!confirm('Reset to AP-only mode and clear saved STA credentials?')) return;
      netRecoverBtn.disabled = true;
      netMsgEl.textContent = 'Applying AP recovery mode...';
      try {
        const body = new URLSearchParams();
        body.set('wipe', '1');
        const r = await fetch('/api/network/recover', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: body.toString()
        });
        const out = await r.json();
        if (out.ok) {
          networkFormDirty = false;
          netPasswordEl.value = '';
          netMsgEl.textContent = 'AP recovery mode enabled. STA credentials cleared.';
        } else {
          netMsgEl.textContent = `Recovery failed: ${out.error || 'unknown error'}`;
        }
      } catch (e) {
        netMsgEl.textContent = 'Recovery request failed.';
      } finally {
        netRecoverBtn.disabled = false;
      }
      scheduleNetworkPoll(120);
      scheduleStatePoll(120);
      scheduleLivePoll(120);
    }

    function inferVersionFromFilename(name) {
      const m = String(name || '').match(/(\d{4}\.\d+\.\d+)/);
      return m ? m[1] : '';
    }

    async function computeSha256Hex(file) {
      if (!window.crypto || !window.crypto.subtle) {
        throw new Error('This browser does not support WebCrypto SHA-256');
      }
      const buf = await file.arrayBuffer();
      const hash = await crypto.subtle.digest('SHA-256', buf);
      const bytes = new Uint8Array(hash);
      let hex = '';
      for (let i = 0; i < bytes.length; i += 1) {
        hex += bytes[i].toString(16).padStart(2, '0');
      }
      return hex;
    }

    async function onOtaFileSelected() {
      const file = otaFileEl.files && otaFileEl.files[0];
      otaSha256El.value = '';
      if (!file) return;

      if (!otaVersionEl.value.trim()) {
        const inferred = inferVersionFromFilename(file.name);
        if (inferred) otaVersionEl.value = inferred;
      }

      if (!window.crypto || !window.crypto.subtle) {
        otaMsgEl.textContent = 'WebCrypto SHA-256 unavailable. Paste SHA-256 manually (PowerShell: Get-FileHash -Algorithm SHA256).';
        return;
      }

      otaMsgEl.textContent = 'Calculating SHA-256...';
      otaProgressBarEl.style.width = '0%';
      try {
        const sha = await computeSha256Hex(file);
        otaSha256El.value = sha;
        otaMsgEl.textContent = 'SHA-256 ready.';
      } catch (e) {
        otaSha256El.value = '';
        otaMsgEl.textContent = `Checksum error: ${e.message || 'failed to compute SHA-256'}`;
      }
    }

    async function uploadOta() {
      const file = otaFileEl.files && otaFileEl.files[0];
      if (!file) {
        otaMsgEl.textContent = 'Select a firmware .bin file first.';
        return;
      }

      const version = otaVersionEl.value.trim();
      if (!/^\d{4}\.\d+\.\d+$/.test(version)) {
        otaMsgEl.textContent = 'Enter firmware version in format YYYY.M.X (for example 2026.3.8).';
        return;
      }

      let sha256 = otaSha256El.value.trim().toLowerCase();
      if (!/^[0-9a-f]{64}$/.test(sha256)) {
        if (!window.crypto || !window.crypto.subtle) {
          otaMsgEl.textContent = 'This browser/context cannot compute SHA-256. Paste a 64-char SHA-256 first.';
          return;
        }
        otaMsgEl.textContent = 'Calculating SHA-256 before upload...';
        try {
          sha256 = await computeSha256Hex(file);
          otaSha256El.value = sha256;
        } catch (e) {
          otaMsgEl.textContent = `Checksum error: ${e.message || 'failed to compute SHA-256'}`;
          return;
        }
      }

      const force = !!otaForceEl.checked;

      otaUploadInFlight = true;
      otaBtnEl.disabled = true;
      otaMsgEl.textContent = 'Uploading firmware...';
      otaProgressBarEl.style.width = '0%';

      const form = new FormData();
      form.append('firmware', file, file.name);

      const xhr = new XMLHttpRequest();
      const uploadUrl = `/api/ota/upload?version=${encodeURIComponent(version)}&sha256=${encodeURIComponent(sha256)}&force=${force ? '1' : '0'}`;
      xhr.open('POST', uploadUrl);
      xhr.upload.onprogress = (e) => {
        if (!e.lengthComputable || e.total <= 0) return;
        const pct = Math.max(0, Math.min(100, Math.round((e.loaded * 100) / e.total)));
        otaProgressBarEl.style.width = `${pct}%`;
      };
      xhr.onload = () => {
        otaUploadInFlight = false;
        otaBtnEl.disabled = false;
        let out = {};
        try { out = JSON.parse(xhr.responseText || '{}'); } catch (_) {}
        if (xhr.status >= 200 && xhr.status < 300 && out.ok) {
          otaMsgEl.textContent = 'OTA complete. Device rebooting...';
        } else {
          otaMsgEl.textContent = `OTA failed: ${out.error || 'unknown error'}`;
          otaProgressBarEl.style.width = '0%';
        }
      };
      xhr.onerror = () => {
        otaUploadInFlight = false;
        otaBtnEl.disabled = false;
        otaMsgEl.textContent = 'OTA upload failed.';
        otaProgressBarEl.style.width = '0%';
      };
      xhr.send(form);
    }

    async function sendCmd(cmd) {
      try {
        const r = await fetch('/api/cmd', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ cmd })
        });
        const out = await r.json();
        msgEl.textContent = out.ok ? `OK: ${out.cmd}` : `Error: ${out.error}`;
      } catch (e) {
        msgEl.textContent = 'Command failed';
      }
      scheduleLivePoll(80);
      scheduleStatePoll(80);
    }

    loadWebTheme();
    loadDisplacementPrefs();
    loadLinearityPrefs();
    syncBrightnessLabel();
    if (surfaceDisplacementCardEl && surfaceDisplacementCardEl.open) renderDisplacement(null);
    if (linearityCardEl && linearityCardEl.open) drawLinearityChart(null);
    refreshLive();
    refreshState();
    refreshNetwork();
  </script>
</body>
</html>
)HTML";