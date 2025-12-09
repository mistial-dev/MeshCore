#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include "../../examples/companion_radio/NodePrefs.h"

struct CorePrefsImage {
  NodePrefs prefs{};
  double node_lat = 0.0;
  double node_lon = 0.0;
};

// Populate with typical defaults from MyMesh constructor.
inline CorePrefsImage makeCorePrefsDefaults() {
  CorePrefsImage cp;
  std::memset(&cp.prefs, 0, sizeof(cp.prefs));
  cp.prefs.airtime_factor = 1.0f;
  std::strcpy(cp.prefs.node_name, "NONAME");
  cp.prefs.freq = 869.525f;
  cp.prefs.sf = 11;
  cp.prefs.bw = 250.0f;
  cp.prefs.cr = 8;
  cp.prefs.tx_power_dbm = 16;
  // remaining fields stay zero
  return cp;
}

// Serialize to the exact layout used by DataStore (companion prefs blob).
inline std::vector<uint8_t> serializeCorePrefs(const CorePrefsImage& cp) {
  std::vector<uint8_t> out;
  out.reserve(85);
  auto append = [&](const void* src, size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(src);
    out.insert(out.end(), p, p + len);
  };
  uint8_t pad8[8] = {0};
  append(&cp.prefs.airtime_factor, sizeof(float));            // 0
  append(&cp.prefs.node_name[0], sizeof(cp.prefs.node_name)); // 4
  append(pad8, 4);                                            // 36
  append(&cp.node_lat, sizeof(double));                       // 40
  append(&cp.node_lon, sizeof(double));                       // 48
  append(&cp.prefs.freq, sizeof(float));                      // 56
  append(&cp.prefs.sf, sizeof(uint8_t));                      // 60
  append(&cp.prefs.cr, sizeof(uint8_t));                      // 61
  append(pad8, 1);                                            // 62
  append(&cp.prefs.manual_add_contacts, sizeof(uint8_t));     // 63
  append(&cp.prefs.bw, sizeof(float));                        // 64
  append(&cp.prefs.tx_power_dbm, sizeof(uint8_t));            // 68
  append(&cp.prefs.telemetry_mode_base, sizeof(uint8_t));     // 69
  append(&cp.prefs.telemetry_mode_loc, sizeof(uint8_t));      // 70
  append(&cp.prefs.telemetry_mode_env, sizeof(uint8_t));      // 71
  append(&cp.prefs.rx_delay_base, sizeof(float));             // 72
  append(&cp.prefs.advert_loc_policy, sizeof(uint8_t));       // 76
  append(&cp.prefs.multi_acks, sizeof(uint8_t));              // 77
  append(pad8, 2);                                            // 78
  append(&cp.prefs.ble_pin, sizeof(uint32_t));                // 80
  append(&cp.prefs.buzzer_quiet, sizeof(uint8_t));            // 84
  return out;
}

inline bool deserializeCorePrefs(const std::vector<uint8_t>& data, CorePrefsImage& out) {
  if (data.size() < 85) return false;
  CorePrefsImage cp{};
  const uint8_t* p = data.data();
  size_t off = 0;
  auto read = [&](void* dst, size_t len) {
    std::memcpy(dst, p + off, len);
    off += len;
  };
  uint8_t pad[8];
  read(&cp.prefs.airtime_factor, sizeof(float));            // 0
  read(&cp.prefs.node_name[0], sizeof(cp.prefs.node_name)); // 4
  read(pad, 4);                                             // 36
  read(&cp.node_lat, sizeof(double));                       // 40
  read(&cp.node_lon, sizeof(double));                       // 48
  read(&cp.prefs.freq, sizeof(float));                      // 56
  read(&cp.prefs.sf, sizeof(uint8_t));                      // 60
  read(&cp.prefs.cr, sizeof(uint8_t));                      // 61
  read(pad, 1);                                             // 62
  read(&cp.prefs.manual_add_contacts, sizeof(uint8_t));     // 63
  read(&cp.prefs.bw, sizeof(float));                        // 64
  read(&cp.prefs.tx_power_dbm, sizeof(uint8_t));            // 68
  read(&cp.prefs.telemetry_mode_base, sizeof(uint8_t));     // 69
  read(&cp.prefs.telemetry_mode_loc, sizeof(uint8_t));      // 70
  read(&cp.prefs.telemetry_mode_env, sizeof(uint8_t));      // 71
  read(&cp.prefs.rx_delay_base, sizeof(float));             // 72
  read(&cp.prefs.advert_loc_policy, sizeof(uint8_t));       // 76
  read(&cp.prefs.multi_acks, sizeof(uint8_t));              // 77
  read(pad, 2);                                             // 78
  read(&cp.prefs.ble_pin, sizeof(uint32_t));                // 80
  read(&cp.prefs.buzzer_quiet, sizeof(uint8_t));            // 84
  out = cp;
  return true;
}
