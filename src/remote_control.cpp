#include "remote_control.h"

#include <WiFi.h>
#include <WebServer.h>
#include <string.h>

#include "fw_version.h"
#include "inclinometer_shared.h"

namespace {

WebServer server(80);
bool remote_ready = false;
char ap_ssid[32] = {0};
const char *ap_password = "incidence-ng";

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
    button:active { transform: scale(0.98); }
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
  </div>

  <div class="card">
    <div id="normalControls" class="row">
      <button onclick="sendCmd('zero')">ZERO</button>
      <button onclick="sendCmd('axis')">AXIS</button>
      <button onclick="sendCmd('freeze')">FREEZE</button>
      <button onclick="sendCmd('rotate')">ROTATE</button>
      <button onclick="sendCmd('offset_cal')">OFFSET CAL</button>
      <button onclick="sendCmd('mode_toggle')">MODE</button>
      <button onclick="sendCmd('align_start')">ALIGN</button>
    </div>
    <div id="modeControls" class="row hidden" style="margin-top:10px;">
      <button onclick="sendCmd('confirm')">CONFIRM</button>
      <button onclick="sendCmd('cancel')">CANCEL</button>
    </div>
    <div id="alignControls" class="row hidden" style="margin-top:10px;">
      <button onclick="sendCmd('capture')">CAPTURE</button>
      <button onclick="sendCmd('cancel')">CANCEL</button>
    </div>
    <div id="offsetCalControls" class="row hidden" style="margin-top:10px;">
      <button onclick="sendCmd('confirm')">CONFIRM</button>
      <button onclick="sendCmd('cancel')">CANCEL</button>
    </div>
    <div id="progressWrap" class="bar hidden"><div id="progressBar"></div></div>
    <div id="msg"></div>
  </div>

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
      <p class="diag-note">Values are live and update every 250 ms.</p>
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
    const normalControls = document.getElementById('normalControls');
    const modeControls = document.getElementById('modeControls');
    const alignControls = document.getElementById('alignControls');
    const offsetCalControls = document.getElementById('offsetCalControls');
    const progressWrap = document.getElementById('progressWrap');
    const progressBar = document.getElementById('progressBar');
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
      diagPhysEl.textContent = `r=${f(s.phys_roll, 2)}°  p=${f(s.phys_pitch, 2)}°`;
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

    async function refreshState() {
      try {
        const r = await fetch('/api/state', { cache: 'no-store' });
        const s = await r.json();
        rollEl.textContent = fmt(s.roll);
        pitchEl.textContent = fmt(s.pitch);
        setAngleClass(rollEl, s.roll);
        setAngleClass(pitchEl, s.pitch);
        fwEl.textContent = s.fw;
        statusEl.textContent =
          `${s.orientation} | ${s.axis} | ROT ${s.rotation} | ${s.live}`;
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
        statusEl.textContent = 'Disconnected';
        clearDiag(true);
      }
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
      setTimeout(refreshState, 80);
    }

    refreshState();
    setInterval(refreshState, 250);
  </script>
</body>
</html>
)HTML";

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

void handle_state() {
  char json[4096];
  char align_instruction[192] = {0};
  if (alignmentIsActive()) {
    alignmentGetInstruction(align_instruction, sizeof(align_instruction));
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
  const float roll = ui_roll;
  const float pitch = ui_pitch;

  char fw_esc[32];
  char orient_esc[24];
  char axis_esc[12];
  char live_esc[12];
  char align_esc[320];
  char mode_target_esc[24];
  char offset_cal_target_esc[24];
  json_escape_copy(fw_esc, sizeof(fw_esc), FW_VERSION);
  json_escape_copy(orient_esc, sizeof(orient_esc), orientation_text());
  json_escape_copy(axis_esc, sizeof(axis_esc), axis_text());
  json_escape_copy(live_esc, sizeof(live_esc), measurementIsFrozen() ? "FROZEN" : "LIVE");
  json_escape_copy(align_esc, sizeof(align_esc), align_instruction);
  json_escape_copy(mode_target_esc, sizeof(mode_target_esc), mode_target);
  json_escape_copy(offset_cal_target_esc, sizeof(offset_cal_target_esc), offset_cal_target);

  int written = snprintf(
    json,
    sizeof(json),
    "{\"fw\":\"%s\",\"roll\":%.2f,\"pitch\":%.2f,"
    "\"orientation\":\"%s\",\"axis\":\"%s\",\"axis_id\":%d,\"rotation\":%d,\"live\":\"%s\","
    "\"align_active\":%s,\"align_instruction\":\"%s\","
    "\"align_capture_active\":%s,\"align_capture_pct\":%.1f,"
    "\"roll_cond_pct\":%.1f,\"roll_cond_low\":%s,"
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
    fw_esc,
    roll,
    pitch,
    orient_esc,
    axis_esc,
    (int)getAxisMode(),
    displayRotated ? 180 : 0,
    live_esc,
    alignmentIsActive() ? "true" : "false",
    align_esc,
    align_capture_active ? "true" : "false",
    align_capture_pct,
    roll_cond_pct,
    roll_cond_low ? "true" : "false",
    mode_active ? "true" : "false",
    mode_confirmed ? "true" : "false",
    mode_target_esc,
    mode_rem_s,
    mode_progress_pct,
    zero_active ? "true" : "false",
    zero_confirmed ? "true" : "false",
    zero_rem_s,
    zero_progress_pct,
    offset_cal_active ? "true" : "false",
    offset_cal_confirmed ? "true" : "false",
    offset_cal_target_esc,
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
  if (written < 0 || written >= (int)sizeof(json)) {
    // Fallback to a compact valid payload if formatting overflows.
    snprintf(
      json,
      sizeof(json),
      "{\"fw\":\"%s\",\"roll\":%.2f,\"pitch\":%.2f,"
      "\"orientation\":\"%s\",\"axis\":\"%s\",\"axis_id\":%d,"
      "\"rotation\":%d,\"live\":\"%s\",\"align_active\":%s,"
      "\"align_instruction\":\"\",\"align_capture_active\":false,\"align_capture_pct\":0.0,"
      "\"roll_cond_pct\":100.0,\"roll_cond_low\":false,"
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
      fw_esc,
      roll,
      pitch,
      orient_esc,
      axis_esc,
      (int)getAxisMode(),
      displayRotated ? 180 : 0,
      live_esc,
      alignmentIsActive() ? "true" : "false",
      mode_active ? "true" : "false",
      mode_target_esc,
      zero_active ? "true" : "false",
      offset_cal_active ? "true" : "false",
      offset_cal_target_esc
    );
  }
  send_json(json);
}

String read_cmd_from_request() {
  auto decode_body = [](const String &body) -> String {
    if (body.indexOf("zero") >= 0) return String("zero");
    if (body.indexOf("axis") >= 0) return String("axis");
    if (body.indexOf("freeze") >= 0) return String("freeze");
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

void start_access_point() {
  uint64_t chip = ESP.getEfuseMac();
  snprintf(ap_ssid, sizeof(ap_ssid), "IncidencePerfectNG-%04X", (uint16_t)(chip & 0xFFFF));

  WiFi.setSleep(false);
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(ap_ssid, ap_password);
  IPAddress ip = WiFi.softAPIP();

  Serial.println();
  Serial.println("[remote] Wi-Fi AP mode");
  Serial.print("[remote] SSID: ");
  Serial.println(ap_ssid);
  Serial.print("[remote] PASS: ");
  Serial.println(ap_password);
  Serial.print("[remote] URL:  http://");
  Serial.println(ip);
  if (!ok) {
    Serial.println("[remote] AP start failed");
  }
}

}  // namespace

void setup_remote_control(void) {
  start_access_point();

  server.on("/", HTTP_GET, handle_root);
  server.on("/health", HTTP_GET, handle_health);
  server.on("/api/state", HTTP_GET, handle_state);
  server.on("/api/cmd", HTTP_OPTIONS, handle_options);
  server.on("/api/cmd", HTTP_POST, handle_cmd);
  server.on("/api/cmd", HTTP_GET, handle_cmd);
  server.onNotFound([]() { server.send(404, "text/plain", "Not Found"); });
  server.begin();
  remote_ready = true;
}

void loop_remote_control(void) {
  if (!remote_ready) return;
  server.handleClient();
}

