# Matryca funkcji — 0.7.41-dev

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
| Shelly BLU Motion/Door/Window przez bramkę Shelly (LAN, tylko odczyt) | tak (0.7.41) | tak (HTTP model, LCD, Web Panel) | brak testu | — |
| Grupy, sceny, dziennik aktywności | tak | tak | brak testu | — |
| Historia energii 24 h | tak | tak | brak testu | — |
| Skan BLE i dodawanie Gen2+ przez BLE | tak (0.7.39: rozpoznanie bez UUID RPC) | tak | nie działało (0.7.38), brak testu poprawki | — |
| OTA z Web Panelu (zgoda LCD, SHA-256) | tak | tak | brak testu | — |
| OTA z pilota, podpis `voltex-ota-2026-1` | tak (0.7.39: host CDN, katalog stronami; 0.7.40: rekordy TLS 16 KiB) | tak | FAIL 0.7.37 (przekierowanie CDN) i 0.7.39 (kod 16/4: I/O po TLS), brak testu 0.7.40 | — |
| Rollback po nieudanym starcie | tak (SDK) | model | brak testu | — |
| Napięcie baterii na żądanie | tak | tak | „?” w nagłówku = brak pomiaru z ostatnich 30 s; pomiar fizyczny niewykonany | — |
| Język: EN domyślny dla nowych instalacji, nazwy użytkownika bez tłumaczenia | tak | tak | brak testu | — |
| Wydanie: jeden build na tag, bez nadpisywania, bramka CI 5/5, Stable z akceptacją | tak (`release.yml`, `tools/release_tool.py`) | tak (decyzje) | nie dotyczy; pierwsze uruchomienie przy dev-v0.7.40 | — |

Poza zakresem tej wersji: Stable, zmiana tablicy partycji, Gen1, sterowanie H&T,
historia Light/RGB/Cover.
