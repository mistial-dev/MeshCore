#pragma once

#include "../../../examples/companion_radio/PagerRoster.h"

inline uint8_t makeGroups(const char* s) {
  return PagerRosterEntry::groupsFromList(s);
}
