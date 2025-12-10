# Dispatch Server (Pager Mode)

This example is the dispatch/pager-aware room server. It builds on the room server behavior but adds:
- Dispatch advert hint (`ADV_FEAT1_DISPATCH`) and enforced name prefix `[DSPCH]`.
- Pager roster persistence (`/pager_roster.bin`), admin-only CLI (`pager list/assign/evict/otar/page`), and dispatch-only filtering.
- Single-packet `[PAGE]` fan-out with per-unicast jitter (20–49 ms) and a 500 ms ACK window.
- NDID/OTAR helpers for assigning IDs/groups and rolling credentials.

See `docs/PAGER.md` for the full protocol and runtime expectations, including timing and CLI details.
