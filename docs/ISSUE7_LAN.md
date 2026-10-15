# Issue #7 A — actual local Shelly discovery

Development version `0.5.0-lan-dev`, draft PR #10, stacked on visual PR #9.
The firmware performs real network requests. Generated/demo discovery lists are
not compiled into firmware. AP provisioning, BLE and multi-device control remain
unimplemented and are explicitly disabled. Physical acceptance is still pending.

## Discovery contract

One separate FreeRTOS task (10 KiB stack, one queued operation) queries
`_shelly._tcp` and `_http._tcp`, 1.2 seconds each. It verifies candidates with
credential-free `GET /shelly`. Only port-80 IPv4 endpoints in the station's current
subnet are admitted; own/network/broadcast/multicast/loopback addresses and
duplicates are excluded. There is no DNS-to-arbitrary-URL proxy or redirect.

Fallback candidates come from passive ARP cache entries read on lwIP's owning
thread, prior verified addresses and configured hosts. There is **no subnet
sweep**. At most 32 hosts are probed sequentially, with a 650ms identity timeout
and an 18-second scan budget, up to 16 verified devices. Progress reports actual
probed/total counts. Cancellation is observed between bounded requests. If the
budget expires, the count can be lower than the candidate total. Missing mDNS,
client isolation, another VLAN, sleeping devices or a full result list can
prevent discovery; manual IP remains an explicit advanced fallback.

Identity parsing requires an actual Gen1 type/MAC/auth or Gen2/3/4 ID/MAC/model/
generation/auth response. IDs/MACs are bounded and normalized where appropriate.
Duplicate stable identity updates its location after DHCP; conflicting ID/MAC
claims are rejected. mDNS names/TXT alone never create an accepted device.

After selection, identity is checked again before authenticated read-only
`/status` (Gen1) or `/rpc/Shelly.GetStatus` (Gen2+). Actual relays/switches, covers,
lights and sensor components are parsed. Channels are not inferred from the
model or assumed to begin at zero. Only existing relay/switch transport can be
bound. Cover/sensor discovery is not a claim that their control is implemented.

Before binding, the existing worker again probes the selected expected ID and
channel, then commits NVS only on success. A changed identity/auth/timeout leaves
the previous binding intact. The shared configuration receives the chosen name
and room through its asynchronous validated service. Rebinding the same active
physical ID at a new DHCP address updates its old configuration location.
There is still **one active connection**, not 16 concurrent controlled devices.

Credentials are supplied only after selection, never broadcast during discovery
or returned in snapshots. Panel calls require the existing paired HTTPS session
and origin protection. Gen1 Shelly Basic HTTP and prototype NVS retain their
previous confidentiality limitations; this change does not reset the network,
disable device auth, or provision a Shelly device without consent.

## API v1 additions

| Endpoint | Behavior |
| --- | --- |
| POST /api/v1/discovery/scan | 202 accepted; 409 when unavailable/busy |
| GET /api/v1/discovery | phase/result/serial, actual progress, verified device records, selected identity and capability masks; no credentials |
| POST /api/v1/discovery/cancel | nonblocking cancellation |
| POST /api/v1/discovery/inspect | host, id, username, password; only a verified candidate can be inspected |
| POST /api/v1/devices/bind | adds optional expected_id and room to preserved binding endpoint |
| GET /api/v1/status | adds device_result alongside device_busy; distinguishes completion from success |

Phases: 0 idle, 1 scanning, 2 complete, 3 cancelled, 4 no Wi-Fi, 5 inspecting,
6 inspected, 7 error. Protected-device auth failures are operation results, not
an HTTP 401 that would log the administrator out of the panel.

## Validation and hardware acceptance

Portable UBSan: 94 core, 545 HTTP/parser/NVS and 222 Wi-Fi checks. Tests include
bad responses, auth, timeout, wrong identity before bind, nonconsecutive channels,
MAC/ID deduplication, DHCP relocation, capacity and subnet policy. Native LVGL
tests inject documented raw API fixtures into the actual C parsers and drive the
real wizard; these fixture providers are not part of the ESP32 build. Playwright
uses the shipped panel with the same C parsers/store and an explicitly simulated
radio provider. CI separately compiles the real mDNS/ARP/HTTP implementation.

**Physically tested Shelly models for 0.5.0: none yet.** Record the exact source
SHA, ES3C35P revision, real Shelly model/generation/firmware/auth mode and free
heap/PSRAM. Do not report the host fixtures as a physical result.

1. Install the matching ES3C35P development artifact using the settings-preserving
   USB procedure in CONFIG_WEB_PL.md; keep the prior artifact for recovery.
2. With no Wi-Fi, open + → Scan. Verify the no-Wi-Fi message and return path.
3. Connect to the same home LAN as a real Shelly. Scan: record timing, actual
   result model/IP/generation and whether credentials are requested. Cancel and
   retry. An unrelated HTTP server must not appear as a Shelly.
4. Select the device. For protected Shelly, enter a wrong password once: verify
   the error and unchanged previous binding, then correct it. Confirm channels
   match its API, choose a name and room, and save.
5. Check ON/OFF, readback and external state changes; restart and verify saved
   credentials, identity, name and room. The previous device remains bound after
   a failed candidate probe; a successful explicit replacement changes it.
6. Change its DHCP address/reservation, rescan and verify one result for the same
   ID/MAC. Rebind at the new IP; confirm no duplicate configured entry for that
   same active device. If another device occupies the old IP, control must fail
   identity verification and must not issue a switch command to it.
7. Repeat selection/inspection/binding from the paired Web Panel. Check no
   credentials are visible in API snapshots or logs. Verify unsupported device
   types have disabled binding, and AP/BLE are clearly unavailable.

Official protocol references: Shelly
[mDNS](https://shelly-api-docs.shelly.cloud/gen2/General/mDNS/),
[identity/status](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Shelly/),
[Gen1](https://shelly-api-docs.shelly.cloud/gen1/),
[authentication](https://shelly-api-docs.shelly.cloud/gen2/General/Authentication/),
[Espressif mDNS](https://docs.espressif.com/projects/esp-protocols/mdns/docs/latest/en/index.html).
