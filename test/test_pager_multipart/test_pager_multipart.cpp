#include <unity.h>

// Assert that multipart support has been removed; paging is single-packet only.
#if __has_include("../../examples/companion_radio/PagerMultipart.h")
#error "PagerMultipart.h should not exist; pager pages are single-part."
#endif

void test_no_multipart_header_present() {
  TEST_ASSERT_TRUE(true); // placeholder to keep suite active
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_no_multipart_header_present);
  return UNITY_END();
}
