#pragma once

#include <cstdint>
#include <cstddef>
#include "../Identity.h"

#ifdef UNIT_TEST
#include <functional>
#include <string>
// For host tests, use a deterministic hash to avoid pulling crypto deps.
inline uint32_t computeAckHash(const mesh::Identity& peer_id, const uint8_t* data, size_t len) {
  std::string s(reinterpret_cast<const char*>(data), len);
  s.append(reinterpret_cast<const char*>(peer_id.pub_key), PUB_KEY_SIZE);
  uint64_t h = std::hash<std::string>{}(s);
  return static_cast<uint32_t>(h & 0xFFFFFFFF);
}
#else
// Computes the truncated ACK hash used for message acknowledgements.
uint32_t computeAckHash(const mesh::Identity& peer_id, const uint8_t* data, size_t len);
#endif
