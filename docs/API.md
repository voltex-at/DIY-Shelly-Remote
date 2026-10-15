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
| Identify generation | `GET /shelly`, legacy type and MAC fields | `GET /shelly`, `gen >= 2` and ID |
| Read selected relay/switch | `GET /relay/<channel>` | `GET /rpc/Switch.GetStatus?id=<channel>` |
| Set explicit output | `GET /relay/<channel>?turn=on` or `off` | `POST /rpc/Switch.Set`, body `{"id":0,"on":true}` |
| Confirm current output | `ison` boolean | `output` boolean |
| Optional power | Not implemented | `apower`, if present and numeric |

Use `/shelly` for identification; do not infer the generation from a marketing name. A newer generation is routed to the documented Gen2+ protocol only where its actual component implements that protocol. The current adapter supports selected switch components, not every Shelly device or component. Channel 0..15 is an implementation limit, not a claim about all model capabilities.

Transport limits: 3-second per-request timeout, 8192-byte response cap, no redirect following, strict dotted IPv4 endpoint, complete JSON object required, explicit 401/403 and 429 classification. HTTP/JSON adapters have native fixture coverage with real cJSON; physical Shelly testing remains pending. Gen1 power meters, dimmers, lights, covers and energy histories are future adapters.

## Authentication

Gen1 uses HTTP Basic; Gen2+ uses ESP-IDF 5.5.4 Digest (including SHA-256). Both optional IDF authentication features are enabled in the board defaults. Credentials are separate bounded strings, never URL parameters or log output; no redirects are followed. The unprotected `/shelly` endpoint is queried without credentials. A response buffer resets on `HTTP_EVENT_HEADERS_SENT` so a 401 challenge body cannot contaminate the successful JSON response. One authorization retry is allowed per request.

A successful user-directed add stores the Gen1 MAC or Gen2+ ID with the endpoint/channel. Subsequent polls and writes compare this identity first. Identity mismatch, authentication failure and rate limiting stop automatic polling until explicit Refresh or re-add. Explicit ON/OFF is followed by readback and is not replayed after timeout. These identity checks guard against accidental IP reuse, not a cryptographic authentication of local HTTP peers.

The fixtures check client configuration and HTTP callback behavior, not IDF's cryptographic implementation or a real protected Shelly. Physical validation is required. Gen1 Basic has no confidentiality on local HTTP. NVS is unencrypted in this development version. Primary IDF reference: [HTTP client v5.5.4](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32s3/api-reference/protocols/esp_http_client.html).

## Discovery, status and Bluetooth

Planned discovery queries `_shelly._tcp` and `_http._tcp`, validates identities, and offers the user devices to add. mDNS does not prove that a device is Shelly. Manual IPv4 addition remains a fallback.

Add WebSocket notifications with resynchronization/polling after reconnect. Never block the UI on probing or reconnection.

BLE work must distinguish passive BLU/BTHome advertisements from writable devices and BLE RPC-capable models. Verify individual capabilities, detect/reject unsupported encrypted payloads until keys and decryption are implemented, deduplicate packet events and never fabricate successful control of a passive sensor. Balance Wi-Fi and BLE radio time; enable NimBLE only on a confirmed capable target. No BLE code is implemented in this checkpoint.

## Cloud

Optional future work. The [official Cloud Control API](https://shelly-api-docs.shelly.cloud/cloud-control-api/) and documented authorization must be assessed separately for devices, rooms and scenes. No import or authentication flow is implemented or promised. Local control remains independent of Cloud.
