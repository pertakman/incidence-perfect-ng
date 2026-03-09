#include "remote_control.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <ctype.h>
#include <mbedtls/sha256.h>
#include <string.h>

#include "fw_version.h"
#include "inclinometer_shared.h"
#include "ui_lvgl.h"

namespace {

WebServer server(80);
bool remote_ready = false;
char ap_ssid[32] = {0};
const char *ap_password = "incidence-ng";

constexpr const char *kPrefsNs = "remote";
constexpr const char *kPrefsMode = "mode";
constexpr const char *kPrefsSsid = "sta_ssid";
constexpr const char *kPrefsPass = "sta_pass";
constexpr const char *kPrefsHost = "host";
constexpr const char *kPrefsBatteryMode = "battery_mode";
constexpr int kActionButtonPin = 0;

constexpr unsigned long kStaConnectTimeoutMs = 12000UL;
constexpr unsigned long kStaRetryIntervalMs = 30000UL;
constexpr unsigned long kNetworkRecoveryHoldMs = 1800UL;
constexpr unsigned long kNetworkRecoveryEntryGuardMs = 250UL;

enum NetworkRunMode {
  RUN_AP_ONLY = 0,
  RUN_STA_ONLY = 1,
  RUN_AP_FALLBACK = 2
};

struct NetworkConfig {
  bool prefer_sta;
  char sta_ssid[33];
  char sta_password[65];
  char hostname[33];
};

NetworkConfig net_cfg = {};
NetworkRunMode net_run_mode = RUN_AP_ONLY;
bool ap_active = false;
bool sta_connected = false;
bool sta_attempt_active = false;
bool mdns_active = false;
unsigned long sta_attempt_started_ms = 0;
unsigned long next_sta_retry_ms = 0;

bool ota_upload_ok = false;
bool ota_upload_in_progress = false;
char ota_error[160] = {0};
bool ota_force_install = false;
bool ota_sha_ctx_active = false;
bool ota_expected_sha256_set = false;
char ota_expected_sha256[65] = {0};
char ota_computed_sha256[65] = {0};
char ota_requested_version[24] = {0};
size_t ota_written_bytes = 0;
mbedtls_sha256_context ota_sha_ctx;

// Keep large response buffers off the request-handler stack to avoid
// stack pressure/reset loops when the web page polls rapidly.
char state_json_buf[4608];
char state_align_instruction_buf[192];
char state_fw_esc[32];
char state_orient_esc[24];
char state_axis_esc[12];
char state_live_esc[12];
char state_align_esc[320];
char state_mode_target_esc[24];
char state_offset_cal_target_esc[24];

const char PAGE_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Incidence Perfect NG</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 16px; background: #0b1020; color: #eef4ff; }
    h1 { margin: 0 0 8px 0; font-size: 24px; }
    .muted { color: #9fb3d9; margin-bottom: 12px; }
    .card { background: #141c34; border-radius: 10px; padding: 14px; margin-bottom: 12px; }
    .card summary { cursor: pointer; font-size: 18px; font-weight: 700; }
    .row { display: flex; gap: 10px; flex-wrap: wrap; }
    .metric-card { flex: 1; min-width: 130px; text-align: center; }
    #metricsRow.single-axis { justify-content: center; }
    #metricsRow.single-axis > .metric-card { flex: 0 1 300px; max-width: 300px; }
    .value { font-size: 36px; font-weight: bold; margin-top: 4px; transition: color .12s linear; }
    .value.warn { color: #FFBF00; }
    .value.crit { color: #DC143C; }
    button {
      border: 0; border-radius: 8px; padding: 12px 14px; min-width: 90px;
      background: #2a7fff; color: white; font-weight: 700; cursor: pointer;
    }
    button:disabled { opacity: .55; cursor: not-allowed; }
    button:active { transform: scale(0.98); }
    input, select {
      width: 100%; box-sizing: border-box;
      border: 1px solid #31406f; border-radius: 8px;
      background: #0d1733; color: #eef4ff; padding: 10px;
    }
    input[type="checkbox"] { width: auto; padding: 0; }
    label { display: block; }
    #msg { min-height: 18px; color: #8cf0c6; margin-top: 8px; }
    .hidden { display: none; }
    .bar { height: 8px; background: #2e354f; border-radius: 99px; overflow: hidden; margin-top: 10px; }
    .bar > div { height: 100%; width: 0%; background: #6fd3ff; transition: width .12s linear; }
    code { color: #8ec7ff; }
    .diag-layout { margin-top: 10px; display: grid; gap: 10px; }
    .diag-pills { display: flex; gap: 8px; flex-wrap: wrap; }
    .pill {
      display: inline-flex; align-items: center; border-radius: 999px; padding: 4px 10px;
      font-size: 11px; font-weight: 700; letter-spacing: .03em; text-transform: uppercase;
      background: #1c2748; color: #c9d8ff;
    }
    .pill.ok { background: #193a2a; color: #8df1c7; }
    .pill.warn { background: #4a2d2d; color: #ffd1d1; }
    .diag-grid { display: grid; gap: 8px; grid-template-columns: 1fr; }
    @media (min-width: 640px) { .diag-grid { grid-template-columns: 1fr 1fr; } }
    .diag-block { background: #101933; border: 1px solid #223056; border-radius: 8px; padding: 8px 10px; }
    .diag-block h3 {
      margin: 0 0 6px 0; font-size: 12px; letter-spacing: .04em;
      text-transform: uppercase; color: #9fb3d9;
    }
    .diag-row {
      display: grid; grid-template-columns: 68px minmax(0, 1fr);
      gap: 10px; margin: 3px 0; align-items: baseline;
    }
    .diag-row span { color: #9fb3d9; font-size: 12px; }
    .diag-row code {
      color: #d9e6ff; font-size: 12px; text-align: right;
      white-space: normal; word-break: break-word; overflow-wrap: anywhere;
    }
    @media (max-width: 460px) {
      .diag-row { grid-template-columns: 1fr; gap: 2px; }
      .diag-row code { text-align: left; }
    }
    .diag-note { color: #8ca2d3; font-size: 11px; margin: 0; }
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
      <label style="min-width:170px;">
        <div class="muted" style="margin:0 0 4px 0;">Battery</div>
        <select id="netBatteryMode">
          <option value="auto">Auto detect</option>
          <option value="present">Installed</option>
          <option value="absent">No battery installed</option>
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
      <button id="netSaveBtn" onclick="saveNetwork()">Save Network</button>
      <button id="netRecoverBtn" onclick="recoverNetwork()">Recover AP Mode</button>
    </div>
    <div id="netMsg" class="muted" style="margin-top:8px;"></div>
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
    const netStatusEl = document.getElementById('netStatus');
    const netAddrEl = document.getElementById('netAddr');
    const netModeEl = document.getElementById('netMode');
    const netBatteryModeEl = document.getElementById('netBatteryMode');
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
    let networkFormDirty = false;
    let otaUploadInFlight = false;

    [netModeEl, netBatteryModeEl, netHostnameEl, netSsidEl, netPasswordEl].forEach((el) => {
      el.addEventListener('input', () => { networkFormDirty = true; });
    });
    otaFileEl.addEventListener('change', onOtaFileSelected);

    function fmt(v) {
      const n = Number(v);
      if (Number.isNaN(n)) return '--';
      return (n >= 0 ? '+' : '') + n.toFixed(2) + '\u00B0';
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
      const mode = String(s.net_mode || 'AP');
      const pref = String(s.net_pref || 'ap').toUpperCase();
      const batteryMode = String(s.battery_mode || 'auto').toUpperCase();
      const statusBits = [`Mode ${mode}`, `Preference ${pref}`];
      statusBits.push(`Battery ${batteryMode}`);
      if (s.sta_connected) statusBits.push('STA connected');
      if (s.ap_active) statusBits.push('AP active');
      netStatusEl.textContent = statusBits.join(' | ');

      const addrBits = [];
      if (s.sta_ip) addrBits.push(`STA ${s.sta_ip}`);
      if (s.ap_ip) addrBits.push(`AP ${s.ap_ip} (${s.ap_ssid || 'AP'})`);
      if (s.hostname_local) addrBits.push(`Host ${s.hostname_local}`);
      netAddrEl.textContent = addrBits.join(' | ');

      if (!networkFormDirty) {
        netModeEl.value = (String(s.net_pref || 'ap').toLowerCase() === 'sta') ? 'sta' : 'ap';
        const batteryMode = String(s.battery_mode || 'auto').toLowerCase();
        netBatteryModeEl.value = (batteryMode === 'present' || batteryMode === 'absent') ? batteryMode : 'auto';
        netHostnameEl.value = s.hostname || '';
        netSsidEl.value = s.sta_ssid || '';
      }
    }

    function applyLiveState(s) {
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
    }

    async function refreshLive() {
      if (otaUploadInFlight) return;
      try {
        const r = await fetch('/api/live', { cache: 'no-store' });
        const s = await r.json();
        applyLiveState(s);
      } catch (e) {
        statusEl.textContent = 'Disconnected';
        batEl.textContent = 'Battery telemetry unavailable';
      }
    }

    async function refreshState() {
      if (otaUploadInFlight) return;
      try {
        const r = await fetch('/api/state', { cache: 'no-store' });
        const s = await r.json();
        applyLiveState(s);
        renderDiag(s);
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
        clearDiag(true);
      }
    }

    async function refreshNetwork() {
      if (otaUploadInFlight) return;
      try {
        const r = await fetch('/api/network', { cache: 'no-store' });
        const s = await r.json();
        renderNetwork(s);
      } catch (e) {
        netStatusEl.textContent = 'Network status unavailable';
      }
    }

    async function saveNetwork() {
      const mode = netModeEl.value === 'sta' ? 'sta' : 'ap';
      const batteryMode = netBatteryModeEl.value === 'present'
        ? 'present'
        : (netBatteryModeEl.value === 'absent' ? 'absent' : 'auto');
      const ssid = netSsidEl.value.trim();
      const hostname = netHostnameEl.value.trim();
      const password = netPasswordEl.value;

      if (mode === 'sta' && !ssid) {
        netMsgEl.textContent = 'STA mode requires an SSID.';
        return;
      }

      netSaveBtn.disabled = true;
      netMsgEl.textContent = 'Saving network settings...';
      try {
        const body = new URLSearchParams();
        body.set('mode', mode);
        body.set('battery_mode', batteryMode);
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
          netMsgEl.textContent = `Save failed: ${out.error || 'unknown error'}`;
        }
      } catch (e) {
        netMsgEl.textContent = 'Network save failed.';
      } finally {
        netSaveBtn.disabled = false;
      }
      setTimeout(refreshNetwork, 120);
      setTimeout(refreshState, 120);
      setTimeout(refreshLive, 120);
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
      setTimeout(refreshNetwork, 120);
      setTimeout(refreshState, 120);
      setTimeout(refreshLive, 120);
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
      setTimeout(refreshLive, 80);
      setTimeout(refreshState, 80);
    }

    refreshLive();
    refreshState();
    refreshNetwork();
    setInterval(refreshLive, 120);
    setInterval(refreshState, 600);
    setInterval(refreshNetwork, 3000);
  </script>
</body>
</html>
)HTML";

uint16_t board_suffix_from_mac() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  // FNV-1a over full MAC gives a stable per-board suffix and avoids
  // endianness pitfalls from masking ESP.getEfuseMac().
  uint32_t hash = 2166136261u;
  for (int i = 0; i < 6; ++i) {
    hash ^= (uint32_t)mac[i];
    hash *= 16777619u;
  }
  return (uint16_t)(hash & 0xFFFFu);
}

void log_board_identity() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  const uint16_t suffix = board_suffix_from_mac();
  Serial.print("[remote] STA MAC: ");
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n",
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print("[remote] Board suffix: ");
  Serial.printf("%04X\n", suffix);
}

const char *orientation_text() {
  return (orientationMode == MODE_SCREEN_VERTICAL) ? "SCREEN VERTICAL" : "SCREEN UP";
}

const char *orientation_text_for(OrientationMode m) {
  return (m == MODE_SCREEN_VERTICAL) ? "SCREEN VERTICAL" : "SCREEN UP";
}

const char *axis_text() {
  switch (getAxisMode()) {
    case AXIS_ROLL: return "ROLL";
    case AXIS_PITCH: return "PITCH";
    default: return "BOTH";
  }
}

void copy_cstr(char *dst, size_t dst_size, const char *src) {
  if (!dst || dst_size == 0) return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, dst_size - 1);
  dst[dst_size - 1] = '\0';
}

void append_if_room(char *dst, size_t dst_size, char c, size_t &len) {
  if (len + 1 >= dst_size) return;
  dst[len++] = c;
  dst[len] = '\0';
}

void build_default_ap_ssid() {
  const uint16_t suffix = board_suffix_from_mac();
  snprintf(ap_ssid, sizeof(ap_ssid), "IncidencePerfectNG-%04X", suffix);
}

void build_default_hostname(char *dst, size_t dst_size) {
  if (!dst || dst_size == 0) return;
  const uint16_t suffix = board_suffix_from_mac();
  snprintf(dst, dst_size, "incidence-perfect-ng-%04x", suffix);
}

const char *battery_presence_mode_to_pref(BatteryPresenceMode mode) {
  switch (mode) {
    case BATTERY_PRESENCE_FORCE_PRESENT: return "present";
    case BATTERY_PRESENCE_FORCE_ABSENT: return "absent";
    default: return "auto";
  }
}

BatteryPresenceMode parse_battery_presence_mode(const String &raw) {
  String s = raw;
  s.trim();
  s.toLowerCase();
  if (s == "present" || s == "installed" || s == "on") {
    return BATTERY_PRESENCE_FORCE_PRESENT;
  }
  if (s == "absent" || s == "none" || s == "off") {
    return BATTERY_PRESENCE_FORCE_ABSENT;
  }
  return BATTERY_PRESENCE_AUTO;
}

void sanitize_hostname(const String &raw, char *dst, size_t dst_size) {
  char fallback[33];
  build_default_hostname(fallback, sizeof(fallback));

  if (!dst || dst_size == 0) return;
  dst[0] = '\0';

  String s = raw;
  s.trim();
  s.toLowerCase();
  if (s.endsWith(".local")) {
    s = s.substring(0, s.length() - 6);
  }

  size_t len = 0;
  bool prev_dash = false;
  for (size_t i = 0; i < (size_t)s.length(); ++i) {
    const char c = s[i];
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      append_if_room(dst, dst_size, c, len);
      prev_dash = false;
    } else {
      if (!prev_dash && len > 0) {
        append_if_room(dst, dst_size, '-', len);
        prev_dash = true;
      }
    }
  }

  while (len > 0 && dst[len - 1] == '-') {
    dst[--len] = '\0';
  }

  if (len == 0) {
    copy_cstr(dst, dst_size, fallback);
  }
}

bool parse_bool_flag(const String &raw) {
  String s = raw;
  s.trim();
  s.toLowerCase();
  return (s == "1" || s == "true" || s == "yes" || s == "on");
}

bool normalize_sha256_hex(const String &raw, char *dst, size_t dst_size) {
  if (!dst || dst_size < 65) return false;
  String s = raw;
  s.trim();
  s.toLowerCase();
  if (s.length() != 64) return false;
  for (int i = 0; i < 64; ++i) {
    const char c = s[i];
    const bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    if (!hex) return false;
    dst[i] = c;
  }
  dst[64] = '\0';
  return true;
}

bool parse_version_triplet(const char *src, int &year, int &minor, int &patch) {
  if (!src || src[0] == '\0') return false;
  const char *p = src;

  auto parse_uint = [&](int &out) -> bool {
    if (!isdigit((unsigned char)*p)) return false;
    long v = 0;
    while (isdigit((unsigned char)*p)) {
      v = (v * 10L) + (long)(*p - '0');
      if (v > 99999L) return false;
      ++p;
    }
    out = (int)v;
    return true;
  };

  if (!parse_uint(year)) return false;
  if (*p != '.') return false;
  ++p;
  if (!parse_uint(minor)) return false;
  if (*p != '.') return false;
  ++p;
  if (!parse_uint(patch)) return false;
  return true; // allow optional suffix after numeric triplet
}

int compare_version_triplet(const char *lhs, const char *rhs, bool *ok = nullptr) {
  int ly = 0, lm = 0, lp = 0;
  int ry = 0, rm = 0, rp = 0;
  const bool lhs_ok = parse_version_triplet(lhs, ly, lm, lp);
  const bool rhs_ok = parse_version_triplet(rhs, ry, rm, rp);
  if (!lhs_ok || !rhs_ok) {
    if (ok) *ok = false;
    return 0;
  }
  if (ok) *ok = true;
  if (ly != ry) return (ly > ry) ? 1 : -1;
  if (lm != rm) return (lm > rm) ? 1 : -1;
  if (lp != rp) return (lp > rp) ? 1 : -1;
  return 0;
}

void bytes_to_hex_lower(const uint8_t *bytes, size_t bytes_len, char *dst, size_t dst_size) {
  static const char hex[] = "0123456789abcdef";
  if (!dst || dst_size == 0) return;
  if (!bytes || dst_size < (bytes_len * 2 + 1)) {
    dst[0] = '\0';
    return;
  }
  size_t j = 0;
  for (size_t i = 0; i < bytes_len; ++i) {
    dst[j++] = hex[(bytes[i] >> 4) & 0x0F];
    dst[j++] = hex[bytes[i] & 0x0F];
  }
  dst[j] = '\0';
}

void apply_network_recovery_defaults(bool clear_sta_credentials) {
  net_cfg.prefer_sta = false;
  if (clear_sta_credentials) {
    net_cfg.sta_ssid[0] = '\0';
    net_cfg.sta_password[0] = '\0';
  }
}

bool action_button_network_recovery_requested() {
  pinMode(kActionButtonPin, INPUT_PULLUP);
  if (digitalRead(kActionButtonPin) != LOW) return false;

  // Recovery intent is "hold ACTION from reboot for ~1.8 s".
  // setup_remote_control() runs after splash/init, so only require the
  // remaining hold time relative to boot uptime, plus a short entry guard
  // to avoid accidental edge-triggering.
  const unsigned long now_ms = millis();
  const unsigned long remaining_from_boot_ms =
    (now_ms >= kNetworkRecoveryHoldMs) ? 0UL : (kNetworkRecoveryHoldMs - now_ms);
  const unsigned long required_hold_ms =
    (remaining_from_boot_ms > kNetworkRecoveryEntryGuardMs)
      ? remaining_from_boot_ms
      : kNetworkRecoveryEntryGuardMs;

  const unsigned long t0 = millis();
  while ((millis() - t0) < required_hold_ms) {
    if (digitalRead(kActionButtonPin) != LOW) {
      return false;
    }
    delay(10);
  }
  return true;
}

const char *network_run_mode_text() {
  if (sta_attempt_active && !sta_connected) return "STA_CONNECTING";
  switch (net_run_mode) {
    case RUN_STA_ONLY: return "STA";
    case RUN_AP_FALLBACK: return "AP_FALLBACK";
    default: return "AP";
  }
}

void ip_to_str(const IPAddress &ip, char *out, size_t out_size) {
  if (!out || out_size == 0) return;
  if ((uint32_t)ip == 0) {
    out[0] = '\0';
    return;
  }
  snprintf(out, out_size, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

void send_json(const char *body, int code = 200) {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", body);
}

void json_escape_copy(char *dst, size_t dst_size, const char *src) {
  if (!dst || dst_size == 0) return;
  dst[0] = '\0';
  if (!src) return;

  size_t j = 0;
  for (size_t i = 0; src[i] != '\0' && j + 1 < dst_size; ++i) {
    const char c = src[i];
    if (c == '\\' || c == '\"') {
      if (j + 2 >= dst_size) break;
      dst[j++] = '\\';
      dst[j++] = c;
      continue;
    }
    if ((unsigned char)c < 0x20) {
      // Keep JSON valid without inflating with \uXXXX escapes.
      dst[j++] = ' ';
      continue;
    }
    dst[j++] = c;
  }
  dst[j] = '\0';
}

bool load_network_config() {
  memset(&net_cfg, 0, sizeof(net_cfg));
  net_cfg.prefer_sta = false;
  build_default_hostname(net_cfg.hostname, sizeof(net_cfg.hostname));

  // Ensure namespace exists so later read-only open does not emit NOT_FOUND.
  Preferences init_prefs;
  if (init_prefs.begin(kPrefsNs, false)) {
    init_prefs.end();
  }

  Preferences prefs;
  if (!prefs.begin(kPrefsNs, true)) {
    return false;
  }

  String mode = prefs.isKey(kPrefsMode) ? prefs.getString(kPrefsMode) : String("ap");
  String ssid = prefs.isKey(kPrefsSsid) ? prefs.getString(kPrefsSsid) : String("");
  String pass = prefs.isKey(kPrefsPass) ? prefs.getString(kPrefsPass) : String("");
  String host = prefs.isKey(kPrefsHost) ? prefs.getString(kPrefsHost) : String(net_cfg.hostname);
  String battery_mode = prefs.isKey(kPrefsBatteryMode) ? prefs.getString(kPrefsBatteryMode) : String("auto");
  prefs.end();

  mode.toLowerCase();
  copy_cstr(net_cfg.sta_ssid, sizeof(net_cfg.sta_ssid), ssid.c_str());
  copy_cstr(net_cfg.sta_password, sizeof(net_cfg.sta_password), pass.c_str());
  sanitize_hostname(host, net_cfg.hostname, sizeof(net_cfg.hostname));
  setBatteryPresenceMode(parse_battery_presence_mode(battery_mode));

  if (mode == "sta" && net_cfg.sta_ssid[0] != '\0') {
    net_cfg.prefer_sta = true;
  }
  return true;
}

bool save_network_config() {
  Preferences prefs;
  if (!prefs.begin(kPrefsNs, false)) {
    return false;
  }
  prefs.putString(kPrefsMode, net_cfg.prefer_sta ? "sta" : "ap");
  prefs.putString(kPrefsSsid, net_cfg.sta_ssid);
  prefs.putString(kPrefsPass, net_cfg.sta_password);
  prefs.putString(kPrefsHost, net_cfg.hostname);
  prefs.putString(kPrefsBatteryMode, battery_presence_mode_to_pref(getBatteryPresenceMode()));
  prefs.end();
  return true;
}

void stop_mdns_if_active() {
  if (mdns_active) {
    MDNS.end();
    mdns_active = false;
  }
}

void start_mdns_if_possible() {
  stop_mdns_if_active();
  if (!sta_connected) return;
  if (MDNS.begin(net_cfg.hostname)) {
    mdns_active = true;
    Serial.print("[remote] mDNS: http://");
    Serial.print(net_cfg.hostname);
    Serial.println(".local");
  } else {
    Serial.println("[remote] mDNS start failed");
  }
}

bool start_access_point(bool keep_sta, bool verbose) {
  WiFi.setSleep(false);
  WiFi.mode(keep_sta ? WIFI_AP_STA : WIFI_AP);
  bool ok = WiFi.softAP(ap_ssid, ap_password);
  ap_active = ok;

  if (verbose) {
    char ip[24];
    ip_to_str(WiFi.softAPIP(), ip, sizeof(ip));
    Serial.println();
    Serial.println("[remote] Wi-Fi AP mode");
    Serial.print("[remote] SSID: ");
    Serial.println(ap_ssid);
    Serial.print("[remote] PASS: ");
    Serial.println(ap_password);
    Serial.print("[remote] URL:  http://");
    Serial.println(ip[0] ? ip : "0.0.0.0");
    if (!ok) {
      Serial.println("[remote] AP start failed");
    }
  }
  return ok;
}

void stop_access_point(bool verbose) {
  if (!ap_active) return;
  WiFi.softAPdisconnect(true);
  ap_active = false;
  if (verbose) {
    Serial.println("[remote] AP disabled after STA link established");
  }
}

void begin_sta_attempt(bool keep_ap) {
  if (net_cfg.sta_ssid[0] == '\0') return;
  if (sta_attempt_active) return;

  if (keep_ap) {
    start_access_point(true, false);
  } else {
    WiFi.mode(WIFI_STA);
  }
  WiFi.setSleep(false);
  WiFi.setHostname(net_cfg.hostname);
  WiFi.begin(net_cfg.sta_ssid, net_cfg.sta_password);
  sta_attempt_active = true;
  sta_attempt_started_ms = millis();

  Serial.print("[remote] STA connect attempt: ");
  Serial.println(net_cfg.sta_ssid);
}

void on_sta_connected() {
  sta_attempt_active = false;
  sta_connected = true;
  net_run_mode = RUN_STA_ONLY;

  char ip[24];
  ip_to_str(WiFi.localIP(), ip, sizeof(ip));
  Serial.print("[remote] STA connected, IP: ");
  Serial.println(ip[0] ? ip : "0.0.0.0");

  start_mdns_if_possible();

  if (ap_active) {
    stop_access_point(true);
    WiFi.mode(WIFI_STA);
  }
}

void on_sta_connect_failed(const char *reason) {
  sta_attempt_active = false;
  sta_connected = false;
  stop_mdns_if_active();
  net_run_mode = RUN_AP_FALLBACK;

  WiFi.disconnect(false, false);
  if (!start_access_point(true, false)) {
    // Last-ditch attempt: drop STA and insist on AP so recovery remains reachable.
    start_access_point(false, true);
    net_run_mode = RUN_AP_ONLY;
  }
  next_sta_retry_ms = millis() + kStaRetryIntervalMs;

  Serial.print("[remote] STA connect failed: ");
  Serial.println(reason ? reason : "unknown");
  Serial.print("[remote] Fallback AP remains active. Retry in ");
  Serial.print(kStaRetryIntervalMs / 1000UL);
  Serial.println(" s");
}

void switch_to_ap_only_mode() {
  sta_attempt_active = false;
  sta_connected = false;
  stop_mdns_if_active();
  WiFi.disconnect(false, false);
  if (!start_access_point(false, true)) {
    Serial.println("[remote] AP-only recovery failed to start AP");
  }
  net_run_mode = RUN_AP_ONLY;
}

void apply_network_config() {
  if (!net_cfg.prefer_sta || net_cfg.sta_ssid[0] == '\0') {
    switch_to_ap_only_mode();
    return;
  }

  net_run_mode = RUN_AP_FALLBACK;
  if (!start_access_point(true, true)) {
    Serial.println("[remote] AP fallback failed to start; forcing AP-only recovery");
    switch_to_ap_only_mode();
    return;
  }
  next_sta_retry_ms = millis();
  sta_attempt_active = false;
  sta_connected = false;
  stop_mdns_if_active();
}

void loop_network_manager() {
  if (!net_cfg.prefer_sta || net_cfg.sta_ssid[0] == '\0') {
    return;
  }

  if (!sta_connected && !ap_active) {
    // Keep local recovery path alive while STA is pending/unavailable.
    if (!start_access_point(true, false)) {
      start_access_point(false, false);
      net_run_mode = RUN_AP_ONLY;
    } else {
      net_run_mode = RUN_AP_FALLBACK;
    }
  }

  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    if (!sta_connected) {
      on_sta_connected();
    }
    return;
  }

  if (sta_connected) {
    on_sta_connect_failed("link dropped");
    return;
  }

  if (sta_attempt_active) {
    const unsigned long elapsed = millis() - sta_attempt_started_ms;
    if (elapsed >= kStaConnectTimeoutMs) {
      on_sta_connect_failed("timeout");
    }
    return;
  }

  if ((long)(millis() - next_sta_retry_ms) >= 0) {
    begin_sta_attempt(true);
  }
}

void send_network_state_json(bool ok = true, const char *error = nullptr, int code = 200) {
  char json[768];
  char mode_esc[32];
  char pref_esc[8];
  char host_esc[40];
  char host_local_esc[48];
  char ssid_esc[40];
  char ap_ssid_esc[40];
  char ap_ip_esc[24];
  char sta_ip_esc[24];
  char err_esc[128];

  char host_local[40];
  snprintf(host_local, sizeof(host_local), "%s.local", net_cfg.hostname);

  char ap_ip[24];
  char sta_ip[24];
  ip_to_str(WiFi.softAPIP(), ap_ip, sizeof(ap_ip));
  ip_to_str(WiFi.localIP(), sta_ip, sizeof(sta_ip));

  json_escape_copy(mode_esc, sizeof(mode_esc), network_run_mode_text());
  json_escape_copy(pref_esc, sizeof(pref_esc), net_cfg.prefer_sta ? "sta" : "ap");
  json_escape_copy(host_esc, sizeof(host_esc), net_cfg.hostname);
  json_escape_copy(host_local_esc, sizeof(host_local_esc), host_local);
  json_escape_copy(ssid_esc, sizeof(ssid_esc), net_cfg.sta_ssid);
  json_escape_copy(ap_ssid_esc, sizeof(ap_ssid_esc), ap_ssid);
  json_escape_copy(ap_ip_esc, sizeof(ap_ip_esc), ap_ip);
  json_escape_copy(sta_ip_esc, sizeof(sta_ip_esc), sta_ip);
  json_escape_copy(err_esc, sizeof(err_esc), error ? error : "");

  snprintf(
    json,
    sizeof(json),
    "{\"ok\":%s,"
    "\"net_mode\":\"%s\",\"net_pref\":\"%s\","
    "\"battery_mode\":\"%s\","
    "\"hostname\":\"%s\",\"hostname_local\":\"%s\","
    "\"sta_ssid\":\"%s\",\"sta_connected\":%s,\"sta_ip\":\"%s\","
    "\"ap_active\":%s,\"ap_ssid\":\"%s\",\"ap_ip\":\"%s\","
    "\"ota_uploading\":%s,"
    "\"error\":\"%s\"}",
    ok ? "true" : "false",
    mode_esc, pref_esc,
    battery_presence_mode_to_pref(getBatteryPresenceMode()),
    host_esc, host_local_esc,
    ssid_esc, sta_connected ? "true" : "false", sta_ip_esc,
    ap_active ? "true" : "false", ap_ssid_esc, ap_ip_esc,
    ota_upload_in_progress ? "true" : "false",
    err_esc
  );
  send_json(json, code);
}

void handle_live() {
  char json[768];
  char fw_esc[32];
  char orient_esc[24];
  char axis_esc[12];
  char live_esc[12];

  json_escape_copy(fw_esc, sizeof(fw_esc), FW_VERSION);
  json_escape_copy(orient_esc, sizeof(orient_esc), orientation_text());
  json_escape_copy(axis_esc, sizeof(axis_esc), axis_text());
  json_escape_copy(live_esc, sizeof(live_esc), measurementIsFrozen() ? "FROZEN" : "LIVE");

  const float roll = get_display_roll();
  const float pitch = get_display_pitch();
  const float roll_cond_pct = rollConditionPercent();
  const bool roll_cond_low = rollConditionIsLow();
  BatteryTelemetry battery = {};
  getBatteryTelemetry(&battery);

  snprintf(
    json,
    sizeof(json),
    "{\"fw\":\"%s\",\"roll\":%.2f,\"pitch\":%.2f,"
    "\"orientation\":\"%s\",\"axis\":\"%s\",\"axis_id\":%d,\"rotation\":%d,\"live\":\"%s\","
    "\"roll_cond_pct\":%.1f,\"roll_cond_low\":%s,"
    "\"battery_valid\":%s,\"battery_voltage_v\":%.2f,\"battery_soc_pct\":%.1f,"
    "\"battery_charging\":%s,\"battery_charging_inferred\":%s,"
    "\"battery_present\":%s,\"battery_present_inferred\":%s}",
    fw_esc,
    roll,
    pitch,
    orient_esc,
    axis_esc,
    (int)getAxisMode(),
    displayRotated ? 180 : 0,
    live_esc,
    roll_cond_pct,
    roll_cond_low ? "true" : "false",
    battery.valid ? "true" : "false",
    battery.voltage_v,
    battery.soc_percent,
    battery.charging ? "true" : "false",
    battery.charging_inferred ? "true" : "false",
    battery.present ? "true" : "false",
    battery.present_inferred ? "true" : "false"
  );
  send_json(json);
}

void handle_state() {
  state_align_instruction_buf[0] = '\0';
  if (alignmentIsActive()) {
    alignmentGetInstruction(state_align_instruction_buf, sizeof(state_align_instruction_buf));
  }
  ImuDiagnosticsSample diag = {};
  CalibrationStateSnapshot cal = {};
  getImuDiagnosticsSample(&diag);
  getCalibrationStateSnapshot(&cal);

  const bool mode_active = modeWorkflowIsActive();
  const bool mode_confirmed = modeWorkflowIsConfirmed();
  const char *mode_target = orientation_text_for(modeWorkflowTarget());
  const float mode_rem_s = modeWorkflowRemainingSeconds();
  const float mode_progress_pct = modeWorkflowProgressPercent();
  const bool zero_active = zeroWorkflowIsActive();
  const bool zero_confirmed = zeroWorkflowIsConfirmed();
  const float zero_rem_s = zeroWorkflowRemainingSeconds();
  const float zero_progress_pct = zeroWorkflowProgressPercent();
  const bool offset_cal_active = offsetCalibrationWorkflowIsActive();
  const bool offset_cal_confirmed = offsetCalibrationWorkflowIsConfirmed();
  const float offset_cal_rem_s = offsetCalibrationWorkflowRemainingSeconds();
  const float offset_cal_progress_pct = offsetCalibrationWorkflowProgressPercent();
  const char *offset_cal_target = orientation_text();
  const bool align_capture_active = alignmentCaptureInProgress();
  const float align_capture_pct = alignmentCaptureProgressPercent();
  const float roll_cond_pct = rollConditionPercent();
  const bool roll_cond_low = rollConditionIsLow();
  const float roll = get_display_roll();
  const float pitch = get_display_pitch();
  BatteryTelemetry battery = {};
  getBatteryTelemetry(&battery);

  json_escape_copy(state_fw_esc, sizeof(state_fw_esc), FW_VERSION);
  json_escape_copy(state_orient_esc, sizeof(state_orient_esc), orientation_text());
  json_escape_copy(state_axis_esc, sizeof(state_axis_esc), axis_text());
  json_escape_copy(state_live_esc, sizeof(state_live_esc), measurementIsFrozen() ? "FROZEN" : "LIVE");
  json_escape_copy(state_align_esc, sizeof(state_align_esc), state_align_instruction_buf);
  json_escape_copy(state_mode_target_esc, sizeof(state_mode_target_esc), mode_target);
  json_escape_copy(state_offset_cal_target_esc, sizeof(state_offset_cal_target_esc), offset_cal_target);

  int written = snprintf(
    state_json_buf,
    sizeof(state_json_buf),
    "{\"fw\":\"%s\",\"roll\":%.2f,\"pitch\":%.2f,"
    "\"orientation\":\"%s\",\"axis\":\"%s\",\"axis_id\":%d,\"rotation\":%d,\"live\":\"%s\","
    "\"align_active\":%s,\"align_instruction\":\"%s\","
    "\"align_capture_active\":%s,\"align_capture_pct\":%.1f,"
    "\"roll_cond_pct\":%.1f,\"roll_cond_low\":%s,"
    "\"battery_valid\":%s,\"battery_voltage_v\":%.2f,\"battery_soc_pct\":%.1f,"
    "\"battery_charging\":%s,\"battery_charging_inferred\":%s,"
    "\"battery_present\":%s,\"battery_present_inferred\":%s,"
    "\"mode_active\":%s,\"mode_confirmed\":%s,\"mode_target\":\"%s\",\"mode_rem_s\":%.2f,\"mode_progress_pct\":%.1f,"
    "\"zero_active\":%s,\"zero_confirmed\":%s,\"zero_rem_s\":%.2f,\"zero_progress_pct\":%.1f,"
    "\"offset_cal_active\":%s,\"offset_cal_confirmed\":%s,\"offset_cal_target\":\"%s\",\"offset_cal_rem_s\":%.2f,\"offset_cal_progress_pct\":%.1f,"
    "\"diag_valid\":%s,"
    "\"sens_ax\":%.3f,\"sens_ay\":%.3f,\"sens_az\":%.3f,\"sens_gx\":%.3f,\"sens_gy\":%.3f,\"sens_gz\":%.3f,"
    "\"map_ax\":%.3f,\"map_ay\":%.3f,\"map_az\":%.3f,\"map_gx\":%.3f,\"map_gy\":%.3f,"
    "\"corr_ax\":%.3f,\"corr_ay\":%.3f,\"corr_az\":%.3f,\"corr_gx\":%.3f,\"corr_gy\":%.3f,"
    "\"phys_roll\":%.2f,\"phys_pitch\":%.2f,"
    "\"bias_ax\":%.4f,\"bias_ay\":%.4f,\"bias_az\":%.4f,\"bias_gx\":%.4f,\"bias_gy\":%.4f,"
    "\"zero_roll\":%.3f,\"zero_pitch\":%.3f,"
    "\"align_roll\":%.3f,\"align_pitch\":%.3f}",
    state_fw_esc,
    roll,
    pitch,
    state_orient_esc,
    state_axis_esc,
    (int)getAxisMode(),
    displayRotated ? 180 : 0,
    state_live_esc,
    alignmentIsActive() ? "true" : "false",
    state_align_esc,
    align_capture_active ? "true" : "false",
    align_capture_pct,
    roll_cond_pct,
    roll_cond_low ? "true" : "false",
    battery.valid ? "true" : "false",
    battery.voltage_v,
    battery.soc_percent,
    battery.charging ? "true" : "false",
    battery.charging_inferred ? "true" : "false",
    battery.present ? "true" : "false",
    battery.present_inferred ? "true" : "false",
    mode_active ? "true" : "false",
    mode_confirmed ? "true" : "false",
    state_mode_target_esc,
    mode_rem_s,
    mode_progress_pct,
    zero_active ? "true" : "false",
    zero_confirmed ? "true" : "false",
    zero_rem_s,
    zero_progress_pct,
    offset_cal_active ? "true" : "false",
    offset_cal_confirmed ? "true" : "false",
    state_offset_cal_target_esc,
    offset_cal_rem_s,
    offset_cal_progress_pct,
    diag.valid ? "true" : "false",
    diag.sens_ax, diag.sens_ay, diag.sens_az, diag.sens_gx, diag.sens_gy, diag.sens_gz,
    diag.map_ax, diag.map_ay, diag.map_az, diag.map_gx, diag.map_gy,
    diag.corr_ax, diag.corr_ay, diag.corr_az, diag.corr_gx, diag.corr_gy,
    diag.angle_roll, diag.angle_pitch,
    cal.bias_ax, cal.bias_ay, cal.bias_az, cal.bias_gx, cal.bias_gy,
    cal.zero_roll, cal.zero_pitch,
    cal.align_roll, cal.align_pitch
  );
  if (written < 0 || written >= (int)sizeof(state_json_buf)) {
    snprintf(
      state_json_buf,
      sizeof(state_json_buf),
      "{\"fw\":\"%s\",\"roll\":%.2f,\"pitch\":%.2f,"
      "\"orientation\":\"%s\",\"axis\":\"%s\",\"axis_id\":%d,"
      "\"rotation\":%d,\"live\":\"%s\",\"align_active\":%s,"
      "\"align_instruction\":\"\",\"align_capture_active\":false,\"align_capture_pct\":0.0,"
      "\"roll_cond_pct\":100.0,\"roll_cond_low\":false,"
      "\"battery_valid\":false,\"battery_voltage_v\":0.0,\"battery_soc_pct\":0.0,"
      "\"battery_charging\":false,\"battery_charging_inferred\":false,"
      "\"battery_present\":true,\"battery_present_inferred\":false,"
      "\"mode_active\":%s,\"mode_confirmed\":false,\"mode_target\":\"%s\",\"mode_rem_s\":0.0,\"mode_progress_pct\":0.0,"
      "\"zero_active\":%s,\"zero_confirmed\":false,\"zero_rem_s\":0.0,\"zero_progress_pct\":0.0,"
      "\"offset_cal_active\":%s,\"offset_cal_confirmed\":false,\"offset_cal_target\":\"%s\",\"offset_cal_rem_s\":0.0,\"offset_cal_progress_pct\":0.0,"
      "\"diag_valid\":false,"
      "\"sens_ax\":0.0,\"sens_ay\":0.0,\"sens_az\":0.0,\"sens_gx\":0.0,\"sens_gy\":0.0,\"sens_gz\":0.0,"
      "\"map_ax\":0.0,\"map_ay\":0.0,\"map_az\":0.0,\"map_gx\":0.0,\"map_gy\":0.0,"
      "\"corr_ax\":0.0,\"corr_ay\":0.0,\"corr_az\":0.0,\"corr_gx\":0.0,\"corr_gy\":0.0,"
      "\"phys_roll\":0.0,\"phys_pitch\":0.0,"
      "\"bias_ax\":0.0,\"bias_ay\":0.0,\"bias_az\":0.0,\"bias_gx\":0.0,\"bias_gy\":0.0,"
      "\"zero_roll\":0.0,\"zero_pitch\":0.0,"
      "\"align_roll\":0.0,\"align_pitch\":0.0}",
      state_fw_esc,
      roll,
      pitch,
      state_orient_esc,
      state_axis_esc,
      (int)getAxisMode(),
      displayRotated ? 180 : 0,
      state_live_esc,
      alignmentIsActive() ? "true" : "false",
      mode_active ? "true" : "false",
      state_mode_target_esc,
      zero_active ? "true" : "false",
      offset_cal_active ? "true" : "false",
      state_offset_cal_target_esc
    );
  }
  send_json(state_json_buf);
}

String read_cmd_from_request() {
  auto decode_body = [](const String &body) -> String {
    if (body.indexOf("zero") >= 0) return String("zero");
    if (body.indexOf("axis") >= 0) return String("axis");
    if (body.indexOf("freeze") >= 0) return String("freeze");
    if (body.indexOf("sleep") >= 0) return String("sleep");
    if (body.indexOf("offset_cal") >= 0) return String("offset_cal");
    if (body.indexOf("rotate") >= 0) return String("rotate");
    if (body.indexOf("mode_toggle") >= 0) return String("mode_toggle");
    if (body.indexOf("mode_up") >= 0) return String("mode_up");
    if (body.indexOf("mode_vertical") >= 0) return String("mode_vertical");
    if (body.indexOf("align_start") >= 0) return String("align_start");
    if (body.indexOf("capture") >= 0) return String("capture");
    if (body.indexOf("confirm") >= 0) return String("confirm");
    if (body.indexOf("cancel") >= 0) return String("cancel");
    return String("");
  };

  if (server.hasArg("plain")) {
    const String body = server.arg("plain");
    const String decoded = decode_body(body);
    if (decoded.length() > 0) return decoded;
  }
  if (server.hasArg("cmd")) {
    const String body = server.arg("cmd");
    const String decoded = decode_body(body);
    if (decoded.length() > 0) return decoded;
    return body;
  }
  return String("");
}

void handle_cmd() {
  const String cmd = read_cmd_from_request();
  if (cmd == "zero") {
    zeroWorkflowStart();
  } else if (cmd == "axis") {
    cycleAxisMode();
  } else if (cmd == "freeze") {
    toggleMeasurementFreeze();
  } else if (cmd == "offset_cal") {
    offsetCalibrationWorkflowStart();
  } else if (cmd == "sleep") {
    requestDeepSleep();
  } else if (cmd == "rotate") {
    toggleRotation();
  } else if (cmd == "mode_toggle") {
    modeWorkflowStartToggle();
  } else if (cmd == "mode_up") {
    modeWorkflowStart(MODE_SCREEN_UP);
  } else if (cmd == "mode_vertical") {
    modeWorkflowStart(MODE_SCREEN_VERTICAL);
  } else if (cmd == "align_start") {
    alignmentStart();
  } else if (cmd == "capture") {
    if (alignmentIsActive()) {
      alignmentCapture();
    } else {
      send_json("{\"ok\":false,\"error\":\"capture requires ALIGN\"}", 409);
      return;
    }
  } else if (cmd == "confirm") {
    if (modeWorkflowIsActive()) {
      modeWorkflowConfirm();
    } else if (zeroWorkflowIsActive()) {
      zeroWorkflowConfirm();
    } else if (offsetCalibrationWorkflowIsActive()) {
      offsetCalibrationWorkflowConfirm();
    } else {
      send_json("{\"ok\":false,\"error\":\"confirm requires active workflow\"}", 409);
      return;
    }
  } else if (cmd == "cancel") {
    if (modeWorkflowIsActive()) {
      modeWorkflowCancel();
    }
    if (zeroWorkflowIsActive()) {
      zeroWorkflowCancel();
    }
    if (offsetCalibrationWorkflowIsActive()) {
      offsetCalibrationWorkflowCancel();
    }
    if (alignmentIsActive()) {
      alignmentCancel();
    }
  } else {
    send_json("{\"ok\":false,\"error\":\"unknown cmd\"}", 400);
    return;
  }

  char json[96];
  snprintf(json, sizeof(json), "{\"ok\":true,\"cmd\":\"%s\"}", cmd.c_str());
  send_json(json);
}

String get_request_value(const char *key) {
  if (server.hasArg(key)) return server.arg(key);
  if (!server.hasArg("plain")) return String("");
  const String body = server.arg("plain");
  const String k = String("\"") + key + "\"";
  const int key_pos = body.indexOf(k);
  if (key_pos < 0) return String("");
  const int colon_pos = body.indexOf(':', key_pos + k.length());
  if (colon_pos < 0) return String("");
  const int q1 = body.indexOf('"', colon_pos + 1);
  if (q1 < 0) return String("");
  const int q2 = body.indexOf('"', q1 + 1);
  if (q2 < 0) return String("");
  return body.substring(q1 + 1, q2);
}

void handle_network_get() {
  send_network_state_json(true, "");
}

void handle_network_post() {
  const String mode_in = get_request_value("mode");
  const String battery_mode_in = get_request_value("battery_mode");
  const String ssid_in = get_request_value("ssid");
  const String pass_in = get_request_value("password");
  const String host_in = get_request_value("hostname");

  bool update_mode = mode_in.length() > 0;
  bool update_battery_mode = battery_mode_in.length() > 0 || server.hasArg("battery_mode");
  bool update_ssid = ssid_in.length() > 0 || server.hasArg("ssid");
  bool update_pass = pass_in.length() > 0 || server.hasArg("password");
  bool update_host = host_in.length() > 0 || server.hasArg("hostname");

  if (update_mode) {
    String mode = mode_in;
    mode.toLowerCase();
    if (mode == "sta") net_cfg.prefer_sta = true;
    else if (mode == "ap") net_cfg.prefer_sta = false;
    else {
      send_network_state_json(false, "mode must be 'ap' or 'sta'", 400);
      return;
    }
  }
  if (update_battery_mode) {
    setBatteryPresenceMode(parse_battery_presence_mode(battery_mode_in));
  }
  if (update_ssid) {
    copy_cstr(net_cfg.sta_ssid, sizeof(net_cfg.sta_ssid), ssid_in.c_str());
  }
  if (update_pass) {
    copy_cstr(net_cfg.sta_password, sizeof(net_cfg.sta_password), pass_in.c_str());
  }
  if (update_host) {
    sanitize_hostname(host_in, net_cfg.hostname, sizeof(net_cfg.hostname));
  }

  if (net_cfg.prefer_sta && net_cfg.sta_ssid[0] == '\0') {
    send_network_state_json(false, "STA mode requires ssid", 400);
    return;
  }

  if (!save_network_config()) {
    send_network_state_json(false, "failed to persist network config", 500);
    return;
  }

  apply_network_config();
  send_network_state_json(true, "");
}

void handle_network_recover_post() {
  const String wipe_in = get_request_value("wipe");
  const bool wipe_sta_credentials = (wipe_in.length() == 0) ? true : parse_bool_flag(wipe_in);
  const String host_in = get_request_value("hostname");

  apply_network_recovery_defaults(wipe_sta_credentials);
  if (host_in.length() > 0 || server.hasArg("hostname")) {
    sanitize_hostname(host_in, net_cfg.hostname, sizeof(net_cfg.hostname));
  }

  if (!save_network_config()) {
    send_network_state_json(false, "failed to persist recovery config", 500);
    return;
  }

  switch_to_ap_only_mode();
  send_network_state_json(true, "");
}

void reset_ota_upload_session_state() {
  ota_upload_ok = false;
  ota_upload_in_progress = false;
  ota_error[0] = '\0';
  ota_force_install = false;
  ota_expected_sha256_set = false;
  ota_expected_sha256[0] = '\0';
  ota_computed_sha256[0] = '\0';
  ota_requested_version[0] = '\0';
  ota_written_bytes = 0;
  if (ota_sha_ctx_active) {
    mbedtls_sha256_free(&ota_sha_ctx);
    ota_sha_ctx_active = false;
  }
}

void handle_ota_upload_data() {
  HTTPUpload &upload = server.upload();
  switch (upload.status) {
    case UPLOAD_FILE_START: {
      reset_ota_upload_session_state();
      ota_upload_in_progress = true;

      String version_in = server.arg("version");
      String sha_in = server.arg("sha256");
      String force_in = server.arg("force");
      ota_force_install = parse_bool_flag(force_in);

      copy_cstr(ota_requested_version, sizeof(ota_requested_version), version_in.c_str());
      int target_year = 0;
      int target_minor = 0;
      int target_patch = 0;
      if (!parse_version_triplet(ota_requested_version, target_year, target_minor, target_patch)) {
        snprintf(ota_error, sizeof(ota_error), "missing/invalid version (YYYY.M.X). Reload UI and provide version+sha256.");
        break;
      }
      (void)target_year;
      (void)target_minor;
      (void)target_patch;

      bool cmp_ok = false;
      const int cmp = compare_version_triplet(ota_requested_version, FW_VERSION, &cmp_ok);
      if (!cmp_ok) {
        snprintf(ota_error, sizeof(ota_error), "version gate unavailable (current or target parse failed)");
        break;
      }
      if (cmp <= 0 && !ota_force_install) {
        snprintf(ota_error, sizeof(ota_error), "version gate rejected: target %s is not newer than current %s", ota_requested_version, FW_VERSION);
        break;
      }

      ota_expected_sha256_set = normalize_sha256_hex(sha_in, ota_expected_sha256, sizeof(ota_expected_sha256));
      if (!ota_expected_sha256_set) {
        snprintf(ota_error, sizeof(ota_error), "missing/invalid sha256 (64 hex). Reload UI and provide version+sha256.");
        break;
      }

      mbedtls_sha256_init(&ota_sha_ctx);
      if (mbedtls_sha256_starts_ret(&ota_sha_ctx, 0) != 0) {
        snprintf(ota_error, sizeof(ota_error), "sha256 init failed");
        mbedtls_sha256_free(&ota_sha_ctx);
        break;
      }
      ota_sha_ctx_active = true;

      if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
        snprintf(ota_error, sizeof(ota_error), "Update.begin failed");
      }
      break;
    }
    case UPLOAD_FILE_WRITE: {
      if (ota_error[0] != '\0') break;
      const size_t written = Update.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        snprintf(ota_error, sizeof(ota_error), "Update.write failed");
        break;
      }
      ota_written_bytes += written;
      if (ota_sha_ctx_active) {
        if (mbedtls_sha256_update_ret(&ota_sha_ctx, upload.buf, upload.currentSize) != 0) {
          snprintf(ota_error, sizeof(ota_error), "sha256 update failed");
        }
      }
      break;
    }
    case UPLOAD_FILE_END: {
      if (ota_error[0] == '\0' && ota_written_bytes == 0) {
        snprintf(ota_error, sizeof(ota_error), "no firmware payload received");
      }

      if (ota_error[0] == '\0' && ota_sha_ctx_active) {
        uint8_t digest[32] = {0};
        if (mbedtls_sha256_finish_ret(&ota_sha_ctx, digest) != 0) {
          snprintf(ota_error, sizeof(ota_error), "sha256 finish failed");
        } else {
          bytes_to_hex_lower(digest, sizeof(digest), ota_computed_sha256, sizeof(ota_computed_sha256));
          if (strcmp(ota_computed_sha256, ota_expected_sha256) != 0) {
            snprintf(ota_error, sizeof(ota_error), "sha256 mismatch");
          }
        }
      }

      if (ota_sha_ctx_active) {
        mbedtls_sha256_free(&ota_sha_ctx);
        ota_sha_ctx_active = false;
      }

      if (ota_error[0] == '\0') {
        if (Update.end(true)) {
          ota_upload_ok = true;
        } else {
          snprintf(ota_error, sizeof(ota_error), "Update.end failed");
        }
      } else {
        if (Update.isRunning()) {
          Update.abort();
        }
      }
      ota_upload_in_progress = false;
      break;
    }
    case UPLOAD_FILE_ABORTED: {
      if (Update.isRunning()) {
        Update.abort();
      }
      if (ota_sha_ctx_active) {
        mbedtls_sha256_free(&ota_sha_ctx);
        ota_sha_ctx_active = false;
      }
      snprintf(ota_error, sizeof(ota_error), "Upload aborted");
      ota_upload_in_progress = false;
      ota_upload_ok = false;
      break;
    }
    default:
      break;
  }
}

void handle_ota_upload_done() {
  if (!ota_upload_ok) {
    char err_esc[128];
    json_escape_copy(err_esc, sizeof(err_esc), ota_error[0] ? ota_error : "unknown OTA error");
    char json[192];
    snprintf(json, sizeof(json), "{\"ok\":false,\"error\":\"%s\"}", err_esc);
    send_json(json, 500);
    return;
  }

  send_json("{\"ok\":true,\"rebooting\":true}");
  delay(250);
  ESP.restart();
}

void handle_options() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

void handle_root() {
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
  server.send(200, "text/html", PAGE_HTML);
}

void handle_health() {
  send_json("{\"ok\":true}");
}

}  // namespace

void setup_remote_control(void) {
  build_default_ap_ssid();
  log_board_identity();
  load_network_config();

  if (action_button_network_recovery_requested()) {
    Serial.println("[remote] ACTION held at boot -> forcing AP recovery defaults");
    apply_network_recovery_defaults(true);
    if (!save_network_config()) {
      Serial.println("[remote] failed to persist AP recovery defaults");
    }
  }

  apply_network_config();

  server.on("/", HTTP_GET, handle_root);
  server.on("/health", HTTP_GET, handle_health);
  server.on("/api/live", HTTP_GET, handle_live);
  server.on("/api/state", HTTP_GET, handle_state);
  server.on("/api/network", HTTP_OPTIONS, handle_options);
  server.on("/api/network", HTTP_GET, handle_network_get);
  server.on("/api/network", HTTP_POST, handle_network_post);
  server.on("/api/network/recover", HTTP_OPTIONS, handle_options);
  server.on("/api/network/recover", HTTP_POST, handle_network_recover_post);
  server.on("/api/cmd", HTTP_OPTIONS, handle_options);
  server.on("/api/cmd", HTTP_POST, handle_cmd);
  server.on("/api/cmd", HTTP_GET, handle_cmd);
  server.on("/api/ota/upload", HTTP_OPTIONS, handle_options);
  server.on("/api/ota/upload", HTTP_POST, handle_ota_upload_done, handle_ota_upload_data);
  server.onNotFound([]() { server.send(404, "text/plain", "Not Found"); });
  server.begin();
  remote_ready = true;
}

void loop_remote_control(void) {
  if (!remote_ready) return;
  loop_network_manager();
  server.handleClient();
}

void prepare_remote_for_deep_sleep(void) {
  stop_mdns_if_active();
  if (remote_ready) {
    server.stop();
    remote_ready = false;
  }
  if (ap_active) {
    stop_access_point(false);
  }
  WiFi.disconnect(false, false);
  WiFi.mode(WIFI_OFF);
  sta_attempt_active = false;
  sta_connected = false;
}

