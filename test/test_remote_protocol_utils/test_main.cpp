#include <unity.h>

#include "remote_protocol_utils.h"

void test_parse_bool_flag_text_accepts_common_true_values() {
  TEST_ASSERT_TRUE(parse_bool_flag_text("1"));
  TEST_ASSERT_TRUE(parse_bool_flag_text(" true "));
  TEST_ASSERT_TRUE(parse_bool_flag_text("YES"));
  TEST_ASSERT_TRUE(parse_bool_flag_text("On"));
}

void test_parse_bool_flag_text_rejects_other_values() {
  TEST_ASSERT_FALSE(parse_bool_flag_text(nullptr));
  TEST_ASSERT_FALSE(parse_bool_flag_text(""));
  TEST_ASSERT_FALSE(parse_bool_flag_text("0"));
  TEST_ASSERT_FALSE(parse_bool_flag_text("false"));
  TEST_ASSERT_FALSE(parse_bool_flag_text("maybe"));
}

void test_normalize_sha256_hex_text_normalizes_uppercase_input() {
  char out[65] = {0};
  TEST_ASSERT_TRUE(normalize_sha256_hex_text(
    "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF",
    out,
    sizeof(out)
  ));
  TEST_ASSERT_EQUAL_STRING(
    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
    out
  );
}

void test_normalize_sha256_hex_text_rejects_invalid_input() {
  char out[65] = {0};
  TEST_ASSERT_FALSE(normalize_sha256_hex_text(nullptr, out, sizeof(out)));
  TEST_ASSERT_FALSE(normalize_sha256_hex_text("abc", out, sizeof(out)));
  TEST_ASSERT_FALSE(normalize_sha256_hex_text(
    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdeg",
    out,
    sizeof(out)
  ));
  TEST_ASSERT_FALSE(normalize_sha256_hex_text(
    "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef",
    out,
    16
  ));
}

void test_parse_version_triplet_text_accepts_release_and_suffix_forms() {
  int year = 0, minor = 0, patch = 0;
  TEST_ASSERT_TRUE(parse_version_triplet_text("2026.3.18", year, minor, patch));
  TEST_ASSERT_EQUAL_INT(2026, year);
  TEST_ASSERT_EQUAL_INT(3, minor);
  TEST_ASSERT_EQUAL_INT(18, patch);

  TEST_ASSERT_TRUE(parse_version_triplet_text("2026.3.18-rc2", year, minor, patch));
  TEST_ASSERT_EQUAL_INT(2026, year);
  TEST_ASSERT_EQUAL_INT(3, minor);
  TEST_ASSERT_EQUAL_INT(18, patch);
}

void test_parse_version_triplet_text_rejects_bad_shapes() {
  int year = 0, minor = 0, patch = 0;
  TEST_ASSERT_FALSE(parse_version_triplet_text(nullptr, year, minor, patch));
  TEST_ASSERT_FALSE(parse_version_triplet_text("", year, minor, patch));
  TEST_ASSERT_FALSE(parse_version_triplet_text("2026", year, minor, patch));
  TEST_ASSERT_FALSE(parse_version_triplet_text("2026.3", year, minor, patch));
  TEST_ASSERT_FALSE(parse_version_triplet_text("2026.x.1", year, minor, patch));
}

void test_compare_version_triplet_text_orders_versions() {
  bool ok = false;
  TEST_ASSERT_EQUAL_INT(1, compare_version_triplet_text("2026.3.19", "2026.3.18", &ok));
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_INT(-1, compare_version_triplet_text("2026.3.17", "2026.3.18", &ok));
  TEST_ASSERT_TRUE(ok);
  TEST_ASSERT_EQUAL_INT(0, compare_version_triplet_text("2026.3.18", "2026.3.18", &ok));
  TEST_ASSERT_TRUE(ok);
}

void test_compare_version_triplet_text_reports_parse_failures() {
  bool ok = true;
  TEST_ASSERT_EQUAL_INT(0, compare_version_triplet_text("bad", "2026.3.18", &ok));
  TEST_ASSERT_FALSE(ok);
}

void test_decode_remote_command_accepts_plain_tokens() {
  TEST_ASSERT_EQUAL_INT(REMOTE_CMD_ZERO, decode_remote_command("zero"));
  TEST_ASSERT_EQUAL_INT(REMOTE_CMD_MODE_VERTICAL, decode_remote_command(" mode_vertical "));
  TEST_ASSERT_EQUAL_STRING("offset_cal", remote_command_text(REMOTE_CMD_OFFSET_CAL));
}

void test_decode_remote_command_accepts_json_cmd_payloads() {
  TEST_ASSERT_EQUAL_INT(REMOTE_CMD_CONFIRM, decode_remote_command("{\"cmd\":\"confirm\"}"));
  TEST_ASSERT_EQUAL_INT(REMOTE_CMD_ALIGN_START, decode_remote_command("{\"cmd\": \"align_start\"}"));
}

void test_decode_remote_command_rejects_ambiguous_or_unknown_payloads() {
  TEST_ASSERT_EQUAL_INT(REMOTE_CMD_NONE, decode_remote_command("{\"note\":\"zero\"}"));
  TEST_ASSERT_EQUAL_INT(REMOTE_CMD_NONE, decode_remote_command("{\"cmd\":\"unknown\"}"));
  TEST_ASSERT_EQUAL_INT(REMOTE_CMD_NONE, decode_remote_command("zero please"));
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  UNITY_BEGIN();
  RUN_TEST(test_parse_bool_flag_text_accepts_common_true_values);
  RUN_TEST(test_parse_bool_flag_text_rejects_other_values);
  RUN_TEST(test_normalize_sha256_hex_text_normalizes_uppercase_input);
  RUN_TEST(test_normalize_sha256_hex_text_rejects_invalid_input);
  RUN_TEST(test_parse_version_triplet_text_accepts_release_and_suffix_forms);
  RUN_TEST(test_parse_version_triplet_text_rejects_bad_shapes);
  RUN_TEST(test_compare_version_triplet_text_orders_versions);
  RUN_TEST(test_compare_version_triplet_text_reports_parse_failures);
  RUN_TEST(test_decode_remote_command_accepts_plain_tokens);
  RUN_TEST(test_decode_remote_command_accepts_json_cmd_payloads);
  RUN_TEST(test_decode_remote_command_rejects_ambiguous_or_unknown_payloads);
  return UNITY_END();
}
