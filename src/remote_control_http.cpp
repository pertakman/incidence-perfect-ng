#include "remote_control_http.h"

#include "fw_version.h"
#include "inclinometer_shared.h"
#include "remote_control_config.h"
#include "remote_control_network.h"
#include "remote_control_ota.h"
#include "remote_control_page.h"
#include "remote_protocol_utils.h"
#include "ui_lvgl.h"

namespace {

WebServer *g_server = nullptr;

char state_json_buf[4608];
char state_align_instruction_buf[192];
char state_fw_esc[32];
char state_orient_esc[24];
char state_axis_esc[12];
char state_live_esc[12];
char state_align_esc[320];
char state_mode_target_esc[24];
char state_offset_cal_target_esc[24];

WebServer &server_ref() {
  return *g_server;
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

void send_json(const char *body, int code = 200) {
  WebServer &server = server_ref();
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
      dst[j++] = ' ';
      continue;
    }
    dst[j++] = c;
  }
  dst[j] = '\0';
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
    "\"zero_on_boot\":%s,"
    "\"display_precision\":%d,"
    "\"touch_enabled\":%s,"
    "\"touch_persist\":%s,"
    "\"display_brightness_pct\":%d,"
    "\"hostname\":\"%s\",\"hostname_local\":\"%s\","
    "\"sta_ssid\":\"%s\",\"sta_connected\":%s,\"sta_ip\":\"%s\","
    "\"ap_active\":%s,\"ap_ssid\":\"%s\",\"ap_ip\":\"%s\","
    "\"ota_uploading\":%s,"
    "\"error\":\"%s\"}",
    ok ? "true" : "false",
    mode_esc, pref_esc,
    battery_presence_mode_to_pref(getBatteryPresenceMode()),
    getAutoZeroOnBootEnabled() ? "true" : "false",
    (int)getDisplayPrecisionMode(),
    getTouchInputEnabled() ? "true" : "false",
    getTouchLockPersistent() ? "true" : "false",
    (int)getDisplayBrightnessPercent(),
    host_esc, host_local_esc,
    ssid_esc, sta_connected ? "true" : "false", sta_ip_esc,
    ap_active ? "true" : "false", ap_ssid_esc, ap_ip_esc,
    ota_is_upload_in_progress() ? "true" : "false",
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
  const int display_precision = (int)getDisplayPrecisionMode();
  const float roll_cond_pct = rollConditionPercent();
  const bool roll_cond_low = rollConditionIsLow();
  BatteryTelemetry battery = {};
  getBatteryTelemetry(&battery);

  snprintf(
    json,
    sizeof(json),
    "{\"fw\":\"%s\",\"roll\":%.*f,\"pitch\":%.*f,"
    "\"orientation\":\"%s\",\"axis\":\"%s\",\"axis_id\":%d,\"rotation\":%d,\"live\":\"%s\","
    "\"display_precision\":%d,"
    "\"roll_cond_pct\":%.1f,\"roll_cond_low\":%s,"
    "\"battery_valid\":%s,\"battery_voltage_v\":%.2f,\"battery_soc_pct\":%.1f,"
    "\"battery_charging\":%s,\"battery_charging_inferred\":%s,"
    "\"battery_present\":%s,\"battery_present_inferred\":%s}",
    fw_esc,
    display_precision, roll,
    display_precision, pitch,
    orient_esc,
    axis_esc,
    (int)getAxisMode(),
    displayRotated ? 180 : 0,
    live_esc,
    display_precision,
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
  const int display_precision = (int)getDisplayPrecisionMode();
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
    "{\"fw\":\"%s\",\"roll\":%.*f,\"pitch\":%.*f,"
    "\"orientation\":\"%s\",\"axis\":\"%s\",\"axis_id\":%d,\"rotation\":%d,\"live\":\"%s\","
    "\"display_precision\":%d,"
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
    display_precision, roll,
    display_precision, pitch,
    state_orient_esc,
    state_axis_esc,
    (int)getAxisMode(),
    displayRotated ? 180 : 0,
    state_live_esc,
    display_precision,
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
      "{\"fw\":\"%s\",\"roll\":%.*f,\"pitch\":%.*f,"
      "\"orientation\":\"%s\",\"axis\":\"%s\",\"axis_id\":%d,"
      "\"rotation\":%d,\"live\":\"%s\",\"display_precision\":%d,\"align_active\":%s,"
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
      display_precision, roll,
      display_precision, pitch,
      state_orient_esc,
      state_axis_esc,
      (int)getAxisMode(),
      displayRotated ? 180 : 0,
      state_live_esc,
      display_precision,
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
  WebServer &server = server_ref();
  if (server.hasArg("plain")) {
    const String body = server.arg("plain");
    const RemoteCommandId decoded = decode_remote_command(body.c_str());
    if (decoded != REMOTE_CMD_NONE) return String(remote_command_text(decoded));
  }
  if (server.hasArg("cmd")) {
    const String body = server.arg("cmd");
    const RemoteCommandId decoded = decode_remote_command(body.c_str());
    if (decoded != REMOTE_CMD_NONE) return String(remote_command_text(decoded));
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
    if (modeWorkflowIsActive()) modeWorkflowCancel();
    if (zeroWorkflowIsActive()) zeroWorkflowCancel();
    if (offsetCalibrationWorkflowIsActive()) offsetCalibrationWorkflowCancel();
    if (alignmentIsActive()) alignmentCancel();
  } else {
    send_json("{\"ok\":false,\"error\":\"unknown cmd\"}", 400);
    return;
  }

  char json[96];
  snprintf(json, sizeof(json), "{\"ok\":true,\"cmd\":\"%s\"}", cmd.c_str());
  send_json(json);
}

String get_request_value(const char *key) {
  WebServer &server = server_ref();
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
  WebServer &server = server_ref();
  const String mode_in = get_request_value("mode");
  const String battery_mode_in = get_request_value("battery_mode");
  const String zero_on_boot_in = get_request_value("zero_on_boot");
  const String display_precision_in = get_request_value("display_precision");
  const String touch_enabled_in = get_request_value("touch_enabled");
  const String touch_persist_in = get_request_value("touch_persist");
  const String display_brightness_in = get_request_value("display_brightness");
  const String ssid_in = get_request_value("ssid");
  const String pass_in = get_request_value("password");
  const String host_in = get_request_value("hostname");

  const bool update_mode = mode_in.length() > 0;
  const bool update_battery_mode = battery_mode_in.length() > 0 || server.hasArg("battery_mode");
  const bool update_zero_on_boot = zero_on_boot_in.length() > 0 || server.hasArg("zero_on_boot");
  const bool update_display_precision = display_precision_in.length() > 0 || server.hasArg("display_precision");
  const bool update_touch_enabled = touch_enabled_in.length() > 0 || server.hasArg("touch_enabled");
  const bool update_touch_persist = touch_persist_in.length() > 0 || server.hasArg("touch_persist");
  const bool update_display_brightness = display_brightness_in.length() > 0 || server.hasArg("display_brightness");
  const bool update_ssid = ssid_in.length() > 0 || server.hasArg("ssid");
  const bool update_pass = pass_in.length() > 0 || server.hasArg("password");
  const bool update_host = host_in.length() > 0 || server.hasArg("hostname");

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
  if (update_battery_mode) setBatteryPresenceMode(parse_battery_presence_mode(battery_mode_in));
  if (update_zero_on_boot) setAutoZeroOnBootEnabled(parse_bool_flag(zero_on_boot_in));
  if (update_display_precision) setDisplayPrecisionMode(parse_display_precision_mode(display_precision_in));
  if (update_touch_persist) setTouchLockPersistent(parse_bool_flag(touch_persist_in));
  if (update_touch_enabled) setTouchInputEnabled(parse_bool_flag(touch_enabled_in));
  if (update_display_brightness) setDisplayBrightnessPercent(parse_display_brightness_percent(display_brightness_in));
  if (update_ssid) copy_cstr(net_cfg.sta_ssid, sizeof(net_cfg.sta_ssid), ssid_in.c_str());
  if (update_pass) copy_cstr(net_cfg.sta_password, sizeof(net_cfg.sta_password), pass_in.c_str());
  if (update_host) sanitize_hostname(host_in, net_cfg.hostname, sizeof(net_cfg.hostname));

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
  WebServer &server = server_ref();
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

void handle_ota_upload_done() {
  if (!ota_is_upload_ok()) {
    char err_esc[128];
    json_escape_copy(err_esc, sizeof(err_esc), ota_last_error()[0] ? ota_last_error() : "unknown OTA error");
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
  WebServer &server = server_ref();
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.send(204);
}

void handle_root() {
  WebServer &server = server_ref();
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "0");
  server.send_P(200, "text/html", PAGE_HTML);
}

void handle_health() {
  send_json("{\"ok\":true}");
}

}  // namespace

void register_remote_control_routes(WebServer &server) {
  g_server = &server;

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
  server.on("/api/ota/upload", HTTP_POST, handle_ota_upload_done, []() { handle_ota_upload_data(server_ref(), FW_VERSION); });
  server.onNotFound([]() { server_ref().send(404, "text/plain", "Not Found"); });
}
