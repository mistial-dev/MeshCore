#pragma once

#include <cstdint>
#include <cstring>

// Host-only helper to plan slot offsets deterministically for pager ACK/telemetry windows.
// This is NOT firmware code; it is used in tests to reason about slotting.
struct SlotPlan {
  uint8_t slot;
  uint16_t offset_ms;
};

inline uint32_t simpleHash32(const uint8_t* data, size_t len) {
  uint32_t h = 0xA5A5A5A5;
  for (size_t i = 0; i < len; i++) {
    h ^= (static_cast<uint32_t>(data[i]) << (8 * (i & 3)));
    h = (h << 5) | (h >> 27);
  }
  return h;
}

inline SlotPlan planSlot(const uint8_t pubkey[32], uint8_t slot_count, uint16_t window_ms) {
  uint32_t h = simpleHash32(pubkey, 32);
  uint8_t slot = slot_count == 0 ? 0 : (h % slot_count);
  uint16_t offset = static_cast<uint16_t>((static_cast<uint32_t>(slot) * window_ms) / (slot_count ? slot_count : 1));
  return {slot, offset};
}
