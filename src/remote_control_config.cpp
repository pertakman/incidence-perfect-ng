#include "remote_control_config.h"

#include <Preferences.h>
#include <string.h>

#include "remote_control_network.h"
#include "remote_protocol_utils.h"

namespace {

constexpr const char *kPrefsNs = "remote";
constexpr const char *kPrefsMode = "mode";
constexpr const char *kPrefsSsid = "sta_ssid";
constexpr const char *kPrefsPass = "sta_pass";
constexpr const char *kPrefsHost = "host";
constexpr const char *kPrefsBatteryMode = "battery_mode";
constexpr int kActionButtonPin = 0;
constexpr unsigned long kNetworkRecoveryHoldMs = 1800UL;
constexpr unsigned long kNetworkRecoveryEntryGuardMs = 250UL;

uint16_t board_suffix_from_mac() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  uint32_t hash = 2166136261u;
  for (int i = 0; i < 6; ++i) {
    hash ^= (uint32_t)mac[i];
    hash *= 16777619u;
  }
  return (uint16_t)(hash & 0xFFFFu);
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

void build_default_hostname(char *dst, size_t dst_size) {
  if (!dst || dst_size == 0) return;
  const uint16_t suffix = board_suffix_from_mac();
  snprintf(dst, dst_size, "incidence-perfect-ng-%04x", suffix);
}

}  // namespace

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

DisplayPrecisionMode parse_display_precision_mode(const String &raw) {
  String s = raw;
  s.trim();
  const int value = s.toInt();
  if (value == 1) return DISPLAY_PRECISION_1DP;
  if (value == 3) return DISPLAY_PRECISION_3DP;
  return DISPLAY_PRECISION_2DP;
}

uint8_t parse_display_brightness_percent(const String &raw) {
  String s = raw;
  s.trim();
  const int value = s.toInt();
  if (value < 10) return 10;
  if (value > 100) return 100;
  const int rounded = ((value + 2) / 5) * 5;
  if (rounded < 10) return 10;
  if (rounded > 100) return 100;
  return (uint8_t)rounded;
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
    } else if (!prev_dash && len > 0) {
      append_if_room(dst, dst_size, '-', len);
      prev_dash = true;
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
  return parse_bool_flag_text(raw.c_str());
}

bool load_network_config() {
  memset(&net_cfg, 0, sizeof(net_cfg));
  net_cfg.prefer_sta = false;
  build_default_hostname(net_cfg.hostname, sizeof(net_cfg.hostname));

  Preferences init_prefs;
  if (init_prefs.begin(kPrefsNs, false)) {
    init_prefs.end();
  }

  Preferences prefs;
  if (!prefs.begin(kPrefsNs, true)) {
    return false;
  }

  const String mode = prefs.isKey(kPrefsMode) ? prefs.getString(kPrefsMode) : String("ap");
  const String ssid = prefs.isKey(kPrefsSsid) ? prefs.getString(kPrefsSsid) : String("");
  const String pass = prefs.isKey(kPrefsPass) ? prefs.getString(kPrefsPass) : String("");
  const String host = prefs.isKey(kPrefsHost) ? prefs.getString(kPrefsHost) : String(net_cfg.hostname);
  const String battery_mode = prefs.isKey(kPrefsBatteryMode) ? prefs.getString(kPrefsBatteryMode) : String("auto");
  prefs.end();

  String mode_norm = mode;
  mode_norm.toLowerCase();
  copy_cstr(net_cfg.sta_ssid, sizeof(net_cfg.sta_ssid), ssid.c_str());
  copy_cstr(net_cfg.sta_password, sizeof(net_cfg.sta_password), pass.c_str());
  sanitize_hostname(host, net_cfg.hostname, sizeof(net_cfg.hostname));
  setBatteryPresenceMode(parse_battery_presence_mode(battery_mode));

  if (mode_norm == "sta" && net_cfg.sta_ssid[0] != '\0') {
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
