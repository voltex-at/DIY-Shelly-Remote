# Window Heating Automation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Wdrożyć zatwierdzoną lokalną regułę okno/drzwi → ogrzewanie z zachowaniem ręcznych zmian i wznowieniem po świeżych odczytach.

**Architecture:** Przenośny model emituje pojedyncze efekty: zapis dziennika, odczyt TRV lub nastawę. Istniejący właściciel floty realizuje je przez zadanie konfiguracji i aktora/workera; LCD/WWW edytują tę samą definicję przez CAS. Każda fizyczna komenda wymaga aktualnego powiązania, danych źródłowych i właściwego potwierdzenia trwałego zapisu.

**Tech Stack:** C11, ESP-IDF 5.5.4, FreeRTOS, cJSON, NVS, LVGL 9.3.0, istniejący panel JS, Python/Cygwin host, Chromium.

**Spec:** [Zatwierdzony projekt](../specs/2026-10-23-window-heating-design.md), akceptacja właściciela w rozmowie 2026-10-23.

## Global Constraints

- ESP-IDF 5.5.4, LVGL 9.3.0, ST77922 2.1.1, mDNS 1.8.2, ES3C35P 320×480.
- LVGL host commit `c033a98afddd65aaafeebea625382a94020fe4a7`.
- Dokładnie Cyber Blue, Grafit, Jasny; polski/angielski/niemiecki.
- Maksymalnie 8 reguł, 1..16 kontaktów i 1..16 celów; cel tylko w jednej regule.
- `target_half`: 8..60, domyślnie 16; żadnego zaokrąglania nieobsługiwanej nastawy bazowej.
- Pełny JSON ≤24576 B, rekord odzyskiwania ≤3072 B, 16 prywatnych tożsamości.
- `dsr_job_t=560`, `dsr_event_t=1192`: asercji nie podnosimy dla wygody.
- Świeżość transportu BLU 45 s; raport kontaktu ≤120 s i późniejszy od bariery wznowienia.
- TRV: bezpośredni odczyt `BluTrv.Call` → `TRV.GetStatus`, bez fallbacku na lustrzany `target_C`.
- Nowe odczyty wszystkich uczestników po restarcie/utracie łączności; brak UTC blokuje automat.
- Brak dodatkowego zadania RTOS, zmian partycji, beacon bez użytkownika, kluczy OTA, flashowania, push i publikacji.
- Zachować obecny zaakceptowany WIP i aktywną gałąź. Nigdy nie dołączać `.local/`, `.agents/`, lokalnego `AGENTS.md`, `tools/dev/` ani `referenz/`.
- Tylko zestaw związany z etapem; nowe rendery ograniczone do dotkniętych ekranów. Pełne CI, atlas i build przed DEV są osobnymi końcowymi bramkami.

## Review Focus

1. Nowy pakiet baterii nie może uwiarygodnić starego kontaktu ani nastawy TRV — testy task 3.
2. Ręczny zamiar o tej samej wartości podczas PREPARED/in-flight/UNCERTAIN musi przetrwać reboot — testy task 1/2/5.
3. Stary ACK dziennika lub workera nie może zatwierdzić nowszej intencji po zmianie kolejności — testy task 1/2/5.
4. Ponownie wykorzystane ID karty, zmieniona bramka lub zmiana definicji nie mogą przejąć dawnej nastawy — testy task 1/4/5.
5. Zmiana strony/klawiatura/zapis CAS podczas ładowania statusów nie może gubić formularza ani uruchamiać reguły — testy task 6/7.

## Wspólne wykonanie i dowody

Runner nowego zakresu: `tools/test_automation.py --case model|store|http|config|runtime|all`, opcje `--sanitize` i `--ubsan-only`; bez domyślnego wywoływania całego `tools/test.py`. Używa `tools/hostcc.py` i istniejących fixture'ów. Na Windows uruchamiamy go przez lokalny Cygwin z `CC=tools/dev/cygwin-cc`, `LIBRARY_PATH`/`C_INCLUDE_PATH` wskazującymi `.test-deps`. Niedostępny UBSan w tym Cygwin nie jest zaliczeniem sanitizerów; te muszą przejść w obsługującym środowisku przed DEV.

Każdy task: mała regresja RED → implementacja → podany zestaw GREEN → selektywny lokalny commit. Logi/obrazy tylko `.local/` i istniejące ignorowane `build-ui/`. Dowody i ewentualne odstępstwa zapisujemy na bieżąco; nie powtarzamy testów po samej akceptacji. Pre-existing zmiany współdzielonych plików trzeba rozpoznać przed stagingiem; checkpoint zaakceptowanego WIP może objąć wyłącznie jawnie wybrane źródła/testy/dokumenty, bez plików środowiska. Żaden task nie upoważnia do push.

### Task 1: Przenośny model cyklu i efektów

**Files:** create `components/dsr_platform/include/dsr_automation_types.h`, `components/dsr_platform/dsr_automation_model.h`, `dsr_automation_model.c`, `tests/test_automation_model.c`, `tools/test_automation.py`.

**Interfaces:**
- Define the shared `dsr_auto_rules_t`, `dsr_auto_observation_t`, `dsr_auto_effect_t`, `dsr_auto_record_t`, `dsr_auto_snapshot_t` in `include/dsr_automation_types.h` so workspace/runtime/UI signatures share one definition; full executor state `dsr_auto_model_t` stays in the private model header. Header visibility does not authorize exporting private records through HTTP. Rules use stable layout IDs and exact private participant keys; observations distinguish contact source time from direct-TRV-read evidence. Effect kinds are `DSR_AUTO_EFFECT_NONE/JOURNAL/READ_TRV/SET_TRV` and carry generation/cycle/intent IDs plus `layout_id` and `target_half` where relevant.
- `void dsr_auto_init(dsr_auto_model_t *m);`
- `dsr_result_t dsr_auto_configure(dsr_auto_model_t *m,const dsr_auto_rules_t *rules);`
- `void dsr_auto_observe(dsr_auto_model_t *m,const dsr_auto_observation_t *observation);`
- `void dsr_auto_pause(dsr_auto_model_t *m,uint32_t now_ms,uint32_t utc);`
- `bool dsr_auto_manual(dsr_auto_model_t *m,uint32_t layout_id,uint32_t intent_id);` false means durable gate not ACKed yet.
- `bool dsr_auto_next(dsr_auto_model_t *m,uint32_t now_ms,uint32_t utc,dsr_auto_effect_t *out);`
- `void dsr_auto_complete(dsr_auto_model_t *m,const dsr_auto_effect_t *effect,dsr_result_t result);` ignores stale effect IDs.

- [x] RED: `first_open_all_closed`, `per_target_baselines`, `no_raise_already_lower`, `manual_during_prepared_inflight_uncertain`, `stale_effect_ack`, `fresh_epoch_all_participants`, `unsupported_baseline`, `binding_reuse`.
  Fixture assertions: `CHECK(effect.kind==DSR_AUTO_EFFECT_JOURNAL); CHECK(writes==0);` before durable ACK; `CHECK(effect.target_half==16);` for 8°C setback; per-target restore `CHECK(restored[0]==40 && restored[1]==44);` for baselines20/22°C; `CHECK(restored_count==0);` after both targets acquire manual overrides.
- [x] Run `python3 tools/test_automation.py --case model`; observe the expected missing cycle/guard behavior before implementing it.
- [x] Implement one effect at a time, per-target baseline/override masks, readback separate from ACK, uncertain writes reconciled only by reads. No I/O, dynamic task, LVGL, credentials, clock restoration or optimistic telemetry in model.
- [x] Run the complete `--case model` set; assert bounded structs and no duplicate SET after unchanged observations or duplicate completion. Commit only task sources/tests/runner.

### Task 2: Trwały rekord i asynchroniczne ACK

**Files:** create `components/dsr_platform/dsr_automation_store.c`, `tests/test_automation_store.c`, fixture NVS under `tests/automation/`; modify `dsr_workspace.c`, `include/dsr_workspace.h`, `CMakeLists.txt`, runner.

**Interfaces:** consumes task 1 record/effect IDs.
- `esp_err_t dsr_auto_store_load(dsr_auto_record_t *out,bool *found);`
- `esp_err_t dsr_auto_store_save(const dsr_auto_record_t *record);`
- `bool dsr_workspace_auto_submit(const dsr_auto_record_t *record,uint32_t generation);`
- `bool dsr_workspace_auto_take_result(uint32_t *generation,esp_err_t *result);`

- [x] RED: serialize/CRC/version/size checks, alternating records, partial set/commit/readback, newer corrupt/future record, queue saturation, older ACK after manual override, reboot after each persisted phase.
  Assert `CHECK(encoded_size<=3072); CHECK(dsr_auto_store_load(&loaded,&found)!=ESP_OK);` when a present newer record is corrupt; `CHECK(ack_generation==requested_generation);` only for the actual completed request. A failed save must not publish success in the ACK mailbox.
- [x] Run `python3 tools/test_automation.py --case store`; verify failure before adding NVS durability/admission.
- [x] Implement explicit versioned encoding ≤3072 B, `dsr_auto` namespace, two records, monotonic sequence, commit and reopened readback. No automatic erase/fallback through a suspect newer record. One shared participant table ≤16; never credential structs.
- [x] Extend existing config-task queue with a tagged journal request and its own ACK mailbox. Journal writes do not change workspace revision or complete a config save. Buffers belong to the queued request until ACK; no 3 KB stack blob and no I/O under LVGL/actor locks.
- [x] Run `--case store` including all fault boundaries and no-replay recovery; commit task files and only related workspace hunks.

### Task 3: Dowody świeżości kontaktu i aktywny odczyt TRV

**Files:** modify `dsr_discovery_model.c`, `include/dsr_discovery.h`, `dsr_http.c`, `include/dsr_platform.h`, `dsr_worker.c`; add focused cases in `tests/test_automation_http.c`, runner.

**Interfaces:**
- `dsr_result_t dsr_http_trv_read(dsr_connection_t *connection);` forwards only read-only `TRV.GetStatus{id:0}` via `BluTrv.Call` after existing identity verification.
- Append internal `DSR_JOB_TRV_READ`; job/event sizes remain unchanged. Correlated event ID and operation kind prove a direct read in this process; normal PROBE never supplies that proof.
- For contact telemetry, `blu_updated_s` carries the actual contact value's source time; missing/conflicting source metadata is UNKNOWN for automation. Direct TRV observation uses its matched read effect token, independently of mirrored fleet values.

- [x] RED: fractional `last_update_ts`/`last_updated_ts`, absent/future/stale timestamp, newer battery-only packet with old contact, duplicate/conflicting contact reports, stale mirrored target despite recent BLE packet, direct read wrong ID/unsupported/malformed/timeout/rebind.
  Assert `CHECK(telemetry.blu_updated_s==1706593991u);` for contact value timestamp1706593991.91 despite newer battery metadata; `CHECK(set_target_calls==0);` for every direct-read case; read proof absent for ordinary mirrored PROBE.
- [x] Run `python3 tools/test_automation.py --case http` and observe missing proof/parser behavior.
- [x] Implement bounded parsing and read-only transport using existing auth/guards. Never call `TRV.SetTarget` from the read helper or classify the read POST as a write attempt. Respect existing rate-limit policy/timeouts; no cache fallback.
- [x] Update job-kind bounds/providers when appending enum; retain 560/1192-byte ABI assertions. Run `--case http` plus relevant existing BLU parsing/request case selected from its runner. Commit only task files/hunks.

### Task 4: Schema 3 i zgodna migracja wszystkich odbiorców

**Files:** modify `components/dsr_config/dsr_config.c`, `include/dsr_config.h`, `dsr_workspace.c`, `components/dsr_platform/web/actions.js`, runtime/UI config consumers; add `tests/test_automation_config.c`; update valid current-schema fixture documents deliberately, preserving frozen schema1/2 fixture paths.

**Interfaces:** consumes task 1 rules.
- `bool dsr_auto_rules_parse(const cJSON *root,dsr_auto_rules_t *out,char error[96]);` in new `dsr_automation_config.c`; presentation names remain config metadata, executor never trusts display kind.
- Public canonical schema is 3 with `automations`; stored 1/2 remain independently strict. `dsr_config_upgrade()` adds empty array in RAM and preserves revision/content; durable upgrade only explicit save.

- [x] RED: 1→3/2→3, existing manual target action preserved, no revision change, idempotent reload, duplicate keys/IDs/targets, empty arrays, invalid half-values, JSON limit, deleting referenced device, schema3 accepted by web actions consumer.
  Assert `CHECK(DSR_CONFIG_SCHEMA==3); CHECK(dsr_config_revision(upgraded)==7); CHECK(cJSON_GetArraySize(automations)==0);` for a stored revision7 schema2 document; action value44 stays44 and an enabled rule with target value7 is rejected.
- [x] Run `python3 tools/test_automation.py --case config`.
- [x] Implement validator/migration and rule parser; reject target reuse across rules and impossible references. Removing a referenced device updates rule references and disables/removes no data silently: reject deletion until references are explicitly resolved, as for existing guarded config entities.
- [x] Audit every literal schema2 consumer and current test fixture. Do not mass-rewrite historical/frozen schema tests. New private enrollment must compare actual bindings, not grant control from `kind=sensor`.
- [x] Run `--case config` plus the relevant config/workspace compatibility cases, then commit only task files/hunks.

### Task 5: Integracja aktora, manual override i wznowienie

**Files:** create `components/dsr_platform/dsr_automation.c`, `include/dsr_automation.h`, `components/dsr_ui/dsr_ui_network_automation.inc`; modify `dsr_actions*`, `dsr_workspace.c`, `dsr_ui_network.c`, `dsr_panel.c`, activity renderers/serializers; add `tests/test_automation_runtime.c` and runner.

**Interfaces:** consumes model/store/direct-read/config APIs from tasks 1–4.
- `esp_err_t dsr_automation_init(void);`
- `void dsr_automation_tick(dsr_fleet_t *fleet,uint32_t now_ms,uint32_t utc,bool online);` called by sole existing network owner; may queue but never perform I/O.
- `bool dsr_automation_manual_ready(uint32_t layout_id,uint32_t intent_id);` all LCD/WWW/scene/group TRV routes use it before physical admission; pending requests keep exact identity/payload/operation ID.
- `bool dsr_automation_config_allowed(const cJSON *candidate,char error[96]);` freezes participant/setback changes in active cycles; allows disabling/name changes.
- `dsr_result_t dsr_actions_auto_request(uint32_t rule_id,uint32_t cycle,uint32_t generation,const uint32_t *ids,const uint8_t *halves,size_t count,uint32_t *run_id);` trusted internal plan, at most16 target steps. New explicit source/kind in public activity; no public JSON origin override.
- `void dsr_automation_snapshot(dsr_auto_snapshot_t *out);` read-only copied status per rule/target; no journal/private identities in public snapshot.

- [x] RED: end-to-end open→JOURNAL ACK→READ→SET→actual readback→allclosed→per-target restore; no SET before durability/proofs; reboot each boundary; manual override every phase and failed persistence; stale ACK, manual groups/scenes, rebind, rule-disable, no active-type conflict, FIFO/fairness with normal work and STOP.
  Assert the captured worker sequence and exact layout IDs, not source-text strings: first direct READ and only then one SET16; closing one of two contacts sends zero restores; allclosed restores40/44 only to unmodified targets. Failed override commit gives `CHECK(manual_writes==0); CHECK(result==DSR_IO);` and a visible failure rather than a success ACK.
- [x] Run `python3 tools/test_automation.py --case runtime` with actual actor/worker/workspace code and explicit radio/NVS/clock providers.
- [x] Wire effects through existing actor and config task. Manual priority cancels unsent auto steps; pending automatic transaction still owns its exact ACK/lease. Persist override for PREPARED/queued/in-flight/uncertain/confirmed cycles before admitting manual write; failure is explicit non-success with retained request draft.
- [x] Gate initial enable/private enrollment, rule epoch, all participants' source proofs and direct TRV-read proof before each write; final worker guard rechecks cached trusted generation/override/identity. No nested workspace/actor mutex calls. Default disabled rules do nothing.
- [x] Reconcile journal after restart only after fresh epoch witnesses; never replay write queues/nonces. External changed target becomes override, own delayed cache does not. Missing/corrupt journal pauses with cause; normal restart resumes without confirmation.
- [x] Run `--case runtime`, `--case model`, `--case store` and directly affected old actor/worker regression; commit implementation/integration with passing evidence.

### Task 6: LCD formularz i czytelne stany reguły

**Files:** create `components/dsr_ui/dsr_ui_automation.c`; modify UI `CMakeLists.txt`, `dsr_ui_internal.h`, `dsr_ui_actions.c`, `dsr_ui_style.c`; add `tests/ui/test_automation_ui.c` and register it in `tools/test_ui.py`.

**Interfaces:** `void dsr_ui_automation_open(void);`, `void dsr_ui_automation_tick(void);`; consume schema3 workspace CAS and `dsr_automation_snapshot`, use verified typed devices for contact/TRV selectors.

- [x] RED: new rule disabled by default, default8°C, contacts/targets selected by actual capability, duplicates rejected, preserve form on failed save/CAS, no command while editing, explicit enable, active-cycle structural edit block, disable/delete semantics, long names/keyboard/back navigation and stale status update after leave.
  Assert `CHECK(find_label(lv_screen_active(),"8.0 °C")); CHECK(set_calls==before_edit);` and actual button boxes ≥44px with no footer/keyboard collision. Inspect the same new selected states rather than unrelated existing cards.
- [x] Run `python3 tools/test_ui.py --jobs 4 --case test_automation_ui` using Windows adapter/pinned LVGL as documented.
- [x] Reuse accepted scene/group controls and three skins: entry from Sceny, no new bottom-nav item or seventh Settings row; forms keep fixed actions outside scrolling. Gold important messages, double chamfered Cyber frames.
- [x] Show Disabled/Waiting data/Active/Manual change/Waiting confirmation/Error and missing participant cause. Explain waiting for new contact reports/beacon without changing device settings automatically. Distinguish requested/readback/baseline temperatures.
- [x] Run only new UI case, save representative changed frames (three skins and PL/EN/DE text checks, not a full atlas), inspect actual pixels, commit UI/tests after correction.

### Task 7: WWW bez osobnego modelu reguł

**Files:** modify `web/panel.js`, `web/panel.css`, `dsr_panel.c`, platform `CMakeLists.txt`; create `dsr_automation_api.c`, `tests/test_panel_automation.cjs`; extend relevant host API runner.

**Interfaces:** `char *dsr_automation_status_json(void);` supplies authenticated read-only `GET /api/v1/automations/status`. Existing config PUT carries definitions and existing CAS/save ACK; no automation execute POST.

- [x] RED: shared schema/data, typed capability filtering, same validation/default, failed save/retry, disabled/uncertain/override statuses, stale async response after page/config/session change, locale change while field focused, HTML in device name treated as text.
  Assert persisted rule `target_half===16`, `enabled===false` until explicitly selected, and zero command POSTs while editing/saving definitions. Status response from an old revision/session must leave current field value and focus intact.
- [x] Run `node tests/test_panel_automation.cjs` with existing local Playwright/Chromium providers.
- [x] Reuse current panel helpers/auth and numeric rule/device IDs. No journal/credentials/private enrollment export. Expose current configuration revision so read-only status from a different revision does not overwrite form state.
- [x] Run focused web test and API auth/shape checks; inspect representative mobile-width form/status screenshot and commit only task files/hunks.

### Task 8: Zintegrowany odbiór etapu i granice wydania

**Files:** update `docs/PROJECT_STATUS_PL.md`, docs schema/development references, plan checkboxes and `.github/workflows/ci.yml` only to register new mandatory tests, not weaken existing gates.

- [x] Run `python3 tools/test_automation.py --case all` once for final integrated source, required affected actor/config/BLU regressions, focused automation LCD/WWW, and documentation/whitespace checks.
- [x] Build ES3C35P at the end of this major stage; inspect stack/memory auditors, confirm exact pins and job/event/record budgets. Missing sanitizer environment remains a release blocker, not a passing result.
- [x] Perform one independent whole-change review with plan/spec and the recorded Review Focus; resolve important findings with focused RED→GREEN evidence. Preserve unrelated accepted WIP and root branch.
- [x] Present actual new LCD/WWW previews and a short list of host/target/hardware limits for the ongoing survey.
- [ ] Record owner acceptance of the new automation editor/status in #74 using a short task-only comment; preview shown, owner answer pending.
- [x] Do not publish DEV/Stable, merge to main, push, dispatch Actions or flash a device here. Final release CI and physical-device acceptance remain the separate gates already agreed with the owner.

## Plan self-review

Spec coverage: model/cycles task1, durable recovery task2, source evidence task3,
schema/migration task4, actor/manual admission task5, LCD task6, WWW task7,
validation/release boundaries task8. The five Review Focus cases are pinned to
named tests above. Signatures and shared types originate in the indicated task;
later tasks consume them rather than defining competing variants. Owner approved inline execution here. Tasks 1–7 and the task 8 integration,
target build and independent review/fix pass are complete locally. All five
Important findings have focused RED→GREEN evidence and a final relevant suite.
New automation previews await owner acceptance; physical acceptance and release
validation remain separate gates. Branch/version retained; no push or release.
