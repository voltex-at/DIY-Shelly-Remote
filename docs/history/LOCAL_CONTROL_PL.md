# DIY Shelly Remote 0.2.0-local-dev — test jednego przekaźnika

Dla LCDWiki ES3C35P, ESP32-S3 N16R8, ST77922 320x480. Wersja rozwojowa. Pierwsza wersja 0.1.0-bringup została uruchomiona na płytce użytkownika; zdjęcia potwierdzają czytelny ekran i reakcję dotyku. Nowa konfiguracja Wi-Fi, logowanie i sterowanie Shelly wymagają osobnego testu fizycznego.

## Wgranie przez USB

Użyj `DIY-Shelly-Remote-es3c35p-local.bin` pod adresem **0x0**. To obraz scalony dla USB, zawierający bootloader, tablicę partycji i aplikację. App-only `diy_shelly_remote.bin` nie jest zamiennikiem obrazu scalonego.

```sh
python -m esptool --chip esp32s3 --port PORT write_flash 0x0 DIY-Shelly-Remote-es3c35p-local.bin
```

Zastąp `PORT` portem płytki. Korzystaj z tego samego sposobu wejścia w tryb programowania, który zadziałał z 0.1.0-bringup. Obraz scalony nadpisuje także obszary ustawień pokryte jego zakresem; nie jest aktualizacją OTA zachowującą konfigurację. Nie używaj obrazów diagnostycznych ESP32/ESP32-S3. `SHA256SUMS.txt` zawiera sumy właściwych BIN-ów. Szczegóły sprzętu i pozostałe adresy są w `BOARD_REFERENCE.md`.

## Konfiguracja na ekranie

1. **Menu → Wi-Fi setup**: wpisz SSID sieci 2.4 GHz i hasło. Znacznik zatwierdzenia klawiatury zamyka klawiaturę; przewijaj formularz, żeby odsłonić dolne pola. **Save & restart** zapisuje dane i uruchamia pilot ponownie. Puste hasło oznacza otwartą sieć.
2. Poczekaj na **Wi-Fi: connected**. Pilot i Shelly muszą mieć dostęp do siebie przez LAN. Błędne SSID/hasło poprawisz w tym samym formularzu. Ponowne połączenie używa narastających odstępów do 32 sekund.
3. **Menu → Add / replace Shelly**: wpisz IPv4 urządzenia, kanał (`0` dla pierwszego przekaźnika), opcjonalną nazwę i login/hasło Shelly. Dla Gen2+ login to `admin`; dla Gen1 podaj login ustawiony w urządzeniu. Przy wyłączonym logowaniu zostaw hasło puste. Dane wpisuj na pilocie, nie do repozytorium.
4. Przewiń do **Check & save device**. Pilot odczyta `/shelly`, wykryje generację i sprawdzi stan wybranego przekaźnika. Zapis następuje dopiero po udanym odczycie. Zapis identyfikuje Gen1 przez MAC, a Gen2+ przez ID. Ta wersja obsługuje jeden zapisany kanał; udane dodanie zastępuje poprzedni.
5. Na **Home** użyj **ON**, potem **OFF**. Stan `confirmed` pojawia się po odczycie zwrotnym z urządzenia. Nie ma optymistycznego przestawiania przełącznika ani automatycznego ponawiania komendy po timeout. Moc pojawia się wyłącznie, gdy Gen2+ zwróci `apower`.

Dane są zapisane lokalnie w NVS. W tym prototypie NVS nie jest szyfrowane. Transport to lokalny HTTP: Gen1 Basic (bez poufności hasła w LAN), Gen2+ Digest SHA-256 przez ESP-IDF. Nie ma przekierowań HTTP, chmury ani Home Assistant. Nie wyłączaj logowania Shelly w celu obejścia błędu.

## Wynik testu do przekazania

- Czy klawiatura, przewijanie i zapis Wi-Fi działają? Czy po restarcie pilot łączy się automatycznie?
- Model Shelly, generacja, wersja firmware i testowany kanał; bez haseł.
- Czy **Check & save device** działa przy włączonym logowaniu? Czy ON/OFF zmienia przekaźnik, a stan wraca poprawnie?
- Zmień stan w interfejsie Shelly: pilot powinien odświeżyć go w około 5 sekund plus czas żądań.
- Odłącz Shelly od sieci: pilot ma pozostać responsywny i pokazać brak potwierdzonego stanu. Po przywróceniu sieci odczyt ma wrócić. Komendy nie są kolejkowane do wykonania po powrocie sieci.
- Błędne hasło lub HTTP 429 zatrzymuje automatyczne próby dla zapisanego urządzenia. Popraw dane i dodaj ponownie; przy limicie prób odczekaj zgodnie z wymaganiami Shelly, następnie **Refresh**.
- Inny identyfikator pod zapisanym IP blokuje sterowanie. Sprawdź adres i jawnie dodaj właściwe urządzenie. Brak mDNS/discovery: zalecana rezerwacja DHCP.
- Uruchom ponownie pilot: zapisany kanał ma wrócić z niepotwierdzonym stanem, potem zostać sprawdzony. Hasła nie powinny pojawić się w logach.

Testy ekranu i dotyku pozostają w **Menu → Hardware tests**; jasność i wygaszanie w **Menu → Settings**. Osobno sprawdź kolory, jasność i pierwsze dotknięcie po wygaszeniu. BLE, BTHome, wiele urządzeń, pokoje/sceny i instalator OTA nie są dostępne w tej wersji.
