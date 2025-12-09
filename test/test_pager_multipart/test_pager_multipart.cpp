#include <unity.h>
#include "../../examples/companion_radio/PagerMultipart.h"

#if defined(PAGER_MODE) && !defined(DISPATCH_NODE)

static const uint8_t kSenderA[PUB_KEY_SIZE] = {1};

void test_single_part_passthrough() {
  PagerMultipartAssembler asmbl;
  const char* out = nullptr;
  auto res = asmbl.ingest(kSenderA, "hello world", 0, nullptr, 0);
  TEST_ASSERT_EQUAL(PagerMultipartAssembler::Result::Single, res);
}

void test_multi_part_complete() {
  PagerMultipartAssembler asmbl;
  char combined[128];
  asmbl.ingest(kSenderA, "1/2 part one", 0, combined, sizeof(combined));
  auto res = asmbl.ingest(kSenderA, "2/2 part two", 1000, combined, sizeof(combined));
  TEST_ASSERT_EQUAL(PagerMultipartAssembler::Result::Complete, res);
  TEST_ASSERT_TRUE(strstr(combined, "part one") != nullptr);
  TEST_ASSERT_TRUE(strstr(combined, "part two") != nullptr);
}

void test_timeout_missing() {
  PagerMultipartAssembler asmbl;
  char combined[64];
  asmbl.ingest(kSenderA, "1/3 first", 0, combined, sizeof(combined));
  TEST_ASSERT_TRUE(asmbl.hasTimedOut(PagerMultipartAssembler::kTimeoutMs + 10));
  TEST_ASSERT_NOT_EQUAL(0, asmbl.missingMask());
}

void test_reject_too_many_parts() {
  PagerMultipartAssembler asmbl;
  char combined[64];
  auto res = asmbl.ingest(kSenderA, "5/6 nope", 0, combined, sizeof(combined));
  TEST_ASSERT_EQUAL(PagerMultipartAssembler::Result::Rejected, res);
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_single_part_passthrough);
  RUN_TEST(test_multi_part_complete);
  RUN_TEST(test_timeout_missing);
  RUN_TEST(test_reject_too_many_parts);
  return UNITY_END();
}

#else
int main(int, char**) { return 0; }
#endif
