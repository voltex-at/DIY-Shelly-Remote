# Development — 0.7.77-dev

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

Background refresh updates cached values in place. Automatic battery reads must
not imitate a tap, disable the Measure button, or show manual-reading feedback.
Rooms and room tiles on Dashboard update their existing aggregate labels without
recreating photos on telemetry changes. Structural changes still rebuild them.
Rooms retains the photo geometry of the original 186 px cards. Its footer frame
fits the wrapped name plus metric badge; rows follow the taller frame of each
pair plus the standard 8 px gap. Dashboard tile dimensions remain configured.
BLU age-only changes update the model without recreating list cards on Dashboard,
Devices or a room's device list. Details still display the signal age; real
contact/capability/identity changes and Cover position remain repaint triggers.
BLU details retain their signal-age label; electrical group details retain
numeric fields. Usage retains the gauge while only sample age changes and
defers genuine rebuilds during a press. Setup-only transitions use
`dsr_network_prepare_home()` without first building an invisible Dashboard.
Action results and OTA labels redraw only when their presented values change.
Memory bars show occupied capacity; labels retain free/minimum-free values.
The stack bar therefore shows peak usage, while BLE is a binary module state.
Drawing a screen/header must never enqueue background I/O. Schedule it in the
owner poll and yield to queued device controls, STOP and active group/scene work.
A real Measure tap may adopt the existing read without queuing a second one.
Actual failures remain visible; requested cover positions are never presented
as confirmed movement.
Cover details update the existing artwork and labels for stable endpoints.
After a transport failure, a last confirmed position may remain on screen only
with an explicit historical caption and the actual error; it never authorizes
movement. Calibration, invalid data, auth failure or a changed endpoint clears
that presentation history. Recovery uses GET readback, never a replayed move.

Interaction has priority: touch/display work stays above the network worker
(existing task priorities 6/5/4). UI callbacks update local feedback immediately;
RPC remains asynchronous. Explicit STOP keeps its independent urgent lane.
At a free worker boundary, native user intents precede actor work, followed by
due reads for the current device/group or cards intersecting the viewport.
After at most two preferred reads, give automations or other due devices a turn.
The open battery page's ADC refresh follows manual controls and precedes unrelated
polling; inactive ADC monitoring runs last. Entering a screen may refresh a healthy sample
older than two seconds; failed-device/login backoff and direct HT sleep remain.
Thermostat writes keep the 1.2 s tap coalescing interval. Known TRVs use typed
status after gateway/MAC verification (three requests per refresh, five per
successful write/readback). Cover ends at the documented null command ACK;
its owner publishes acceptance before scheduling a separate background GET.
ACK never refreshes sample time or substitutes target position for actual data.
One failed transport read after ACK may retry quietly with the previous sample
at its original age; persistent loss and auth/identity/data/safety errors remain
visible independently of the accepted command. Urgent STOP has its own ACK and
does not borrow a superseded move's phase or erase an unsent move's old sample.
An acknowledged TRV write may recover one lost status GET; uncertain writes
are never replayed. A delayed TRV mirror gets
at most three early GET-only follow-ups, ending on convergence or error before
normal cadence/backoff resumes. Pending targets remain separate from reported data.
The existing serial worker finishes in-flight I/O: queue priority cannot shorten
an exchange already waiting on Shelly/BLE. Measure touch-to-feedback, queue wait
and device response separately on hardware; host tests do not prove latency.

Cover recovery after an uncertain WRITE schedules one immediate GET opportunity
before routine unrelated reads; manual commands and urgent STOP retain priority.
Movement/recovery PROBE jobs omit advisory name/config I/O so verified telemetry
can be delivered promptly. Ordinary polls still refresh metadata. When the actor
converts a typed intent to a normal PROBE, clear both reused intent bytes.
`dsr_http` logs failed/slow Cover legs and command responses without hosts,
credentials or bodies. The 0.7.76 hardware timeout itself remains under diagnosis:
[evidence and focused changes](RESPONSIVENESS_AUDIT_0_7_76_PL.md).

After native OTA, UI snapshot/battery updates run after consuming worker results.
Only restart admission runs after the network poll releases its own activity
lease. Automatic countdown stays at zero while accepted work drains; a manual
Restart tap latches the same request. Pending manual TRV intents must drain even
before they take a worker lease. Defer new ordinary background reads/automation
work while waiting; never use forced restart to bypass write/boot/OTA gates.

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

Local web OTA uses the same temporary HTTPS transport reservation as backup;
the server remains available for status and STOP. It consumes the LCD consent
once, bounds the transfer to 180 seconds and checks the authenticated session
between chunks and before boot selection. Explicit LCD preview refresh requests
a new capture generation; browser polling waits for that capture for at most
eight seconds and does not schedule further captures in the background.

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

Before preparing or approving a release, complete the mandatory
[download and device-update checklist](RELEASE_CHECKLIST_PL.md). It requires
anonymous public catalogue/manifest/BIN downloads and an actual upgrade from
the previous supported DEV, restart/data-retention checks, and local web OTA.
CI success alone is not hardware acceptance. The checklist also defines the
explicit candidate stage needed because draft assets are not publicly readable.

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
