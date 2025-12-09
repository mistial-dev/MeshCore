#include <unity.h>
#include "../../src/helpers/AdvertDataHelpers.h"
#include "../../src/helpers/AdvertDataHelpers.cpp"

void test_dispatch_feat1_round_trip() {
#ifndef ADV_FEAT1_DISPATCH
  TEST_IGNORE_MESSAGE("Pager mode disabled; dispatch feat1 not defined");
  return;
#endif
  AdvertDataBuilder b(ADV_TYPE_ROOM, "DispatchRoom");
  b.setFeat1(ADV_FEAT1_DISPATCH);
  uint8_t buf[MAX_ADVERT_DATA_SIZE] = {0};
  uint8_t len = b.encodeTo(buf);
  TEST_ASSERT(len > 0);

  AdvertDataParser p(buf, len);
  TEST_ASSERT_TRUE(p.isValid());
  TEST_ASSERT_EQUAL(ADV_TYPE_ROOM, p.getType());
  TEST_ASSERT_TRUE(p.hasName());
  TEST_ASSERT_EQUAL_STRING("DispatchRoom", p.getName());
  TEST_ASSERT_EQUAL_UINT16(ADV_FEAT1_DISPATCH, p.getFeat1() & ADV_FEAT1_DISPATCH);
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_dispatch_feat1_round_trip);
  return UNITY_END();
}
