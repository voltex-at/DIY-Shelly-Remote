# Shelly protocol decisions

Primary documentation reviewed 2026-10-15:

- [Gen1 local HTTP API](https://shelly-api-docs.shelly.cloud/gen1/)
- [Gen2+ RPC transport](https://shelly-api-docs.shelly.cloud/gen2/General/RPCChannels/)
- [Gen2+ Switch component](https://shelly-api-docs.shelly.cloud/gen2/ComponentsAndServices/Switch/)
- [Gen2+ authentication](https://shelly-api-docs.shelly.cloud/gen2/General/Authentication/)
- [Gen2+ mDNS](https://shelly-api-docs.shelly.cloud/gen2/General/mDNS/)
- [Shelly BLE documentation](https://shelly-api-docs.shelly.cloud/docs-ble/)
- [Shelly BLE RPC guidance](https://kb.shelly.cloud/knowledge-base/kbsa-communicating-with-shelly-devices-via-bluetoo)

## First implemented source path

| Operation | Gen1 | Gen2+ |
| --- | --- | --- |
| Identify generation | `GET /shelly`, legacy type field | `GET /shelly`, `gen >= 2` and ID |
| Read selected relay/switch | `GET /relay/<channel>` | `GET /rpc/Switch.GetStatus?id=<channel>` |
| Set explicit output | `GET /relay/<channel>?turn=on` or `off` | `POST /rpc/Switch.Set`, body `{"id":0,"on":true}` |
| Confirm current output | `ison` boolean | `output` boolean |
| Optional power | Not implemented | `apower`, if present and numeric |

Use `/shelly` for identification; do not infer the generation from a marketing name. A newer generation is routed to the documented Gen2+ protocol only where its actual component implements that protocol. The current adapter supports selected switch components, not every Shelly device or component. Channel 0..15 is an implementation limit, not a claim about all model capabilities.

Transport limits: 3-second per-request timeout, 8192-byte response cap, no redirect following, strict dotted IPv4 endpoint, complete JSON object required, explicit 401/403 classification. The adapter is written but not target-compiled or hardware-tested. Gen1 power meters, dimmers, lights, covers and energy histories are future adapters.

## Authentication

Gen1 and Gen2+ authentication differ. Gen2+ documents Digest/SHA-256; adding a generic Basic Auth flag is not sufficient. Implement/test the actual challenge/response with correct nonce handling before claiming protected-device support. Current `DSR_AUTH_REQUIRED` tells the caller to show an unsupported-authentication message. Never ask users to disable authentication to satisfy a feature claim.

## Discovery, status and Bluetooth

Planned discovery queries `_shelly._tcp` and `_http._tcp`, validates identities, and offers the user devices to add. mDNS does not prove that a device is Shelly. Manual IPv4 addition remains a fallback.

Add WebSocket notifications with resynchronization/polling after reconnect. Never block the UI on probing or reconnection.

BLE work must distinguish passive BLU/BTHome advertisements from writable devices and BLE RPC-capable models. Verify individual capabilities, detect/reject unsupported encrypted payloads until keys and decryption are implemented, deduplicate packet events and never fabricate successful control of a passive sensor. Balance Wi-Fi and BLE radio time; enable NimBLE only on a confirmed capable target. No BLE code is implemented in this checkpoint.

## Cloud

Optional future work. The [official Cloud Control API](https://shelly-api-docs.shelly.cloud/cloud-control-api/) and documented authorization must be assessed separately for devices, rooms and scenes. No import or authentication flow is implemented or promised. Local control remains independent of Cloud.
