#ifndef UNIT_TEST
#include "AckHash.h"
#include <Utils.h>

uint32_t computeAckHash(const mesh::Identity& peer_id, const uint8_t* data, size_t len) {
  uint32_t ack_hash;
  mesh::Utils::sha256((uint8_t*)&ack_hash, 4, data, len, peer_id.pub_key, PUB_KEY_SIZE);
  return ack_hash;
}
#endif
