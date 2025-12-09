#pragma once

#include <stdint.h>
#include <string.h>

// Pager-only persistent cache of the last joined dispatch room. Kept separate
// from existing prefs/contacts to avoid upstream layout changes.
struct PagerDispatchRecord {
  static constexpr uint16_t kVersion = 2;
  uint16_t version = kVersion;
  uint8_t dispatch_pub_key[32] = {0};
  char dispatch_name[32] = {0};
  uint32_t last_join_time = 0;  // RTC seconds when we last joined
  uint8_t reserved[20] = {0};

  bool isSet() const {
    for (size_t i = 0; i < sizeof(dispatch_pub_key); i++) {
      if (dispatch_pub_key[i] != 0) return true;
    }
    return false;
  }

  static bool loadFromBuffer(const uint8_t* buf, size_t len, PagerDispatchRecord& out) {
    if (buf == nullptr || len != sizeof(PagerDispatchRecord)) return false;
    PagerDispatchRecord tmp{};
    memcpy(&tmp, buf, sizeof(PagerDispatchRecord));
    if (tmp.version != kVersion) return false;
    out = tmp;
    return true;
  }

  void clear() {
    memset(dispatch_pub_key, 0, sizeof(dispatch_pub_key));
    memset(dispatch_name, 0, sizeof(dispatch_name));
    last_join_time = 0;
    memset(reserved, 0, sizeof(reserved));
  }
};
