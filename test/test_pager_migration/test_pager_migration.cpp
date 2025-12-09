#include <unity.h>
#include <string>
#include "../lib/core_prefs/CorePrefs.h"
#include "../lib/pager_prefs/Migration.h"

void test_upgrade_preserves_core_sets_pager_defaults() {
  CorePrefsImage old = makeCorePrefsDefaults();
  std::strcpy(old.prefs.node_name, "NODE123");
  old.prefs.tx_power_dbm = 18;
  old.node_lat = 12.34;
  old.node_lon = -45.67;

  auto upgraded = upgradeToPager(old);
  TEST_ASSERT_EQUAL_STRING("NODE123", upgraded.core.prefs.node_name);
  TEST_ASSERT_EQUAL_UINT8(18, upgraded.core.prefs.tx_power_dbm);
  TEST_ASSERT_EQUAL_FLOAT(12.34f, upgraded.core.node_lat);
  TEST_ASSERT_EQUAL_FLOAT(-45.67f, upgraded.core.node_lon);

  TEST_ASSERT_FALSE(upgraded.pager.pager_enabled);
  TEST_ASSERT_TRUE(upgraded.pager.two_shot_ack);
  TEST_ASSERT_EQUAL_UINT16(120, upgraded.pager.heartbeat_secs);
  TEST_ASSERT_EQUAL_UINT8(8, upgraded.pager.ack_slot_count);
  TEST_ASSERT_EQUAL_UINT16(2400, upgraded.pager.ack_window_ms);
}

void test_upgrade_from_serialized_blob() {
  CorePrefsImage old = makeCorePrefsDefaults();
  old.prefs.sf = 9;
  old.prefs.cr = 7;
  old.prefs.bw = 125.0f;
  old.prefs.tx_power_dbm = 14;
  auto blob = serializeCorePrefs(old);

  CorePrefsImage parsed;
  TEST_ASSERT_TRUE(deserializeCorePrefs(blob, parsed));

  auto upgraded = upgradeToPager(parsed);
  TEST_ASSERT_EQUAL_UINT8(9, upgraded.core.prefs.sf);
  TEST_ASSERT_EQUAL_UINT8(7, upgraded.core.prefs.cr);
  TEST_ASSERT_EQUAL_FLOAT(125.0f, upgraded.core.prefs.bw);
  TEST_ASSERT_EQUAL_UINT8(14, upgraded.core.prefs.tx_power_dbm);
  TEST_ASSERT_TRUE(upgraded.pager.two_shot_ack);
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_upgrade_preserves_core_sets_pager_defaults);
  RUN_TEST(test_upgrade_from_serialized_blob);
  return UNITY_END();
}
