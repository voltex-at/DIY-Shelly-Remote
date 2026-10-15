# Architecture

| Module | Present responsibility | Next work |
| --- | --- | --- |
| `dsr_core` | Portable bounded model/registry, request construction, version/OTA metadata checks | Capability enumeration, stable device identity, rooms/scenes |
| `dsr_board` | Explicitly unconfigured board interface | Confirmed LCD, touch, memory, backlight, power, buttons, LED |
| `dsr_platform/dsr_http.c` | Blocking IPv4 HTTP probe/relay/switch adapter | Auth, discovery, WebSocket and broader components |
| `dsr_platform/dsr_worker.c` | One network task, copied jobs/results and backpressure | Poll scheduling, cancellation and multi-device fairness |
| `dsr_platform/dsr_wifi.c` | Wi-Fi STA and reconnect timer with 1..32-second backoff | Onboarding, network changes, lifecycle cleanup |
| `dsr_platform/dsr_settings.c` | Validated settings, NVS schema version, no automatic erasure | Migration, encrypted storage, device/room/scene persistence |
| Future `dsr_ui` | Not implemented | LVGL navigation and component-driven controls |
| Future `dsr_ble` | Not implemented | NimBLE scan, BTHome, encryption and device-specific BLE RPC |
| Future `dsr_ota` | No installer yet | Verified manifests/images, progress, rollback, user approval |

All LVGL calls must be owned by one UI task or the chosen LVGL port lock. The UI submits a copied `dsr_job_t` without waiting for network I/O and consumes a copied `dsr_event_t`. A single outstanding job deliberately limits memory use in this foundation; `DSR_BUSY` prevents command loss. The UI must consume results regularly to release the slot.

No ambiguous control commands are retried. ON/OFF uses an explicit desired state rather than toggle. A successful request is followed by a readback; if the readback disagrees, report failure and show the observed state. A timeout may occur after a device has acted, so report uncertain state and refresh rather than claim success or blindly replay.

The initial registry uses IPv4+channel. Before automatic discovery is released, replace this identity with verified Shelly device ID/MAC+component so DHCP address changes cannot select the wrong physical device. Use explicit units and measurement validity; missing power must not be represented as a measured zero.

The diagnostic entry point initializes NVS without erasing it and then stops at the missing board port. No network commands are issued on boot. A production boot sequence, scheduler, UI and provisioning still have to be connected.
