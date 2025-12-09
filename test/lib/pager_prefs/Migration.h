#pragma once

#include <utility>
#include "../core_prefs/CorePrefs.h"
#include "PagerPrefs.h"

// Bundle of upgraded prefs: original core prefs plus pager-specific defaults.
struct UpgradedPrefs {
  CorePrefsImage core;
  PagerPrefs pager;
};

// Upgrade function: copy core prefs and apply pager defaults. Additional migration
// rules can be added later (e.g., deriving pager-enabled from a stored flag).
inline UpgradedPrefs upgradeToPager(const CorePrefsImage& legacy) {
  UpgradedPrefs u{};
  u.core = legacy;
  u.pager = PagerPrefs::defaults();
  return u;
}
