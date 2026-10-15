# Architecture

| Module | Present responsibility | Next work |
| --- | --- | --- |
| `dsr_core` | Portable bounded model/registry, request construction, version/OTA metadata checks | Capability enumeration, stable device identity, rooms/scenes |
| `dsr_board` | Diagnostic fallback and ES3C35P display/touch/backlight port, memory checks, LVGL owner/tick | Physical validation, battery/LED, other profiles |
| `dsr_platform/dsr_http.c` | Blocking IPv4 HTTP, Basic/Digest configuration, identity and readback | Physical auth tests, discovery, WebSocket and broader components |
| `dsr_platform/dsr_worker.c` | One network task, copied jobs/results, NVS writes and backpressure | Cancellation and multi-device fairness |
| `dsr_platform/dsr_wifi.c` | Wi-Fi STA and reconnect timer with 1..32-second backoff | Runtime lifecycle cleanup; onboarding currently uses explicit restart |
| `dsr_platform/dsr_settings.c` | Validated settings, NVS schema version, no automatic erasure | Migration, encrypted storage, rooms/scenes; device saved separately |
| `dsr_ui` | One-switch Home, Wi-Fi/device forms, five-second polling and hardware tests | Physical network workflow test, multiple devices, rooms/scenes and history |
| Future `dsr_ble` | Not implemented | NimBLE scan, BTHome, encryption and device-specific BLE RPC |
| Future `dsr_ota` | No installer yet | Verified manifests/images, progress, rollback, user approval |

The ES3C35P build creates LVGL objects before starting its sole UI owner task. DMA completion only signals flush-ready; the tick timer only increments LVGL time. The touch polling task publishes a synchronized sample and makes no LVGL calls. All subsequent UI object calls belong to the UI task. The UI submits a copied `dsr_job_t` without waiting for network I/O and consumes a copied `dsr_event_t`. A single outstanding job deliberately limits memory use in this foundation; `DSR_BUSY` prevents command loss. The UI must consume results regularly to release the slot.

No ambiguous control commands are retried. ON/OFF uses an explicit desired state rather than toggle. A successful request is followed by a readback; if the readback disagrees, report failure and refresh before enabling another action. A timeout may occur after a device has acted, so report uncertain state and refresh rather than claim success or blindly replay.

The portable registry is an unused IPv4+channel collection. Current control uses a separate saved binding to Gen1 MAC / Gen2+ ID plus channel; it verifies identity before read/write. Discovery and automatic address relocation remain future work. Use explicit units and measurement validity; missing power must not be represented as a measured zero.

The generic diagnostic entry point initializes NVS without erasing it and stops at the unconfigured board gate. The ES3C35P entry point initializes its board, starts one network worker, loads Wi-Fi settings and starts the station when configured, then creates the UI before its owner task runs. On-device Wi-Fi changes are saved in the worker and applied by explicit restart. One saved device/channel is probed at five-second intervals and re-identified before requests. The LVGL timer consumes result events, stops automatic retries for auth/rate/identity errors, and shows explicit ON/OFF only when a state was confirmed. No command is replayed on reconnect. Full multi-device scheduling, discovery, BLE and OTA installation remain future work.
