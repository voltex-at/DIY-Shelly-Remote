# Development — 0.7.72-dev

Current code is on `work/dev-0.7.70`; `main` is an older baseline. Read
[PROJECT_STATUS_PL.md](PROJECT_STATUS_PL.md) and [issue #74](https://github.com/voltex-at/DIY-Shelly-Remote/issues/74)
for requested work, implementation status and remaining acceptance. Historical
development branches and completed review outputs were removed on 2026-10-22.
Published tags and OTA release assets are retained.

## Layout and compatibility

- `dsr_core` / `dsr_config`: model, schema and persistent configuration.
- `dsr_platform`: Wi-Fi, BLE, Shelly RPC, workers, Web Panel and OTA controller.
- `dsr_board`: ES3C35P hardware; `dsr_ui`: LVGL; `dsr_ota` / `dsr_crypto`: verification.
- Board: LCDWiki ES3C35P N16R8, ESP32-S3, portrait 320×480.
- Keep ESP-IDF 5.5.4, LVGL 9.3.0, ST77922 2.1.1 and mDNS 1.8.2.
- The host LVGL commit is `c033a98afddd65aaafeebea625382a94020fe4a7`.
- Three skins: Cyber Blue (`cyber_blue`, 3), Grafit (`graphite`, 0), Jasny (`light`, 2).
  Midnight/1 is migration compatibility, not a selectable fourth skin.

Check Git status, branch and component manifests before editing. Preserve
uncommitted work, device data, the OTA trust key and partition compatibility.
Machine-local helpers under `tools/dev/`, `.agents/` and the setup report are
local environment configuration, not required firmware sources.

## Focused validation

After a small change, run the directly affected module or UI case. Inspect the
actual affected LVGL pixels. Do not generate a full atlas or run every test
after each spacing adjustment. Keep summaries short and logs under ignored
`.local/`. Source tests and generators remain available after output cleanup.

```sh
python3 tools/test_ui.py --case test_dev_069_ui
python3 tools/test_ui.py --cases test_cover_ui test_cyber_reference_ui
python3 tools/test_restart.py --backend-case confirmed-busy
python3 tools/check_docs.py
```

Use an existing case appropriate to the changed module; inspect runner options
before selecting one. `DSR_UI_ATLAS_SCREENS` filters exact changed atlas routes.
A full three-skin, PL/EN/DE review belongs at the end of a major UI stage.
Full CI for the exact release commit remains mandatory before publication.
ASan/UBSan and negative safety cases must not be weakened to pass a change.
Host rendering and target compilation are not physical-device acceptance.

## Local contact/heating rules

The canonical workspace schema is 3. Stored schemas 1 and 2 are validated by
their frozen validators and upgraded in RAM; only an explicit save writes the
upgrade. `automations` contains at most eight `contact_heating` definitions,
disabled by default, with contact/target layout IDs and `target_half` (8..60;
default 16 = 8°C). A thermostat belongs to at most one rule.

Use Scenes → Automations on LCD or the web panel. Both use the shared workspace
CAS save; editing never sends a device command. Any open contact limits heating
without raising a lower target. All contacts must close before individual saved
targets are restored. Manual intent takes priority, including the same value.
Disabling/deleting cancels future automatic writes without restoring a target.

The executor uses the existing config task, actor and sole worker. Its private
CRC journal is separate from config revisions; it records ownership/overrides
before physical writes. Resume requires new source reports from every contact,
valid UTC and direct TRV reads. The implemented direct-read contract is limited
to Gateway Gen3 `BluTrv.Call` → `TRV.GetStatus`; mirrors never prove a target.
Beacon mode is not enabled automatically. An external same-value write cannot
be detected; physical Gateway/TRV readback and power-cut recovery remain release
acceptance work.

Focused checks: `python3 tools/test_automation.py --case model|store|http|config|runtime|api|rate`,
`python3 tools/test_ui.py --case test_automation_ui`, and
`node tests/test_panel_automation.cjs`. Run `--case all` once at an integration
boundary. New API `GET /api/v1/automations/status` requires the existing local
same-origin/pairing gates and exports public status/roles for one config revision,
without enrollment keys or journal contents. There is no automation execute POST.

## System data maintenance

System → Backup and reset uses native LCD consent and the paired HTTPS panel.
Activity retains 128 entries in RAM. Password-encrypted `.dsrbak` files contain
configuration, photos, connections and local Bluetooth bonds; restoration is
restricted to the same physical remote. Password processing stays in Web Crypto.
Import has separate upload and candidate confirmation, with date/version/counts.
Reset requires a physical 3 s hold. Both destructive operations require a fresh
battery estimate ≥20%, or physical stable-USB consent when the estimate is unknown.

Snapshot/staging run on the existing config owner, with an acknowledged BLE
barrier. One temporary 16 KiB HTTPS transport task keeps browser STOP/status/cancel
serviceable during socket or owner waits; it never performs RPC or source writes.
The upload ID remains a cancellation alias until candidate commit. Boot recovery
precedes ordinary services; ambiguous journals fail closed with local recovery UI.
Full reset erases only the explicit user namespaces and data filesystem, preserving
firmware and OTA trust. Never exercise reset/restore on hardware without approval.

Focused checks: `python3 tools/test_system_data.py --case all`,
`python3 tools/test_ui.py --case test_system_data_ui`,
`node tests/test_panel_backup.cjs`, and `node tests/test_panel_backup_browser.cjs`.
Host providers model Flash, radio, ADC and physical consent. Device durability,
reconnection and stack high-water marks remain separate acceptance work.

## Windows and firmware builds

Keep toolchains under `.local/`, host dependencies under `.test-deps/`, and
generated firmware/UI output under ignored `build*/`. Preserve the active MCP
runtime `.local/esp32-ai-setup/mcp/`. The removed logs and build caches can be
created again when needed; they are not part of the source checkout.

On this machine, activate `tools/dev/Enter-EspIdf.ps1` and use
`tools/dev/Build-Firmware.ps1` when a board build is actually needed.
Portable ESP-IDF invocation:

```sh
idf.py -B build-board -D DSR_BOARD=lcdwiki-es3c35p set-target esp32s3
idf.py -B build-board -D DSR_BOARD=lcdwiki-es3c35p build
```

`-D DSR_VERSION=x.y.z-dev` overrides the CMake project version for release builds.
These build commands do not authorize flashing or erasing Flash/NVS.

## CI and releases

`.github/workflows/ci.yml` checks native C/sanitizers, real LVGL and browser UI,
the ES3C35P board build, and diagnostic ESP32/ESP32-S3 compile coverage. Diagnostic
targets are not additional supported boards. Target auditors check memory,
stack, dependency and OTA trust contracts without raising budgets.

`.github/workflows/release.yml` starts from a new release tag on an exact commit
with CI 5/5: `dev-vX.Y.Z` is DEV, `vX.Y.Z` is Stable. Signing keys belong only to
the protected `release` / `stable-release` environment with owner approval and
tag-only deployment policy. The public manifest uses `voltex-ota-2026-1`.
Never print or commit private keys, credentials, SSIDs or device backups.

One release build produces app BIN, signed manifest, FACTORY, app-only UPDATE
and SHA256SUMS. Validate provenance, signatures, target/version policy and public
download digests before declaring publication complete. Existing tags/assets
remain immutable. A new DEV, Stable, merge to main or device flash requires
the owner's explicit authorization for that operation.

The old `.github/dev-release-*.json` requests no longer drive publication and
were removed. Historical release process details remain in Git tags rather
than in the current development guide. Old Actions artifacts/logs are disposable;
retain the latest CI and Signed release record as the current release evidence.
