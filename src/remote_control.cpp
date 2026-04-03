#include "remote_control.h"

#include <WiFi.h>
#include <WebServer.h>
#include <string.h>

#include "fw_version.h"
#include "remote_control_config.h"
#include "remote_control_http.h"
#include "remote_control_network.h"
#include "remote_control_ota.h"
#include "inclinometer_shared.h"
#include "remote_protocol_utils.h"
#include "ui_lvgl.h"

namespace {

WebServer server(80);
bool remote_ready = false;

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

void build_default_ap_ssid() {
  const uint16_t suffix = board_suffix_from_mac();
  snprintf(ap_ssid, sizeof(ap_ssid), "IncidencePerfectNG-%04X", suffix);
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

  register_remote_control_routes(server);
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

