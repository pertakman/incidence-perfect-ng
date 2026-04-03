#pragma once

#include <WiFi.h>

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

extern char ap_ssid[32];
extern const char *ap_password;
extern NetworkConfig net_cfg;
extern NetworkRunMode net_run_mode;
extern bool ap_active;
extern bool sta_connected;
extern bool sta_attempt_active;
extern bool mdns_active;
extern unsigned long sta_attempt_started_ms;
extern unsigned long next_sta_retry_ms;

const char *network_run_mode_text();
void ip_to_str(const IPAddress &ip, char *out, size_t out_size);
void stop_mdns_if_active();
void stop_access_point(bool verbose);
void switch_to_ap_only_mode();
void apply_network_config();
void loop_network_manager();
