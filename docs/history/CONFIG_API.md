# Shared configuration API v1

The canonical document has schema 1 and a monotonic revision. There is no
telemetry, Wi-Fi password or Shelly credential in this document. Both editors
use the same validator, CAS replacement and persistence service.

| Collection / field | Shape and limits |
| --- | --- |
| Root | schema, revision, language (pl/en/de), theme (graphite/midnight), accent (24-bit RGB), active_dashboard, rooms, devices, scenes, dashboards |
| Rooms | max 8; id, name, icon (0–3), photo (empty or 32 lowercase hex characters) |
| Devices | max 16; id, name, host (empty or IPv4), channel (0–15), kind (light/relay/plug/cover/contact/sensor), room (0 or room ID) |
| Scenes | max 8; id, name, actions (max 8: device ID and on/off/open/close/stop) |
| Dashboards | max 4; id, name, tiles (max 24 each) |
| Tiles | id, type (device/room/scene), ref, span (1/2), height (1/2), fields (0–255 bitmask); device and scene span must be 2 |
| IDs / names | IDs 1–1000000, unique per collection; names up to 47 UTF-8 bytes |
| Document | max 24576 bytes, depth max 16; strict types, unknown/duplicate keys and broken references rejected |

fields bits: 1 power, 2 voltage, 4 battery, 8 temperature, 16 humidity,
32 position, 64 contact, 128 additional data. Unknown live values remain absent.
The model describes scenes; executing them is not implemented at this milestone.

HTTPS is mandatory for administrative operations. Pairing requires physical
access, a random one-time code and the same-origin request checks. Subsequent
requests require the paired cookie and `X-DSR-Request: 1`. No CORS is enabled.
Only local peers are accepted. HTTP port 80 is a GET redirect, not an API server.

| Method / path | Contract |
| --- | --- |
| POST /api/v1/pair | Raw 16-hex touch-displayed code; 120 seconds, max 5 attempts; sets secure cookie |
| GET /api/v1/config | Current canonical configuration |
| PUT /api/v1/config | Complete canonical document with its current `revision: N`; 202 queued, 400 invalid, 409 stale/busy |
| GET /api/v1/status | Published revision, save/bind busy and result messages, firmware version |
| GET /api/v1/preview.raw | 320×480 RGB565 snapshot; 202 until ready; unavailable during pairing |
| POST /api/v1/photos/{hash} | Exactly 320×180 RGB565 bytes; hash is first 128 bits of SHA-256 |
| POST /api/v1/devices/bind | name, host, channel, username, password; bridge to existing verified one-device worker |
| POST /api/v1/ota | App BIN and X-SHA256; requires separate physical authorization; local development installer |
| POST /api/v1/restart | Explicit restart after authenticated request |

An accepted save is not a durable-success response. Poll status/revision until
the write finishes. A separate bounded configuration task validates and writes
the older of two independent CRC-checked checkpoints, flushes and reads it back,
then publishes the new snapshot. Failure leaves the prior published revision.
Boot loads the newest valid record, falls back after a torn write and refuses
to overwrite a valid future schema. A corrupt nonempty filesystem is preserved.

HTTP and configuration tasks never call LVGL. The UI owner polls revision and
bind mailboxes and rebuilds its own view. Preview uses LVGL snapshots on that
owner; network sends a copied framebuffer outside the short publication lock.
Image reads/cache initialization occur outside LVGL. No restart is needed for
dashboard/content/style edits. Original Wi-Fi reconfiguration still restarts.
