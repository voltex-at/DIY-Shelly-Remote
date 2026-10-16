# UPDATE-APP-ONLY — zachowanie istniejącej konfiguracji

Ta paczka zawiera wyłącznie `diy_shelly_remote.bin` jako segment firmware,
jego SHA256, identyfikację źródła i licencje. Nie zawiera obrazu dsrdata,
bootloadera, tablicy partycji, otadata ani scalonego obrazu Flash.
**Bez Erase All i bez FACTORY-CLEAN-16MB.**

Aktualizacja app-only wymaga niezmienionej tablicy partycji i poprawnie
zamontowanego dotychczasowego magazynu. Nie naprawia częściowo sformatowanego
lub uszkodzonego SPIFFS. Najpierw zachowuje się kopię i rozstrzyga stan danych.

OTA wybiera nieaktywny slot przez SDK i wymaga zgodnego podpisanego manifestu
oraz zatwierdzonego klucza wydawcy. Obecny produkcyjny truststore jest EMPTY:
sam CI BIN nie jest podpisanym wydaniem, więc aplikacja słusznie odmówi OTA.
Paczka nie dodaje klucza, podpisu, Stable ani obejścia weryfikacji.

Ewentualny zatwierdzony test USB zapisuje samą aplikację do aktywnego slotu
potwierdzonego aktualnym logiem: ota_0 0x20000 albo ota_1 0x620000, oba po
0x600000. Nie zakładaj 0x20000 po OTA; wybór złego slotu nie aktualizuje
działającej aplikacji. Nie zapisuj NVS/dsrdata/partition/otadata i nie używaj
krótszego historycznego obrazu `local.bin` do zachowawczej aktualizacji.

Po zatwierdzonym teście potwierdź start i zachowanie Wi-Fi, powiązań,
pomieszczeń, zdjęć oraz sterowania i log pierwszego startu. Host/CI nie zastępuje
tego odbioru. Bez automatycznego restartu, kasowania i publikacji Stable.
