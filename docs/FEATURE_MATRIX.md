# Matryca funkcji — 0.7.54-dev

Kolumny: **Kod** (zaimplementowane na `work/p0-spiffs-ota-brand`), **CI** (testy
hostowe/LVGL/target dla dokładnego SHA), **ES3C35P** (fizyczne potwierdzenie
właściciela, z wersją), **Odbiór** (akceptacja właściciela). Zielone CI nie jest
testem fizycznym. Opis szczegółowy do 0.7.33: [history/FEATURE_MATRIX_0.7.33.md](history/FEATURE_MATRIX_0.7.33.md).

| Obszar | Kod | CI | ES3C35P | Odbiór |
| --- | --- | --- | --- | --- |
| Start, wyświetlacz, dotyk, podświetlenie | tak | tak | potwierdzone (0.1.0) | — |
| Ekran startowy z logo, styl Grafitowy/Nocny/Jasny | tak | tak (kontrast ≥4:1) | brak testu | — |
| Pierwsze formatowanie SPIFFS z postępem na LCD | tak | tak (IDF 5.5.4 SPIFFS, NOR model) | formatowanie zaliczone (0.7.37) | — |
| Wznowienie przerwanego formatowania (109/313 bloków) | tak | tak | brak testu | — |
| Wi-Fi, jeden Shelly ON/OFF (1PM Mini Gen3) | tak | tak | potwierdzone (0.6.x) | — |
| Wiele Shelly (do 16), trwały rejestr, ponowne dodanie | tak | tak | błędy zgłoszone w 0.7.23, poprawione w kodzie, brak ponownego testu | — |
| Pomieszczenia, pulpity, edytor LCD i Web Panel | tak | tak (układ PL/EN/DE) | ucięty tekst w edytorze (0.7.37), poprawione, brak ponownego testu | — |
| Rolety (Cover Gen2+) | tak | tak | sterowanie potwierdzone (0.7.37) | — |
| Light/Dimmer Gen2+, RGBW, H&T Gen3 | tak | tak | brak testu | — |
| Termostat BLU TRV przez bramkę (odczyt, cel 4-30 °C, łuk i pastylka jak w Shelly) | tak (0.7.47; 0.7.48 wygląd) | tak (HTTP model, kolejka, LCD, Web Panel odczyt) | brak testu | — |
| Wbudowane zdjęcia pomieszczeń (8) | tak (0.7.47) | tak (LCD) | brak testu | — |
| Shelly BLU Motion/Door/Window przez bramkę Shelly (LAN, tylko odczyt) | tak (0.7.42; 0.7.45: lista z GetComponents) | tak (HTTP model, LCD, Web Panel) | 0.7.44: BLU Gateway Gen3 bez kanałów (GetStatus bez BTHome), poprawione w 0.7.45, brak testu | — |
| Grupy i sceny: bezpośredni kreator LCD/WWW, karty, pokoje i zdjęcia | tak (0.7.54; ręczne sceny, grupy Switch; automatyczne warunki i pozostałe typy grup poza zakresem) | host/LVGL; bramka CI 5/5 przed wydaniem | nowy kreator: brak testu | — |
| Zużycie: zegar mocy na stronie urządzenia | tak (0.7.49) | tak | brak testu | — |
| Historia energii 24 h, strona Energia, wykres WWW | usunięte w 0.7.51 (decyzja właściciela 2026-10-18; pliki w SPIFFS nietknięte) | — | — | — |
| Ikony urządzeń do wyboru (20), zegar w pasku stanu, pasek przewijania | tak (0.7.51) | tak (LCD, schemat) | brak testu | — |
| Skan BLE i dodawanie Gen2+ przez BLE | tak (0.7.39: rozpoznanie bez UUID RPC) | tak | nie działało (0.7.38), brak testu poprawki | — |
| OTA z Web Panelu (zgoda LCD, SHA-256) | tak | tak | brak testu | — |
| OTA z pilota, podpis `voltex-ota-2026-1` | tak (0.7.39: host CDN, katalog stronami; 0.7.40: rekordy TLS 16 KiB; 0.7.43: limit pobierania obrazu, kasowanie sekwencyjne) | tak | instalacja 0.7.53-dev udana — potwierdzenie właściciela 2026-10-18; rollback bez testu | — |
| Rollback po nieudanym starcie | tak (SDK); zatwierdzenie pending wymaga gotowych SPIFFS/workspace również wewnątrz API | model (fazy/błędy magazynu, UI odzyskiwania dostępne bez zatwierdzenia) | brak testu | — |
| Napięcie baterii na żądanie (dolny pasek: diagnostyka baterii zamiast Energia; 0.7.52: RAM, minimum, pamięć grafiki, stos ekranu, stan BLE) | tak | tak | „?” w nagłówku = brak pomiaru z ostatnich 30 s; pomiar fizyczny niewykonany | — |
| Język: EN domyślny dla nowych instalacji, nazwy użytkownika bez tłumaczenia | tak | tak | brak testu | — |
| Wydanie: jeden build na tag, bez nadpisywania, bramka CI 5/5, Stable z akceptacją | tak (`release.yml`, `tools/release_tool.py`) | tak (decyzje) | nie dotyczy; pierwsze uruchomienie przy dev-v0.7.40 | — |

| Nazwa zamiast SKU w kreatorze, na kartach i w Web Panelu (decyzja 2026-10-18) | tak | tak (UTF-8, wybór etykiety) | odbiór LCD/WWW na sprzęcie do wykonania | — |
| Pole modelu w Settings wypełniane z `model` (zamiast „Modell nicht ausgelesen”) | tak (model, firmware, generacja) | tak (parser, tożsamość) | odbiór na sprzęcie do wykonania | — |
| Synchronizacja nazwy z Shelly, wariant B — nazwa lokalna ma pierwszeństwo (decyzja 2026-10-18) | tak | tak (odczyt, błędy i trwałość) | odbiór na sprzęcie do wykonania | — |
| Rozpoznawanie typu po `profile` i komponentach, wszystkie kanały (decyzja 2026-10-18) | tak | tak (profile i Cover 0/1) | fizyczny Pro Dual Cover do odbioru | — |

### Decyzje właściciela 2026-10-18 (szczegóły do powyższych wierszy)

1. **Nazwa zamiast SKU.** Kolejność wyświetlania: nazwa lokalna → `device.name` z `Sys.GetConfig`
   Shelly → przyjazna nazwa modelu → `id`. SKU jako druga linia. Przy duplikacie
   lub braku nazwy dopisek końcówki MAC (np. „Lampa · 2b84”). Klucz wiązania
   pozostaje `id`/MAC; nazwa nie trafia do nazw plików SPIFFS. Sprawdzić znaki
   PL/DE w czcionce LVGL i skracanie długich nazw.
2. **Synchronizacja nazwy — wariant B.** Osobno przechowywane: nazwa lokalna i
   ostatnio odczytana nazwa z Shelly (odświeżana w tle zawsze). Nazwa lokalna ma
   pierwszeństwo. „Karte bearbeiten” ma opcję „Użyj nazwy z Shelly”, która czyści
   nazwę lokalną. Pilot nie zapisuje nazwy w Shelly (bez `Sys.SetConfig`).
   Odbiór: zmiana nazwy w aplikacji Shelly widoczna na LCD i w WWW po kolejnym
   odświeżeniu statusu (karty bez nazwy lokalnej); nazwa lokalna zachowana;
   reset przywraca synchronizację; pusta `name` → nazwa modelu; brak duplikatów
   i utraty pokoju, kanału i historii; stan zachowany po restarcie.
3. **Rozpoznawanie typu.** `profile` (tylko urządzenia wieloprofilowe) określa
   tryb; lista komponentów (`switch:N`, `cover:N`, `light:N`, `rgb:N`, `rgbw:N`,
   `temperature:N`, `humidity:N`…) jest źródłem prawdy; `app` tylko podpowiedź.
   Brak znanego komponentu → „nieobsługiwane”, nigdy zastępczy przekaźnik.
   Urządzenia wielokanałowe: zaproponować wszystkie kanały.
4. **Gen1 odrzucone** — brak urządzeń do testów.

Implementacja (1)–(3) w 0.7.53-dev. Odbiór fizyczny pozostaje przed nami.
SPIFFS: przyczyna dawnych przerwań potwierdzona 2026-10-18 — zewnętrzny reset przy braku komunikatu.
Pusta nazwa lokalna w schemacie 2 włącza synchronizację; przed downgrade do 0.7.52 lub starszego ustaw niepuste nazwy lokalne.

Poza zakresem tej wersji: Stable, zmiana tablicy partycji, Gen1 (decyzja
właściciela 2026-10-18: odrzucone), sterowanie H&T, historia Light/RGB/Cover.
