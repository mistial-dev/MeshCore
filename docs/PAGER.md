# Pager Mode Design & Plan

## Overview & Goals
- Keep payload type/version unchanged: pager rides on `PAYLOAD_TYPE_TXT`, so non-pager nodes still render messages.
- Discover dispatchers via advert `feat1` bit (`ADV_FEAT1_DISPATCH`) plus a human hint (`[DISPATCH]` prefix in the room name). Unknown bits are ignored by existing clients.
- All pager logic is gated by `PAGER_MODE`/`DISPATCH_NODE`; core prefs/contacts remain intact.

## Protocol & Message Shape
- Pages stay plain text with inline hints: `[PRIO:X]`, `[GRP:A,C]`, and counters like `1/3`. No new TLVs or payload types.
- Multi-part pages: first part carries the tags, later parts append body. Pager clients assemble up to 4 parts and NACK the dispatch if incomplete after ~6s or if parts exceed limits. Complete text is used for alert parsing and display.
- Admin-only paging: dispatch firmware drops plain text from non-admins so pagers cannot page each other.
- Two-shot ACKs stay on by default; ACK/telemetry replies can be slotted to reduce collisions.
- CLI for pagers is accepted only from the joined dispatch and exposed over USB; BLE CLI is disabled unless `PAGER_ALLOW_BLE` is set.

## Dispatcher Behavior (`env:t1000e_dispatch_usb`)
- Advertises the dispatch bit and should prefix the room name with `[DISPATCH]`.
- Heartbeat expectation: emit at least one packet per minute; pagers watch for >60s of silence.
- Telemetry is pull-only: dispatcher requests, pagers respond in a slotted window (default 8 slots/2400 ms keyed by pager pubkey hash). Two-shot ACKs are expected for pages/telemetry.
- Filters paging traffic to admins only; other room traffic remains unchanged for compatibility.

## Pager Client Behavior (`env:t1000e_pager_client_usb`, optional `_ble`)
- Inert until connected to a dispatch (advert bit present). Sends while disconnected return `RESP_CODE_DISABLED`, buzz, and blink the LED if available.
- Caches the last joined dispatch in `/pager_dispatch.bin` (v2: pubkey/name/timestamp), enforcing a single-dispatch rule without touching existing prefs/contacts. Loader accepts v1 for forward compat.
- Auto reconnect: after 60s of silence/disconnect, attempt passwordless re-login to the cached dispatch, then back off to every 5 minutes. Passwords are not cached; relies on the room remembering the node.
- Heartbeat/watchdog: >60s without dispatch traffic raises alert level C and LED blink until reconnect or user acknowledgement.
- Alerts (A–E) with RTTTL tones live in `examples/companion_radio/PagerAlertTones.h`; level E latches until a button press. Alert defaults: dispatch text/channel messages map to D unless `[PRIO:X]` overrides. LED mirrors alert state when present.
- Telemetry/ACK replies use slotting only when talking to dispatch. Pager CLI stays USB-only; BLE is off unless `PAGER_ALLOW_BLE` is defined (default off with `NO_BLE_FOR_PAGER`).

## Persistence & Migration
- Core prefs are unchanged; pager prefs are additive defaults (pager disabled unless built for it, two-shot ACK on, slot defaults) under `PAGER_MODE`.
- Dispatch cache `/pager_dispatch.bin` keeps pager-specific state separate from prefs/contacts. Write path uses v2; read path accepts v1.
- Migration helpers load existing prefs, preserve radio/UI fields, and layer pager defaults when enabled. Tests cover round-trips and upgrade from serialized old data.

## Build & Targets
- Use `PLATFORMIO_CORE_DIR=.pio-core` when the global `~/.platformio` cache is blocked.
- Dispatch: `pio run -e t1000e_dispatch_usb`.
- Pager clients: `pio run -e t1000e_pager_client_usb` (USB-only, BLE off) or `pio run -e t1000e_pager_client_ble` (optional BLE with `PAGER_ALLOW_BLE`).

## Testing
- Host suites: `PLATFORMIO_CORE_DIR=.pio-core pio test -e native_test`.
- Coverage highlights: core prefs blob round-trip, pager prefs defaults, pager migration from existing data, dispatch advert bit, ACK hash helper, slot/delay math, dispatch cache persistence, alert tones/priority parsing, multipart assembly/NACK timeout, and pager text parsing.

## Open Follow-Ups
- Add runtime coverage for pager gating in `MyMesh` (dispatch-only sends/CLI) and reconnect/heartbeat timing.
- Expose a configurable heartbeat knob in the dispatch firmware if operators need tighter/looser cadence.
- Exercise slotting/telemetry timing on hardware once radios are available.
