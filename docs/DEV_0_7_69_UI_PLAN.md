# 0.7.69-dev — plan i kryteria odbioru UI/UX

Podstawa: zlecenie właściciela z 2026-10-22 i dziewięć zdjęć LCD.
Kontynuacja `work/dev-0.7.68`, baza `ec56dc4`; wcześniejsze poprawki pozostają.
Właściciel zlecił samodzielną realizację oraz publikację wyłącznie DEV po walidacji.

## Projekt

- Dokładnie Cyber Blue, Grafit, Jasny; identyfikatory danych bez zmian.
- Ekran 320×480, ESP-IDF 5.5.4, LVGL 9.3.0, bez zmian partycji i klucza OTA.
- Rytm: margines 12 px, odstęp 8 px, mały odstęp 4 px, cele dotykowe ≥44 px.
- Status: zegar, Wi-Fi i bateria w jednym rzędzie. Wartość wewnątrz pola
  baterii; szacunek nadal oznaczony `~`, nieznany stan nadal jawny.
- Dashboard: pełny wiersz wyboru nazwy oraz osobny rząd akcji. Zakładki
  odsunięte od krawędzi i statusu. Treść kończy się przed nawigacją.
- Karty: czytelna nazwa i pomiary blisko opisu; miejsce dla ikon,
  pokoju i dużej akcji. Kompaktowy termostat ma sterowanie po prawej, przyciski
  48 px i brak pozycji zaworu na karcie (pozostaje w szczegółach).
- Ustawienia: przewijana lista z końcowym odstępem; ikona, etykieta i strzałka
  mają oddzielne miejsca. Ostatnia pozycja jest w całości dostępna po przewinięciu.
- Dashboard = dom, pomieszczenia = grid, trzecia ikona = procesor we wszystkich skórkach.
- Animacja rolety, podgląd przeciągania, STOP i wygaszanie wspólne dla skórek;
  kolory pozostają właściwe danej skórce. Podgląd nie zastępuje odczytanego stanu.
- AP: rozwinięcie istniejącego aktora `dsr_ap` i UI `dsr_ui_ap`, bez drugiego
  backendu. Skan, pusta lista, wybór, potwierdzenie połączenia, osobna zgoda
  na przekazanie Wi-Fi, weryfikacja w LAN i zapis istniejącą ścieżką.
  Retry po błędzie nie powtarza zapisu Wi-Fi. Anulowanie przywraca sieć domową.

## Realizacja i walidacja

- [x] 1. Wspólne elementy: status, ustawienia, nawigacja, nazwy skórek.
  Pliki `dsr_ui_style.*`, `dsr_ui_battery.c`, `dsr_ui.c`, selektory LCD/WWW.
  Sprawdzenie: nowy skupiony przypadek LVGL i obrazy ustawień/statusu.
- [x] 2. Dashboard i karty: `dsr_ui_mobile.c`, pełne nazwy, pomiary, termostat,
  okno, długie teksty. Sprawdzenie: wybrane obrazy DE/PL i dotyk akcji.
- [x] 3. Rolety: istniejący renderer i obsługa gestów, paleta Grafit/Jasny,
  zgodność STOP i stanów nieznanych. Sprawdzenie: skupiony test rolety.
- [x] 4. AP: czytelne kroki i błędy, retry, anulowanie, wszystkie trzy skórki.
  Sprawdzenie: skupiony UI i istniejące testy AP/hand-off/bezpieczeństwa.
- [x] 5. Końcowy przegląd ekranów i języków, jeden pełny zestaw UI oraz build.
  Obejrzeć obrazy, nie utożsamiać liczby asercji z jakością obsługi.
- [ ] 6. Wersja 0.7.69-dev, dokumentacja, przegląd zmian, CI dokładnego SHA,
  tag `dev-v0.7.69`, podpisane wydanie, kontrola opublikowanych zasobów.

Po małej poprawce wyłącznie związany przypadek. Atlas dopiero po głównej
serii zmian. Długie logi w `.local/handoff-0.7.69/`. Odbiór fizycznego dotyku
i radia raportować oddzielnie; host nie potwierdza zachowania sprzętu.

## Szczególne ryzyka do sprawdzenia

1. Długie niemieckie nazwy i polskie znaki: pełna nazwa/wrapping, bez samych kropek.
2. 24 kafelki: zachować limit 128 KiB LVGL i zapas; nie zwiększać budżetu.
3. Polling podczas gestu i uśpienia: bez fałszywego stanu i zgubionego STOP.
4. Błąd AP, utrata Wi-Fi i anulowanie: odzyskanie sieci bez ponownego provisioningu.
5. Zmiana skórki/języka i powrót: bez wiszących timerów lub wskaźników LVGL.

## Doprecyzowania właściciela podczas oglądania renderów

Karty TRV/okna zmniejszone ponownie; usunięto powtórzoną ikonę TRV, zachowano
48 px przyciski. Przekaźniki podniesione o 6 px. Ikony kafelka pokoju rozdzielone.
Na rolecie bez ołówka, tytuł wyśrodkowany/powiększony i odsunięty od powrotu.
Cyber: ramki AP, restartu i niedostępnej konfiguracji; czerwone akcje usuwania.
Lokalny restart ma pomijać zajętość operacji; dodano osobną ścieżkę potwierdzoną,
bez czekania na BLE/Wi-Fi, ze sprzętową ochroną procedury zamykania SDK.
