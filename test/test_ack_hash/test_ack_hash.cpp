#include <unity.h>
#include <string>
#include "../../src/helpers/AckHash.h"
#include "../../src/Identity.h"

void test_ack_hash_matches() {
  mesh::Identity peer{};
  memset(peer.pub_key, 0xAB, PUB_KEY_SIZE);
  const std::string body = "1/1 [GRP:A,C] [TON:A] EMS 2030 Benson Road";

  uint32_t h1 = computeAckHash(peer, reinterpret_cast<const uint8_t*>(body.data()), body.size());
  uint32_t h2 = computeAckHash(peer, reinterpret_cast<const uint8_t*>(body.data()), body.size());
  TEST_ASSERT_EQUAL_UINT32(h1, h2);
}

void test_ack_hash_differs_on_body_change() {
  mesh::Identity peer{};
  memset(peer.pub_key, 0xAB, PUB_KEY_SIZE);
  const std::string body1 = "1/1 [GRP:A,C] [TON:A] EMS 2030 Benson Road";
  const std::string body2 = "1/1 [GRP:A,C] [TON:A] Fire at Elm Street";

  uint32_t h1 = computeAckHash(peer, reinterpret_cast<const uint8_t*>(body1.data()), body1.size());
  uint32_t h2 = computeAckHash(peer, reinterpret_cast<const uint8_t*>(body2.data()), body2.size());
  TEST_ASSERT_NOT_EQUAL(h1, h2);
}

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_ack_hash_matches);
  RUN_TEST(test_ack_hash_differs_on_body_change);
  return UNITY_END();
}
