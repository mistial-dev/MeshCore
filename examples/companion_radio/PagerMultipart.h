#pragma once

#if defined(PAGER_MODE) && (!defined(DISPATCH_NODE) || DISPATCH_NODE==0)

#include <stdint.h>
#include <string.h>
#include <MeshCore.h>

// Lightweight assembler for multi-part pager pages. Assumes small part counts and
// one in-flight assembly at a time. Keeps parsing simple to avoid impacting firmware size.
class PagerMultipartAssembler {
public:
  static constexpr uint8_t kMaxParts = 4;
  static constexpr unsigned long kTimeoutMs = 6000; // allow parts to arrive, then NACK
  static constexpr size_t kMaxBodyLen = 160;        // per-part payload slice

  PagerMultipartAssembler() { reset(); }

  // Result of ingesting a part.
  enum class Result { Single, Waiting, Complete, Rejected };

  struct Parsed {
    uint8_t part = 0;
    uint8_t total = 0;
    const char* body = nullptr;
  };

  void reset() {
    active = false;
    memset(sender_pubkey, 0, sizeof(sender_pubkey));
    memset(part_mask, 0, sizeof(part_mask));
    memset(part_bodies, 0, sizeof(part_bodies));
    memset(part_lens, 0, sizeof(part_lens));
    total_parts = 0;
    started_ms = 0;
  }

  // Parse "n/m" counter from the start of text. Returns false if no counter.
  static bool parseCounter(const char* text, Parsed& out) {
    if (text == nullptr || text[0] == 0) return false;
    uint8_t num = 0, total = 0;
    int i = 0;
    while (text[i] >= '0' && text[i] <= '9') { num = num * 10 + (text[i++] - '0'); }
    if (text[i] != '/') return false;
    i++;
    while (text[i] >= '0' && text[i] <= '9') { total = total * 10 + (text[i++] - '0'); }
    if (num == 0 || total == 0 || num > total) return false;
    out.part = num;
    out.total = total;
    while (text[i] == ' ') i++; // skip space after counter
    out.body = &text[i];
    return true;
  }

  Result ingest(const uint8_t* sender, const char* text, unsigned long now_ms, char* out_combined, size_t out_len) {
    Parsed p{};
    if (!parseCounter(text, p) || p.total <= 1) {
      // Single-part page or non-pager text.
      if (out_combined && out_len) {
        strncpy(out_combined, text, out_len - 1);
        out_combined[out_len - 1] = 0;
      }
      return Result::Single;
    }
    if (p.total > kMaxParts) return Result::Rejected;

    if (!active || memcmp(sender_pubkey, sender, PUB_KEY_SIZE) != 0 || total_parts != p.total || p.part == 1) {
      // Start a new assembly if different sender/size, or part 1 appears.
      reset();
      memcpy(sender_pubkey, sender, PUB_KEY_SIZE);
      total_parts = p.total;
      active = true;
      started_ms = now_ms;
    }

    if (!active || p.part == 0 || p.part > total_parts) return Result::Rejected;
    storePart(p.part, p.body);

    if (isComplete()) {
      combine(out_combined, out_len);
      reset();
      return Result::Complete;
    }
    return Result::Waiting;
  }

  bool hasTimedOut(unsigned long now_ms) const {
    return active && now_ms > started_ms && (now_ms - started_ms) > kTimeoutMs;
  }

  // Returns a bitmask of missing parts (LSB = part 1) if active.
  uint8_t missingMask() const {
    if (!active) return 0;
    uint8_t mask = 0;
    for (uint8_t i = 1; i <= total_parts; ++i) {
      if (!hasPart(i)) mask |= (1 << (i - 1));
    }
    return mask;
  }

private:
  bool active;
  uint8_t sender_pubkey[PUB_KEY_SIZE];
  uint8_t total_parts;
  unsigned long started_ms;
  char part_bodies[kMaxParts][kMaxBodyLen];
  uint8_t part_lens[kMaxParts];
  bool part_mask[kMaxParts];

  bool hasPart(uint8_t idx) const {
    if (idx == 0 || idx > kMaxParts) return false;
    return part_mask[idx - 1];
  }

  void storePart(uint8_t idx, const char* body) {
    if (idx == 0 || idx > kMaxParts) return;
    size_t len = strnlen(body, kMaxBodyLen - 1);
    strncpy(part_bodies[idx - 1], body, kMaxBodyLen - 1);
    part_bodies[idx - 1][kMaxBodyLen - 1] = 0;
    part_lens[idx - 1] = static_cast<uint8_t>(len);
    part_mask[idx - 1] = true;
  }

  bool isComplete() const {
    if (!active) return false;
    for (uint8_t i = 1; i <= total_parts; ++i) {
      if (!hasPart(i)) return false;
    }
    return true;
  }

  void combine(char* out, size_t out_len) const {
    if (out == nullptr || out_len == 0) return;
    out[0] = 0;
    size_t pos = 0;
    for (uint8_t i = 1; i <= total_parts && pos < out_len - 1; ++i) {
      const char* part = part_bodies[i - 1];
      size_t l = part_lens[i - 1];
      if (pos + l + 1 >= out_len) l = out_len - pos - 1;
      memcpy(out + pos, part, l);
      pos += l;
      if (pos < out_len - 1) {
        out[pos++] = ' '; // separate parts
      }
    }
    if (pos > 0 && out[pos - 1] == ' ') pos--; // trim trailing space
    out[pos] = 0;
  }
};

#endif // PAGER_MODE && !DISPATCH_NODE
