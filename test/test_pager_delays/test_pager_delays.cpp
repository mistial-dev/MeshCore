#include <unity.h>
#include <cstdint>
#include "../../examples/companion_radio/PagerSlotHelper.h"
#include "../lib/pager_prefs/SlotPlanner.h"

static const uint8_t kKeyA[32] = {0x01, 0x02, 0x03, 0x04};
static const uint8_t kKeyB[32] = {0x10, 0x20, 0x30, 0x40};

void test_helper_matches_planner() {
  auto plan = planSlot(kKeyA, 8, 2400);
  uint16_t helper = pagerSlotDelayMs(kKeyA, 8, 2400);
  TEST_ASSERT_EQUAL_UINT16(plan.offset_ms, helper);
}

void test_in_range_and_different_keys() {
  uint16_t a = pagerSlotDelayMs(kKeyA, 8, 2400);
  uint16_t b = pagerSlotDelayMs(kKeyB, 8, 2400);
  TEST_ASSERT_LESS_THAN_UINT16(2400, a + 1);
  TEST_ASSERT_LESS_THAN_UINT16(2400, b + 1);
  TEST_ASSERT_NOT_EQUAL(a, b); // different pubkeys likely land in different slots
}

void test_zero_values_safe() {
  TEST_ASSERT_EQUAL_UINT16(0, pagerSlotDelayMs(kKeyA, 0, 2400));
  TEST_ASSERT_EQUAL_UINT16(0, pagerSlotDelayMs(kKeyA, 4, 0));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_helper_matches_planner);
  RUN_TEST(test_in_range_and_different_keys);
  RUN_TEST(test_zero_values_safe);
  return UNITY_END();
}
