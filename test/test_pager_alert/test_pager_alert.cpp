#include <unity.h>
#include "../../examples/companion_radio/PagerAlertTones.h"
#include <cstring>

void test_has_five_levels() {
  TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(PagerAlertLevel::COUNT), 5);
}

void test_latched_level_e() {
  const auto& toneE = getPagerAlertTone(PagerAlertLevel::E);
  TEST_ASSERT_TRUE(toneE.latched);
  TEST_ASSERT_NOT_NULL(toneE.melody);
}

void test_unique_names() {
  const char* names[5];
  for (int i = 0; i < 5; i++) {
    names[i] = kPagerAlertTones[i].name;
  }
  for (int i = 0; i < 5; i++) {
    for (int j = i + 1; j < 5; j++) {
      TEST_ASSERT_NOT_EQUAL(0, strcmp(names[i], names[j]));
    }
  }
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_has_five_levels);
  RUN_TEST(test_latched_level_e);
  RUN_TEST(test_unique_names);
  return UNITY_END();
}
