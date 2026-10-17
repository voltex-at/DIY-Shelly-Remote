# Niezależny język Web Panelu — 0.7.22

Istniejące #5/#6, [zlecenie właściciela z16.10.2026](https://github.com/voltex-at/DIY-Shelly-Remote/pull/5#issuecomment-6214082384).
To locale strony WWW, NIE nowe ustawienie firmware ani nowy endpoint.

## Kontrakt

- Widoczny wybór Polski/English/Deutsch w prawym górnym nagłówku, również
  przed parowaniem i po401. Na telefonie bez poziomego scrolla i zasłaniania akcji.
- Preferencja wyłącznie lokalna `dsr.web.locale`; wspierany język przeglądarki
  lubPL przy braku preferencji. Niedostępny storage nie może zatrzymać panelu.
  `document.documentElement.lang` i dostępne etykiety zmieniają się razem zUI.
- Pełne PL/EN/DE dla stałych i dynamicznych tekstów panelu: nawigacja, parowanie,
  statusy/błędy, formularze, preview, zdjęcia, LAN/BLE, Save/CAS i dev OTA.
  Nazwy użytkownika, SSID, adresy, firmware/rawpreview nie są tłumaczone.
- Zmiana języka nie uruchamia `changed`, `save`, `load`, destrukcyjnego `render`
  ani zapytań API. Nie zmienia `config.language`, revision, dirty/conflict,
  aktywnej podstrony, parowania ani modelu pilota. Wartości i tożsamość pól,
  focus/selection, file inputs, hasła i kod parowania pozostają zachowane.
- Wygląd rozróżnia `Język pilota` (wspólny workspace, wymaga Save) od
  `Język Web Panelu` (ta przeglądarka, bez Save). Dwie przeglądarki niezależne.

Bez zmian backendu, NVS, auth, cookie/tokenów, partycji, SPIFFS, native OTA lub
dozwolonych kanałów/wydawcy. Przełącznik nie wykonuje tłumaczenia obrazu LVGL;
preview nadal pokazuje rzeczywisty język pilota. Zmiana locale nie jest dowodem
pełnego audytu TLS/parowania/OTA.

## Weryfikacja

Audit rzeczywistego słownika wykonuje237wpisów, wykrywa duplikaty przed
zbudowaniem Map, sprawdza PL/EN/DE i zgodność placeholderów oraz wszystkie
literalne klucze JS/HTML. Root uruchomił też negatywne mutacje: duplicate,
missingkey, emptytranslation i utrata placeholdera odrzucane bez zmian źródła.
Znane obecne błędy backendu i standardowe network errors są zmapowane;
nieznane przyszłe/techniczne diagnostyki pozostają raw, bez zgadywania danych.

Przegląd produkcji i niezależne testy Playwright używają rzeczywistych shipped
HTML/JS/CSS. API/radio/reconnect/reboot w fixture są jawnie MODEL; istniejący
test panelu z C persistence/LVGL pozostaje osobno. Raport wyników, renderów,
exact-source CI oraz rzeczywistego artefaktu w opisie Draft PR. Brak fizycznego
odbioru tego locale do czasu potwierdzenia właściciela.

Fizyczny retest bez erase: otworzyć panel przed parowaniem, wybrać trzy języki,
sparować bez utraty kodu, zmienić nazwę testowego pokoju bez Save i przełączyć
locale — szkic/focus pozostają. Zapisać, odświeżyć, sprawdzić locale tej
przeglądarki i niezmieniony język pilota. W drugiej przeglądarce wybrać inne
locale. Przy konflikcie WWW↔LCD zmiana locale nie odblokowuje Save ani nie
nadpisuje danych. Telefon: nagłówek/selektor klikalne, brak poziomego scrolla.
Nie publikować prywatnych SSID/MAC/IP/cookie/certyfikatów ani zdjęć.

P0 osobno: na0.7.18/preformatowanym SPIFFS właściciel potwierdził pokoje po
resecie i działający Web; blank-only format nadal nieodebrany. PR48 poprawia
Save-return/Delete i wymaga własnego fizycznego retestu. Nie kasować/przywracać
dsrdata ponownie, nie scalać/main/auto-merge/Stable bez osobnej zgody.
