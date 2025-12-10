#pragma once

#include <stdint.h>
#include <string.h>
#include "PagerRoster.h"

struct NDIDMessage {
  bool valid = false;
  bool evict = false;
  uint16_t pager_id = 0;
  uint8_t groups = 0;
};

struct OTARMessage {
  bool valid = false;
  char password[32] = {0};
};

// Returns true if text starts with an admin-only pager tag like "[PAGE]" or "[OTAR]".
// Pattern: '[' + 4 uppercase letters + ']'.
inline bool isPagerAdminTag(const char* text) {
  if (!text || text[0] != '[') return false;
  // Require at least "[XXXX]" plus either space or null afterwards.
  for (int i = 1; i <= 4; ++i) {
    char c = text[i];
    if (c < 'A' || c > 'Z') return false;
  }
  if (text[5] != ']') return false;
  char next = text[6];
  return next == '\0' || next == ' ';
}

// Parse "[NDID] ID:<num> GROUP:<list>". GROUP:X signals eviction.
inline NDIDMessage parseNDID(const char* text) {
  NDIDMessage m;
  if (!text || strncmp(text, "[NDID]", 6) != 0) return m;
  const char* idp = strstr(text, "ID:");
  const char* grp = strstr(text, "GROUP:");
  if (!idp || !grp) return m;

  m.pager_id = static_cast<uint16_t>(atoi(idp + 3));
  const char* gstart = grp + 6;
  if (*gstart == 'X') {
    m.evict = true;
  } else {
    m.groups = PagerRosterEntry::groupsFromList(gstart);
  }
  m.valid = true;
  return m;
}

// Parse "[OTAR] PASS:<pw>".
inline OTARMessage parseOTAR(const char* text) {
  OTARMessage m;
  if (!text || strncmp(text, "[OTAR]", 6) != 0) return m;
  const char* pass = strstr(text, "PASS:");
  if (!pass) return m;
  strncpy(m.password, pass + 5, sizeof(m.password) - 1);
  m.password[sizeof(m.password) - 1] = 0;
  m.valid = m.password[0] != 0;
  return m;
}

// Return true if a non-admin message should be rejected for using an admin-only tag.
// Optionally writes a short error string to out_err (if provided).
inline bool pagerAdminRejectMessage(bool is_admin, const char* text, char* out_err, size_t err_len) {
  if (is_admin) return false;
  if (!isPagerAdminTag(text)) return false;
  if (out_err && err_len > 0) {
    strncpy(out_err, "ERR admin-only tag", err_len - 1);
    out_err[err_len - 1] = 0;
  }
  return true;
}
