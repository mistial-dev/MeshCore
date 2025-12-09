#include <unity.h>
#include "../../examples/companion_radio/PagerHelpers.h"

void test_default_fallback() {
  auto lvl = parsePagerPriority("hello", PagerAlertLevel::D);
  TEST_ASSERT_EQUAL(PagerAlertLevel::D, lvl);
}

void test_uppercase_prio() {
  auto lvl = parsePagerPriority("[PRIO:E] message", PagerAlertLevel::A);
  TEST_ASSERT_EQUAL(PagerAlertLevel::E, lvl);
}

void test_lowercase_prio() {
  auto lvl = parsePagerPriority("prefix [prio:b] body", PagerAlertLevel::A);
  TEST_ASSERT_EQUAL(PagerAlertLevel::B, lvl);
}

void test_invalid_keeps_fallback() {
  auto lvl = parsePagerPriority("[PRIO:Z]", PagerAlertLevel::C);
  TEST_ASSERT_EQUAL(PagerAlertLevel::C, lvl);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_default_fallback);
  RUN_TEST(test_uppercase_prio);
  RUN_TEST(test_lowercase_prio);
  RUN_TEST(test_invalid_keeps_fallback);
  return UNITY_END();
}
