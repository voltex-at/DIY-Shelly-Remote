# OTA implementation and remaining release work

**0.4.0-config-dev has a local development upload installer.** A paired HTTPS
session and a separate 120-second touch authorization are mandatory. It accepts
an application BIN, verifies its size, ESP32-S3/project metadata, exact embedded
ES3C35P board marker and supplied SHA-256, writes the inactive OTA slot and uses
ESP-IDF image validation before selecting that slot. Restart is explicit. After
five seconds of local UI operation with touch, configuration and panel readiness,
the new app confirms its boot. It does not require an Internet/Shelly connection.

**The 0.7.11 backend verifies schema-v2 Ed25519 manifests; its production trust
store is deliberately empty. The GitHub Releases downloader/installer remains
unimplemented.** See [signed bytes, tests and remaining gates](OTA_MANIFEST_PL.md).
0.7.14 adds a [bounded manifest HTTPS transport backend](OTA_FETCH_PL.md), not
release discovery, UI orchestration, application download or installation.
Target certificate date validation is enabled; unset system UTC refuses fetch.
Trusted time setup and physical TLS validation are still open gates.
A checksum does not authenticate the publisher. Only deliberately
selected development images belong in this installer; it is not the signed
release pipeline described below. No eFuses are written. Physical power-loss and
rollback validation remains pending. See [installation guide](CONFIG_WEB_PL.md).

Primary reference: [ESP-IDF v5.5.4 OTA documentation](https://docs.espressif.com/projects/esp-idf/en/v5.5.4/esp32/api-reference/system/ota.html).

## Storage contract

The ES3C35P 16 MiB profile preserves NVS, otadata, PHY and both 6 MiB app slots
at their original offsets. The previously unused final 0x3e0000 bytes now hold
`dsrdata` SPIFFS. OTA writes only an application slot; configuration, photos and
NVS remain outside it. Never alter this layout through application OTA. The first
upgrade from 0.2.0 requires USB to install the new partition table. Use separate
files from the guide to retain NVS; the merged USB image fills intervening gaps.

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

The schema-v2 wire contract in `docs/update-manifest.schema.json` now has a real
verifier. It binds board, partition layout, version, explicitly selected channel,
URL, size, digest and key ID. This is not a signed release or a complete updater.
Key rotation/recovery and an owner-approved anchor must be reviewed before any
release. The old core metadata helper remains stable-only syntax policy; the
wrapper explicitly distinguishes stable/dev manifests and accepts only trusted
numeric local release bases, not arbitrary build suffixes.

## Local upload status

Implemented as an authenticated, origin-checked development endpoint with a
one-attempt local authorization window. Board/slot/hash/image checks are active;
publisher signatures remain future work. Port 80 only redirects GET to HTTPS.

## UI acceptance

Settings -> System -> Firmware Update: current/latest version, check, release notes, confirmation, download progress, installation status, restart status and actionable errors. Never auto-install. OTA regression tests include malformed/forged manifests, wrong board/revision/chip, wrong checksum/signature, image too large, low power, interruption, failed health check and retained Wi-Fi/device/room/scene configuration.
