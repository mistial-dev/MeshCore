#include <unity.h>
#include <filesystem>
#include <fstream>
#include <cstring>
#include "../../examples/companion_radio/PagerDispatchRecord.h"

void test_defaults_and_clear() {
  PagerDispatchRecord rec;
  TEST_ASSERT_FALSE(rec.isSet());
  // set one byte then clear
  rec.dispatch_pub_key[0] = 0xAA;
  TEST_ASSERT_TRUE(rec.isSet());
  rec.clear();
  TEST_ASSERT_FALSE(rec.isSet());
}

void test_round_trip_file() {
  PagerDispatchRecord rec;
  rec.dispatch_pub_key[0] = 0x01;
  rec.dispatch_pub_key[1] = 0x02;
  strcpy(rec.dispatch_name, "dispatch");
  rec.last_join_time = 1234;

  auto path = std::filesystem::temp_directory_path() / "pager_dispatch.bin";
  {
    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    ofs.write(reinterpret_cast<const char*>(&rec), sizeof(rec));
  }

  PagerDispatchRecord loaded{};
  {
    std::ifstream ifs(path, std::ios::binary);
    ifs.read(reinterpret_cast<char*>(&loaded), sizeof(loaded));
  }

  TEST_ASSERT_EQUAL_UINT16(rec.version, loaded.version);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(rec.dispatch_pub_key, loaded.dispatch_pub_key, sizeof(rec.dispatch_pub_key));
  TEST_ASSERT_EQUAL_STRING(rec.dispatch_name, loaded.dispatch_name);
  TEST_ASSERT_EQUAL_UINT32(rec.last_join_time, loaded.last_join_time);
}

void test_truncated_file_rejected() {
  uint8_t buf[sizeof(PagerDispatchRecord) - 4] = {0};
  PagerDispatchRecord out{};
  out.dispatch_pub_key[0] = 0xAA; // sentinel
  bool ok = PagerDispatchRecord::loadFromBuffer(buf, sizeof(buf), out);
  TEST_ASSERT_FALSE(ok);
  TEST_ASSERT_EQUAL_UINT8(0xAA, out.dispatch_pub_key[0]); // unchanged
}

void test_wrong_version_rejected() {
  PagerDispatchRecord rec{};
  rec.version = PagerDispatchRecord::kVersion + 1;
  PagerDispatchRecord out{};
  bool ok = PagerDispatchRecord::loadFromBuffer(reinterpret_cast<const uint8_t*>(&rec), sizeof(rec), out);
  TEST_ASSERT_FALSE(ok);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_defaults_and_clear);
  RUN_TEST(test_round_trip_file);
  RUN_TEST(test_truncated_file_rejected);
  RUN_TEST(test_wrong_version_rejected);
  return UNITY_END();
}
