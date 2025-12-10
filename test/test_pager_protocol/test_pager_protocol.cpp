#include <unity.h>
#include "../../examples/companion_radio/PagerProtocol.h"

void test_parse_ndid_basic() {
  auto m = parseNDID("[NDID] ID:5 GROUP:A,C");
  TEST_ASSERT_TRUE(m.valid);
  TEST_ASSERT_FALSE(m.evict);
  TEST_ASSERT_EQUAL_UINT16(5, m.pager_id);
  TEST_ASSERT_EQUAL_UINT8((1u << 0) | (1u << 2), m.groups);
}

void test_parse_ndid_evict() {
  auto m = parseNDID("[NDID] ID:42 GROUP:X");
  TEST_ASSERT_TRUE(m.valid);
  TEST_ASSERT_TRUE(m.evict);
  TEST_ASSERT_EQUAL_UINT16(42, m.pager_id);
}

void test_parse_ndid_ignores_beyond_h() {
  auto m = parseNDID("[NDID] ID:2 GROUP:A,J");
  TEST_ASSERT_TRUE(m.valid);
  TEST_ASSERT_FALSE(m.evict);
  TEST_ASSERT_EQUAL_UINT8(1u << 0, m.groups); // J ignored
}

void test_parse_otar() {
  auto m = parseOTAR("[OTAR] PASS:onetimer");
  TEST_ASSERT_TRUE(m.valid);
  TEST_ASSERT_EQUAL_STRING("onetimer", m.password);
}

void test_admin_tag_detection() {
  TEST_ASSERT_TRUE(isPagerAdminTag("[PAGE] body"));
  TEST_ASSERT_TRUE(isPagerAdminTag("[OTAR]"));
  TEST_ASSERT_FALSE(isPagerAdminTag("PAGE plain"));
  TEST_ASSERT_FALSE(isPagerAdminTag("[Page] body"));
  TEST_ASSERT_FALSE(isPagerAdminTag("[PAGER longer]"));
  TEST_ASSERT_FALSE(isPagerAdminTag(nullptr));
}

void test_admin_rejects_non_admin() {
  char err[32];
  bool rej = pagerAdminRejectMessage(false, "[PAGE] hi", err, sizeof(err));
  TEST_ASSERT_TRUE(rej);
  TEST_ASSERT_EQUAL_STRING("ERR admin-only tag", err);
}

void test_admin_allows_admin() {
  char err[8] = {};
  bool rej = pagerAdminRejectMessage(true, "[PAGE] hi", err, sizeof(err));
  TEST_ASSERT_FALSE(rej);
  TEST_ASSERT_EQUAL_STRING("", err);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_parse_ndid_basic);
  RUN_TEST(test_parse_ndid_evict);
  RUN_TEST(test_parse_ndid_ignores_beyond_h);
  RUN_TEST(test_parse_otar);
  RUN_TEST(test_admin_tag_detection);
  RUN_TEST(test_admin_rejects_non_admin);
  RUN_TEST(test_admin_allows_admin);
  return UNITY_END();
}
