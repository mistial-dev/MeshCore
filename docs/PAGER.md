# Pager Mode Design & Plan

## Overview & Goals
- Keep payload type/version unchanged: pager rides on `PAYLOAD_TYPE_TXT`, so non-pager nodes still render messages.
- Discover dispatchers via advert `feat1` bit (`ADV_FEAT1_DISPATCH`) and a human hint (`[DSPCH]` prefix in the room name). Unknown bits are ignored by existing clients.
- All pager logic is gated by `PAGER_MODE`/`DISPATCH_NODE`; core prefs/contacts remain intact.

## Protocol & Message Shape
- Pages stay plain text with inline hints: `[PRIO:X]`, `[GRP:A,C]` (comma-separated, A–H), and counters like `1/1`. No new TLVs or payload types; pages must be single-packet (multipart code removed).
- Admin-only commands: any text starting with `[XXXX]` (e.g., `[PAGE]`, `[OTAR]`, `[NDID]`, `[AUDT]`) is accepted only from admins; non-admin senders get an error and no dispatching occurs.
- Two-shot ACKs stay on by default; ACK/telemetry replies can be slotted to reduce collisions.
- CLI for pagers is accepted only from the joined dispatch and exposed over USB; BLE CLI is disabled unless `PAGER_ALLOW_BLE` is set.

## Dispatcher Behavior (`env:t1000e_dispatch_usb`)
- Advertises the dispatch bit and should prefix the room name with `[DSPCH]`.
- Heartbeat expectation: emit at least one packet per minute; pagers watch for >60s of silence. The dispatch polls clients round-robin with a lightweight telemetry request (~100 ms airtime ≈0.17% duty at 60s) and skips nodes that already saw outbound traffic within the interval.
- Telemetry is pull-only: dispatcher requests, pagers respond immediately (no added delay); keep the cadence at 60s to hold airtime ≈0.17% at BW=250 kHz, SF10, CR5.
- Filters paging traffic to admins only; other room traffic remains unchanged for compatibility.
- Reserved tags: any text starting with `[XXXX] ` is admin-only. Non-admin attempts are rejected; admins can send `[PAGE]`, `[OTAR]`, `[NDID]`, etc.
- Default credentials (dispatch builds): `ROOM_PASSWORD=pagerUser`, `ADMIN_PASSWORD=pagerAdmin`. OTAR can be used to roll credentials without disconnecting remembered nodes.
- Paging is single-part; `[PAGE]` messages are sent only to pager nodes (dispatch knows the target set) with a tiny 20–49 ms jitter between unicasts to stagger ACKs. ACKs use a 500 ms window with slotting to avoid collisions. Non-pager clients may still see the text, but pagers alert only on dispatch-origin `[PAGE]`.
- Pager roster: dispatcher keeps a persistent roster keyed by pubkey → pager_id (short), group (A–H bitmask), is_pager flag, last seen. IDs are reused when a pager is evicted/leave frees the slot. Stored in `/pager_roster.bin` and updated when telemetry/paging traffic is seen.
- Admin CLI (implemented): `pager list`, `pager assign <pubkey|id> <group|X>`, `pager evict <pubkey|id>`, `pager otar <pubkey|id> <password>`, `pager page id <id> <text>`, `pager page group <A-H> <text>`. `pager page` auto-wraps the body with `[PAGE] `. Non-admin `[XXXX]` is rejected.
- OTAR flow: `[OTAR]` carries a one-time password. Once a pager ACKs an OTAR packet—or a client connects using that OTAR password—the old node is evicted, and the new node is added and remembered. This allows credential rotation without lingering access.
- NDID flow: `[NDID] ID:<short> GROUP:<A-H[,B,...]>` tells a pager its assigned ID and one or more groups (comma-separated) or updates them. Using `GROUP:X` signals eviction (ID freed for reuse). Groups are stored as an 8-bit mask.
- Audit CLI: `audit enable` (and `audit disable`) to log all messages with metadata on the dispatcher for review.

## Pager Client Behavior (`env:t1000e_pager_client_usb`, optional `_ble`)
- Inert until connected to a dispatch (advert bit present). Sends while disconnected return `RESP_CODE_DISABLED`, buzz, and blink the LED if available.
- Caches the last joined dispatch in `/pager_dispatch.bin` (v2: pubkey/name/timestamp), enforcing a single-dispatch rule without touching existing prefs/contacts. Loader accepts v1 for forward compat. Non-dispatch adverts are ignored/dropped so the app only surfaces dispatch rooms.
- Auto reconnect: after 60s of silence/disconnect, attempt passwordless re-login to the cached dispatch, then back off to every 5 minutes. Passwords are not cached; relies on the room remembering the node.
- Heartbeat/watchdog: >60s without dispatch traffic raises alert level C and LED blink until reconnect or user acknowledgement.
- Alerts (A–E) with RTTTL tones live in `examples/companion_radio/PagerAlertTones.h`; level E latches until a button press. Alert defaults: dispatch text/channel messages map to D unless `[PRIO:X]` overrides. LED mirrors alert state when present.
- Telemetry/ACK replies use slotting only when talking to dispatch. Pager CLI stays USB-only; BLE is off unless `PAGER_ALLOW_BLE` is defined (default off with `NO_BLE_FOR_PAGER`).
- Only accepts messages from the cached dispatch contact; ignores direct messages and other sources. Alerts on `[PAGE]` from dispatch; other tags obey admin-only rules enforced server-side. Discovery is filtered to dispatch adverts only.

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
- Coverage highlights: core prefs blob round-trip, pager prefs defaults, pager migration from existing data, dispatch advert bit, ACK hash helper, slot/delay math, dispatch cache persistence, alert tones/priority parsing, admin-tag parsing, and pager text parsing.

## Open Follow-Ups
- Add runtime/host coverage for pager gating in `MyMesh` (dispatch-only sends/CLI) and reconnect/heartbeat timing.
- Expose a configurable heartbeat knob in the dispatch firmware if operators need tighter/looser cadence.
- Exercise slotting/telemetry timing on hardware once radios are available.
