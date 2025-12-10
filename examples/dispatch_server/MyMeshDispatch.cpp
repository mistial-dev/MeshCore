#include "MyMesh.h"
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstring>

#if defined(NRF52_PLATFORM)
extern void kickWatchdog();
#endif

#ifndef FILE_READ
#define FILE_READ "r"
#endif
#ifndef FILE_WRITE
#define FILE_WRITE "w"
#endif

#ifdef PAGER_MODE

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
  client->extra.room.last_outbound_ms = millis();
#ifdef PAGER_MODE
  maybeMarkPagerSeen(client);
#endif
  return true;
}

void MyMesh::pollTelemetryRoundRobin() {
  unsigned long now = millis();
  if (next_telem_poll != 0 && !millisHasNowPassed(next_telem_poll)) return;
  if (acl.getNumClients() == 0) {
    next_telem_poll = futureMillis(DISPATCH_HEARTBEAT_INTERVAL_MS);
    return;
  }

  if (next_telem_idx >= acl.getNumClients()) next_telem_idx = 0;
  bool sent = false;

  for (int checked = 0; checked < acl.getNumClients(); ++checked) {
    auto client = acl.getClientByIdx(next_telem_idx);
    next_telem_idx = (next_telem_idx + 1) % acl.getNumClients();

    if (client->permissions == 0) continue; // deleted
    if (client->out_path_len < 0) continue; // no path yet
    if (client->last_activity == 0) continue;

    // Skip if we recently sent something to this client inside the heartbeat window.
    if (client->extra.room.last_outbound_ms != 0 &&
        now - client->extra.room.last_outbound_ms < DISPATCH_HEARTBEAT_INTERVAL_MS) {
      continue;
    }

    if (sendTelemetryRequest(client)) {
      sent = true;
      break;
    }
  }

  next_telem_poll = futureMillis(sent ? DISPATCH_HEARTBEAT_INTERVAL_MS : DISPATCH_HEARTBEAT_INTERVAL_MS / 4);
}

ClientInfo* MyMesh::findClientByPub(const uint8_t* pub) {
  for (int i = 0; i < acl.getNumClients(); ++i) {
    auto c = acl.getClientByIdx(i);
    if (c->permissions == 0) continue;
    if (memcmp(c->id.pub_key, pub, PUB_KEY_SIZE) == 0) return c;
  }
  return nullptr;
}

PagerRosterEntry* MyMesh::findRosterByPub(const uint8_t* pub) {
  for (int i = 0; i < pager_roster_size; ++i) {
    if (pager_roster[i].isSet() && memcmp(pager_roster[i].pubkey, pub, PUB_KEY_SIZE) == 0) return &pager_roster[i];
  }
  return nullptr;
}

PagerRosterEntry* MyMesh::findRosterById(uint16_t id) {
  if (id == 0) return nullptr;
  for (int i = 0; i < pager_roster_size; ++i) {
    if (pager_roster[i].isSet() && pager_roster[i].pager_id == id) return &pager_roster[i];
  }
  return nullptr;
}

uint16_t MyMesh::allocatePagerId() const {
  uint16_t max_id = 0;
  for (int i = 0; i < pager_roster_size; ++i) {
    if (pager_roster[i].isSet() && pager_roster[i].pager_id > max_id) {
      max_id = pager_roster[i].pager_id;
    }
  }
  return max_id + 1;
}

void MyMesh::loadPagerRoster() {
  pager_roster_size = 0;
#if defined(NRF52_PLATFORM)
  File f = _fs->open("/pager_roster.bin", FILE_O_READ);
#elif defined(RP2040_PLATFORM)
  File f = _fs->open("/pager_roster.bin", "r");
#elif defined(ESP32)
  File f = _fs->open("/pager_roster.bin", FILE_READ);
#else
  File f = _fs->open("/pager_roster.bin", "r");
#endif
  if (!f) {
    MESH_DEBUG_PRINTLN("pager_roster: no file");
    return;
  }
  while (f.available() && pager_roster_size < kMaxPagerRoster) {
    PagerRosterEntry e{};
    int r = f.read((uint8_t*)&e, sizeof(e));
    if (r != (int)sizeof(e)) break;
    if (e.version == PagerRosterEntry::kVersion && e.isSet()) {
      pager_roster[pager_roster_size++] = e;
    }
  }
  f.close();
#if defined(NRF52_PLATFORM)
  kickWatchdog();
#endif
}

void MyMesh::savePagerRoster() {
#if defined(NRF52_PLATFORM)
  static uint32_t next_roster_save = 0;
  uint32_t now = millis();
  if (next_roster_save != 0 && !millisHasNowPassed(next_roster_save)) {
    return; // debounce saves to avoid flash churn
  }
#endif
#if defined(NRF52_PLATFORM)
  File f = _fs->open("/pager_roster.bin", FILE_O_WRITE);
#elif defined(RP2040_PLATFORM)
  File f = _fs->open("/pager_roster.bin", "w");
#elif defined(ESP32)
  File f = _fs->open("/pager_roster.bin", FILE_WRITE);
#else
  File f = _fs->open("/pager_roster.bin", "w");
#endif
  if (!f) {
    MESH_DEBUG_PRINTLN("pager_roster: save open failed");
    return;
  }
  for (int i = 0; i < pager_roster_size; ++i) {
    size_t w = f.write((uint8_t*)&pager_roster[i], sizeof(pager_roster[i]));
    if (w != sizeof(pager_roster[i])) {
      MESH_DEBUG_PRINTLN("pager_roster: save short write");
      break;
    }
  }
  f.close();
#if defined(NRF52_PLATFORM)
  next_roster_save = futureMillis(5000);
#endif
  // Kick watchdog after potentially long flash writes.
#if defined(NRF52_PLATFORM)
  kickWatchdog();
#endif
}

bool MyMesh::sendPagerText(ClientInfo* client, const char* text) {
  if (!client || !text) return false;
  uint8_t temp[200];
  uint32_t ts = getRTCClock()->getCurrentTimeUnique();
  memcpy(temp, &ts, 4);
  temp[4] = (TXT_TYPE_PLAIN << 2);
  size_t len = strnlen(text, sizeof(temp) - 5);
  memcpy(&temp[5], text, len);
  auto pkt = createDatagram(PAYLOAD_TYPE_TXT_MSG, client->id, client->shared_secret, temp, 5 + len);
  if (!pkt) return false;
  if (client->out_path_len < 0) {
    sendFlood(pkt);
  } else {
    // Add a small jitter to spread subsequent pager sends and stagger return ACKs.
    uint32_t jitter = 20 + getRNG()->nextInt(0, 30); // 20–49 ms
    sendDirect(pkt, client->out_path, client->out_path_len, jitter);
  }
  client->extra.room.last_outbound_ms = millis();
  return true;
}

bool MyMesh::sendServerNotice(ClientInfo* client, const char* text) {
  if (!client || !text) return false;
  uint8_t temp[200];
  uint32_t ts = getRTCClock()->getCurrentTimeUnique();
  memcpy(temp, &ts, 4);
  temp[4] = (TXT_TYPE_PLAIN << 2);
  size_t len = strnlen(text, sizeof(temp) - 5);
  memcpy(&temp[5], text, len);
  auto pkt = createDatagram(PAYLOAD_TYPE_TXT_MSG, client->id, client->shared_secret, temp, 5 + len);
  if (!pkt) return false;
  if (client->out_path_len < 0) {
    sendFlood(pkt);
  } else {
    sendDirect(pkt, client->out_path, client->out_path_len);
  }
  client->extra.room.last_outbound_ms = millis();
  return true;
}

static std::string groupsToList(uint8_t mask) {
  std::string s;
  for (int i = 0; i < 8; ++i) {
    if (mask & (1u << i)) {
      if (!s.empty()) s.push_back(',');
      s.push_back('A' + i);
    }
  }
  return s;
}

bool MyMesh::sendPagerNDID(ClientInfo* client, uint16_t id, uint8_t groups, bool evict) {
  if (!client) return false;
  char buf[96];
  if (evict) {
    snprintf(buf, sizeof(buf), "[NDID] ID:%u GROUP:X", id);
  } else {
    auto g = groupsToList(groups);
    snprintf(buf, sizeof(buf), "[NDID] ID:%u GROUP:%s", id, g.c_str());
  }
  return sendPagerText(client, buf);
}

bool MyMesh::sendPagerOTAR(ClientInfo* client, const char* password) {
  if (!client || !password) return false;
  char buf[80];
  snprintf(buf, sizeof(buf), "[OTAR] PASS:%s", password);
  return sendPagerText(client, buf);
}

void MyMesh::maybeMarkPagerSeen(const ClientInfo* client) {
  if (!client) return;
  auto r = findRosterByPub(client->id.pub_key);
  if (!r) return;
  r->last_seen = getRTCClock()->getCurrentTime();
  // Defer writes to reduce flash churn. Caller can decide when to persist.
}

bool MyMesh::handlePagerCLI(const char* cmd, char* reply) {
  if (!cmd) return false;
  // pager list
  if (strcmp(cmd, "list") == 0) {
    Serial.println("Pager roster:");
    for (int i = 0; i < pager_roster_size; ++i) {
      auto& e = pager_roster[i];
      if (!e.isSet()) continue;
      Serial.printf("id=%u groups=%02X is_pager=%u last=%u pub[0]=%02X\n",
                    e.pager_id, e.groups, e.is_pager, e.last_seen, e.pubkey[0]);
    }
    strcpy(reply, "OK");
    return true;
  }

  // pager assign <hexpub|id> <groups>
  if (strncmp(cmd, "assign ", 7) == 0) {
    const char* arg = cmd + 7;
    const char* sp = strchr(arg, ' ');
    if (!sp) { strcpy(reply, "Err args"); return true; }
    std::string key(arg, sp - arg);
    const char* groups = sp + 1;
    uint8_t mask = PagerRosterEntry::groupsFromList(groups);
    bool evict = (groups[0] == 'X');

    PagerRosterEntry* entry = nullptr;
    ClientInfo* client = nullptr;
    if (!key.empty() && std::all_of(key.begin(), key.end(), [](char c){ return isdigit((unsigned char)c); })) {
      uint16_t id = atoi(key.c_str());
      entry = findRosterById(id);
      if (!entry) { strcpy(reply, "Err id"); return true; }
      client = findClientByPub(entry->pubkey);
    } else {
      uint8_t pub[PUB_KEY_SIZE] = {0};
      if (!mesh::Utils::fromHex(pub, PUB_KEY_SIZE, key.c_str())) { strcpy(reply, "Err pub"); return true; }
      entry = findRosterByPub(pub);
      if (!entry && pager_roster_size < kMaxPagerRoster) {
        pager_roster[pager_roster_size] = PagerRosterEntry();
        entry = &pager_roster[pager_roster_size++];
        memcpy(entry->pubkey, pub, PUB_KEY_SIZE);
        entry->pager_id = allocatePagerId();
      }
      client = findClientByPub(pub);
    }

    if (!entry) { strcpy(reply, "Err roster"); return true; }
    if (evict) {
      if (client) sendPagerNDID(client, entry->pager_id, 0, true);
      memset(entry, 0, sizeof(*entry));
    } else {
      entry->groups = mask;
      entry->is_pager = 1;
      entry->last_seen = getRTCClock()->getCurrentTime();
      if (client) sendPagerNDID(client, entry->pager_id, mask, false);
    }
    savePagerRoster();
    strcpy(reply, "OK");
    return true;
  }

  // pager evict <id|pub>
  if (strncmp(cmd, "evict ", 6) == 0) {
    const char* arg = cmd + 6;
    PagerRosterEntry* entry = nullptr;
    if (std::all_of(arg, arg + strlen(arg), [](char c){ return isdigit((unsigned char)c); })) {
      entry = findRosterById(atoi(arg));
    } else {
      uint8_t pub[PUB_KEY_SIZE] = {0};
      if (mesh::Utils::fromHex(pub, PUB_KEY_SIZE, arg)) entry = findRosterByPub(pub);
    }
    if (!entry) { strcpy(reply, "Err roster"); return true; }
    ClientInfo* client = findClientByPub(entry->pubkey);
    if (client) sendPagerNDID(client, entry->pager_id, 0, true);
    memset(entry, 0, sizeof(*entry));
    savePagerRoster();
    strcpy(reply, "OK");
    return true;
  }

  // pager otar <id|pub> <password>
  if (strncmp(cmd, "otar ", 5) == 0) {
    const char* arg = cmd + 5;
    const char* sp = strchr(arg, ' ');
    if (!sp) { strcpy(reply, "Err args"); return true; }
    std::string key(arg, sp - arg);
    const char* password = sp + 1;
    PagerRosterEntry* entry = nullptr;
    if (std::all_of(key.begin(), key.end(), [](char c){ return isdigit((unsigned char)c); })) {
      entry = findRosterById(atoi(key.c_str()));
    } else {
      uint8_t pub[PUB_KEY_SIZE] = {0};
      if (mesh::Utils::fromHex(pub, PUB_KEY_SIZE, key.c_str())) entry = findRosterByPub(pub);
    }
    if (!entry) { strcpy(reply, "Err roster"); return true; }
    ClientInfo* client = findClientByPub(entry->pubkey);
    if (!client || !sendPagerOTAR(client, password)) { strcpy(reply, "Err send"); return true; }
    strcpy(reply, "OK");
    return true;
  }

  // pager page id <id> <text> or pager page group <G> <text>
  if (strncmp(cmd, "page ", 5) == 0) {
    const char* arg = cmd + 5;
    const char* sp = strchr(arg, ' ');
    if (!sp) { strcpy(reply, "Err args"); return true; }
    std::string mode(arg, sp - arg);
    const char* rest = sp + 1;
    const char* sp2 = strchr(rest, ' ');
    if (!sp2) { strcpy(reply, "Err args"); return true; }
    std::string target(rest, sp2 - rest);
    const char* body = sp2 + 1;

    std::vector<ClientInfo*> targets;
    if (mode == "id") {
      int id = atoi(target.c_str());
      auto e = findRosterById(id);
      if (!e) { strcpy(reply, "Err id"); return true; }
      auto c = findClientByPub(e->pubkey);
      if (c) targets.push_back(c);
    } else if (mode == "group" && !target.empty()) {
      uint8_t mask = PagerRosterEntry::groupsFromList(target.c_str());
      for (int i = 0; i < pager_roster_size; ++i) {
        auto& e = pager_roster[i];
        if (!e.isSet()) continue;
        if ((e.groups & mask) == 0) continue;
        auto c = findClientByPub(e.pubkey);
        if (c) targets.push_back(c);
      }
    }

    char wrapped[200];
    snprintf(wrapped, sizeof(wrapped), "[PAGE] %s", body);
    int sent = 0;
    for (auto c : targets) {
      if (sendPagerText(c, wrapped)) sent++;
    }
    snprintf(reply, 32, "OK %d", sent);
    return true;
  }

  return false;
}

void MyMesh::ensureDispatchPrefix() {
  const char prefix[] = "[DSPCH] ";
  const size_t prelen = sizeof(prefix) - 1;
  const char* src = _prefs.node_name;
  if (strncmp(src, prefix, prelen) == 0) {
    src += prelen;
  }
  char buf[sizeof(_prefs.node_name)];
  // Avoid duplicating prefix if the configured name already starts with another dispatch hint.
  snprintf(buf, sizeof(buf), "%s%s", prefix, src);
  StrHelper::strncpy(_prefs.node_name, buf, sizeof(_prefs.node_name));
}
#endif // PAGER_MODE
