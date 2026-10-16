# Project status

## 0.7.32 — shared device onboarding candidate

The LCD opens one method sheet above the suspended HOME view, with explicit
Next for Bluetooth, factory AP, LAN or advanced manual IP. Shared steps, lists,
keyboard and fixed footers keep the same flow in PL/EN/DE. Web preserves method,
DOM, locale and session ownership while using the existing transport contracts.
AP/B1 retain both physical consents and typed identity verification before BIND.

Integrated with PR60 groups/scenes/activity and the existing PR50 revocation
port. Full native UBSan and actual LVGL5552/204, Cover112/7 and restart14/23/18/25
passed. Independent review reproduced a late prior BIND taking over a newly
started method; Next now waits through RAM, workspace and private ACK stages.
Its actual LCD regression passes29 checks. Late LAN/Web replies also retain
session ownership. This source still needs exact-source target/browser CI and
received-artifact verification. Physical RF, first boot and retention remain
open; no hardware writes or Stable publication.

## 0.7.31 — shared groups, scenes and volatile activity candidate

The scene card now runs explicit actions; its editor is separate. Groups have
Switch-only ON/OFF and per-step partial results. LCD and authenticated Web use
one bounded RAM actor, frozen typed identity/locator, exact job ACK ownership,
nonce/admission generation and a180s deadline. Manual Cover STOP keeps priority.
Activity distinguishes observed availability from command acknowledgements.
Workspace schema2 upgrades validated schema1 in RAM; only explicit Save writes
the new version through the existing dual-checkpoint/CAS transaction.

Includes the reviewed port of existing PR50/b128694 (author voltex-at), keeping
single-key pairing revoke separate from user settings, photos, TLS and reset.
Base PR59/a194554 has5/5 CI38922784329 and all five received artifacts verified,
including APP/ELF/partition/SPIFFS provenance and actual LCD/browser renders.
This source still needs its own target/browser/artifact acceptance. Independent
review has reproduced stale Web responses, availability/command log aliasing
and post-write uncertainty; their regression fixes are integration gates.
No physical device action, main merge, Stable or automation change.

## Existing PR50 — shared Web pairing revocation port

Preserves the implementation and authorship of [PR50](https://github.com/voltex-at/DIY-Shelly-Remote/pull/50),
source `b1286945ef0551a934644ce2adffa2f2bdc71119`, on current Cover/history/P0/
photo/native/BLE parent `faa623f`. No duplicate PR or version bump. Separate LCD
confirmation queues the existing worker: invalidate current RAM token, pairing
code and local Web OTA window before erasing ONLY dsr_panel/token_hash; commit
and NOT_FOUND readback precede success. No factory reset, namespace wipe, Wi-Fi/
workspace/photo/TLS identity change or restart. Errors stay explicitly uncertain
about Flash and never restore the old RAM credential; already authorized work
may finish. New physical pairing remains possible.

Current STOP registration/operation namespaces, history and native updater stay
intact. Revocation is a payload-free appended job kind; job560/event1192 and all
budgets unchanged. local_request/authorized and the complete current-parent API
route table retain exact bytes; no remote revoke endpoint. Host/actual LVGL,
target CI, received artifact and physical acceptance are distinct gates.
Host full-native UBSan and actual LVGL5137/169 + Cover105/7 PASS; pairing786,
focused worker4170 (including same-number revoke/STOP ownership), prior history/
OTA/P0/B1/AP intact. [Original contract and integration evidence](docs/PAIRING_REVOKE_PL.md).

## 0.7.30 — Cover integration candidate

Stacked on exact PR58/f240d15: 5/5 CI38919291117, all five received artifacts
verified, APP/BIN/ELF/partition/SPIFFS provenance and PL/EN/DE renders checked.
This candidate adds the complete typed Cover family and the P0 evidence/reset
diagnostic correction below. Independent review found two concrete safety
counterexamples; the queued-movement/STOP and canceled-Web-gesture tests were
RED before their fixes and now pass. New target/browser/received-artifact
validation remains required for this source. Physical P0 and movement remain
open; no device writes, main merge, Stable or automation changes.

## P0 evidence correction — physical test was 0.7.23 / PR51

Owner [correction6217253815](https://github.com/voltex-at/DIY-Shelly-Remote/issues/28#issuecomment-6217253815)
supersedes the earlier0.7.25 attribution: the device ran0.7.23/PR51, not PR53.
The reported109/992 markers and883erased blocks come from the owner's separate
private analysis; that dump is unavailable here. Erase All explains lost data,
not interrupted formatting. PR51/PR53 share storage blobbaa433f649d69d768877d492b5a4036984d8d817.
The app now logs the SDK reset reason before NVS/board/storage, alongside the
existing blank-check/start/finish logs. No watchdog/yield/mount/geometry change,
new erase or hardware write. A complete first-boot USB log remains required;
the format interruption and physical acceptance remain open.

## #23 — complete Gen2+ Cover family on current support stack

Typed discovery/manual BIND and durable schema3 registry → Cover.GetStatus →
explicit Open/Close/Stop/GoToPosition → genuine LCD and same authenticated Web
intent path. Frozen legacy structs import Switch0 explicitly; installed offsets,
record footprints, workspace schema1/hash and existing relay contract retained.
Fresh identity/status gates all movement; calibrated known position gates GOTO,
errors/calibration block movement, null ACK followed by mandatory readback.
No profile/calibration/slats/Gen1 Cover action or POST replay.

Single bounded urgent STOP lane has its own operation/result/lease, no outer
queue/id/semaphore reuse or new task. It runs between HTTP legs; a pending outer
result is consumed before the STOP snapshot. Existing SDK I/O cannot be preempted.
LCD sends one drag-release intent, keeps confirmed position separate from preview
and cancels on invalidation. PL/EN/DE actual frames inspected. WWW preserves DOM,
keeps STOP available during movement, and fails closed after uncertain ACK.

Integrated on immutable history/settings/P0/photo/native/BLE1324fcc. Actual
endpoint_kind reaches every history boundary; Cover1 is explicitly unsupported,
never a Switch history alias. Five Cover flags reuse energy telemetry32 B;
connection296/slot496/fleet7952 B, job560/event1192 B retained. B1 Cover-only
Plus2PM follows both physical consents and station/identity verification before
typed BIND; security/preflight gates retained.

Independent RED counterexamples reproduced and fixed: queued OPEN→STOP cannot
move afterwards, invalidated WWW drag cannot replay on re-enable, and older
GET serials cannot unlock a new command across uint32 wrap. STOP admission uses
a bounded160 B secret-free registration/tombstone under its existing mutex;
no job/event/task growth or SDK I/O under the mutex. Poll's STOP result is in a
separate bounded frame; target validation still required.

Full portable UBSan exit0: HTTP/NVS/fleet2173, actual worker4089, API9719 with
exhaustive allocation faults, mailbox635, history2304+20+29+34+83+82. Actual JS
runtime165/history32, LCD4619/161 + Cover105/7 + P0 restart14/23/18/25, review
2635/24 PASS. Playwright pointer/locale cases await real browser CI; no local
browser PASS claimed. Target ELF/BSS/stack and physical movement/persistence
remain open. No hardware, erase, main merge or Stable. [Contract](docs/COVER_GEN2_PL.md).

## 0.7.29 — integrated energy history, contextual settings and photo filenames

Stacked on exact P0 PR57/51124b4 (5/5 CI and received FACTORY/UPDATE/BIN/ELF
proof). This branch adds #22 with empty-bucket and monotonic-clock regression
fixes, one canonical SNTP owner shared with native OTA, contextual settings
ported from PR34 and reconciled with concurrent PR55, and compact full128bit
photo names compatible with unchanged SPIFFS geometry. No duplicate #29 PR.
Integrated native/actual LVGL and final target/browser/physical acceptance
remain distinct gates. No device writes, main merge or Stable.

## 0.7.28 — P0 durable BIND publication and separate FACTORY/UPDATE packages

BIND verification now stays in RAM. Exact durable workspace ACK precedes one
full private NVS record with its final layout ID; only matching final ACK and
fresh mapping activate the endpoint. Failed rebind retains the previous private
record and sibling channels. Registry-v2 ID0 is quarantined; only proven legacy
v1 origin keeps its earlier fallback. Explicit retry, Web deletion/CAS and fresh
boot windows have actual worker/LVGL counterexamples and regression coverage.
No raw NVS schema/size, partition, mount or automatic formatting change.

Owner [issue28 decision6217161680](https://github.com/voltex-at/DIY-Shelly-Remote/issues/28#issuecomment-6217161680)
confirms Erase All and requests a complete clean installer. FACTORY16MiB now
includes preformatted SPIFFS at0xC20000, all992block magic, exact segments and FF
gaps, partition MD5 and CRC32/SHA256. A separate app-only UPDATE has no storage
or partition image. Original pinned IDF mount on simulated NOR plus real config
save/remount is required before upload. This does not explain interrupted blank
formatting, and does not authorize physical erase/flash or close physical P0.
[Recovery and installation contracts](docs/STORAGE_RECOVERY_PL.md).

Parent PR54 e69e24a CI38907543775 and PR56 3b96534 CI38910724169 are each5/5 PASS,
with received BIN/ELF/source/partition proof recorded in their PR bodies. This
stack includes native signed OTA (production trust EMPTY) and open-window B1
BLE, not bonding/BLU/BTHome. Current0.7.28 exact-head CI/artifact and physical
acceptance remain open. History/settings/onboarding and Cover continue in
isolated host work; do not call the entire backlog complete. No main/Stable,
user data access, device writes or automation changes.

## P0 — publish a binding only after both durable acknowledgements (#28 / PR53)

HTTP verification is RAM-only. Exact durable workspace ACK precedes the sole
full private registry save with the final layout ID; a fresh mapping check also
precedes UI activation. Until then a new endpoint cannot receive a card fallback
or SET, and rebind preserves the previous private host/credentials and siblings.
No compensating delete or command replay. Errors distinguish verification,
workspace/CAS and private persistence; failed attempts require explicit re-add.
Restart after workspace ACK but before NVS leaves an unbound inactive card.
Old schema2 ID0 entries are quarantined even if a locator matches; only genuine
schema1 origin retains legacy fallback. Raw NVS/schema/partitions are unchanged.
Known BLU Gateway Gen3 has an explicit unsupported-relay message and HTTP guard.

Genuine host worker/workspace/fleet/LVGL regressions cover IO/CAS, siblings and B,
Web deletion in both finalization windows and four fresh-process restart modes.
Full portable UBSan exit0: platform1396, worker3684 and workspace/runtime562;
actual LVGL4227/143 frames plus boot14/20/18/20, review2635/24 frames and
dictionary2103/266 entries PASS. Deterministic Web runtime47/AP46/BLE28 retained.
The previous active-fallback/SET counterexample failed on 267c32f, 8735107 and
d9dbf27 before this fix. SDK/NVS/mount providers remain modeled; no physical
operations. [Transaction and limits](docs/MULTI_DEVICE_PL.md). Storage mount and
factory packaging are separate P0 work; this change does not alter them.

## 0.7.26 — native signed OTA controller and installer (#24)

Actual LCD Stable/explicit Dev catalogue → genuine signed manifest check →
one-shot120s physical stable-USB consent → fresh signature/exact tuple →
bounded inactive-slot stream/write/end/boot selection → manual restart.
Existing sole worker, no job/event growth; synchronized progress/cancel/hide
and late results. CHECK never downloads APP. Untrusted release notes render
as bounded plain labels, independent of publisher authentication.

Shared Web/native staging reserves all writes; confirmed STAGED or uncertain
boot-selection error blocks further writes until restart, with activity released.
Exact actual partition preflight, lazy explicit-check SNTP with normal TLS dates,
no full BIN allocation/replay/automatic restart. [Contract](docs/NATIVE_OTA_PL.md).

Production trust remains EMPTY: no approved key or signed release was introduced,
so this build refuses installation before APP/Flash. Genuine host signature/
hash/fetch/image + modeled SDK fault/cancel/concurrency/uncertain suites PASS;
independent actual actor1936, discovery3316/55cases/61OOM, actual discovery fetch
136982, SNTP gate162 and LVGL3992/134 frames PASS. Target CI/ELF/BIN audit is a
separate current-source gate. Physical transfer/rollback/persistence is open;
no device writes, main merge or Stable publication occurred.
## #22 — INTERNAL 24h history host vertical; P0 storage integration held

Actual correlated endpoint readbacks feed a bounded PSRAM 5-minute ring. LCD
Energy/Charts and authenticated WWW share exact identity/kind/channel queries,
1h/24h W/V/A and actual Wh counters, UTC gaps/reset/missing/zero distinction.
The sole worker owns two bounded CRC journals and confirmed readback; no new
task or filesystem geometry/mount/format changes. Unmounted/future/corrupt
storage remains preserved and RAM-only. Canonical lazy SNTP must be shared
with the native OTA bridge at integration, never two services.
[Contract, footprint, write volume and limits](docs/ISSUE22_HISTORY_PL.md).
Host tests do not prove physical retention/endurance or final target budgets.
Publication/new Flash integration is held pending the active P0 storage work.

## #29A — contextual Device Settings integrated with current fleet

Ports existing PR34 source693d869 onto PR53 base267c32f, preserving current
workspace CAS, exact durable ACK, CRUD return/delete and private fleet mapping.
Settings and local name/room editing follow stable IDs through reorder/back;
Refresh resolves only the selected bound endpoint and leaves other endpoints'
auth/rate/command errors intact. Unbound/reused-host and changed-locator cards
cannot read someone else's Shelly. No registry/NVS/schema/migration changes.
Actual LVGL tests cover deferred IO/retry, SaveA/editB, conflict, PL/EN/DE,
room context, long names, missing IDs and repeated navigation. See
[scope and physical acceptance](docs/DEVICE_SETTINGS_PL.md). Current target CI
and physical acceptance are pending. History/Charts #22/#29B and model-specific
RPC settings #23 remain open; this does not close #29 or the frozen backlog.
Local UBSan: actual LVGL 4086/137 frames, review 2635/24, full native and config 265
PASS; 40 editor cycles warmed LVGL heap 97432→97432. Radio/Flash providers are MODEL.

## 0.7.25 — factory AP onboarding with physical consent (#19)

Real radio scan, temporary BSSID/channel-pinned RAM join, API identity and two
LCD consents. Exactly one Gen1 form/Gen2+ JSON Wi-Fi provisioning POST, no
authorization retry/redirect/reboot; restore home on success/failure/cancel,
then exact LAN ID/MAC/generation + STA SSID/IP readback before existing BIND.
Preserves settings/NVS/workspace and other endpoints. LAN drain barrier before
radio change; SDK restore failure never reconnects an AP as home. Hidden cleanup,
late ACK, busy verification and uptime deadline regressions covered.

Authenticated WWW opens the actual LCD wizard through a 10-second mailbox;
no configuration/credentials body and both physical consents remain on LCD.
Explicit disconnected/unknown/expired states, no POST replay, locale preserves
DOM. [Protocol/scope/recovery/acceptance](docs/ISSUE19_AP_PL.md).
Open factory AP with unauthenticated HTTP only; HTTPS/PKI/enhanced-security
profiles remain unsupported. New target CI/artifact and physical AP acceptance
are gates; no device writes/main merge/Stable/erase.

Parent PR51 07331b8 exact CI38898040624 5/5, downloaded source/BIN/ELF/FLEET
verified: app1836112 B SHA2567304ac8598509f215a0c2dbbda285c5ea6afc4cda0f14248cb1d4a961ca114e6,
internalBSS64992 B; two physical Shelly acceptance remains open.

## 0.7.23 — persistent independent Shelly relay endpoints (#20)

Adding a second verified endpoint retains the first. Private NVS schema2 registry
with exact reread, legacy record untouched, unchanged workspace schema1/numeric IDs.
RAM verification→durable layout ACK→full private registry ACK, targeted removal, identity checked before
credentials/SET and mandatory readback. Per-endpoint FIFO intent, state/auth/backoff;
serial bounded transport with honest latency, no automatic SET retry. LCD/W/V/A and
same authenticated Web runtime/output API, browser commands correlated by epoch/id.
Web controls expire locally even if network hangs; no optimistic output or POST replay.
LAN marks already added identities while allowing another channel and verified DHCP.
[Contract, migration, limits and two-device retest](docs/MULTI_DEVICE_PL.md).

Core independent review/coordinator found and resolved mapping, rollback, queue and
OOM issues before integration. Integrated full tools/test.py UBSan exit0; actual
LVGL3576/116 with independent targeted CRUD/two-store failure/recovery tests PASS.
Late review regressions also prove durable-delete readback failure/retry convergence
and concurrent Web deletion winning over stale published-ID CAS migration.
A missing published ID returns explicit PL/EN/DE DSR_CONFLICT, never recreation.
Platform1093/worker3559/config265 and all prior native regressions preserved.
Integrated exact-source CI/target/artefacts remain separate gates. Runtime API5029 checks with
cJSON allocation faults; actual workspace mailbox164; actual JS47 deterministic
race/freshness/epoch checks. Browser route/RTOS/NVS/device providers are MODEL.
Target own-frame/static-array checker and internalBSS<=81920B budget compare parent
48952B; unchanged LVGL131072B PSRAM/UI8192B. Not runtime heap/transitive peak proof.
Physical min.two Shelly, latency/coexistence/restart remain OPEN; #20 not closed.

Parent PR48 f9bac66 and PR49 8637296 both exact CI5/5 PASS. Existing owner report
confirms rooms retained after reset and Web service on preformatted SPIFFS;
blank-init, full TLS/pairing and new feature acceptance remain separate. No erase,
partition changes, main merge or Stable. Next frozen backlog: AP#19/BLE#18, then
full signed OTA#24, device settings/history#29/#22, device types#23/groups#21/#32.
Pairing revocation is separately published as Draft PR50, not in this build.

## 0.7.22 — browser-local Web Panel language (#5/#6)

Owner [PR5comment6214082384](https://github.com/voltex-at/DIY-Shelly-Remote/pull/5#issuecomment-6214082384)
requested independent PL/EN/DE locale after physical Web service success.
Header selector stays available before pairing/after401; browser-local
dsr.web.locale and supported navigator language fallback, no remote language
or revision changes. Explicit UI text/attribute bindings update existing DOM,
not destructive form render; user names/credentials/file inputs/raw LVGL remain.
Appearance distinguishes shared remote language from browser preference.
[Contract](docs/WEB_LOCALE_PL.md). No backend/auth/NVS/partition/OTA changes.
Independent Playwright cases cover state/no-request/input identity/dirty/CAS/
pending save/locale/storage failures/mobile geometry. Local Chromium launch
fails SIGTRAP in restricted environment: syntax/review is not browser PASS.
Actual CI/renders/exact artifact and physical locale acceptance separate gates.

Parent Draft [PR48](https://github.com/voltex-at/DIY-Shelly-Remote/pull/48)
sourcef9bac663 exact CI38892424314 5/5 and downloaded firmware/debug proof in PR.
App1780240 B SHA2569f9bfae58ef04482083883d7c3ff776fd4f29ff15967b4cf84e86efb72594856;
UI3223/111, own target frames and full regressions PASS, physical new UI still open.
P0 owner0.7.18/preformatted SPIFFS rooms survive reset and Web works, not blank
init or full TLS/pairing acceptance. No further erase/restore/main/Stable.
Frozen pairing revoke remains separate/unpublished; do not duplicate it.

## P0 update — 2026-10-16 10:54 UTC / 0.7.21-editor-acceptance-dev

Fresh owner hardware report [PR33comment6213932010](https://github.com/voltex-at/DIY-Shelly-Remote/pull/33#issuecomment-6213932010):
owner restored a complete PRE-FORMATTED SPIFFS image ONLY to dsrdata under
separate scoped consent. Actual0.7.18 now opens editor/rooms/model/form/keyboard.
Later [owner report11:02UTC](https://github.com/voltex-at/DIY-Shelly-Remote/pull/33#issuecomment-6214082938)
confirms rooms survive reset and Web Panel works on this preformatted image.
This is physical retention/service evidence, NOT blank-format success,
full TLS/pairing or full #28 PASS. Do not repeat erase or
replace/restore dsrdata again. Private photos/video not retrieved/published.

New P0 fixes on current stacked UI: fresh exact own ACK + clean draft A + same
screen generation returns to captured parent context; B edits/IO/CAS/mismatch
stay. Context room return resolves stable ID after published model reread,
deleted ID falls back Rooms. Generic dashboard/room delete is visible48px,
named confirmation before draft mutation, own_save/membership/refs rechecked,
last dashboard protected and active fallback valid. Separate Save→ACK required;
no persistence/schema/OTA/auth/partition/format changes. [Contract/retest](docs/EDITOR_ACCEPTANCE_PL.md).
Independent real LVGL UBSan3223/111 screenshots PASS; root repeated, reviewer
authored regressions, coordinator cross-review without blockers. PL/EN/DE,
47-character names/ellipsis and48px buttons covered;40cycles heap104456→104456.
Full native/config265 UBSan PASS; target own-frame checker smoke2positive/
8negative PASS. Providers explicitly MODEL, not physical new UI acceptance.
Exact CI/artifact and physical new UI retest still gates. Main/Stable
unchanged. Native OTA/full backlog/history24h internal/later optional SD remain.

Independent pairing-revocation work is preserved UNPUBLISHED on
work/issue-6-web-pairing-revoke, not part of this P0 build: backend786/worker3570/
UI3356/110 host tests passed, but new hardware P0 took publication priority.
Do not duplicate or claim revocation included here; resume after P0 review.

## 0.7.20 — confirmed LCD restart through shared backend (#6/#24)

Settings → System → Restart remote now opens an explicit confirmation screen,
separate from HOME pencil/editor. Opening, Cancel and Back submit no request.
Confirm calls the existing nonblocking admission backend, never esp_restart or
storage APIs from LVGL. BUSY/UNAVAILABLE remain retryable with translated status;
ACCEPTED disables both action buttons and latches for this process, including
leave/reopen and synthetic repeated events. Screen pointers are cleared on hide;
the accepted latch is not. It is an accepted request, not a promise a frame or
HTTP response was delivered or that shutdown has completed. No public cancel
after acceptance, retry timer, reset/erase/format or new backend policy.

PL/EN/DE use existing scroll-body and 48-pixel buttons. Export/import/reset and
full native OTA remain unimplemented; the old LCD-restart-unimplemented copy is
replaced without claiming those other functions complete. Backend parent PR46
e72074c fixes new harness cJSON include discovery after initial CI4/5; own final
CI/artifact proof remains separate. Independent real LVGL UBSan2838/102 frames,
PL/EN/DE non-overlap and buttons48px, retry/zero-call cancel/duplicate guards
PASS; 40cycles heap106712→106712. Reviewer authored tests, coordinator independently
reviewed production/lifecycle/renders without blockers; root full native/config265
UBSan PASS. Backend provider in UI is explicitly MODEL; genuine admission and
dispatcher tests remain in parent. Physical restart/retention still required.
Do not replace owner's P0 test0.7.18 mid-test.
[Contract](docs/RESTART_PL.md). Global backlog and hardware P0 remain open.

## P0 hardware update — 2026-10-16 09:56 UTC

Historical plan below superseded by10:54 preformatted-image outcome above;
do not interpret the old pending first-boot plan as permission to repeat erase.

Fresh PR33 comments6212871631/6212940294: owner tested0.7.18, UI/boot/Wi-Fi
observed working, but SPIFFS remains nonempty/unmounted, workspace unavailable
and HTTPS not started. This does NOT exercise the blank-only initialization fix;
the merged app image does not touch dsrdata. No new physical app/ELF hash log,
blank-format/workspace/HTTPS/Rooms persistence acceptance yet.

Owner separately authorized ONE dsrdata-only erase at0xC20000 length0x3E0000,
end0x1000000, performed LOCALLY by owner with existing private backup. This is
not full-flash/NVS/bootloader/OTA-slot permission or an automated recovery rule;
do not repeat erase. Await uninterrupted FIRST boot log on existing0.7.18,
format result/timing/reset reason, workspace/HTTPS READY and retained Salon.
No remote device action performed here; raw backups/photos/network data private.
[Evidence and scope](https://github.com/voltex-at/DIY-Shelly-Remote/pull/33#issuecomment-6212940294).

## 0.7.19 — atomic operation admission and deferred restart (#24/#6)

Shared single-word atomic admission spans queued/running/event-pending writes
and real UI poll handoff to persistent follow-up. Restart reserves only idle;
new operations refuse thereafter. NVS settings/connection, workspace save/bind,
pair token, photo fsync/cache, OTA receive+boot selection and boot health write
are covered. No snapshot-only check. Boot-only provisioning/GC/TLS remain before
boot-ready; no SPIFFS mount/format changes. Underflow failclosed, no public reset.
[Contract/inventory/tests](docs/RESTART_PL.md).

Dedicated deferred dispatcher, configured stack4096/priority1, static<=16 plus
atomic word4; SDK/TCB/shutdown/transitive peak not measured. IDF atomic32 wrappers
permitted for PSRAM workaround, not a lock-free/wait-free claim. Task/notify/
boot failures do not restart or leave a false accepted reservation. At most one
dispatch500ms outside LVGL/HTTP, no application mutex held through shutdown.
Existing authenticated Web restart now uses shared ACCEPTED/BUSY/UNAVAILABLE,
200/409/503; auth/local-only preserved. Source API guard normalizes only exact
restart routing line, not skipped. LCD confirmation still NEXT, not completed.

Independent atomic16052/dispatcher131/pair+restart97 and uploader234559 UBSan
PASS; root full native and real UI2334/96 PASS, including receive-handoff
admission attempt. More writer faults/cross-review/exact target/CI/artifact
still gates. No physical restart, persistence/data-retention or P0 acceptance.
Final independent actual-writer UBSan: NVS935, worker3534, workspace123 and
photos101 PASS; fault cleanup reopens admission and accepted restart admits
no new write. Root read these genuine source harnesses and repeats full native;
RTOS/NVS/filesystem/shutdown providers remain MODEL. Both cross-reviews without
blockers. Startup nonfatal dispatcher failure and boot-health write guard covered.
Parent PR45 source0790e6e8 CI38878544578 5/5, actual BIN/ELF/segments/partitions
and full genuine chain with two software SHA providers verified; full OTA still
open with empty publisher trust and unresolved owner-approved Release/time/etc.

P0 PR33 remains81e734d with no new physical PASS, main51c5840 unchanged. No
erase/nonempty-format/partition/main merge/auto-merge/Stable. Global Settings,
full backlog/history24h internal/later optional microSD remain required.

## 0.7.18 — explicit Release → signed app validation/DISCARD (#24)

Public bridge takes explicit release/STABLE and trusted local manifest/STABLE
policies, never caller-created candidate/verified. Genuine metadata fetch/parser
then same genuine manifest/app chain; candidate.manifest_size binds actual
MANIFEST bytes/CL/chunked exact before crypto and app request, not BIN size.
Tag stays opaque routing, no inferred semver or new same-tag app policy. Each
phase advances only after full FETCH_OK; every failure zero out, late cancel
included. Production publisher trust EMPTY -> ZERO app requests. No Flash/sink,
esp_ota/slot/reboot/UI/worker/latest/discovery/cache/retry or new naming/dev/time
policy. Success retains no app bytes, not installation or approved Release.
[Contract and gates](docs/OTA_RELEASE_CHAIN_PL.md).

Bridge heap<=1024; release16KiB/cJSON freed before manifest/app. Compile-time
phase bounds are not SDK/TLS/cJSON/transitive peak measurements; phase budgets
remain separate cooperative30s/SDK5s, never hardtotal30s. Existing standalone
manifest/app APIs retain expectedlength0 behavior; no second verifier/provider.
Independent tests, cross-review, exact-source CI/artefact remain gates.
Independent chain UBSan system233256 HOST/186385 empty (66/64 OOM), original
pinned cJSON238418/191535 (70/68 OOM) PASS. Actual parent PR44 app as HOST-only
signed fixture9045950/192549 (70/68 OOM) PASS; empty production always0app.
Opaque tag vs numeric signed version, exact manifest length before key lookup,
three separate29s phase progress and metadata heap release before manifest
verified. Coordinator code/memory review without blockers. SDK transport/TLS
MODEL, crypto/hash genuine, no hardware/Flash proof. Exact-source CI/artefact
for this chain still separate gate; root repeats independent test corpus.
Parent PR44 source784f99ae CI38877323489 5/5 PASS; downloaded actual app1776368 B
SHA1ec31b8ee165a6d99cd2c30d81553c03f0a7989c7a3cd906db3048ac818b21cf,
ELF5e58e9750f904fdd81409cc79a6ed9ad29880fc762953f33d9322218c93c5932,
ZIP/source/version/chip/segments/ELF descriptor/unchanged partitions verified.
Own metadata wrapper frame64 B, shared fetch80 B, BSS/UI stack unchanged.

P0 PR33 still81e734d without physical PASS; main51c5840 unchanged. No erase,
nonempty format/partitions/main merge/Stable. Publisher anchor/rotation/signed
Release, correct UTC/real TLS, naming/dev/time/power/consent/serialized install/
health/rollback/data-retention remain separate gates. Global backlog,24h
internal history and later optional microSD still required. #24 NOT complete.

## 0.7.17 — explicit single-Release HTTPS metadata (#24), not updater

Shared existing TLS/framing transport, exact own-repo api.github.com tag URL,
zero redirects/auth/retries/query. Explicit caller tag/asset/STABLE policy,
GitHub Accept and API version2022-11-28. HTTP200 only, identity, canonical CL
or chunked, actual1..16384 B/complete; statuses never mean no new firmware.
Close/cleanup and free transport context before genuine candidate parser.
Every error zero candidate, including final cancel/time. Only full FETCH_OK
publishes UNVERIFIED routing. No manifest/app bridge/download, Flash/UI/worker,
discovery/latest/list/cache, credentials or new production naming/dev policy.
[Contract and limits](docs/OTA_RELEASE_FETCH_PL.md).

Separate release workspace<=16640 B, existing transport<=8192 unchanged;
own transport-phase bound24832 B excludes SDK/TLS/header allocations. Parser
phase releases transport context but adds cJSON allocations. No16KiB stack
buffer, measured heap or transitive-stack claim. SDK5s/cooperative30s is NOT
a hard total DNS/TLS/header/positive-progress deadline. Existing last pending
trailer callback limitation remains explicit. Correct UTC/trusted time setup,
approved publisher anchor/rotation/signed Release and physical TLS remain gates.

Parent PR43 source2fb3e452 exact CI38871611118 5/5 and artifacts verified;
app1775408 B SHA e3e6bfc6393c154beae3fee87b0970288d998e672db83c76c54c28a2eccd9a1d.
This stage needs independent regressions/review, exact-source CI and artifact.
Independent release-fetch UBSan:129009 checks/system cJSON and133415 with original
pinned IDF cJSON,29/31 genuine allocation faults PASS. Root repeated the pinned
corpus; full native/config265/rawHTTP3012 cases/provider and existing manifest/
app regressions PASS. Reviewer transport MODEL, not physical TLS. Coordinator
implementation/memory review without blockers; exact target/CI still gates.
P0 PR33 remains81e734d without physical PASS, main51c5840 unchanged. No erase,
nonempty format/partition/main/Stable. Global backlog,24h internal history and
later optional microSD remain required; full OTA and hardware acceptance open.

## 0.7.16 — pure Stable release candidate parser (#24), not approval/discovery

Single GitHub Release object, explicit caller tag/asset/STABLE policy. No latest
or list selection, tag-to-semver inference, network/UI/Flash or new production
convention. Root id/same own-repo API URL, exact tag, typed draft/prerelease=false
and exactly one uploaded named manifest asset,size1..2048, exact repo download
URL. Output is only UNVERIFIED CANDIDATE routing, not approved release or signed
metadata. Author/digest/immutable/notes cannot authenticate publisher. Empty
production trust remains unchanged. [Contract](docs/OTA_RELEASE_PL.md).

Alloc-free strict preflight<=16KiB/depth8containers/512values, canonical uintsafe53
numbers, valid UTF8/escapes/surrogates, global raw/decoded-NUL rejection; genuine
cJSON decode and duplicate-key rejection after decode in EVERY object, including
ignored nested metadata. All failures zero output. Bounds are not measurements
of cJSON heap/recursive-transitive stack peak. Target own-frame/symbol/dependency
audit prepared; independent corpus/sanitizers and exact CI/artifact are gates.
Coordinator primary-source and implementation review: no blockers.
Independent reviewer corpus:27989 UBSan checks/1162 cases,34 positive candidates,
27 real cJSON allocation faults; exact-sized non-NUL-terminated input buffers.
Python stdlib oracle, all baseline truncations,700 seeded mutations and strict
UTF8/decoded-dupe/resource/policy boundaries pass. Root repeated independently;
reviewer and coordinator code review: no blockers. Full sanitizers/target pending.
Additional exact IDF cJSON c859b25 source/header HEAD/SHA pins:29868 UBSan
checks/1162 cases/29 real allocation faults PASS. Separate CI instruments the
dependency itself, not only a wrapper linked with uninstrumented system cJSON.
Local LSan /proc/ptrace restriction is NOT a sanitizer PASS. Transient empty
LVGL cached object quarantined/rebuilt; unchanged UI2237/96,review2635/24,
full native/config265 UBSan pass after rebuild, without UI source changes.

Latest parent PR42 source1153d710: [exact CI38865021733](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38865021733)
5/5 PASS. Downloaded firmware12031475637/debug12030762817: CRC/SHA/source/version/
chip/project/all segments/checksum/ELF SHA/license/unchanged partitions checked.
App1772880 B SHA cb04113b3d786e8a32a742bef3a1c9a914c67194557d48cfd490a049a3e5c2a7;
ELFdbd5c147ac4d8434ac681161e666b603bd6f8250abeb87573a5277f1a2b17cff.
Actual file passes unchanged signed HOST-fixture chain with OpenSSL and original
pinned Mbed TLS SOFTWARE SHA:10015211 HOST/169661 production-empty checks,
36/34 alloc faults; production always zero app requests. No physical TLS/Flash/
auth/rollback/P0 claim. Diagnostic-S3 allocated sections lack full board marker.

P0 PR33 still81e734d, no new physical PASS; main51c5840 unchanged. No erase,
nonempty format/partition/main/Stable. Correct UTC, approved publisher anchor/
rotation/signed Release, production naming/dev policy, release metadata transport,
LCD/worker/progress/consent/power/serialized install/health/rollback remain open.
Global backlog,24h internal history and later optional microSD still required.

## 0.7.15 — manifest-first app stream validation/discard (#24), not installer

Public chain fetches/verifies manifest first; only DSR_FETCH_OK permits any app
request. No API accepts caller-created verified metadata. Signed URL/size/hash
feed the existing real image helper via the same private HTTPS transport, with
fresh app clients/one-hop allowlist/clock/date/framing policy. CL must equal
signed size; chunked is bounded by actual signed size and final completeness.
App heap buffer reads<=1024 B; each byte reaches the helper once. All app bytes
are discarded; no Flash/OTA/slot/reboot/UI/worker or installable-image claim.
Only full-chain OK publishes metadata; all failures zero output. Each phase has
its own cooperative30s budget, not a hard DNS/TLS/header/positive-progress-read
deadline. Fetch<=8192 plus chain<=512 target-compiled bounds exclude image
context and all SDK/TLS/JSON allocations; actual peak requires hardware.

Root raw-wire regression first reproduced an SDK-visible late Location bypassing
the initial final-200 policy; post-body/cleanup check now rejects it before
manifest/image acceptance. Raw-parser UBSan3012 cases/provider PASS
(1615474 HOST/2110802 production-empty checks); previous fetch350791/345251
checks unchanged. This is host policy evidence, not a physical failure or a
cause of P0 SPIFFS interruption. Independent app-chain regressions and exact
target CI/artifact are separate gates. [Contract](docs/OTA_DOWNLOAD_PL.md).
Pinned SDK does not emit the final pending trailer at message-complete; the
public API exposes no read-only getter. Rejection covers delivered header
events, not all raw trailers. Harness now preserves this behavior; negative
trailers use X-After to trigger dispatch. No Location from200 is followed, and
signed metadata/exact size/hash remain mandatory; no authenticity-bypass claim.
Independent chain UBSan:220127 HOST/168689 production-empty checks,36/34
real allocation faults; empty trust always starts zero app requests. Signed
size511, invalid signed URL, both redirects, CL/chunked/cache, size/hash/header/
marker/IO/crypto/cleanup faults and post-finish cancel/deadline covered.
Actual PR41 app fixture (exact SHA below) also passes independently HOST-signed
chain:10013219 HOST/169661 production-empty checks. Test signature is not
publisher trust or a Release. Full native/config265, UI2237/96 and review2635/24
UBSan PASS; actual target/full sanitizer CI remains separate before acceptance.

Latest parent PR #41 a298e686: exact-source CI5/5
[38859714148](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38859714148),
full raw-parser sanitizers. Downloaded firmware12029106340/debug12028941876
verified CRC/SHA/source/version/chip/project/all segments/checksum/ELF SHA/
license/unchanged partitions. App1772528 B SHA256
516700a62369e957abb9ddf6d6e8e090a68f9b893eae27ea0fa799670365e076;
ELF5b65a4d2a73efc60b1d2cabc7d60ff724225c15c9307fe052e82ed3060c90081.
Actual receiver+pinned software SHA memory-sink passes chunks1/511/512/4096.
Downloaded diagnostic-S3 has no full board marker in any allocated file-backed
section. No physical TLS/install/rollback/P0 acceptance; no erase/main/Stable.
Publisher trust remains empty; UTC, approved anchor/rotation/signed Release,
release discovery, LCD/progress/consent/power/serialized staging/health/rollback
remain open. Full global backlog/history24h internal/later microSD unchanged.

## 0.7.14 — bounded HTTPS manifest transport (#24), no installer

Explicit repo manifest URL, one manually checked GitHub-to-release-assets hop,
fresh TLS client/CA bundle/hostname validation, strict HTTP framing and <=2048
raw bytes delivered to the existing Ed25519 verifier. No app download/Flash,
slot selection/reboot, UI caller or release picker. Every error zeros output.
Date checks are now enabled on target; unset UTC refuses before network. This
stage establishes no trusted time/SNTP and no publisher anchor. Empty production
trust still refuses all manifests; physical TLS/time/redirect remains untested.
Cooperative timeout/cancel and own context bound are NOT total SDK heap/stack
or hard DNS/TLS/header deadline claims.
Independent reviewer fixtures pass UBSan: 350791 HOST-trust and 345251
production-empty checks, each including 33 genuine allocation failures (own
context + cJSON hooks). Post-crypto cancellation/deadline rejects and zeros
output even when the verifier itself succeeded. Coordinator review: no blockers.
Full native/config265, unchanged LVGL2237/96 and review2635/24 pass locally;
exact-source target CI/artifact and physical network tests are separate gates.
[Contract, source references and remaining gates](docs/OTA_FETCH_PL.md).

PR #41 source 7744651: exact-source CI 5/5
[run 38858870344](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38858870344).
Downloaded firmware12028162155/debug12028097266 pass archive CRC/SHA,
source/version/chip/project, every flasher/merged segment, app XOR/appended SHA,
ELF descriptor SHA, license and unchanged partition table. App1772528 B SHA256
8b3b3ee48bd12c7464a03ab8765544e9ef7b5ddaa2f395c147ed92b80ebc974a;
ELF8899cf7c3b5d3474ca517feccb2f42ab3e91fed5a2ef5aa3719669c27380aaa7.
Target fetch frames32/48/64 B, heap context compile-time bound<=8192 B;
BSS48904 internal/145504 external and UI stack8192 unchanged. Actual BIN also
passes existing receiver/pinned software SHA memory sink chunks1/511/512/4096.
These are target/build/host proofs, not device TLS, installation or P0 acceptance.

Additional test-only follow-up uses original IDF HTTP parser7355072 with both
file SHA256 pins, real fetch/verifier/Ed25519 and independent HOST signatures:
3010 raw-wire cases/provider,1614269 HOST-trust/2109597 production-empty
UBSan checks PASS. Each response fragment size, every truncated CL/chunked
prefix, trailers, duplicate/conflicting headers, body cached with headers,
malformed wire and modified signed bytes covered. SDK header assembly/cache/
transport remain explicit models; no sockets/TLS/hardware proof. Full sanitizer
CI for this follow-up is pending; local LSan is blocked by restricted /proc.

PR #40 source 3501a67: exact-source CI 5/5
[run 38854805117](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38854805117).
Firmware/debug CRC/SHA/source/version/chip/segments/checksum/ELF SHA/license/
unchanged partitions checked. App1697200 B SHA256
08bf48f5401030ed97bbaaaa45119a297e5988a5ef020fb164330fd59e48bdaf.
Actual receiver + original pinned Mbed TLS software SHA passes the actual BIN
chunks1/511/512/4096, each write byte-compared in memory only. Both actual
diagnostic BINs lack board marker; downloaded diagnostic-S3 ELF also lacks it
in allocated sections. No physical Flash/auth/rollback or P0 acceptance claim.

## 0.7.13 — integrate streamed validation into existing dev Web upload (#24)

The actual receiver uses the shared validator for every byte, retains the first
512 B until header acceptance and writes prefix/remainder once. Heap workspace
4608 B replaces large local buffers/probe. All receive/hash/write failures abort
only a live handle; end invalidates it even on failure. Boot selection remains
in the existing endpoint, after success only. No auth/partition/restart change.
Explicit dev checksum factory does not pretend client X-SHA256 is a signed
manifest; uppercase checksum letters now reject before Flash (never succeeded
previously). Native signed trust remains empty and separate.
Additional review gate: PR39 diagnostic S3 ELF really retained the full board
marker in loadable .flash.rodata; image-helper force retention is now LCDWiki-
only, with negative checks on actual diagnostic BINs in both target builds.
Local UBSan: upload/actual endpoint136002, image2741363, native/config265 and
LVGL2237/96 PASS. New exact CI/artifact and physical acceptance are separate.
[Contract, regressions and physical gates](docs/OTA_UPLOAD_STREAM_PL.md).

PR #39 source 2a3e7dd passed exact-source CI 5/5
[run 38853370330](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38853370330).
Downloaded firmware/debug ZIP CRC/SHA, segments, app checksum/appended SHA,
chip/version/source/ELF SHA/license/unchanged partitions verified. App1697472 B,
SHA256 c758358ed7e2b3b49ab3ef54192d139c27016a1871b26d33c48203f74ae21e5a.
Real helper target context724 B, own frames32/64 B; BSS/UI stack unchanged.
Actual app passes OpenSSL+Python and original pinned software Mbed TLS with
chunks1/511/512/4096. No physical pass, installation or main/Stable claim.

## 0.7.12 — bounded streamed app-BIN validation (#24), no installer

Independent backend copies accepted manifest size/hash, hashes every byte,
checks exact length, ESP32-S3/app/project headers and NUL-terminated board
marker across every chunk boundary. Sticky failures and explicit reset; bounded
heap context, no 4096+80 B stack probe, no network/Flash/boot selection.
Host regression confirms old scanner skips accumulated prefix bytes and loses
multi-chunk marker carry. The local uploader is deliberately unchanged pending
defensive tests and independent review. This is not a physical failure report.
Independent reviewer/coordinator found no helper blockers. Local UBSan passes
2741363 stream checks, including a real six-MiB fixture and SIZE_MAX refusal,
all splits/marker positions/one-byte chunks, metadata copy and reset/crypto/OOM.
Native/config (265), UI (2237/96) and isolated review (2635/24) pass.
Target CI, exact artifact integrity and hardware remain separate gates.
[Contract, tests and remaining integration gates](docs/OTA_IMAGE_STREAM_PL.md).

PR #38 source c9dd6f7: exact-source CI 5/5,
[run 38848223833](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38848223833);
downloaded artifact CRC/SHA/chip/version/source/segments/license verified.
App SHA256 c529fee3a8bb44ab3fcb30063e8ee07922ea2727d6a9c39aac06008a7a16f67e.
Production trust store still empty; no complete OTA or hardware acceptance.

## 0.7.11 — signed OTA manifest verifier (#24), not installer

Real schema-v2 Ed25519 verification with strict bounded flat JSON, canonical
signed bytes, pinned board/layout/channel/size/hash/URL policy and zero output
on every failure. Production trust store is empty; no RFC/test anchor ships.
Monocypher 4.0.3 bytes/license pinned and checked; no handwritten curve crypto.
Backend only: no network, Flash or UI claim of complete OTA. Local Web upload,
P0 SPIFFS image and partition/NVS configuration are unchanged.
Local UBSan: RFC KAT/mutations 2315 checks; independent Python cryptography
117-case corpus passes 1494 host-trust and 1329 production-empty checks,
including 32 allocation failures per binary. Exact-source target CI, link
audit and hardware timing/stack remain separate gates.
[Contract and open updater work](docs/OTA_MANIFEST_PL.md).

## 0.7.10 — dedicated global Settings (#6, existing #24/#25 scope)

Six directly reachable sections separate system preferences from the HOME
pencil. Appearance uses fresh shared JSON/CAS with dirty/pending guards and
preserves later edits B after ACK A. Standalone WWW shows actual address,
readiness and fingerprint; explicit code generation and accepted local OTA
consent expire visibly. System memory/Flash is read-only hardware information.
Native signed OTA, LCD restart, session revocation and export/reset remain
explicitly unavailable, not implemented by this UI stage.
Local LVGL: 2237 checks / 96 frames; 40 WWW/System cycles preserve heap
(99704 → 99704 B). Existing isolated review: 2635 checks / 24 frames.
Production permission helper: 22 checks; system snapshot: 30. Exact-source
f7fb16d passed CI 5/5 [run 38843761384](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38843761384),
artifact CRC/SHA/chip/version/source/segments verified. Hardware remains open.
[Scope, remaining backend work and hardware gate](docs/GLOBAL_SETTINGS_PL.md).

## 0.7.9 — persistent display settings (#6/#24)

Independent existing-backlog stage: brightness and idle timeout use existing
settings NVS and the existing worker, preserving current Wi-Fi fields. No new
schema or SPIFFS dependency. Explicit errors distinguish persistence from PWM
application; restart/wake must retain brightness. P0 SPIFFS and full signed
OTA/global-settings acceptance remain open.
Local UBSan: native suite including 3425 worker and 167 startup checks; UI
1734 checks / 88 frames, review 2635 / 24 frames. Source 2641ecd passed CI 5/5
[run 38839617735](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38839617735);
downloaded firmware CRC/SHA/segments/version/source verified; hardware pending.
[Scope and physical acceptance](docs/DISPLAY_SETTINGS_PL.md).

## 0.7.8 — independent on-demand BAT voltage (#6/#25)

First-stage battery work follows the owner-approved existing backlog while PR #33
awaits hardware. GPIO8 ADC1, median and IDF calibration run through the existing
worker without Wi-Fi/workspace; UI exposes voltage/age/errors, no invented charge
state or percent. No startup/periodic sampling or new task/timer. Host ADC/worker
3367 checks and native UI regressions pass. Exact-source 37dd187 CI passed 5/5
[run 38838860433](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38838860433), including actual target build and full sanitizers; hardware remains open. Percent, low battery, runtime and charging remain open.
[Scope and physical acceptance](docs/BATTERY_VOLTAGE_PL.md).


## 0.7.7 — single-pass blank SPIFFS provisioning (#28 / PR #33)

New reviewer evidence supersedes the earlier pending clean test: the owner
reports physical **0.7.6 clean-install FAIL**. The review describes a private
dump with exactly 313/992 valid block markers, the remaining blocks erased and
no files; the raw dump was not accessed here. Physical logs confirm the pencil
hit/callback and the workspace-unavailable diagnostic. The event that stopped
the first format (watchdog/reset/power/I/O) remains unproven.

IDF's esp_spiffs_format on an unmounted blank partition formats in init and
then again. 0.7.7 replaces that with register-with-format only after every byte
is confirmed 0xff. The actual IDF/core test now requires **exactly 992 sector
erases**, not just successful mount. Nonempty/partial data remain protected.
During this synchronous startup-only operation the caller shares idle priority
under IDF's preemptive time slicing, then restores its prior priority on success
or error. Blank scan yields every 256 KiB. Existing Flash-driver yield defaults
are retained and checked in CI. No watchdog disable/feed/timeout increase or
new task/stack. Logs show checked/total bytes, format start/finish with elapsed
time/result/pass completion, mount_ready, workspace_ready and https_ready.
Public IDF API has no per-sector progress callback; no invented live count.

Local native/config/UI/review regressions pass (boot/storage now 16040 checks).
The real IDF test passes 131789 checks: one format, ten remounts, priority policy,
I/O failures, synthetic 313-marker prefix and actual formatter interrupted after
313 sectors, followed by byte preservation, plus the generator image. Host
timing/priority mocks do not establish physical watchdog health. The test-only
d3b9472 passed CI 5/5; firmware fix 81e734d also passed exact-source CI 5/5
[run 38835176199](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38835176199). Physical acceptance remains open.
No repeated full erase, private dump publication, main merge or Stable.
[Current acceptance and evidence](docs/PR33_ACCEPTANCE.md).

The following 0.7.6 entries are historical checkpoints, superseded above.

## PR #33 verification follow-up — owner testing clean 0.7.6

The owner has authorized a one-time full erase of their own board and is testing
source `ecdac4ee0d3332a4ee1a4ad4f0d3df664b1d4818`. Its exact-source CI
[run 38831522391](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38831522391)
was independently checked: all five jobs succeeded. Physical acceptance remains
open; no repeated erase is proposed. The earlier backup prerequisite below does
not block this explicitly authorized clean test. Unknown existing data on any
other installation still must be preserved.

A test-only follow-up now runs real `dsr_storage.c` with original pinned IDF
5.5.4 mount/format wrapper and SPIFFS core on simulated NOR. Blank provisioning,
file persistence over ten remounts, nonempty-byte preservation, I/O failures and
the upstream-generated image pass locally with UBSan. The documented upstream
unaligned-index exception is limited to dependency code. Native CI repeats with
ASan/UBSan/LSan; its new exact-head result is separate from the green ecdac4e run.
Firmware source/version/partitions and the owner's test binary are unchanged.
Local existing native/config/UI/review regressions also pass. No actual Flash,
RTOS, RF or physical pass is inferred. [Evidence, queue and remaining acceptance](docs/PR33_ACCEPTANCE.md).

## 0.7.6 — storage blocker diagnostics and read-only recovery (#28 / PR #33)

The owner's physical 0.7.4 log confirms nonempty, unmountable dsrdata with
SPIFFS_ERR_NOT_A_FS (-10025). Workspace never starts; silent editor returns
made Home pencil and all Add room actions appear dead, and HTTPS was skipped.
0.7.4 **failed physical acceptance**. The raw partition cause/content is still
unknown. The boot log does not establish the exact source commit.

This correction stays in existing Draft PR #33. Failed snapshot, parse,
validation, baseline/object allocation now show a translated error with Back
and read-only diagnostics. Workspace APIs fail closed until config task
startup succeeds, including partial mutex/queue/task failure. No stale draft
survives a failed open. Boot storage/workspace/HTTPS codes remain accessible
without Flash; USB logs click/callback/snapshot/parse/baseline and memory counts,
never JSON/SSID/credentials. Pairing requires a ready HTTPS server; no false PIN.
No driver, LCD memory, single-Shelly transport, security or partition changes.

A read-only USB helper backs up the partition table, private NVS and dsrdata
with duplicate reads, profile/MD5 validation and SHA-256. Offline analysis can
extract complete indexed SPIFFS file candidates and verify DSR checkpoint CRC.
It does not mount/repair/write Flash, guess that unknown data is worthless,
or authorize erasure. No owner dump was available here. Actual recovery and
the physical Add/Save/restart/HTTPS acceptance remain blocked on private backup
analysis and, only if needed later, explicit approval of a concrete operation.
[Backup, diagnostic and hardware procedure](docs/STORAGE_RECOVERY_PL.md).

Local UBSan: 1627 integrated LVGL checks / 73 frames, including EN/DE diagnostic
captures, 2635 review / 24 frames, core 94/platform 847/Wi-Fi 222/
BLE 2138/config 265/boot-storage 15948/startup 163. Seven actual workspace
startup scenarios cover unavailable Flash, bad checkpoints, read-only save,
mutex/queue/task OOM and snapshot OOM; nine offline tests cover backup,
geometry/length magic, complete/long indexes, torn/ambiguous pages and CRC.
A fixed synthetic image from the actual IDF 5.5.4 generator independently
checks slash-prefixed VFS names and exact multi-index file recovery.
Exact-source target/sanitizer/browser CI and real LVGL diagnostics 320×480
are required; no physical pass or main merge is claimed.

The in-progress #20 implementation is preserved separately on the local
multi-device branch (registry, multi-peer queue, durable binding transaction
and native regression). It is not shipped by this repair, not accepted on hardware and
does not bypass the dependency blocker. PR #34 remains blocked by storage too.

## 0.7.4 — contextual room editor (Issue #28)

Branch `work/rooms-context-editor`, stacked on draft PR #31. Rooms now has a
labelled Add room action in its toolbar and empty state; its bottom + has the
same context. Each room card has a separate 44×44 edit target, and the room's
device view has Edit room. Other pages retain the device wizard. Editing opens
the existing shared configuration by persistent room ID, with name, icon and
selection of stored photos. File upload remains in the authenticated Web Panel.
There is no second configuration model or schema/partition migration.

The focused form reuses the existing immutable async submission, CAS revision
and durable acknowledgement. Pending saves prevent premature Back; unsaved
changes require explicit discard. Errors retain the draft, and superseding Web
changes cannot be reported as successful LCD saves or trap Back indefinitely.
Deletion requires confirmation followed by Save. Device and dashboard references
block deletion; devices/scenes/binding are never silently removed or unassigned.
Existing dual-checkpoint boot-only photo GC is retained. Published rooms are
re-read on return, so LCD, LAN wizard, dashboard editor and Web share the result
without restart. PL/EN/DE and the existing two-column photo cards are retained.

Local UBSan: 1476 integrated LVGL checks / 64 frames, plus 2635 review checks /
24 frames. Pointer regression covers empty Add, two rooms, IDs after Web reorder,
UTF-8 length, icon/photo selection, async failure/retry, immutable A vs later B,
concurrent Web publication, discard/cancel, deletion/reference preservation,
limit 8, LAN/dashboard selectors and other-page +. Forty edit/Back cycles at
eight rooms retain the exact warmed LVGL free heap (66872 bytes; largest block
66768 in the host fixture). Core 94/platform 847/Wi-Fi 222/BLE 2138/config 265,
boot/storage 15943 and startup 163 checks pass. Actual 320×480 software-rendered
frames are exported to the CI UI artifact; populated host-fixture captures carry
TEST / DEMO. Full target build/sanitizer/browser CI apply to the exact PR head.
[Hardware procedure and limits](docs/ROOM_EDITOR_PL.md). No physical acceptance
of this editor or merge into main is claimed.

### Owner's physical feedback — 2026-10-16 (Europe/Vienna)

The owner reports that their board now boots and BLE searching works. The photos
show a real scan with ten results and a received advertiser. The installed exact
version/SHA was not supplied with this confirmation; it is not an exact-head
acceptance of 0.7.3/0.7.4, forced-fault recovery, OTA health or rollback. Record
boot and BLE reception as observed, with pairing/provisioning still absent.
Repeated coexistence, LAN/relay, HTTPS, persistence and fault tests stay open.
Private photos and advertiser identifiers are not included in this repository.

## 0.7.3 — controlled startup diagnostics after PR #30 review

Branch `work/fix-startup-recovery`, stacked on PR #30, not a replacement for
the owner's ongoing 0.7.2 physical test. The review correctly identified that
the post-task `ESP_ERROR_CHECK` could repeat the abort/reset class despite the
SRAM fix. Critical returned NVS/board/worker/UI-task/backlight errors now latch
DIAGNOSTIC and return normally from app_main; no new automatic reset, retry,
rollback selection, format or erase. USB serial records stage/error and heap.
This does not intercept SDK assertions, panics, watchdog or power failures.

The LVGL owner waits on an atomic startup-ready callback, so a task created
before a backlight failure cannot process hidden input/timers or auto-confirm
OTA. Local admin API/pair/OTA approval reject incomplete/diagnostic startup.
OTA health confirmation separately checks readiness, propagates confirmation
failure for retry, and writes validity only for a pending image after the
existing UI/storage/panel/touch health delay. No OTA data write on a USB image
with no selection record. Existing auth and local-only restrictions remain.

Local UBSan: 163 checks execute actual app_main plus the actual boot/OTA guard
under injected UI NO_MEM/INVALID_STATE, backlight, worker, NVS and board errors,
subsequent explicit boot and OTA state/write failures. The real static task
loop is tested before readiness and after gate closure; boot/storage regression
now totals 15943 checks. Core 94/platform 847/Wi-Fi 222/BLE 2138/config 265
remain passing. These are host fault fixtures, not physical recovery/rollback
evidence. Integrated LVGL 1176 checks/47 frames and review 2635 checks/24 frames
also pass locally, with unchanged host heap across repeated navigation/BLE/
cover cycles. Full target/ASan/browser CI follows on the exact head.

The 128 KiB PSRAM LVGL pool, internal 8192 B stack/TCB, DMA/pins/drivers,
transports, OTA slots and settings layout remain preserved. No visible UI
feature change; Issue #28 (contextual room CRUD) is next after this blocking
safety patch. [Policy, evidence and hardware test](docs/STARTUP_RECOVERY_PL.md).
No main merge, Stable or physical-pass claim.

## 0.7.2 — boot SRAM regression fix; physical acceptance pending

Branch `work/fix-071-boot-memory`, stacked on draft PR #27. The user confirmed
that 0.6.3 boots on the same ES3C35P and 0.7.1 does not (2026-10-16,
Europe/Vienna). Their USB log establishes the fatal failure, not an offset guess:
`dsr_board_start_ui_task()` returns `ESP_ERR_NO_MEM`, then `ESP_ERROR_CHECK`
aborts before the backlight is enabled. Flash=16 MiB, PSRAM=8 MiB/test OK,
ST77922 and five-point touch initialization all precede the failure.
**0.7.1 failed physical boot and must not be treated as accepted firmware.**

The 128 KiB built-in LVGL pool is now linked into PSRAM with an exact-object
linker fragment. Its size and allocator stay unchanged. The 8192-byte UI stack
and TCB are statically reserved in internal BSS before optional services can
consume their space; no external task stack or reduced UI stack is used.
Target CI must check actual ELF symbols/section bounds, not just sdkconfig.
Startup logs report internal free/largest and PSRAM at each service boundary.
LCD/touch/QSPI/PWM pins, BLE scanner, relay/LAN transport and OTA partitions are
retained. No extra UI features or visual redesign are included in this repair.

The same log also reports SPIFFS mount failure. That is a separate observation,
not proof of corruption or a reason to erase. Mounting now retains the real
provisioning/read error, never formats on allocation/state errors, and provisions
only a fully read, entirely erased data partition. Regression fixtures exercise
static UI creation and storage failure paths (15938 local UBSan checks pass).
Core 94, platform 847, Wi-Fi 222, BLE 2138 and config 265 checks also pass locally.
Full target/sanitizer/browser CI
and the new ELF check follow on the exact commit; **no physical pass is claimed**.
See [BOOT_MEMORY_FIX_PL.md](docs/BOOT_MEMORY_FIX_PL.md) for recovery and acceptance.

## 0.7.1 — feature audit and working dashboard information (Issue #25)

Branch `work/issue-25-feature-audit`, stacked on BLE PR #26. The versioned
[FEATURE_MATRIX.md](docs/FEATURE_MATRIX.md) traces visible LCD/Web interactions
through workers/API/state, lists supported types, implementation/test status,
issues and required hardware evidence. It explicitly separates configuration
entries, one active relay, BLE discovery, pairing, sensor data and true Cover.
The audit does not mark the unfinished global quality gate complete.

The previously inert toolbar Info now opens actual dashboard/device/room/online
counts with a close action and explicit DEMO label in test models. Current tabs
without callbacks are static selection indicators, no longer inert touch targets.
Other unavailable features retain disabled affordances and their tracked issues.
Local UBSan passes 2635 review checks/24 frames and 1176 integrated UI checks/47
frames. Pointer/language/raster checks cover open/close and live update of the
Info action. Drivers, transports,
OTA, NVS and BLE scanner are unchanged by this isolated audit patch. Target CI
and physical acceptance remain separate; no main merge or stable publication.

BLE PR #26 (`c4ca872`) completed 5/5 CI jobs in
[run 38807134240](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38807134240):
actual ES3C35P firmware, effective NimBLE observer/PSRAM/coexistence configuration,
native ASan/UBSan, LVGL and 28 browser checks. Physical RF remains pending.

## 0.7.0 — real NimBLE scanner, first BLE milestone (Issue #18 / #7B)

Branch `work/issue-18-ble-scan`, stacked on verified PR #17. Actual ESP-NimBLE
legacy active discovery runs for <=20 s in a separate manager/host task, with
software coexistence, cancel, retry, generation fencing and a 16-row cap. LCD
and authenticated local HTTPS use the same bounded snapshot. RF/GATT callbacks
never touch LVGL. NimBLE starts lazily and retains a fixed host baseline after
completion; there is no perpetual scan or repeated init/deinit churn.

Manufacturer/UUID evidence separates Shelly-advertised RPC, known Shelly BLU
and unverified BTHome/other advertisements. Optional manufacturer model IDs use
the documented table. Names never establish authenticated identity or generation.
Generic BTHome is not mislabelled Shelly. Scan response merges by address/type;
malformed/truncated AD, stale generations and overflow are bounded. MACs and
secrets are not logged or returned by the new authenticated scan API.

This is discovery only: **BLE bonding, Wi-Fi provisioning, persistent BLU sensors
and BLE RPC control are not implemented**. Production results come only from the
NimBLE radio; host radio providers are test-only and renders have TEST / DEMO.
Local UBSan passes: 2138 BLE parser/adapter checks (including 2000 deterministic
AD inputs), 1176 integrated LVGL checks/47 frames, 2615 review checks/23 frames.
BLE UI heap is 100352 -> 100352 B after ten scan/cancel/exit cycles; host figures
are not ESP32/PSRAM measurements. Core 94, platform 847, Wi-Fi 222 and config 265
regressions remain retained. Real 320x480 frames accompany the PR.
Target, sanitizer and browser CI follows on the exact source commit. Physical
RF, coexistence, touch/FPS, heap/PSRAM and >=10 cycles remain unverified; the user
must supply an actual board result before acceptance. [Full scope and test](docs/BLE_PLAN.md).
Existing LCD/touch/backlight, Wi-Fi, relay HTTP/auth/NVS and OTA partitions are
preserved. No merge/main/stable or physical-test claim.

## 0.6.3 — stable background reads and one deferred user command (Issue #12)

Branch `work/issue-12-stable-polling`, stacked on cover PR #16. A periodic PROBE
retains confirmed state/metrics and enabled ON/OFF. Unchanged samples update
only freshness text, preserving card objects and scroll. An accepted tap during
a read queues one explicit target ON/OFF, disables duplicates, verifies the same
device/host/generation/channel after the read and dispatches before config jobs.
Read/auth/Wi-Fi/dispatch failures cancel the intent; SET is never auto-replayed.
The confirmed output changes only after mandatory SET/readback. Manual Refresh
can join an existing read. Physical drivers, relay transport and NVS are retained.

Local UBSan: 1101 integrated LVGL checks/38 frames and 2615 review checks/23
frames pass, including two stable 5/10 s polls, pointer tap during a read, one
deferred SET, double tap, auth failure, Wi-Fi loss, dispatch rejection and sample
age. Repeated UI and cover preview cycles retain the same host heap. Tests and
renders inject clearly marked TEST/DEMO transport events, not physical readings.
[Exact regression and physical test](docs/STABLE_POLLING.md). Target/browser/ASan
CI runs on this draft; physical regression remains pending. No merge/stable.

PR #16 (`41137b6`, cover view 0.6.2) completed all five CI jobs successfully:
[run 38800101997](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38800101997).
This includes the actual ES3C35P firmware and host ASan/browser checks. It does
not establish a physical Cover transport test or visual acceptance.

## 0.6.2 — Shelly-inspired vertical cover details (Issue #15)

Branch `work/issue-15-cover-details`, stacked on PR #14. The existing cover view
now uses a name/edit header, compact scrolling position/W/V/Hz chips, a dominant
vertical blue opening indicator, explicit percentage, 148x64 Open/Close targets
and a separate 64x64 Stop. Unknown/offline/failed samples hide stale position and
measurements. Opening, closing, stopped and pending command are separate model
states; background reads do not replace the cover status. PL/EN/DE are covered.

The production menu includes an explicit isolated Cover DEMO preview for visual
review on ES3C35P. Preview controls never invoke network/edit callbacks or persist
fake data; current live updates survive exiting the preview. Ordinary production
models still reject injected demo flags. Real cover commands require capabilities
and callbacks, which the current relay adapter does not provide. Cover transport,
calibration and physical cover control are NOT implemented by this visual PR.
The informational indicator never sends GoToPosition.

Native regression/raster tests cover 0/65/100%, unknown/offline/motion/error,
metrics overflow, PL/EN/DE, actual pointer geometry, transport gating, preview
isolation and 80 repeated preview/control/exit cycles. Local UBSan passes: 2615
review checks/23 frames (cover cycle heap 89368 -> 89368), 1039 integrated UI
checks/35 frames, 94 core, 847 HTTP/NVS, 222 Wi-Fi and 265 configuration checks.
Target/ASan/browser checks
run in the new draft PR; physical ES3C35P visual, heap/PSRAM and regression tests
remain pending. [Comparison, exact renders and physical test](docs/ISSUE15_COVER.md).
Drivers, relay HTTP/NVS schema, discovery, OTA partitions and main are unchanged.
The separate HOME polling/queued SET correction requested in Issue #12 remains
the next functional stage. No merge/stable or visual acceptance is claimed.

## 0.6.1 — optional V/A and visible command outcomes (Issue #12)

Branch `work/issue-12-telemetry-errors`, a small isolated change on UI PR #13.
Real Gen2+/Gen3 Switch.GetStatus voltage/current are parsed only when present,
numeric, finite and within a generous display bound (absolute value <= 1e9;
V/A nonnegative). Missing fields remain unavailable, not zero. Invalid optional
types reject the sample. Volatile telemetry is excluded from an explicit exact
schema-1 NVS record, preserving existing stored bindings and passwords.

Authenticated read/write identity checks, request transport and no-replay policy
are retained. Cards/details show auth, unreachable, identity, response/state and
storage errors; failed-command notices remain until explicit Refresh or another
successful command. Refresh only reads state, never replays SET. Pending reads
and pending commands have different labels. Worker logs transaction outcomes
and availability flags without IP, identity or credentials.

Local UBSan: 847 HTTP/parser/NVS checks, plus the retained 94 core and 222 Wi-Fi
checks. 1026 native UI checks/34 frames and 2496 review checks pass. Tests drive
V/A, auth failure, timeout, failed-command latching,
explicit read-only refresh and old-format NVS persistence. Renders of these
injected event samples carry TEST / DEMO; they are not physical measurements.
[Exact Gen3 hardware regression](docs/ISSUE12_TELEMETRY.md). Target/ASan/browser
CI belongs to the new draft PR. Physical V/A/error tests remain pending.
Previous user-reported ON/OFF success remains recorded; no main merge.

## 0.6.0 — shared UI shell and readable keyboard (Issues #6 / #8)

Branch `work/issue-6-8-unified-ui`, stacked on the retained 0.5.1 PR #11.
Five equally spaced navigation targets replace the misleading sparkle shortcut;
scene icons/editor functionality remain available. Device parameter backgrounds
are measured from the actual translated text, with padding and a width cap.
HOME and native Wi-Fi/Shelly/settings/editor screens share header, graphite
button/card tokens, fonts and active colors. Forms scroll the focused field
above the keyboard; native password inputs remain masked with a show toggle.

The shared 240px keyboard uses 22px Roboto glyphs, QWERTY (PL/EN), QWERTZ (DE),
all printable ASCII, separate PL/DE diacritics and numeric/IP mode. The bottom
row is 123 / cursor left / compact space / cursor right / localized Done.
Actual LVGL pointer tests cover insertion/deletion inside hidden UTF-8 text,
cursor limits, all ASCII keys, numeric mode, dimensions and single submission.
Local UBSan: 986 integrated UI checks/29 frames and 2496 design-review checks
(including 576 icon raster cases) pass. Target/browser/ASan CI follows in the PR.
Real screenshots cover the complete Wi-Fi/Shelly flow and settings/editor.
[Visual comparison and hardware regression procedure](docs/UNIFIED_UI.md).

User-reported physical results received 2026-10-15 via the independent reviewer:
Wi-Fi scan, three real Gen2/Gen3 LAN results, binding a Shelly 1PM Mini Gen3
(`S3SW-001P8EU`), roughly 4.0–4.1 W and explicit ON/OFF control work. This is
evidence for the previously delivered build, not a controlled test of 0.6.0.
Private footage, SSIDs and addresses are not published. Visual acceptance is
still open; new hardware regression is pending. Communication/drivers/OTA are
unchanged. Optional real V/A and visible command errors follow separately in
Issue #12. No main merge or stable release is authorized.

## PR #5 review fixes — durable acknowledgement and photo reclamation

Branch `work/config-save-photo-fixes`, stacked on PR #10. Native and browser
editors retain an immutable submitted snapshot A and preserve edits B made
while A is being saved. Only a matching, published durable document confirms
A; B stays visibly unsaved and can be submitted against the new revision.
Failure keeps the draft. A second Save while pending does not lose tracking.
Native messages, tile size and accent choices use PL/EN/DE; Polish glyphs and
symbol fallback are covered by the Issue #7 font correction.

At boot, the verified current document refreshes the older checkpoint. Then
unused content-addressed photos are removed before the immutable photo cache
or LVGL is created. Both retained checkpoint references are protected; any
remaining corrupt/future checkpoint disables deletion. Live LVGL photo pointers
are never freed. Save removals and restart to reclaim Flash/cache slots.

Local UBSan: 265 configuration checks (real checkpoint files/GC), 378 integrated
LVGL checks, 94 core, 545 HTTP/parser/NVS and 222 Wi-Fi checks pass. Browser
regression edits B during delayed A acknowledgement. Target CI is required for
this commit. [Acceptance procedure and limits](docs/REVIEW_FIXES.md).
No physical ESP32 validation or main merge is claimed.

## Issue #7 A — real LAN discovery and wizard

Dedicated branch `work/issue-7-lan-discovery`, stacked on the visual PR #9.
Portable identity/capability/deduplication/subnet policy and real mDNS + passive
ARP/known-host probing are implemented in a separate bounded network task.
HTTP identity reads carry no credentials; protected component inspection uses
the retained Basic/Digest transport and checks the selected ID/MAC again.
545 HTTP/parser/NVS checks, 94 core and 222 Wi-Fi checks pass with UBSan.
Touch wizard and authenticated panel API now use this service. Users select a
verified device, authenticate, choose an actual switch channel, name and room;
the old binding is retained on a failed probe. DHCP rebind updates the prior
configuration entry for the same saved physical ID. Unsupported covers/sensors
are identified but cannot be falsely bound as a relay. AP/BLE stay disabled.
Native LVGL: 362 checks and 19 frames including no-Wi-Fi, discovery, auth error,
nonconsecutive channels and room binding. The service checkpoint `efe4db2` passed
all five jobs in run 38748844770. Integrated UI/API `8a83628` passed all five
jobs in [run 38752429842](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38752429842).
Browser onboarding tests use explicit radio fixtures plus the actual C parser;
they do not establish physical LAN operation. [Physical procedure](docs/ISSUE7_LAN.md).

## Issue #6 — rounded icon family and configurable photographic HOME

Follow-up: native labels/forms/dropdowns use the licensed Roboto font explicitly,
including Polish/German glyphs and LVGL keyboard-symbol fallback. 2400 review
checks (13 glyph checks added), 330 integrated checks and real native frames
pass with UBSan. This correction is also present in the stacked LAN branch.

2026-10-15: branch `work/issue-6-visual-system`, based on PR #5's reviewed
`5f6a653`. Source version `0.4.1-ui-review`. Dedicated draft PR; no main merge.
Replaced the diagnostic line/arc icon renderer with 24 Apache-2.0 Material
Symbols Rounded masks, filtered at build time and during LVGL scaling. Added
44px toolbar/tab targets, graphite tokens, room photo headers and a dashboard
picker. Device cards remain horizontal; four photographic room cards fit two
columns on HOME. Added a distinct cover detail view and PL/EN/DE strings for
Wi-Fi, manual onboarding, diagnostics and editor forms.

13 real review frames include both HOME variants, cover/light details, offline
and EN/DE. Licensed CC-BY-2.0 test photos are kept outside firmware. Native
review and integrated navigation tests pass; target CI evidence belongs to the
dedicated PR. Physical appearance/photograph persistence is **pending user test**,
not established by these host renders. [Exact test procedure](docs/ISSUE6_UI.md).
The save race/photo reclamation review items remain separate tracked fixes;
AP/BLE are explicitly disabled, LAN discovery follows in Issue #7.

## Current: shared configuration and local panel review

2026-10-15, `0.4.0-config-dev`, branch `work/config-web-panel`, draft PR #5
(base `ui/shelly-smart-control`). No merge to `main` is authorized or performed.

Implemented: full-width typed device cards; shared schema/revisions; dashboard,
room, device and scene editors on touch and web; drag ordering on web; appearance;
durable two-checkpoint configuration; photo upload/crop; real LVGL preview;
physical pairing, per-device TLS identity, local authenticated administration;
operator-selected OTA upload with inactive-slot validation and boot confirmation.
Original LCD/touch/backlight and Shelly request/transport modules are retained.

All five CI jobs passed for source `5d309013bdfeb64b628b2c1c2c7e6475561fe126`
in [run 38737249365](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38737249365):
ESP32-S3 board package, both diagnostic builds, native sanitizers and real
LVGL/browser tests. See [current validation](docs/CONFIG_VALIDATION.md).

Limits: one actual relay connection; no new cover/sensor transport or scene
executor; partial PL/EN/DE translation; no publisher-signed OTA release updater.
Physical HTTPS, Flash interruption, OTA/rollback and long-run memory tests remain
outstanding. [Operation and installation](docs/CONFIG_WEB_PL.md).

The following sections are historical checkpoints, not the current feature list.

## Active branch: UI review, awaiting user approval

2026-10-15: development priority changed to a closer visual/interaction match to Shelly Smart Control. `reference/0.2.0-local-dev` preserves the working baseline; `work/wifi-onboarding` preserves pending Wi-Fi work; `ui/shelly-smart-control` isolates the UI proposal. Source version here is `0.3.0-ui-review`, not a public release. Do not merge or expand further screens before approval.

Five core views, shared styles/original icons/Roboto fonts, and actual LVGL RGB565 rendering are implemented. Eight 320×480 review states include clearly marked DEMO data and a genuinely empty real-mode view. Transport and board code are unchanged from the frozen Wi-Fi checkpoint. Review source/data separation and no-demo-command behavior are tested; target compilation is tracked separately. Details: [UI review](docs/ui-review/README.md).

2026-10-15 icon review v2: fixed border-aware alignment of circular power,
thumbnail, toolbar and navigation icons, balanced the original glyph geometry,
and added a ninth actual LVGL frame showing all 18 icons. Native raster checks
cover 432 size/border combinations, with visible glyph centres within 1px of
their containers. The user's additional PL/EN/DE language selector and OTA
installer requests are recorded as pending, not working features. No merge,
stable release, firmware-install request or physical UI result is implied.

The historical record below describes the previously delivered 0.2.0 firmware; it is not a claim that the new UI has passed physical testing.

Checkpoint: 2026-10-15, Europe/Vienna. Current source version: `0.2.0-local-dev`.
Repository: https://github.com/voltex-at/DIY-Shelly-Remote

## Physical evidence received

The user reported “Działa” and supplied two photographs of the actual ES3C35P running the previously supplied `0.1.0-bringup` image. The Home screen is readable in native portrait and reports `Touch controller: responding`. The touch-test photograph shows `Corner 4 detected` and `Touch count: 4`.

This confirms physical boot/display rendering and touch responses. At 02:55 Europe/Vienna on 2026-10-15, the user additionally confirmed working colors, brightness adjustment and wake in `0.1.0-bringup`, before installing 0.2.0. These are user-reported physical test results, without measured color accuracy, brightness values or wake timing. Complete four-corner mapping, first-touch action suppression, long-duration operation, battery, Wi-Fi and Shelly control remain separate checks. No photos were copied into the public repository.

## Implementation and verification

| Area | Current implementation | Evidence / outstanding test |
| --- | --- | --- |
| Portable requests, registry, versions, OTA metadata policy | Implemented | 94 assertions; prior CI ASan/UBSan passed |
| ES3C35P hardware profile and ST77922 LCD/touch | Implemented | Previous full board CI passed; user boot/touch photos received |
| Test UI, colors/corners/brightness/off/wake | Implemented; tests retained in Menu | Readable UI and touch responses photographed; colors, brightness and wake user-confirmed on 0.1.0; full corner mapping/stability pending |
| On-device Wi-Fi entry and restart | Implemented | All four new CI jobs passed; physical Wi-Fi test pending |
| One saved manual IPv4 switch/channel | Implemented | Real cJSON + fake HTTP fixtures; actual Shelly test pending |
| Gen1 Basic / Gen2+ Digest authentication | ESP-IDF transport configured; both auth features enabled | Configuration/challenge-body fixture and board compilation passed; actual protected device test pending |
| Identity before read/write, explicit ON/OFF/readback, five-second polling | Implemented | Identity mismatch blocks command in fixture; physical network recovery pending |
| NVS settings and device binding | Validated schema; save after successful probe; credentials local | Fake-NVS adapter tests; persistence on actual board pending; NVS unencrypted |
| Multiple devices, rooms/scenes, energy history | Not implemented | Future milestone |
| BLE / BTHome / BLE RPC | Not implemented | Future milestone |
| OTA installation/signatures/health-confirmed rollback | No installer; metadata policy and dual-slot layout only | Future milestone |

Current local validation: **94 portable + 371 platform checks** passed both normally and with UBSan. Platform tests compile the real request/HTTP/connection/settings code and use real cJSON with mocked HTTP and NVS APIs. They check malformed/oversized JSON, authentication errors/429, challenge-body reset, explicit command/readback, identity mismatch, no command replay and invalid NVS schemas. They do not test IDF Digest cryptography, actual flash commits, Wi-Fi or radio coexistence. Full ASan/UBSan, both diagnostic builds and ES3C35P build/package passed in [run 38662049616](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38662049616). No compiler warnings/errors were reported by the ES3C35P build. The license packager retained 63 dependency notices plus its index.

## Hardware and artifact history

Profile: LCDWiki ES3C35P, ESP32-S3 N16R8, 16 MiB QSPI Flash, 8 MiB OPI PSRAM, ST77922 320x480 QSPI display and integrated I2C touch. GPIOs are manufacturer-sourced. Exact-board upstream initialization is attributed to its MIT source. Runtime checks enforce expected memory sizes. The display/touch port is preserved from the working bring-up.

PR #1 foundation passed [run 38657250244](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38657250244). PR #2 board bring-up passed all four jobs in [run 38658803879](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38658803879). The prior [0.1.0 artifact](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38658803879/artifacts/11938610486) was downloaded and its ZIP/BIN checksums, image metadata, Flash addresses and version verified. Exact values remain in [VALIDATION.md](docs/VALIDATION.md).

## Current limits and next test

Use [the Polish local-control guide](docs/LOCAL_CONTROL_PL.md) for the new development firmware; [the board reference](boards/lcdwiki-es3c35p/README.md) records hardware and original bring-up. No stable Release is claimed.

- One stored relay/switch channel, manual IPv4 only. No discovery, automatic DHCP address relocation, capability enumeration or notifications. Reserve the Shelly IP in DHCP; a different saved identity blocks control until explicitly added again.
- Local HTTP: Gen1 Basic has no transport confidentiality; Gen2+ uses IDF Digest SHA-256. No redirects, credentials in URLs/source/logs, or cloud dependency. Auth failure/429/identity mismatch pauses automatic polls; explicit Refresh or re-add resumes a user-directed attempt.
- Wi-Fi start is once per boot. The explicit Save & restart button applies a new network after NVS success. Reconnect uses 1–32 second backoff. Network operations run in a bounded worker; no LVGL call runs in that worker.
- NVS is not encrypted. Schema migration/production secure storage remain pending. NVS initialization/schema errors do not automatically erase settings. Brightness changes are temporary; screen off remains 60 seconds.
- Merged USB installation overwrites covered Flash contents including settings; it is not settings-preserving OTA. Both app slots are 6 MiB but installation and post-OTA health confirmation are not implemented.
- No battery, charging, RGB LED, deep sleep, audio, SD or pogo accessory behavior is claimed.

Next physical milestone: configure Wi-Fi on the screen, add an actual authenticated Shelly switch, verify ON/OFF and external state changes, reboot persistence and recovery after disconnection. Record exact Shelly model, firmware, generation, channel and auth mode without passwords. Colors, brightness and wake were confirmed on 0.1.0; check them for regressions after installing 0.2.0.

## Verified 0.2.0 development artifact

[PR #3](https://github.com/voltex-at/DIY-Shelly-Remote/pull/3) implements the one-switch milestone. Source commit `425682dadf7992e068afddf395f9527ae62e98e6`; tested PR merge `6fdbd9d604ca98cacdc234fa7ee7424f8007b874`. All four jobs passed in [run 38662049616](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38662049616). The actual [artifact 11939312624](https://github.com/voltex-at/DIY-Shelly-Remote/actions/runs/38662049616/artifacts/11939312624) was downloaded and its ZIP CRC/digest, both BIN hashes, ESP32-S3 image header/version, embedded app at 0x20000, Flash metadata and cJSON/lwIP notices were verified. Merged USB BIN: 1245776 bytes, flash at **0x0**. App: 1114704 bytes with 82% of the 6 MiB slot remaining. Exact hashes are in [VALIDATION.md](docs/VALIDATION.md). This is development firmware awaiting actual Wi-Fi/Shelly testing, not a stable Release.

Latest user checkpoint: 2026-10-15 02:55 Europe/Vienna. The user confirmed colors/brightness/wake in 0.1.0 and planned to install 0.2.0 next. No 0.2.0 physical result has been reported yet. OTA installation is intentionally unavailable in both versions; the 0.2.0 package is installed over USB.
