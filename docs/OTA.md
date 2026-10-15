# OTA design and current limits

**No OTA installer exists yet.** The portable core validates limited metadata syntax, stable version ordering, exact board ID and available slot capacity. It neither checks an actual image digest/signature nor writes Flash. This distinction must remain explicit in the UI and release notes.

Primary reference: [ESP-IDF v5.5.4 OTA documentation](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32/api-reference/system/ota.html).

## Storage contract

The documented ES3C35P 16 MiB profile now includes NVS, otadata, PHY and two 6 MiB app slots in `boards/lcdwiki-es3c35p/partitions.csv`. This layout is compiled for hardware bring-up and still requires physical validation. No installer, signature validation or post-update health confirmation is present. Ensure the final signed image fits both slots. Do not change the partition layout through routine OTA; a layout change requires an explicit USB migration guide. Preserve configuration and schema migrations. The merged first-install USB image may overwrite settings; it is not an OTA update.

## GitHub Releases path

1. Fetch stable release metadata over verified HTTPS using the real repository ID/URL. Ignore drafts and prereleases. Do not ship a repository URL before that repository exists.
2. Validate a versioned manifest with exact hardware revision, stable version, image URL, image size, SHA-256 and signature/key ID. Numeric version parsing must not compare strings lexicographically.
3. Authenticate the manifest/image with a trusted public key embedded in firmware. A checksum alone proves neither publisher identity nor authenticity. Define canonical signed bytes or verify an immutable manifest byte stream before parsing; reject unknown keys and algorithms.
4. Require user confirmation and an adequate power state if the board can measure it. Serialize all update operations; show a state/progress indicator and let network work run outside the UI task.
5. Download to the inactive application slot with HTTPS verification and an allowlist for the verified repository's asset origins/redirects. Enforce declared size, slot bounds and HTTP failures. Do not put private tokens in public firmware.
6. Check the downloaded image SHA-256, board/revision and ESP image metadata, and cryptographic signature before selecting it to boot. ESP chip ID alone does not validate the LCD/touch/pin map. Investigate ESP-IDF signed-app verification without hardware secure boot for the confirmed target; no eFuses are written by this project foundation.
7. Retain the current application until the verified download is complete. A failed/interrupted download leaves the current slot bootable; restart the download unless a separately validated resume design is implemented.
8. After reboot run a local health check: NVS migration, display, touch registration, UI task and worker readiness. Only then call `esp_ota_mark_app_valid_cancel_rollback()`. A temporary lack of Internet/Shelly device must not invalidate otherwise healthy firmware.
9. With rollback enabled, a crash/reset before health confirmation restores the preceding valid image. Test this on physical hardware, including power interruption and USB recovery.

The JSON schema in `docs/update-manifest.schema.json` is a proposed wire contract, not an example signed release or a functioning verifier. A signed manifest must bind the board ID to the exact image digest. Key rotation and recovery must be reviewed before stable releases. The current development version is outside the stable-only OTA policy.

## Local upload

Optional, later stage. It must be disabled by default, opened for a short period by a physical/UI action, require a per-device credential over a suitably protected connection, validate origin/CSRF as appropriate and enforce the same signed-image, board and slot checks as remote OTA. An HTTP password on an open network alone does not secure firmware upload. Do not add an unauthenticated upload endpoint as a shortcut.

## UI acceptance

Settings -> System -> Firmware Update: current/latest version, check, release notes, confirmation, download progress, installation status, restart status and actionable errors. Never auto-install. OTA regression tests include malformed/forged manifests, wrong board/revision/chip, wrong checksum/signature, image too large, low power, interruption, failed health check and retained Wi-Fi/device/room/scene configuration.
