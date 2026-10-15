# Architecture

| Module | Present responsibility | Next work |
| --- | --- | --- |
| `dsr_core` | Portable bounded model/registry, request construction, version/OTA metadata checks | Capability enumeration, stable device identity, rooms/scenes |
| `dsr_board` | Diagnostic fallback and ES3C35P display/touch/backlight port, memory checks, LVGL owner/tick | Physical validation, battery/LED, other profiles |
| `dsr_platform/dsr_http.c` | Blocking IPv4 HTTP probe/relay/switch adapter | Auth, discovery, WebSocket and broader components |
| `dsr_platform/dsr_worker.c` | One network task, copied jobs/results and backpressure | Poll scheduling, cancellation and multi-device fairness |
| `dsr_platform/dsr_wifi.c` | Wi-Fi STA and reconnect timer with 1..32-second backoff | Onboarding, network changes, lifecycle cleanup |
| `dsr_platform/dsr_settings.c` | Validated settings, NVS schema version, no automatic erasure | Migration, encrypted storage, device/room/scene persistence |
| `dsr_ui` | LVGL navigation shells, hardware color/touch tests and backlight controls | Real device views, onboarding, rooms/scenes and energy |
| Future `dsr_ble` | Not implemented | NimBLE scan, BTHome, encryption and device-specific BLE RPC |
| Future `dsr_ota` | No installer yet | Verified manifests/images, progress, rollback, user approval |

The ES3C35P build creates LVGL objects before starting its sole UI owner task. DMA completion only signals flush-ready; the tick timer only increments LVGL time. The touch polling task publishes a synchronized sample and makes no LVGL calls. All subsequent UI object calls belong to the UI task. The UI submits a copied `dsr_job_t` without waiting for network I/O and consumes a copied `dsr_event_t`. A single outstanding job deliberately limits memory use in this foundation; `DSR_BUSY` prevents command loss. The UI must consume results regularly to release the slot.

No ambiguous control commands are retried. ON/OFF uses an explicit desired state rather than toggle. A successful request is followed by a readback; if the readback disagrees, report failure and show the observed state. A timeout may occur after a device has acted, so report uncertain state and refresh rather than claim success or blindly replay.

The initial registry uses IPv4+channel. Before automatic discovery is released, replace this identity with verified Shelly device ID/MAC+component so DHCP address changes cannot select the wrong physical device. Use explicit units and measurement validity; missing power must not be represented as a measured zero.

The generic diagnostic entry point initializes NVS without erasing it and stops at the unconfigured board gate. The ES3C35P entry point initializes its board port and hardware-test UI. Neither starts network/device commands in this milestone. The production scheduler, real-device UI and provisioning remain to be connected.
