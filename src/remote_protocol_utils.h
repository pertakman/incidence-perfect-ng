#pragma once

#include <stddef.h>

enum RemoteCommandId {
  REMOTE_CMD_NONE = 0,
  REMOTE_CMD_ZERO,
  REMOTE_CMD_AXIS,
  REMOTE_CMD_FREEZE,
  REMOTE_CMD_SLEEP,
  REMOTE_CMD_OFFSET_CAL,
  REMOTE_CMD_ROTATE,
  REMOTE_CMD_MODE_TOGGLE,
  REMOTE_CMD_MODE_UP,
  REMOTE_CMD_MODE_VERTICAL,
  REMOTE_CMD_ALIGN_START,
  REMOTE_CMD_CAPTURE,
  REMOTE_CMD_CONFIRM,
  REMOTE_CMD_CANCEL
};

const char *remote_command_text(RemoteCommandId cmd);
RemoteCommandId decode_remote_command(const char *raw);

bool parse_bool_flag_text(const char *raw);
bool normalize_sha256_hex_text(const char *raw, char *dst, size_t dst_size);
bool parse_version_triplet_text(const char *src, int &year, int &minor, int &patch);
int compare_version_triplet_text(const char *lhs, const char *rhs, bool *ok = nullptr);
