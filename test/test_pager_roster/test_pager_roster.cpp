#include <unity.h>
#include "../lib/pager_roster/Roster.h"

void test_groups_from_list() {
  TEST_ASSERT_EQUAL_UINT8(0, makeGroups(nullptr));
  TEST_ASSERT_EQUAL_UINT8(0, makeGroups(""));
  TEST_ASSERT_EQUAL_UINT8(1u << 0, makeGroups("A"));
  TEST_ASSERT_EQUAL_UINT8(1u << 0, makeGroups("a"));
  TEST_ASSERT_EQUAL_UINT8((1u << 0) | (1u << 2), makeGroups("A,C"));
  TEST_ASSERT_EQUAL_UINT8((1u << 0) | (1u << 1) | (1u << 2), makeGroups("abc"));
  // groups beyond H are ignored (mask stays 8 bits)
  TEST_ASSERT_EQUAL_UINT8(1u << 0, makeGroups("A,J"));
}

void test_roster_entry_defaults() {
  PagerRosterEntry e;
  TEST_ASSERT_FALSE(e.isSet());
  TEST_ASSERT_EQUAL_UINT16(PagerRosterEntry::kVersion, e.version);
  TEST_ASSERT_EQUAL_UINT8(0, e.groups);
  TEST_ASSERT_EQUAL_UINT16(0, e.pager_id);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_groups_from_list);
  RUN_TEST(test_roster_entry_defaults);
  return UNITY_END();
}
