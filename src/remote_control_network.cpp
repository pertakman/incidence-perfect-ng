#include "remote_control_network.h"

#include <ESPmDNS.h>

char ap_ssid[32] = {0};
const char *ap_password = "incidence-ng";
NetworkConfig net_cfg = {};
NetworkRunMode net_run_mode = RUN_AP_ONLY;
bool ap_active = false;
bool sta_connected = false;
bool sta_attempt_active = false;
bool mdns_active = false;
unsigned long sta_attempt_started_ms = 0;
unsigned long next_sta_retry_ms = 0;

namespace {

constexpr unsigned long kStaConnectTimeoutMs = 12000UL;
constexpr unsigned long kStaRetryIntervalMs = 30000UL;

bool start_access_point(bool keep_sta, bool verbose) {
  WiFi.setSleep(false);
  WiFi.mode(keep_sta ? WIFI_AP_STA : WIFI_AP);
  const bool ok = WiFi.softAP(ap_ssid, ap_password);
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

void stop_access_point_internal(bool verbose) {
  if (!ap_active) return;
  WiFi.softAPdisconnect(true);
  ap_active = false;
  if (verbose) {
    Serial.println("[remote] AP disabled after STA link established");
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
    stop_access_point_internal(true);
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

}  // namespace

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

void stop_mdns_if_active() {
  if (mdns_active) {
    MDNS.end();
    mdns_active = false;
  }
}

void stop_access_point(bool verbose) {
  stop_access_point_internal(verbose);
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
