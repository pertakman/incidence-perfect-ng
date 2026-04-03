#pragma once

#include <Arduino.h>

#include "inclinometer_shared.h"

const char *battery_presence_mode_to_pref(BatteryPresenceMode mode);
BatteryPresenceMode parse_battery_presence_mode(const String &raw);
DisplayPrecisionMode parse_display_precision_mode(const String &raw);
uint8_t parse_display_brightness_percent(const String &raw);
void sanitize_hostname(const String &raw, char *dst, size_t dst_size);
bool parse_bool_flag(const String &raw);

bool load_network_config();
bool save_network_config();

void apply_network_recovery_defaults(bool clear_sta_credentials);
bool action_button_network_recovery_requested();
