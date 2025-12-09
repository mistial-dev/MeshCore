#pragma once

#include <cstdint>
#include <vector>

// Simple pager prefs container for host-side tests. Keeps serialization stable and
// versioned so migrations can be tested without touching firmware code yet.
struct PagerPrefs {
  static constexpr uint16_t kVersion = 1;

  uint16_t version = kVersion;
  bool pager_enabled = false;
  bool two_shot_ack = true;
  uint16_t heartbeat_secs = 120;   // periodic heartbeat to dispatcher
  uint8_t ack_slot_count = 8;      // number of slots to spread ACKs
  uint16_t ack_window_ms = 2400;   // total window over which slots are spread

  static PagerPrefs defaults() { return PagerPrefs{}; }

  std::vector<uint8_t> serialize() const {
    std::vector<uint8_t> out;
    out.reserve(2 + 1 + 1 + 2 + 1 + 2);
    auto appendLE = [&](auto value) {
      using T = decltype(value);
      for (size_t i = 0; i < sizeof(T); i++) {
        out.push_back(static_cast<uint8_t>((value >> (8 * i)) & 0xFF));
      }
    };
    appendLE(version);
    out.push_back(pager_enabled ? 1 : 0);
    out.push_back(two_shot_ack ? 1 : 0);
    appendLE(heartbeat_secs);
    out.push_back(ack_slot_count);
    appendLE(ack_window_ms);
    return out;
  }

  static bool deserialize(const std::vector<uint8_t>& data, PagerPrefs& out) {
    if (data.size() < 9) return false;
    auto readLE = [&](size_t offset, auto* dst) {
      using T = std::remove_reference_t<decltype(*dst)>;
      T v = 0;
      for (size_t i = 0; i < sizeof(T); i++) {
        v |= static_cast<T>(data[offset + i]) << (8 * i);
      }
      *dst = v;
    };
    PagerPrefs p;
    readLE(0, &p.version);
    if (p.version != kVersion) return false;
    p.pager_enabled = data[2] != 0;
    p.two_shot_ack = data[3] != 0;
    readLE(4, &p.heartbeat_secs);
    p.ack_slot_count = data[6];
    readLE(7, &p.ack_window_ms);
    out = p;
    return true;
  }
};
