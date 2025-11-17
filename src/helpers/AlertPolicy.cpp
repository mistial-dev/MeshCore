#include "AlertPolicy.h"
#include "TxtDataHelpers.h"
#include <string.h>

uint8_t resolveAlertPolicy(uint8_t globalPolicy, uint8_t channelOptions) {
  uint8_t ov = (uint8_t)(channelOptions & 0x3);
  if (ov == 1) return ALERT_POLICY_OFFLINE_ONLY;
  if (ov == 2) return ALERT_POLICY_BELL_ONLY;
  return globalPolicy;
}

bool shouldAlert(uint8_t policy, bool appConnected, const char* text) {
  if (policy == ALERT_POLICY_BELL_ONLY) {
    return StrHelper::hasBellMarker(text);
  }
  // offline-only
  return !appConnected;
}

#ifdef MESH_PAGER_MODE
bool isDefaultPublicChannelSecret(const uint8_t* secret, int len) {
  static const uint8_t PUBLIC_PSK_SECRET[16] = {
    0x8b,0x33,0x87,0xe9,0xc5,0xcd,0xea,0x6a,0xc9,0xe5,0xed,0xba,0xa1,0x15,0xcd,0x72
  };
  if (secret == nullptr || len < 16) return false;
  return memcmp(secret, PUBLIC_PSK_SECRET, 16) == 0;
}
#endif

