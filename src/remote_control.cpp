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
    .row { display: flex; gap: 10px; flex-wrap: wrap; }
    .value { font-size: 36px; font-weight: bold; margin-top: 4px; }
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
  </style>
</head>
<body>
  <h1>Incidence Perfect NG</h1>
  <div class="muted">FW <span id="fw"></span></div>

  <div class="card">
    <div class="row">
      <div id="rollCard" style="flex:1;min-width:130px;">
        <div>ROLL</div>
        <div id="roll" class="value">--</div>
      </div>
      <div id="pitchCard" style="flex:1;min-width:130px;">
        <div>PITCH</div>
        <div id="pitch" class="value">--</div>
      </div>
    </div>
    <div style="margin-top:8px;">
      <code id="status">Loading...</code>
    </div>
  </div>

  <div class="card">
    <div id="normalControls" class="row">
      <button onclick="sendCmd('zero')">ZERO</button>
      <button onclick="sendCmd('axis')">AXIS</button>
      <button onclick="sendCmd('freeze')">FREEZE</button>
      <button onclick="sendCmd('rotate')">ROTATE</button>
      <button onclick="sendCmd('recal')">RECAL</button>
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
    <div id="recalControls" class="row hidden" style="margin-top:10px;">
      <button onclick="sendCmd('confirm')">CONFIRM</button>
      <button onclick="sendCmd('cancel')">CANCEL</button>
    </div>
    <div id="progressWrap" class="bar hidden"><div id="progressBar"></div></div>
    <div id="msg"></div>
  </div>

  <script>
    const statusEl = document.getElementById('status');
    const msgEl = document.getElementById('msg');
    const fwEl = document.getElementById('fw');
    const rollEl = document.getElementById('roll');
    const pitchEl = document.getElementById('pitch');
    const rollCard = document.getElementById('rollCard');
    const pitchCard = document.getElementById('pitchCard');
    const normalControls = document.getElementById('normalControls');
    const modeControls = document.getElementById('modeControls');
    const alignControls = document.getElementById('alignControls');
    const recalControls = document.getElementById('recalControls');
    const progressWrap = document.getElementById('progressWrap');
    const progressBar = document.getElementById('progressBar');

    function fmt(v) {
      const n = Number(v);
      if (Number.isNaN(n)) return '--';
      return (n >= 0 ? '+' : '') + n.toFixed(2) + '\u00B0';
    }

    async function refreshState() {
      try {
        const r = await fetch('/api/state', { cache: 'no-store' });
        const s = await r.json();
        rollEl.textContent = fmt(s.roll);
        pitchEl.textContent = fmt(s.pitch);
        fwEl.textContent = s.fw;
        statusEl.textContent =
          `${s.orientation} | ${s.axis} | ROT ${s.rotation} | ${s.live}`;
        if (s.axis === 'ROLL' || s.axis_id === 1) {
          rollCard.classList.remove('hidden');
          pitchCard.classList.add('hidden');
        } else if (s.axis === 'PITCH' || s.axis_id === 2) {
          rollCard.classList.add('hidden');
          pitchCard.classList.remove('hidden');
        } else {
          rollCard.classList.remove('hidden');
          pitchCard.classList.remove('hidden');
        }
        normalControls.classList.toggle('hidden', s.mode_active || s.align_active || s.recal_active);
        modeControls.classList.toggle('hidden', !s.mode_active || s.align_active || s.recal_active);
        alignControls.classList.toggle('hidden', !s.align_active || s.recal_active);
        recalControls.classList.toggle('hidden', !s.recal_active);
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
        } else if (s.recal_active) {
          if (s.recal_confirmed) {
            msgEl.textContent = `RECAL: hold still ${s.recal_rem_s.toFixed(1)} s`;
            progressWrap.classList.remove('hidden');
            progressBar.style.width = `${Math.max(0, Math.min(100, s.recal_progress_pct || 0))}%`;
          } else {
            msgEl.textContent = `RECAL target: ${s.recal_target}`;
          }
        } else {
          msgEl.textContent = '';
        }
      } catch (e) {
        statusEl.textContent = 'Disconnected';
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
  char json[1536];
  char align_instruction[192] = {0};
  if (alignmentIsActive()) {
    alignmentGetInstruction(align_instruction, sizeof(align_instruction));
  }

  const bool mode_active = modeWorkflowIsActive();
  const bool mode_confirmed = modeWorkflowIsConfirmed();
  const char *mode_target = orientation_text_for(modeWorkflowTarget());
  const float mode_rem_s = modeWorkflowRemainingSeconds();
  const float mode_progress_pct = modeWorkflowProgressPercent();
  const bool recal_active = recalibrationWorkflowIsActive();
  const bool recal_confirmed = recalibrationWorkflowIsConfirmed();
  const float recal_rem_s = recalibrationWorkflowRemainingSeconds();
  const float recal_progress_pct = recalibrationWorkflowProgressPercent();
  const char *recal_target = orientation_text();
  const bool align_capture_active = alignmentCaptureInProgress();
  const float align_capture_pct = alignmentCaptureProgressPercent();
  const float roll = ui_roll;
  const float pitch = ui_pitch;

  char fw_esc[32];
  char orient_esc[24];
  char axis_esc[12];
  char live_esc[12];
  char align_esc[320];
  char mode_target_esc[24];
  char recal_target_esc[24];
  json_escape_copy(fw_esc, sizeof(fw_esc), FW_VERSION);
  json_escape_copy(orient_esc, sizeof(orient_esc), orientation_text());
  json_escape_copy(axis_esc, sizeof(axis_esc), axis_text());
  json_escape_copy(live_esc, sizeof(live_esc), measurementIsFrozen() ? "FROZEN" : "LIVE");
  json_escape_copy(align_esc, sizeof(align_esc), align_instruction);
  json_escape_copy(mode_target_esc, sizeof(mode_target_esc), mode_target);
  json_escape_copy(recal_target_esc, sizeof(recal_target_esc), recal_target);

  int written = snprintf(
    json,
    sizeof(json),
    "{\"fw\":\"%s\",\"roll\":%.2f,\"pitch\":%.2f,"
    "\"orientation\":\"%s\",\"axis\":\"%s\",\"axis_id\":%d,\"rotation\":%d,\"live\":\"%s\","
    "\"align_active\":%s,\"align_instruction\":\"%s\","
    "\"align_capture_active\":%s,\"align_capture_pct\":%.1f,"
    "\"mode_active\":%s,\"mode_confirmed\":%s,\"mode_target\":\"%s\",\"mode_rem_s\":%.2f,\"mode_progress_pct\":%.1f,"
    "\"recal_active\":%s,\"recal_confirmed\":%s,\"recal_target\":\"%s\",\"recal_rem_s\":%.2f,\"recal_progress_pct\":%.1f}",
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
    mode_active ? "true" : "false",
    mode_confirmed ? "true" : "false",
    mode_target_esc,
    mode_rem_s,
    mode_progress_pct,
    recal_active ? "true" : "false",
    recal_confirmed ? "true" : "false",
    recal_target_esc,
    recal_rem_s,
    recal_progress_pct
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
      "\"mode_active\":%s,\"mode_confirmed\":false,\"mode_target\":\"%s\",\"mode_rem_s\":0.0,\"mode_progress_pct\":0.0,"
      "\"recal_active\":%s,\"recal_confirmed\":false,\"recal_target\":\"%s\",\"recal_rem_s\":0.0,\"recal_progress_pct\":0.0}",
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
      recal_active ? "true" : "false",
      recal_target_esc
    );
  }
  send_json(json);
}

String read_cmd_from_request() {
  auto decode_body = [](const String &body) -> String {
    if (body.indexOf("zero") >= 0) return String("zero");
    if (body.indexOf("axis") >= 0) return String("axis");
    if (body.indexOf("freeze") >= 0) return String("freeze");
    if (body.indexOf("recal") >= 0) return String("recal");
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
    setZeroReference();
  } else if (cmd == "axis") {
    cycleAxisMode();
  } else if (cmd == "freeze") {
    toggleMeasurementFreeze();
  } else if (cmd == "recal") {
    recalibrationWorkflowStart();
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
    } else if (recalibrationWorkflowIsActive()) {
      recalibrationWorkflowConfirm();
    } else {
      send_json("{\"ok\":false,\"error\":\"confirm requires MODE/RECAL workflow\"}", 409);
      return;
    }
  } else if (cmd == "cancel") {
    if (modeWorkflowIsActive()) {
      modeWorkflowCancel();
    }
    if (recalibrationWorkflowIsActive()) {
      recalibrationWorkflowCancel();
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

