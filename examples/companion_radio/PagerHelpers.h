#pragma once

#include <stdint.h>
#include "PagerAlertTones.h"

inline PagerAlertLevel parsePagerPriority(const char* text, PagerAlertLevel fallback) {
  if (text == nullptr) return fallback;
  const char* p = text;
  while (*p) {
    if (*p == '[' && (p[1] == 'P' || p[1] == 'p') &&
        (p[2] == 'R' || p[2] == 'r') &&
        (p[3] == 'I' || p[3] == 'i') &&
        (p[4] == 'O' || p[4] == 'o') &&
        p[5] == ':' && p[6] != 0) {
      char code = p[6];
      if (code >= 'a' && code <= 'z') code = code - 'a' + 'A';
      switch (code) {
        case 'A': return PagerAlertLevel::A;
        case 'B': return PagerAlertLevel::B;
        case 'C': return PagerAlertLevel::C;
        case 'D': return PagerAlertLevel::D;
        case 'E': return PagerAlertLevel::E;
        default: break;
      }
    }
    ++p;
  }
  return fallback;
}
