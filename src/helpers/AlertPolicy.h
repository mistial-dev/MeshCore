#pragma once

#include <stdint.h>

#ifndef ALERT_POLICY_OFFLINE_ONLY
#define ALERT_POLICY_OFFLINE_ONLY 0
#endif
#ifndef ALERT_POLICY_BELL_ONLY
#define ALERT_POLICY_BELL_ONLY 1
#endif

// Resolve effective alert policy given a global policy and a per-channel options byte.
// channelOptions: low 2 bits = 0 inherit, 1 offline-only, 2 bell-only
uint8_t resolveAlertPolicy(uint8_t globalPolicy, uint8_t channelOptions);

// Decide if an alert should fire given policy, app connection, and message text.
bool shouldAlert(uint8_t policy, bool appConnected, const char* text);

#ifdef MESH_PAGER_MODE
// Returns true if channel secret matches the default Public room PSK (first 16 bytes).
bool isDefaultPublicChannelSecret(const uint8_t* secret, int len);
#endif

