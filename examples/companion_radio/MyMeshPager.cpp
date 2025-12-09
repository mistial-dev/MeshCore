#include "MyMesh.h"
#include "PagerMultipart.h"
#include "PagerSlotHelper.h"

#ifdef PAGER_MODE

bool MyMesh::isPagerClient() const {
#ifndef DISPATCH_NODE
  return true;
#else
  return false;
#endif
}

bool MyMesh::isDispatchMatch(const ContactInfo& contact) const {
  if (!isDispatchContact(contact)) return false;
  if (!pager_dispatch_cache.isSet()) return true;
  return memcmp(contact.id.pub_key, pager_dispatch_cache.dispatch_pub_key, PUB_KEY_SIZE) == 0;
}

bool MyMesh::requireDispatchTarget(const ContactInfo* contact) {
  if (!isPagerClient()) return true;
  if (contact && isDispatchMatch(*contact)) return true;
  writeDisabledFrame();
#ifdef DISPLAY_CLASS
  if (_ui) _ui->notify(UIEventType::contactMessage);
#endif
  return false;
}

bool MyMesh::requireDispatchConnection(uint8_t cmd) {
  if (!isPagerClient()) return true;
  if (pager_connected && pager_dispatch_cache.isSet() && hasConnectionTo(pager_dispatch_cache.dispatch_pub_key)) return true;
  writeDisabledFrame();
#ifdef DISPLAY_CLASS
  if (_ui) _ui->notify(UIEventType::contactMessage);
#endif
  return false;
}

#if defined(PIN_BUZZER)
uint32_t MyMesh::getAckDelayMillis(const ContactInfo& dest, bool is_multi) const {
  (void)is_multi;
  if (isPagerClient() && pager_dispatch_cache.isSet() && isDispatchMatch(dest)) {
    return calcPagerSlotDelay();
  }
  return 0;
}

uint32_t MyMesh::getRequestResponseDelayMillis(const ContactInfo& contact, uint8_t req_type) const {
  if (isPagerClient() && pager_dispatch_cache.isSet() && isDispatchMatch(contact) && req_type == REQ_TYPE_GET_TELEMETRY_DATA) {
    return calcPagerSlotDelay();
  }
  return 0;
}

void MyMesh::pagerHandleLoginSuccess(const ContactInfo& contact) {
  if (!isDispatchMatch(contact)) return;
  pager_dispatch_cache.version = PagerDispatchRecord::kVersion;
  memcpy(pager_dispatch_cache.dispatch_pub_key, contact.id.pub_key, sizeof(pager_dispatch_cache.dispatch_pub_key));
  StrHelper::strzcpy(pager_dispatch_cache.dispatch_name, contact.name, sizeof(pager_dispatch_cache.dispatch_name));
  pager_dispatch_cache.last_join_time = getRTCClock()->getCurrentTime();
  _store->savePagerDispatchRecord(pager_dispatch_cache);
  pager_connected = true;
  pager_last_dispatch_rx = _ms->getMillis();
  pager_disconnect_alerting = false;
  pager_auto_login_pending = false;
}

bool MyMesh::pagerShouldAcceptCLI(const ContactInfo& from) {
  if (!isPagerClient()) return true;
  return isDispatchMatch(from) && hasConnectionTo(from.id.pub_key);
}

bool MyMesh::pagerAllowTelemetry(const ContactInfo& from) {
  if (!isPagerClient()) return true;
  return isDispatchMatch(from) && hasConnectionTo(from.id.pub_key);
}

bool MyMesh::pagerPreprocessIncoming(const ContactInfo& from, const char*& text, bool is_channel) {
  (void)is_channel;
  if (!isPagerClient()) return true;
  if (!isDispatchMatch(from)) return true;
  pager_last_dispatch_rx = _ms->getMillis();
  auto mp = handlePagerMultipart(from, text, &text);
  if (mp == PagerMultipartAssembler::Result::Waiting || mp == PagerMultipartAssembler::Result::Rejected) return false;
  PagerAlertLevel lvl = parsePagerPriority(text, PagerAlertLevel::D);
  startPagerAlert(lvl);
  return true;
}

bool MyMesh::pagerPreprocessChannel(const char*& text) {
  if (!isPagerClient() || !pager_dispatch_cache.isSet()) return true;
  if (!hasConnectionTo(pager_dispatch_cache.dispatch_pub_key)) return true;
  pager_last_dispatch_rx = _ms->getMillis();
  ContactInfo* dispatch_contact = lookupContactByPubKey(pager_dispatch_cache.dispatch_pub_key, 6);
  if (dispatch_contact) {
    auto mp = handlePagerMultipart(*dispatch_contact, text, &text);
    if (mp == PagerMultipartAssembler::Result::Waiting || mp == PagerMultipartAssembler::Result::Rejected) return false;
  }
  PagerAlertLevel lvl = parsePagerPriority(text, PagerAlertLevel::D);
  startPagerAlert(lvl);
  return true;
}

uint32_t MyMesh::calcPagerSlotDelay() const {
  if (!isPagerClient()) return 0;
  return pagerSlotDelayMs(self_id.pub_key, pager_ack_slots, pager_ack_window_ms);
}

void MyMesh::pagerLoop() {
#if !defined(DISPATCH_NODE) || DISPATCH_NODE==0
  // Drop connection flag if the dispatch disappeared.
  if (pager_connected && pager_dispatch_cache.isSet() && !hasConnectionTo(pager_dispatch_cache.dispatch_pub_key)) {
    pager_connected = false;
  }
#endif

  pager_alert.loop();

#if defined(LED_PIN)
  if (isPagerClient()) {
    bool connected = pager_connected && pager_dispatch_cache.isSet() && hasConnectionTo(pager_dispatch_cache.dispatch_pub_key);
    handlePagerLED(connected);
  }
#endif

#if !defined(DISPATCH_NODE) || DISPATCH_NODE==0
  if (isPagerClient() && pager_dispatch_cache.isSet()) {
    if (pager_next_disconnect_check == 0) {
      pager_next_disconnect_check = futureMillis(60000);
      pager_last_dispatch_rx = _ms->getMillis();
      pager_auto_login_pending = false;
    }
    if (millisHasNowPassed(pager_next_disconnect_check)) {
      pager_next_disconnect_check = futureMillis(60000);
      bool connected_now = pager_connected && hasConnectionTo(pager_dispatch_cache.dispatch_pub_key);
      bool timed_out = false;
      if (pager_last_dispatch_rx != 0) {
        unsigned long elapsed = _ms->getMillis() - pager_last_dispatch_rx;
        timed_out = elapsed > 60000;
      }
      if ((!connected_now || timed_out) && !pager_disconnect_alerting) {
        startPagerAlert(PagerAlertLevel::C);
        pager_disconnect_alerting = true;
        pager_auto_login_pending = true;
      } else if (connected_now) {
        pager_disconnect_alerting = false;
        pager_auto_login_pending = false;
      } else {
        // if we stay disconnected, allow another attempt after backoff
        pager_auto_login_pending = true;
      }
    }
    checkPagerMultipartTimeout();
    if (pager_auto_login_pending && millisHasNowPassed(pager_next_login_attempt)) {
      // backoff cap: every 5 minutes after two failed attempts
      if (pager_next_login_attempt == 0) {
        pager_next_login_attempt = futureMillis(60000);
      } else {
        pager_next_login_attempt = futureMillis(300000);
      }
      ContactInfo* dispatch_contact = lookupContactByPubKey(pager_dispatch_cache.dispatch_pub_key, 6);
      if (dispatch_contact) {
        uint32_t est_timeout;
        // Attempt passwordless login; dispatcher may remember the node and allow it
        int result = sendLogin(*dispatch_contact, "", est_timeout);
        if (result != MSG_SEND_FAILED) {
          memcpy(&pending_login, dispatch_contact->id.pub_key, 4);
        } else {
          pager_auto_login_pending = false; // give up until next disconnect check
        }
      }
    }
  }
#endif
}

#if defined(LED_PIN)
void MyMesh::handlePagerLED(bool connected) {
  // Keep LED quiet when connected and no alert; otherwise blink at severity-dependent rate.
  if (!pager_alert.isActive() && connected) {
    digitalWrite(LED_PIN, LOW);
    pager_led_state = false;
    pager_led_next = futureMillis(1500);
    return;
  }

  unsigned long interval = pager_alert.isActive() ? 250 : 800;
  if (millisHasNowPassed(pager_led_next)) {
    pager_led_next = futureMillis(interval);
    pager_led_state = !pager_led_state;
    digitalWrite(LED_PIN, pager_led_state ? HIGH : LOW);
  }
}
#endif // LED_PIN

PagerMultipartAssembler::Result MyMesh::handlePagerMultipart(const ContactInfo& from, const char* text, const char** out_text) {
  if (out_text) *out_text = text;
  if (!isPagerClient() || !pager_dispatch_cache.isSet() || !isDispatchMatch(from) || text == nullptr) {
    return PagerMultipartAssembler::Result::Single;
  }
  auto now_ms = _ms->getMillis();
  auto res = pager_multipart.ingest(from.id.pub_key, text, now_ms, pager_combined_msg, sizeof(pager_combined_msg));
  if (res == PagerMultipartAssembler::Result::Complete) {
    if (out_text) *out_text = pager_combined_msg;
  } else if (res == PagerMultipartAssembler::Result::Rejected) {
    // Rejects (bad counter/too many parts) are surfaced to dispatch so the sender can retry.
    uint8_t missing = pager_multipart.missingMask();
    ContactInfo* dispatch_contact = lookupContactByPubKey(pager_dispatch_cache.dispatch_pub_key, 6);
    if (dispatch_contact && hasConnectionTo(dispatch_contact->id.pub_key)) {
      snprintf(pager_combined_msg, sizeof(pager_combined_msg), "NACK page rejected missing_mask=%02X", missing);
      uint32_t est;
      sendCommandData(*dispatch_contact, now_ms / 1000, 0, pager_combined_msg, est);
    }
    pager_multipart.reset();
  }
  return res;
}

void MyMesh::checkPagerMultipartTimeout() {
  if (!isPagerClient() || !pager_multipart.hasTimedOut(_ms->getMillis())) return;
  uint8_t missing = pager_multipart.missingMask();
  ContactInfo* dispatch_contact = lookupContactByPubKey(pager_dispatch_cache.dispatch_pub_key, 6);
  if (dispatch_contact && hasConnectionTo(dispatch_contact->id.pub_key)) {
    // Timeouts are treated as NACKs so dispatch can resend the whole page.
    snprintf(pager_combined_msg, sizeof(pager_combined_msg), "NACK page timeout missing_mask=%02X", missing);
    uint32_t est;
    sendCommandData(*dispatch_contact, _ms->getMillis() / 1000, 0, pager_combined_msg, est);
  }
  pager_multipart.reset();
}
#else
void MyMesh::pagerLoop() {}
#endif // PIN_BUZZER

#endif // PAGER_MODE
