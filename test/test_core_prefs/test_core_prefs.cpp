#include <unity.h>
#include <filesystem>
#include <fstream>
#include "../lib/core_prefs/CorePrefs.h"

void test_core_round_trip_memory() {
  auto orig = makeCorePrefsDefaults();
  auto buf = serializeCorePrefs(orig);
  CorePrefsImage parsed;
  TEST_ASSERT_TRUE(deserializeCorePrefs(buf, parsed));
  TEST_ASSERT_EQUAL_FLOAT(orig.prefs.airtime_factor, parsed.prefs.airtime_factor);
  TEST_ASSERT_EQUAL_STRING(orig.prefs.node_name, parsed.prefs.node_name);
  TEST_ASSERT_EQUAL_FLOAT(orig.prefs.freq, parsed.prefs.freq);
  TEST_ASSERT_EQUAL_UINT8(orig.prefs.sf, parsed.prefs.sf);
  TEST_ASSERT_EQUAL_UINT8(orig.prefs.cr, parsed.prefs.cr);
  TEST_ASSERT_EQUAL_FLOAT(orig.prefs.bw, parsed.prefs.bw);
  TEST_ASSERT_EQUAL_UINT8(orig.prefs.tx_power_dbm, parsed.prefs.tx_power_dbm);
  TEST_ASSERT_EQUAL_FLOAT(orig.node_lat, parsed.node_lat);
  TEST_ASSERT_EQUAL_FLOAT(orig.node_lon, parsed.node_lon);
}

void test_core_round_trip_file() {
  auto orig = makeCorePrefsDefaults();
  orig.prefs.tx_power_dbm = 20;
  orig.node_lat = 37.1234;
  orig.node_lon = -122.9876;
  auto buf = serializeCorePrefs(orig);

  auto path = std::filesystem::temp_directory_path() / "meshcore_core_prefs.bin";
  {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    ofs.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
  }
  std::vector<uint8_t> read_buf;
  {
    std::ifstream ifs(path, std::ios::binary);
    read_buf.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
  }
  CorePrefsImage parsed;
  TEST_ASSERT_TRUE(deserializeCorePrefs(read_buf, parsed));
  TEST_ASSERT_EQUAL_UINT8(orig.prefs.tx_power_dbm, parsed.prefs.tx_power_dbm);
  TEST_ASSERT_EQUAL_FLOAT(orig.node_lat, parsed.node_lat);
  TEST_ASSERT_EQUAL_FLOAT(orig.node_lon, parsed.node_lon);
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_core_round_trip_memory);
  RUN_TEST(test_core_round_trip_file);
  return UNITY_END();
}
