#include <unity.h>
#include "../lib/pager_prefs/SlotPlanner.h"

static const uint8_t kKeyA[32] = {0x01,0x02,0x03,0x04};
static const uint8_t kKeyB[32] = {0x10,0x20,0x30,0x40};

void test_deterministic_slot() {
  auto a1 = planSlot(kKeyA, 8, 2400);
  auto a2 = planSlot(kKeyA, 8, 2400);
  TEST_ASSERT_EQUAL_UINT8(a1.slot, a2.slot);
  TEST_ASSERT_EQUAL_UINT16(a1.offset_ms, a2.offset_ms);
  TEST_ASSERT_LESS_THAN_UINT16(2400, a1.offset_ms + 1); // offset < window
}

void test_different_keys_likely_different_slots() {
  auto a = planSlot(kKeyA, 8, 2400);
  auto b = planSlot(kKeyB, 8, 2400);
  TEST_ASSERT_NOT_EQUAL(a.slot, b.slot);
}

void test_zero_slots_safe() {
  auto a = planSlot(kKeyA, 0, 2400);
  TEST_ASSERT_EQUAL_UINT8(0, a.slot);
  TEST_ASSERT_EQUAL_UINT16(0, a.offset_ms);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_deterministic_slot);
  RUN_TEST(test_different_keys_likely_different_slots);
  RUN_TEST(test_zero_slots_safe);
  return UNITY_END();
}
