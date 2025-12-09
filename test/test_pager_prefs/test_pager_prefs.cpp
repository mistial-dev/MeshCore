#include <unity.h>
#include <filesystem>
#include <fstream>
#include "../lib/pager_prefs/PagerPrefs.h"

void test_defaults_round_trip() {
  PagerPrefs p = PagerPrefs::defaults();
  auto buf = p.serialize();
  PagerPrefs loaded;
  TEST_ASSERT_TRUE(PagerPrefs::deserialize(buf, loaded));
  TEST_ASSERT_EQUAL_UINT16(PagerPrefs::kVersion, loaded.version);
  TEST_ASSERT_EQUAL(true, loaded.two_shot_ack);
  TEST_ASSERT_EQUAL(false, loaded.pager_enabled);
  TEST_ASSERT_EQUAL_UINT16(120, loaded.heartbeat_secs);
  TEST_ASSERT_EQUAL_UINT8(8, loaded.ack_slot_count);
  TEST_ASSERT_EQUAL_UINT16(2400, loaded.ack_window_ms);
}

void test_file_round_trip() {
  PagerPrefs p = PagerPrefs::defaults();
  p.pager_enabled = true;
  p.heartbeat_secs = 90;
  auto buf = p.serialize();

  auto path = std::filesystem::temp_directory_path() / "meshcore_pager_prefs.bin";
  {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    ofs.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(buf.size()));
  }

  std::vector<uint8_t> read_buf;
  {
    std::ifstream ifs(path, std::ios::binary);
    read_buf.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
  }

  PagerPrefs loaded;
  TEST_ASSERT_TRUE(PagerPrefs::deserialize(read_buf, loaded));
  TEST_ASSERT_TRUE(loaded.pager_enabled);
  TEST_ASSERT_EQUAL_UINT16(90, loaded.heartbeat_secs);
  TEST_ASSERT_EQUAL_UINT8(8, loaded.ack_slot_count);
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_round_trip);
  RUN_TEST(test_file_round_trip);
  return UNITY_END();
}
