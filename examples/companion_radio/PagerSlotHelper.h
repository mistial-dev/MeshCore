#pragma once

#include <stdint.h>
#include <MeshCore.h>

inline uint32_t pagerSimpleHash32(const uint8_t* data, size_t len) {
  uint32_t h = 0xA5A5A5A5;
  for (size_t i = 0; i < len; i++) {
    h ^= (static_cast<uint32_t>(data[i]) << (8 * (i & 3)));
    h = (h << 5) | (h >> 27);
  }
  return h;
}

inline uint16_t pagerSlotDelayMs(const uint8_t* pubkey, uint8_t slot_count, uint16_t window_ms) {
  if (slot_count == 0 || window_ms == 0) return 0;
  uint32_t h = pagerSimpleHash32(pubkey, PUB_KEY_SIZE);
  uint8_t slot = h % slot_count;
  // Deterministic offset inside the reply window to stagger ACK/telemetry and avoid collisions.
  return static_cast<uint16_t>((static_cast<uint32_t>(slot) * window_ms) / slot_count);
}
