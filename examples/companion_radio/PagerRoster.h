#pragma once

#include <stdint.h>
#include <string.h>

// Persistent roster entry for pagers on the dispatcher.
// Maps a pubkey prefix to a short pager ID and group bitmask (A-J -> bits 0-9).
struct PagerRosterEntry {
  static constexpr uint16_t kVersion = 1;
  uint16_t version = kVersion;
  uint8_t pubkey[32] = {0};
  uint16_t pager_id = 0;       // 0 means unassigned
  uint8_t groups = 0;          // bitmask for groups A-H (0-7)
  uint8_t is_pager = 0;        // whether this node advertises dispatch feat
  uint32_t last_seen = 0;      // RTC seconds
  uint8_t reserved[12] = {0};

  bool isSet() const {
    for (size_t i = 0; i < sizeof(pubkey); i++) {
      if (pubkey[i] != 0) return true;
    }
    return false;
  }

  static uint8_t groupsFromList(const char* s) {
    if (!s) return 0;
    uint8_t mask = 0;
    for (size_t i = 0; s[i]; i++) {
      char c = s[i];
      if (c >= 'a' && c <= 'z') c -= 32; // upper
      if (c >= 'A' && c <= 'H') {
        mask |= (1u << (c - 'A'));
      }
    }
    return mask;
  }
};
