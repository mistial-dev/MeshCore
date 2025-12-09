#include "MyMesh.h"

#ifdef PAGER_MODE

#ifndef DISPATCH_TELEM_POLL_INTERVAL_MS
#define DISPATCH_TELEM_POLL_INTERVAL_MS 120000UL
#endif

bool MyMesh::sendTelemetryRequest(ClientInfo* client) {
  if (client == nullptr) return false;
  uint8_t temp[13];
  uint32_t tag = getRTCClock()->getCurrentTimeUnique();
  memcpy(temp, &tag, 4);   // uniqueness for hash/ACK
  temp[4] = REQ_TYPE_GET_TELEMETRY_DATA;
  memset(&temp[5], 0, 4);  // reserved mask/params
  getRNG()->random(&temp[9], 4);

  auto pkt = createDatagram(PAYLOAD_TYPE_REQ, client->id, client->shared_secret, temp, sizeof(temp));
  if (!pkt) return false;

  if (client->out_path_len < 0) {
    sendFlood(pkt);
  } else {
    sendDirect(pkt, client->out_path, client->out_path_len);
  }
  return true;
}

void MyMesh::pollTelemetryRoundRobin() {
  unsigned long now = millis();
  if (next_telem_poll != 0 && !millisHasNowPassed(next_telem_poll)) return;
  if (acl.getNumClients() == 0) {
    next_telem_poll = futureMillis(DISPATCH_TELEM_POLL_INTERVAL_MS);
    return;
  }

  if (next_telem_idx >= acl.getNumClients()) next_telem_idx = 0;
  bool sent = false;

  for (int checked = 0; checked < acl.getNumClients(); ++checked) {
    auto client = acl.getClientByIdx(next_telem_idx);
    next_telem_idx = (next_telem_idx + 1) % acl.getNumClients();

    if (client->permissions == 0) continue; // deleted
    if (client->out_path_len < 0) continue; // no path yet
    // throttle polls to recently active nodes; skip idle nodes with no activity ever
    if (client->last_activity == 0) continue;

    if (sendTelemetryRequest(client)) {
      sent = true;
      break;
    }
  }

  next_telem_poll = futureMillis(sent ? DISPATCH_TELEM_POLL_INTERVAL_MS : DISPATCH_TELEM_POLL_INTERVAL_MS / 4);
}

#endif // PAGER_MODE
