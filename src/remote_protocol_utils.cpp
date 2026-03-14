#include "remote_protocol_utils.h"

#include <ctype.h>
#include <string.h>

namespace {

struct CommandEntry {
  RemoteCommandId id;
  const char *text;
};

const CommandEntry kCommands[] = {
  {REMOTE_CMD_ZERO, "zero"},
  {REMOTE_CMD_AXIS, "axis"},
  {REMOTE_CMD_FREEZE, "freeze"},
  {REMOTE_CMD_SLEEP, "sleep"},
  {REMOTE_CMD_OFFSET_CAL, "offset_cal"},
  {REMOTE_CMD_ROTATE, "rotate"},
  {REMOTE_CMD_MODE_TOGGLE, "mode_toggle"},
  {REMOTE_CMD_MODE_UP, "mode_up"},
  {REMOTE_CMD_MODE_VERTICAL, "mode_vertical"},
  {REMOTE_CMD_ALIGN_START, "align_start"},
  {REMOTE_CMD_CAPTURE, "capture"},
  {REMOTE_CMD_CONFIRM, "confirm"},
  {REMOTE_CMD_CANCEL, "cancel"},
};

const char *skip_ws(const char *p) {
  while (p && *p && isspace((unsigned char)*p)) {
    ++p;
  }
  return p;
}

size_t trimmed_length(const char *raw, const char **trimmed_start) {
  if (!raw) {
    if (trimmed_start) *trimmed_start = nullptr;
    return 0;
  }
  const char *start = skip_ws(raw);
  const char *end = start + strlen(start);
  while (end > start && isspace((unsigned char)end[-1])) {
    --end;
  }
  if (trimmed_start) *trimmed_start = start;
  return (size_t)(end - start);
}

bool text_equals(const char *start, size_t len, const char *literal) {
  const size_t literal_len = strlen(literal);
  return len == literal_len && strncmp(start, literal, len) == 0;
}

RemoteCommandId parse_command_token(const char *start, size_t len) {
  for (size_t i = 0; i < (sizeof(kCommands) / sizeof(kCommands[0])); ++i) {
    if (text_equals(start, len, kCommands[i].text)) {
      return kCommands[i].id;
    }
  }
  return REMOTE_CMD_NONE;
}

} // namespace

const char *remote_command_text(RemoteCommandId cmd) {
  for (size_t i = 0; i < (sizeof(kCommands) / sizeof(kCommands[0])); ++i) {
    if (kCommands[i].id == cmd) {
      return kCommands[i].text;
    }
  }
  return "";
}

RemoteCommandId decode_remote_command(const char *raw) {
  const char *trimmed = nullptr;
  const size_t trimmed_len = trimmed_length(raw, &trimmed);
  if (!trimmed || trimmed_len == 0) {
    return REMOTE_CMD_NONE;
  }

  const RemoteCommandId direct = parse_command_token(trimmed, trimmed_len);
  if (direct != REMOTE_CMD_NONE) {
    return direct;
  }

  const char *cmd_key = strstr(trimmed, "\"cmd\"");
  if (!cmd_key) {
    return REMOTE_CMD_NONE;
  }
  const char *colon = strchr(cmd_key + 5, ':');
  if (!colon) {
    return REMOTE_CMD_NONE;
  }
  const char *value = skip_ws(colon + 1);
  if (!value || *value != '"') {
    return REMOTE_CMD_NONE;
  }
  ++value;
  const char *end_quote = strchr(value, '"');
  if (!end_quote) {
    return REMOTE_CMD_NONE;
  }
  return parse_command_token(value, (size_t)(end_quote - value));
}

bool parse_bool_flag_text(const char *raw) {
  const char *trimmed = nullptr;
  const size_t len = trimmed_length(raw, &trimmed);
  if (!trimmed || len == 0) return false;

  char lower[8];
  if (len >= sizeof(lower)) return false;
  for (size_t i = 0; i < len; ++i) {
    lower[i] = (char)tolower((unsigned char)trimmed[i]);
  }
  lower[len] = '\0';

  return strcmp(lower, "1") == 0 ||
         strcmp(lower, "true") == 0 ||
         strcmp(lower, "yes") == 0 ||
         strcmp(lower, "on") == 0;
}

bool normalize_sha256_hex_text(const char *raw, char *dst, size_t dst_size) {
  if (!dst || dst_size < 65) return false;
  const char *trimmed = nullptr;
  const size_t len = trimmed_length(raw, &trimmed);
  if (!trimmed || len != 64) return false;

  for (size_t i = 0; i < 64; ++i) {
    const char c = (char)tolower((unsigned char)trimmed[i]);
    const bool hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    if (!hex) return false;
    dst[i] = c;
  }
  dst[64] = '\0';
  return true;
}

bool parse_version_triplet_text(const char *src, int &year, int &minor, int &patch) {
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
  return true;
}

int compare_version_triplet_text(const char *lhs, const char *rhs, bool *ok) {
  int ly = 0, lm = 0, lp = 0;
  int ry = 0, rm = 0, rp = 0;
  const bool lhs_ok = parse_version_triplet_text(lhs, ly, lm, lp);
  const bool rhs_ok = parse_version_triplet_text(rhs, ry, rm, rp);
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
