# SYS-02 — Aktywność LCD Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Dodać czytelny widok LCD istniejącej historii 128 działań od uruchomienia.

**Architecture:** Nowy widok LVGL czyta ten sam log `dsr_actions` co WWW. Nie dodaje
magazynu, taska ani drugiego silnika historii. Jest samodzielną częścią SYS-02;
backup i reset mają odrębny plan z uwagi na wspólną warstwę trwałości.

**Tech Stack:** C11, LVGL 9.3.0, ESP-IDF 5.5.4, istniejące testy hosta i LVGL.

**Spec:** [Zatwierdzony projekt SYS-02](../specs/2026-10-24-system-data-design.md).

## Global Constraints

- ES3C35P N16R8, ESP32-S3, 320×480; ESP-IDF 5.5.4, LVGL 9.3.0, ST77922 2.1.1, mDNS 1.8.2.
- Cyber Blue/Grafit/Jasny oraz PL/EN/DE; istniejące sfazowane podwójne ramki i złote ważne komunikaty.
- 128 wpisów RAM, rekord 32 B, backend strony maksymalnie 32; LCD pobiera do 6.
- UTC albo czas od uruchomienia, numeryczne ID; obserwacja nie jest ACK polecenia.
- Brak flash/NVS/SNTP, nowych zadań lub ujawniania tożsamości/sekretów.
- Zachować lokalne edycje i gałąź; bez push, flashowania, resetu lub wydania DEV.
- Metoda: implementacja tutaj, jedna niezależna recenzja na końcu SYS-02; plan do odbioru przed kodem.

## Review Focus

1. WWW czyści historię, gdy LCD ogląda starszą stronę: wrócić do bieżącej epoki bez starych obiektów (Task 1: `epoch_changed_while_open`).
2. Nowe wpisy zmieniają offset podczas przeglądania: odświeżenie jest jawne, numeracja i pozycja strony pozostają poprawne (Task 1: `append_and_wrap`).
3. Brak UTC i przepełnienie czasu monotonicznego: pokazać czas od startu bez fałszywej daty (Task 1: `unset_time`).
4. Anulowanie czyszczenia i nawigacja: niczego nie wyczyścić, callback nie dotyka usuniętego ekranu (Task 1: `cancel_and_leave`).
5. Długie niemieckie wyniki: przewijana treść i dostępne akcje bez kolizji z nawigacją (Task 1: `locale_geometry`).

## Pliki i granice

- Create `components/dsr_ui/dsr_ui_activity.c`: stan widoku, pobieranie strony, prezentacja i lokalne potwierdzenie czyszczenia.
- Modify `components/dsr_ui/dsr_ui.c`, `dsr_ui_internal.h`, `dsr_ui_network.c`, `dsr_ui_style.c`, `CMakeLists.txt`: wejście z System, cykl hide/tick, tłumaczenia i rejestracja.
- Create `tests/ui/test_activity_ui.c`; modify `tests/ui/test_ui.c`, `tools/test_ui.py`: nowy izolowany case produkcyjnego UI.
- Istniejący backend: `components/dsr_platform/include/dsr_actions.h`, `dsr_actions.c`, `dsr_actions_model.c`; bez zmiany formatu lub powiększenia logu.
- Wyniki w `.local/` / ignorowanym `build-ui/`; nie dodawać ich do Git.

### Task 1: Działający widok historii i bezpieczne czyszczenie

**Interfaces:**
- Consumes existing `bool dsr_actions_log(uint32_t epoch,unsigned offset,unsigned limit,dsr_action_log_page_t *page);` and `void dsr_actions_log_clear(void);`.
- Produces `void dsr_ui_activity_open(lv_event_t *event);`, `void dsr_ui_activity_hide(void);`, `void dsr_ui_activity_tick(void);` declared in `dsr_ui_internal.h`.
- Stan strony to epoka/offset i kopia do 6 rekordów, nigdy wskaźniki do żywego logu. Tick nie odświeża sam starszej strony po każdym wpisie.

- [x] **Step 1 — RED:** Dodać case `test_activity_ui` do runnera. Zasilić prawdziwy log produkcyjny rekordami przez istniejące fixture, bez zastępowania odczytu atrapą zwracającą stały sukces. Sprawdzić `empty`, `page_six`, `epoch_changed_while_open`, `append_and_wrap`, `unset_time`, `cancel_and_leave`, `locale_geometry`.
  Asercje: `CHECK(page.total==128); CHECK(page.count<=6); CHECK(observed_ack_labels==0); CHECK(clear_calls==0);` po anulowaniu; po potwierdzeniu `CHECK(log.total==0 && config_writes==0 && nvs_writes==0);`. Dla zmienionej epoki brak starych etykiet; po powrocie brak odwołań do usuniętych LVGL obiektów.
- [x] **Step 2 — RED run:** `python3 tools/dev/run_host.py test_ui.py --case test_activity_ui --jobs 4`; oczekiwany błąd brakującego widoku/zachowania, nie awaria środowiska.
- [x] **Step 3 — Implement:** Zbudować widok z `dsr_ui_screen` i obecnych helperów stylu. Nagłówek/status bar i nawigacja zachowują pozycje; lista ma własny viewport, a akcje są poza nim. Odczyt `epoch=0` przy otwarciu, starsze/nowsze po 6, przy konflikcie epoki jawne przeładowanie najnowszej strony. Obserwacje, wykonanie, błędy/niepewność i źródło odróżnione tekstem/ikoną; ważne błędy złote. Tylko lokalne potwierdzenie uruchamia clear; wyjście kasuje stan i callbacki. Nie przebudowywać zaakceptowanego menu głównych sześciu ustawień.
- [x] **Step 4 — GREEN:** Uruchomić tylko ten sam case; obejrzeć kadry pusty/log/wynik błędu/potwierdzenie w trzech skórkach i PL/EN/DE. Dla reprezentatywnych ramek sprawdzić brak kolizji, końcowe wiersze i odstęp od dolnej nawigacji. Oczekiwany `PASS` i exit 0. Drobne korekty powtarzają tylko dotknięty stan, bez atlasu i pełnego builda.
- [x] **Step 5 — Commit:** Selektywnie stage źródła/test/runner; lokalny commit `feat: show shared activity log on LCD`. Nie stage pre-existing fixtures schema1, środowiska ani renderów.

### Task 2: Odbiór widoku i zapis statusu

**Files:** `docs/PROJECT_STATUS_PL.md`, opis wyniku w #74; nie zmieniać zachowania backendu.

**Interfaces:** consumes task 1 screenshots and fresh focused command result; produces truthful SYS-02 activity acceptance status, separate from backup/reset.

- [x] Pokazać wybrane rzeczywiste kadry LVGL i zapytać w ankiecie o wygląd Aktywności. Nie powtarzać testów wyłącznie po odpowiedzi „zaakceptowane”.
- [x] Zapisać polecenie, wynik, zakres i ograniczenie: backend/LVGL hosta nie są odbiorem na urządzeniu. Sprawdzić `python3 -X utf8 tools/check_docs.py` i `git diff --check`, lokalnie commit dokumentacji. Sprzęt i końcowa integracja należą do drugiego planu; brak nowego DEV na tym etapie.
