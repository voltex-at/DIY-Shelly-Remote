# GitHub import and releases

The account `voltex-at` was accessible at the initial checkpoint, but the connected toolset could not create a new repository. No remote commit or Release is claimed.

Create a **public** repository named exactly `DIY-Shelly-Remote`, with this description:

> An unofficial open-source ESP32 touchscreen remote controller for Shelly smart home devices, with local Wi-Fi and Bluetooth connectivity.

Use the included source tree as the initial commit; include `.github/workflows/ci.yml`. GitHub's browser upload can omit hidden files, so verify that the workflow is present. Use an authenticated Git client or the connected contents integration for importing sources. Do not paste access tokens into scripts, URLs, logs or this repository. Preserve existing remote work if someone initialized the repository first; use a branch/PR if necessary.

## Foundation CI

`ci.yml` runs portable tests and ESP-IDF v5.5.4 compile coverage for ESP32/ESP32-S3. Diagnostic targets do not support either physical board by implication. Artifacts are diagnostic ELF/map files only. Check real CI results before marking this foundation target-compiled.

## Firmware release gate (future)

No automatic release workflow is enabled in the foundation. Add one only with a confirmed board profile and verified signing method. It must build a specific commit with pinned/resolved dependencies, run tests, bind the hardware ID, calculate image size/digest, sign the manifest/image, preserve third-party notices and create a **draft/prerelease** for human hardware validation.

Production assets must distinguish the USB first-install image/layout from the OTA app-only image. Attach board/MCU/revision, version, checksums, signatures, source commit, build instructions, release notes and tested-model matrix. Never upload diagnostic builds as installable firmware. Never promote a release to stable automatically before physical validation.

Keep signing private keys in an appropriate external signing service or protected release secrets, never in source. Embed only trusted public verification keys in the released firmware. Signing/key rotation and repository authenticity need a documented review before OTA is enabled.
