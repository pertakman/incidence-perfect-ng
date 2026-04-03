#include "remote_control_ota.h"

#include <Update.h>
#include <mbedtls/sha256.h>
#include <string.h>

#include "remote_protocol_utils.h"

namespace {

bool ota_upload_ok = false;
bool ota_upload_in_progress = false;
char ota_error[160] = {0};
bool ota_force_install = false;
bool ota_sha_ctx_active = false;
bool ota_expected_sha256_set = false;
char ota_expected_sha256[65] = {0};
char ota_computed_sha256[65] = {0};
char ota_requested_version[24] = {0};
size_t ota_written_bytes = 0;
mbedtls_sha256_context ota_sha_ctx;

void copy_cstr(char *dst, size_t dst_size, const char *src) {
  if (!dst || dst_size == 0) return;
  if (!src) {
    dst[0] = '\0';
    return;
  }
  strncpy(dst, src, dst_size - 1);
  dst[dst_size - 1] = '\0';
}

void bytes_to_hex_lower(const uint8_t *bytes, size_t bytes_len, char *dst, size_t dst_size) {
  static const char hex[] = "0123456789abcdef";
  if (!dst || dst_size == 0) return;
  if (!bytes || dst_size < (bytes_len * 2 + 1)) {
    dst[0] = '\0';
    return;
  }
  size_t j = 0;
  for (size_t i = 0; i < bytes_len; ++i) {
    dst[j++] = hex[(bytes[i] >> 4) & 0x0F];
    dst[j++] = hex[bytes[i] & 0x0F];
  }
  dst[j] = '\0';
}

bool parse_bool_flag(const String &raw) {
  return parse_bool_flag_text(raw.c_str());
}

bool normalize_sha256_hex(const String &raw, char *dst, size_t dst_size) {
  return normalize_sha256_hex_text(raw.c_str(), dst, dst_size);
}

bool parse_version_triplet(const char *src, int &year, int &minor, int &patch) {
  return parse_version_triplet_text(src, year, minor, patch);
}

int compare_version_triplet(const char *lhs, const char *rhs, bool *ok = nullptr) {
  return compare_version_triplet_text(lhs, rhs, ok);
}

}  // namespace

bool ota_is_upload_in_progress() {
  return ota_upload_in_progress;
}

bool ota_is_upload_ok() {
  return ota_upload_ok;
}

const char *ota_last_error() {
  return ota_error;
}

void reset_ota_upload_session_state() {
  ota_upload_ok = false;
  ota_upload_in_progress = false;
  ota_error[0] = '\0';
  ota_force_install = false;
  ota_expected_sha256_set = false;
  ota_expected_sha256[0] = '\0';
  ota_computed_sha256[0] = '\0';
  ota_requested_version[0] = '\0';
  ota_written_bytes = 0;
  if (ota_sha_ctx_active) {
    mbedtls_sha256_free(&ota_sha_ctx);
    ota_sha_ctx_active = false;
  }
}

void handle_ota_upload_data(WebServer &server, const char *current_version) {
  HTTPUpload &upload = server.upload();
  switch (upload.status) {
    case UPLOAD_FILE_START: {
      reset_ota_upload_session_state();
      ota_upload_in_progress = true;

      String version_in = server.arg("version");
      String sha_in = server.arg("sha256");
      String force_in = server.arg("force");
      ota_force_install = parse_bool_flag(force_in);

      copy_cstr(ota_requested_version, sizeof(ota_requested_version), version_in.c_str());
      int target_year = 0;
      int target_minor = 0;
      int target_patch = 0;
      if (!parse_version_triplet(ota_requested_version, target_year, target_minor, target_patch)) {
        snprintf(ota_error, sizeof(ota_error), "missing/invalid version (YYYY.M.X). Reload UI and provide version+sha256.");
        break;
      }
      (void)target_year;
      (void)target_minor;
      (void)target_patch;

      bool cmp_ok = false;
      const int cmp = compare_version_triplet(ota_requested_version, current_version, &cmp_ok);
      if (!cmp_ok) {
        snprintf(ota_error, sizeof(ota_error), "version gate unavailable (current or target parse failed)");
        break;
      }
      if (cmp <= 0 && !ota_force_install) {
        snprintf(ota_error, sizeof(ota_error), "version gate rejected: target %s is not newer than current %s", ota_requested_version, current_version);
        break;
      }

      ota_expected_sha256_set = normalize_sha256_hex(sha_in, ota_expected_sha256, sizeof(ota_expected_sha256));
      if (!ota_expected_sha256_set) {
        snprintf(ota_error, sizeof(ota_error), "missing/invalid sha256 (64 hex). Reload UI and provide version+sha256.");
        break;
      }

      mbedtls_sha256_init(&ota_sha_ctx);
      if (mbedtls_sha256_starts_ret(&ota_sha_ctx, 0) != 0) {
        snprintf(ota_error, sizeof(ota_error), "sha256 init failed");
        mbedtls_sha256_free(&ota_sha_ctx);
        break;
      }
      ota_sha_ctx_active = true;

      if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
        snprintf(ota_error, sizeof(ota_error), "Update.begin failed");
      }
      break;
    }
    case UPLOAD_FILE_WRITE: {
      if (ota_error[0] != '\0') break;
      const size_t written = Update.write(upload.buf, upload.currentSize);
      if (written != upload.currentSize) {
        snprintf(ota_error, sizeof(ota_error), "Update.write failed");
        break;
      }
      ota_written_bytes += written;
      if (ota_sha_ctx_active) {
        if (mbedtls_sha256_update_ret(&ota_sha_ctx, upload.buf, upload.currentSize) != 0) {
          snprintf(ota_error, sizeof(ota_error), "sha256 update failed");
        }
      }
      break;
    }
    case UPLOAD_FILE_END: {
      if (ota_error[0] == '\0' && ota_written_bytes == 0) {
        snprintf(ota_error, sizeof(ota_error), "no firmware payload received");
      }

      if (ota_error[0] == '\0' && ota_sha_ctx_active) {
        uint8_t digest[32] = {0};
        if (mbedtls_sha256_finish_ret(&ota_sha_ctx, digest) != 0) {
          snprintf(ota_error, sizeof(ota_error), "sha256 finish failed");
        } else {
          bytes_to_hex_lower(digest, sizeof(digest), ota_computed_sha256, sizeof(ota_computed_sha256));
          if (strcmp(ota_computed_sha256, ota_expected_sha256) != 0) {
            snprintf(ota_error, sizeof(ota_error), "sha256 mismatch");
          }
        }
      }

      if (ota_sha_ctx_active) {
        mbedtls_sha256_free(&ota_sha_ctx);
        ota_sha_ctx_active = false;
      }

      if (ota_error[0] == '\0') {
        if (Update.end(true)) {
          ota_upload_ok = true;
        } else {
          snprintf(ota_error, sizeof(ota_error), "Update.end failed");
        }
      } else {
        if (Update.isRunning()) {
          Update.abort();
        }
      }
      ota_upload_in_progress = false;
      break;
    }
    case UPLOAD_FILE_ABORTED: {
      if (Update.isRunning()) {
        Update.abort();
      }
      if (ota_sha_ctx_active) {
        mbedtls_sha256_free(&ota_sha_ctx);
        ota_sha_ctx_active = false;
      }
      snprintf(ota_error, sizeof(ota_error), "Upload aborted");
      ota_upload_in_progress = false;
      ota_upload_ok = false;
      break;
    }
    default:
      break;
  }
}
