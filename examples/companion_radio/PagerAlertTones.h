#pragma once

#include <stddef.h>
#include <stdint.h>

enum class PagerAlertLevel : uint8_t { A = 0, B, C, D, E, COUNT };

struct PagerAlertTone {
  const char* name;
  const char* melody;    // RTTTL string
  bool latched;          // if true, repeat until explicitly stopped
};

static constexpr PagerAlertTone kPagerAlertTones[] = {
    {"A", "A_lvl:d=8,o=6,b=90:c6", false},                           // soft chirp
    {"B", "B_lvl:d=16,o=6,b=110:c6,p,c6", false},                    // double chirp
    {"C", "C_lvl:d=16,o=6,b=120:c6,e6,g6", false},                   // short rising triple
    {"D", "D_lvl:d=16,o=6,b=140:g6,p,g6,p,g6", false},               // urgent triple
    {"E", "E_lvl:d=16,o=6,b=150:c7,p,c7,p,c7,p,c7,p,c7", true},      // latched, keeps repeating
};

static_assert(static_cast<size_t>(PagerAlertLevel::COUNT) == sizeof(kPagerAlertTones)/sizeof(kPagerAlertTones[0]),
              "Tone table must match PagerAlertLevel count");

inline const PagerAlertTone& getPagerAlertTone(PagerAlertLevel lvl) {
  return kPagerAlertTones[static_cast<size_t>(lvl)];
}
