#include <unity.h>
#include "../../src/helpers/AdvertDataHelpers.h"
#include "../../src/helpers/AdvertDataHelpers.cpp"

static void makeAdvert(uint8_t type, uint16_t feat1, uint8_t* out, uint8_t& len) {
  AdvertDataBuilder b(type);
  b.setFeat1(feat1);
  len = b.encodeTo(out);
}

void test_non_dispatch_adv_flagged() {
  uint8_t app[MAX_ADVERT_DATA_SIZE];
  uint8_t len = 0;
  makeAdvert(ADV_TYPE_ROOM, 0, app, len);
  AdvertDataParser p(app, len);
  TEST_ASSERT_TRUE(p.isValid());
  TEST_ASSERT_EQUAL_UINT8(ADV_TYPE_ROOM, p.getType());
  TEST_ASSERT_EQUAL_UINT16(0, p.getFeat1());
}

void test_dispatch_adv_flagged() {
  uint8_t app[MAX_ADVERT_DATA_SIZE];
  uint8_t len = 0;
  makeAdvert(ADV_TYPE_ROOM, ADV_FEAT1_DISPATCH, app, len);
  AdvertDataParser p(app, len);
  TEST_ASSERT_TRUE(p.isValid());
  TEST_ASSERT_EQUAL_UINT8(ADV_TYPE_ROOM, p.getType());
  TEST_ASSERT_TRUE(p.getFeat1() & ADV_FEAT1_DISPATCH);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_non_dispatch_adv_flagged);
  RUN_TEST(test_dispatch_adv_flagged);
  return UNITY_END();
}
