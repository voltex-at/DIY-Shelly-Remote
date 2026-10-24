'use strict';
/* Only explicit UI messages are bound. Device names, credentials, addresses and
 * configuration values remain plain text. Weak bindings follow the existing
 * DOM: changing locale neither reconstructs forms nor touches remote state. */
const WEB_MESSAGES=new Map([
 ["Pobieranie obrazu z pilota…", "Fetching the remote screen…", "Bild der Fernbedienung wird geladen…"],
 ["Pilot jeszcze nie udostępnił nowej klatki. Odśwież podgląd ponownie.", "The remote has not provided a new frame yet. Refresh the preview again.", "Die Fernbedienung hat noch kein neues Bild bereitgestellt. Vorschau erneut aktualisieren."],
 ["Nieprawidłowy obraz z pilota. Odśwież podgląd ponownie.", "Invalid screen image from the remote. Refresh the preview again.", "Ungültiges Bild der Fernbedienung. Vorschau erneut aktualisieren."],
 ["Nie udało się pobrać obrazu. Sprawdź połączenie i odśwież podgląd.", "Could not fetch the screen image. Check the connection and refresh the preview.", "Bild konnte nicht geladen werden. Verbindung prüfen und Vorschau aktualisieren."],
 ["Battery below 20%; recharge the remote and renew OTA consent", "Bateria poniżej 20%; doładuj pilot i ponownie zezwól na OTA", "Akku unter 20%; Fernbedienung laden und OTA erneut erlauben"],
 ["Battery check unavailable; renew OTA consent and retry", "Pomiar baterii nie jest dostępny; ponownie zezwól na OTA i spróbuj jeszcze raz", "Akkuprüfung nicht verfügbar; OTA erneut erlauben und wiederholen"],
 ["Przed wysłaniem potwierdź stabilne USB na ekranie pilota. Zgoda jest jednorazowa i ważna 120 s. Znany poziom baterii poniżej 20% blokuje instalację.", "Before uploading, confirm stable USB power on the remote. Consent is one-time and valid for 120 s. A known battery level below 20% blocks installation.", "Vor dem Senden stabiles USB am Bildschirm der Fernbedienung bestätigen. Einmalige Freigabe für 120 s. Bekannter Akkustand unter 20% sperrt die Installation."],
 ["Plik: {0} ({1} B)","File: {0} ({1} B)","Datei: {0} ({1} B)"],
 ["Instaluj", "Install", "Installieren"],
 ["Wybierz plik aplikacji BIN.", "Select the application BIN file.", "Anwendungs-BIN-Datei auswählen."],
 ["Przed wysłaniem zezwól na OTA na ekranie pilota. Zgoda jest jednorazowa i ważna 120 s. Podłącz stabilne zasilanie USB.", "Before uploading, allow OTA on the remote screen. Approval is one-time and valid for 120 s. Connect stable USB power.", "Vor dem Senden OTA am Bildschirm der Fernbedienung erlauben. Einmalige Freigabe für 120 s. Stabile USB-Versorgung anschließen."],
 ["Wybierz BIN aplikacji o rozmiarze od 512 B do 6 MiB.", "Select an application BIN between 512 B and 6 MiB.", "Anwendungs-BIN zwischen 512 B und 6 MiB auswählen."],
 ["Obliczanie sumy SHA-256…", "Calculating SHA-256…", "SHA-256 wird berechnet…"],
 ["Wysyłanie i weryfikacja obrazu…", "Uploading and verifying the image…", "Abbild wird übertragen und geprüft…"],
 ["Nie potwierdzono wyniku aktualizacji. Sprawdź pilot przed ponowieniem.", "Update outcome unconfirmed. Check the remote before retrying.", "Update-Ergebnis unbestätigt. Fernbedienung vor erneutem Versuch prüfen."],
 ["Nie potwierdzono restartu. Sprawdź pilot.", "Restart unconfirmed. Check the remote.", "Neustart unbestätigt. Fernbedienung prüfen."],
 ["OTA staging or restart busy; upload not started", "OTA lub restart zajęte; przesyłanie nie rozpoczęło się", "OTA oder Neustart belegt; Übertragung nicht gestartet"],
 ["Boot selection unconfirmed; OTA blocked until restart", "Nie potwierdzono wyboru startowego; OTA zablokowane do restartu", "Startauswahl unbestätigt; OTA bis zum Neustart gesperrt"],
 ["Disable the active automation before changing its contacts, targets or temperature","Przed zmianą aktywnej reguły wyłącz ją i zapisz.","Aktive Regel vor Änderungen deaktivieren und speichern."],
 ["Automatyzacje","Automations","Automatisierungen"],
 ["Stan reguły","Rule status","Regelstatus"],["Reguła ogrzewania","Heating rule","Heizungsregel"],
 ["Bezpośredni odczyt TRV lub nastawa nieobsługiwane.","Direct TRV read or target is unsupported.","Direkte TRV-Abfrage oder Sollwert nicht unterstützt."],
 ["Wymagane logowanie do bramki.","Gateway authentication required.","Gateway-Anmeldung erforderlich."],
 ["Powiązanie termostatu zmienione. Sprawdź konfigurację.","Thermostat binding changed. Review configuration.","Thermostat-Zuordnung geändert. Konfiguration prüfen."],
 ["Limit zapytań. Oczekiwanie przed ponowieniem.","Rate limited. Waiting before retry.","Abfragelimit. Wartezeit vor Wiederholung."],
 ["Oczekiwanie na dostęp do urządzenia.","Waiting for device access.","Warte auf Gerätezugriff."],
 ["Brak odpowiedzi termostatu. Oczekiwanie na nowy odczyt.","Thermostat did not respond. Waiting for a new read.","Thermostat antwortet nicht. Warte auf neue Abfrage."],
 ["Niepoprawny odczyt termostatu.","Invalid thermostat reading.","Ungültiger Thermostatwert."],
 ["Błąd pamięci.","Storage error.","Speicherfehler."],
 ["Błąd dziennika. Automatyzacja wstrzymana.","Journal error. Automation paused.","Journalfehler. Automatik angehalten."],
 ["Odśwież","Refresh","Aktualisieren"],
 ["+ Nowa reguła","+ New rule","+ Neue Regel"],["Nazwa reguły","Rule name","Regelname"],
 ["Temperatura przy otwarciu","Temperature while open","Temperatur bei Öffnung"],
 ["Okna i drzwi","Windows and doors","Fenster und Türen"],["Termostaty","Thermostats","Thermostate"],
 ["Włącz regułę","Enable rule","Regel aktivieren"],["Zapisz regułę","Save rule","Regel speichern"],
 ["Edytuj regułę","Edit rule","Regel bearbeiten"],["Usuń regułę","Delete rule","Regel löschen"],
 ["Odśwież stan reguł","Refresh rule status","Regelstatus aktualisieren"],
 ["Szukaj urządzeń","Search devices","Geräte suchen"],
 ["Wyłączona","Disabled","Deaktiviert"],["Oczekiwanie na dane","Waiting for data","Warte auf Daten"],
 ["Aktywna","Active","Aktiv"],["Ręczna zmiana","Manual change","Manuell geändert"],
 ["Oczekiwanie na potwierdzenie","Waiting for confirmation","Bestätigung ausstehend"],["Błąd reguły","Rule error","Regelfehler"],
 ["Brak nowego raportu: {0}","Waiting for a new report: {0}","Neuer Bericht fehlt: {0}"],
 ["Poprzednia: {0} · Żądana: {1} · Odczytana: {2}","Previous: {0} · Requested: {1} · Read back: {2}","Vorher: {0} · Angefordert: {1} · Ausgelesen: {2}"],
 ["Nowe raporty wszystkich kontaktów i odczyty TRV są wymagane po przerwie. Tryb beacon można włączyć samodzielnie w Shelly.","After a pause, new reports from every contact and TRV reads are required. Beacon mode can be enabled manually in Shelly.","Nach einer Pause sind neue Berichte aller Kontakte und TRV-Abfragen erforderlich. Beacon-Modus bei Bedarf selbst in Shelly aktivieren."],
 ["Reakcja natychmiastowa. Powrót po zamknięciu wszystkich kontaktów. Ręczne zmiany zostają zachowane.","Immediate response. Restore after all contacts close. Manual changes are preserved.","Sofortige Reaktion. Rückkehr nach Schließen aller Kontakte. Manuelle Änderungen bleiben erhalten."],
 ["Wyłączenie lub usunięcie zatrzymuje przyszłe polecenia bez przywracania temperatury. Aktywną regułę wyłącz i zapisz przed zmianą czujników, celów lub nastawy.","Disabling or deleting stops future commands without restoring temperature. Disable and save an active rule before changing contacts, targets or setback.","Deaktivieren oder Löschen stoppt künftige Befehle ohne Temperaturrückkehr. Aktive Regel vor Änderungen an Kontakten, Zielen oder Sollwert deaktivieren und speichern."],
 ["Brak zweryfikowanych urządzeń. Dodaj kontakt BLU lub termostat TRV przez bramkę.","No verified devices. Add a BLU contact or TRV through its gateway.","Keine verifizierten Geräte. BLU-Kontakt oder TRV über das Gateway hinzufügen."],
 ["Zajęty przez inną regułę","Used by another rule","Von anderer Regel verwendet"],
 ["Status innej rewizji. Odśwież lub wczytaj konfigurację; szkic pozostaje.","Status has another revision. Refresh or reload configuration; draft retained.","Status hat andere Revision. Aktualisieren oder Konfiguration neu laden; Entwurf bleibt erhalten."],
 ["Stan reguł niedostępny. Odśwież, aby spróbować ponownie.","Rule status unavailable. Refresh to retry.","Regelstatus nicht verfügbar. Zum Wiederholen aktualisieren."],
 ["Przed zapisem podaj nazwę (maks. 47 bajtów), wybierz kontakty i termostaty oraz temperaturę 4–30°C co 0,5°C.","Before saving, enter a name (max. 47 bytes), select contacts and thermostats, and a temperature of 4–30°C in 0.5°C steps.","Vor dem Speichern Namen (max. 47 Bytes), Kontakte, Thermostate und Temperatur 4–30°C in 0,5°C-Schritten angeben."],
 ["Odrzucić niezapisane zmiany reguły?","Discard unsaved rule changes?","Ungespeicherte Regeländerungen verwerfen?"],
 ["Usunąć regułę? Przyszłe polecenia zostaną anulowane bez przywracania temperatury.","Delete rule? Future commands will be cancelled without restoring temperature.","Regel löschen? Künftige Befehle werden ohne Temperaturrückkehr abgebrochen."],
 ["Przed zmianą aktywnej reguły wyłącz ją i zapisz.","Disable and save an active rule before changing it.","Aktive Regel vor Änderungen deaktivieren und speichern."],
 ["Nastawa {0} °C","Target {0} °C","Soll {0} °C"],
 ["Ustaw temperaturę","Set temperature","Temperatur einstellen"],
 ["Temperatura zadana","Target temperature","Soll"],
 ["Odczyt typów urządzeń…","Reading device types…","Gerätetypen werden gelesen…"],
 ["Nie można odczytać typów urządzeń.","Cannot read device types.","Gerätetypen können nicht gelesen werden."],
 ['Odśwież podgląd','Refresh preview','Vorschau aktualisieren'],
 ['Na żądanie','On demand','Auf Abruf'],
 ["Zapis", "Save", "Speichern"],
 ["Gotowe", "Done", "Fertig"],
 ["Nazwa grupy", "Group name", "Gruppenname"],
 ["Nazwa sceny", "Scene name", "Szenenname"],
 ["np. Lampy na parterze", "e.g. Downstairs lights", "z. B. Lichter im Erdgeschoss"],
 ["np. Dobranoc", "e.g. Good night", "z. B. Gute Nacht"],
 ["Jedno dotknięcie uruchomi wybrane polecenia.", "One tap runs the selected commands.", "Ein Tippen startet die gewählten Befehle."],
 ["Wspólne włączanie i wyłączanie wybranych przekaźników i gniazdek.", "Turn selected relays and plugs on or off together.", "Gewählte Relais und Steckdosen gemeinsam ein- oder ausschalten."],
 ["Pokój i zdjęcie (opcjonalnie)", "Room and photo (optional)", "Raum und Bild (optional)"],
 ["Pomieszczenie (opcjonalnie)", "Room (optional)", "Raum (optional)"],
 ["Zdjęcie (opcjonalnie)", "Photo (optional)", "Bild (optional)"],
 ["Własne zdjęcie", "Your photo", "Eigenes Bild"],
 ["Opcjonalne ustawienia", "Optional settings", "Optionale Einstellungen"],
 ["1/3 • Nazwa", "1/3 • Name", "1/3 • Name"],
 ["2/3 • Urządzenia", "2/3 • Devices", "2/3 • Geräte"],
 ["2/3 • Polecenia", "2/3 • Commands", "2/3 • Befehle"],
 ["3/3 • Podsumowanie", "3/3 • Review", "3/3 • Prüfen"],
 ["Polecenia", "Commands", "Befehle"],
 ["Podsumowanie", "Review", "Prüfen"],
 ["Wybierz urządzenie", "Choose device", "Gerät wählen"],
 ["Wybierz polecenie", "Choose command", "Befehl wählen"],
 ["Zmień urządzenie", "Change device", "Gerät ändern"],
 ["Zmień nazwę i opcje", "Change name and options", "Name und Optionen ändern"],
 ["Co ma zrobić scena?", "What should this scene do?", "Was soll diese Szene tun?"],
 ["Dodaj urządzenie i wybierz dla niego polecenie.", "Add a device and choose its command.", "Gerät hinzufügen und Befehl wählen."],
 ["+ Dodaj polecenie", "+ Add command", "+ Befehl hinzufügen"],
 ["Sprawdź listę i zapisz.", "Review the list and save.", "Liste prüfen und speichern."],
 ["Zapisz scenę", "Save scene", "Szene speichern"],
 ["Zapisz grupę", "Save group", "Gruppe speichern"],
 ["Brak wyników. Zmień wyszukiwanie.", "No matches. Change your search.", "Keine Treffer. Suche ändern."],
 ["+ Nowa grupa", "+ New group", "+ Neue Gruppe"],
 ["+ Nowa scena", "+ New scene", "+ Neue Szene"],
 ["Uzupełnij nazwę i wybierz co najmniej jedno urządzenie lub polecenie.", "Enter a name and choose at least one device or command.", "Namen eingeben und mindestens ein Gerät oder einen Befehl wählen."],
 ["Poczekaj na zakończenie zapisu.", "Wait for saving to finish.", "Warten, bis das Speichern beendet ist."],
 ["Odrzucić zmiany?", "Discard changes?", "Änderungen verwerfen?"],
 ["Niezapisane zmiany tego formularza zostaną utracone.", "Unsaved changes in this form will be lost.", "Ungespeicherte Änderungen in diesem Formular gehen verloren."],
 ["Zostań", "Stay", "Bleiben"],
 ["Odrzuć", "Discard", "Verwerfen"],
 ["Usuń wpis", "Delete item", "Eintrag löschen"],
 ["Brak nazwy", "No name", "Kein Name"],
 ["Wstecz","Back","Zurück"],["Zapisz","Save","Speichern"],
 ["Edytuj grupę", "Edit group", "Gruppe bearbeiten"],
 ["Edytuj scenę", "Edit scene", "Szene bearbeiten"],
 ["Typ grupy", "Group type", "Gruppentyp"],
 ["Sceny uruchamiane ręcznie", "Manually started scenes", "Manuell gestartete Szenen"],
 ["Przekaźniki i gniazdka", "Relays and plugs", "Relais und Steckdosen"],
 ["Światła • wkrótce", "Lights • coming later", "Lichter • später verfügbar"],
 ["Rolety i termostaty • wkrótce", "Covers and thermostats • later", "Rollläden und Thermostate • später"],
 ["Przycisk uruchamia wybrane działania po kolei. Warunki i harmonogramy nie są jeszcze obsługiwane.", "The button runs selected actions in order. Conditions and schedules are not supported yet.", "Die Taste führt Aktionen nacheinander aus. Bedingungen und Zeitpläne werden noch nicht unterstützt."],
 ["Wybrano: {0}", "Selected: {0}", "Ausgewählt: {0}"],
 ["Szukaj", "Search", "Suchen"],
 ["Cały dom", "Whole home", "Ganzes Zuhause"],
 ["Wybierz pomieszczenie", "Choose a room", "Raum wählen"],
 ["Wybierz zdjęcie", "Choose a photo", "Bild wählen"],
 ["Zdjęcie z pomieszczenia", "Photo from a room", "Bild aus einem Raum"],
 ["Bez zdjęcia", "No photo", "Ohne Bild"],
 ["Brak zgodnych urządzeń. Dodaj je na stronie Urządzenia.", "No compatible devices. Add them on the Devices page.", "Keine kompatiblen Geräte. Auf der Seite Geräte hinzufügen."],
 ["Użyj nazwy z Shelly","Use name from Shelly","Namen von Shelly verwenden"],
 ["Rodzina API", "API family", "API-Familie"],
 ["H&T Gen3 • read only", "H&T Gen3 • read only", "H&T Gen3 • nur lesen"],
 ["Light poza grupami Switch", "Light excluded from Switch groups", "Light außerhalb von Switch-Gruppen"],
 ["Grupy", "Groups", "Gruppen"],
 ["Grupa", "Group", "Gruppe"],
 ["Aktywność", "Activity", "Aktivität"],
 ["Nowa grupa", "New group", "Neue Gruppe"],
 ["Grupa: do 16 kanałów Switch, jawne Włącz i Wyłącz.", "Group: up to 16 Switch channels, explicit On and Off.", "Gruppe: bis zu 16 Switch-Kanäle, explizit Ein und Aus."],
 ["Actions unavailable", "Wykonawca niedostępny", "Ausführung nicht verfügbar"],
 ["Anuluj","Cancel","Abbrechen"],
 ["Krok {0} z 3", "Step {0} of 3", "Schritt {0} von 3"],
 ["Sieć lokalna", "Local network", "Lokales Netzwerk"],
 ["Reklamy BLE • także bez Wi-Fi", "BLE advertisements • also without Wi-Fi", "BLE-Advertisements • auch ohne WLAN"],
 ["Otwarty punkt dostępowy Shelly", "Shelly open access point", "Offener Shelly-Zugangspunkt"],
 ["Urządzenia w domowej sieci", "Devices on your home network", "Geräte im Heimnetz"],
 ["Brak połączenia z pilotem. Odśwież panel.", "Remote unreachable. Refresh the panel.", "Fernbedienung nicht erreichbar. Panel aktualisieren."],
 ["Dodaj ręcznie (IP)", "Add manually (IP)", "Manuell hinzufügen (IP)"],
 ["Adres IP • zaawansowane", "IP address • advanced", "IP-Adresse • erweitert"],
 ["Dalej", "Next", "Weiter"],
 ["Zaawansowane: adres, tożsamość API i stan zostaną sprawdzone przed dodaniem.", "Advanced: address, API identity and state will be checked before adding.", "Erweitert: Adresse, API-Identität und Zustand werden vor dem Hinzufügen geprüft."],
 ["Kanał 0–15", "Channel 0–15", "Kanal 0–15"],
 ["Sprawdź adres, nazwę i kanał 0–15.", "Check the address, name and channel 0–15.", "Adresse, Name und Kanal 0–15 prüfen."],
 ["Shelly • reklamowane RPC", "Shelly • advertised RPC", "Shelly • beworbenes RPC"],
 ["Nierozpoznane urządzenie", "Unrecognized device", "Nicht erkanntes Gerät"],
 ["Reklama: {0}", "Advertisement: {0}", "Advertisement: {0}"],
 ["bez nazwy", "unnamed", "ohne Namen"],

 ["Otwórz kreator BLE na pilocie","Open BLE wizard on the remote","BLE-Assistent auf der Fernbedienung öffnen"],
 ["Dokończ kreator na LCD pilota. Obie zgody wymagają dotknięcia ekranu; samo żądanie WWW nie wysyła Wi-Fi.","Complete the wizard on the remote LCD. Both consents require a screen touch; the web request alone sends no Wi-Fi details.","Assistent auf dem LCD abschließen. Beide Zustimmungen erfordern eine Berührung; die Web-Anfrage allein sendet keine WLAN-Daten."],
 ["Skan działa na pilocie. Dla reklamowanego RPC możesz otworzyć prawdziwy kreator na LCD. Dopiero dwie fizyczne zgody pozwalają sprawdzić urządzenie i przekazać Wi-Fi. Reklama nie dowodzi autentyczności.","The scan runs on the remote. For advertised RPC you can open a real LCD wizard. Two physical consents are required to verify the device and share Wi-Fi. Advertisements do not prove authenticity.","Die Suche läuft auf der Fernbedienung. Für beworbenes RPC lässt sich der echte LCD-Assistent öffnen. Zwei physische Zustimmungen sind für Geräteprüfung und WLAN-Weitergabe nötig. Werbung beweist keine Echtheit."],
 ["Tylko obsługiwany Gen2+ lub H&T Gen3 w otwartym setup, bez bondingu i z HTTP w LAN. Fabryczne HTTPS 2.0, BLU, BTHome i urządzenia zabezpieczone nie są obsługiwane tym kreatorem. Pilot pozostaje w LAN.","Only supported Gen2+ or H&T Gen3 in open setup, without bonding and with LAN HTTP. Factory HTTPS 2.0, BLU, BTHome and secured devices are unsupported by this wizard. The remote stays on LAN.","Nur unterstütztes Gen2+ oder H&T Gen3 mit offenem Setup, ohne Bonding und mit HTTP im LAN. Werks-HTTPS 2.0, BLU, BTHome und gesicherte Geräte werden nicht unterstützt. Pilot bleibt im LAN."],
 ["Expected current BLE scan serial and index only","Wymagany wyłącznie numer bieżącego skanu BLE i indeks","Nur aktuelle BLE-Suchnummer und Index erwartet"],
 ["BLE wizard unavailable; refresh the scan and check the remote","Kreator BLE niedostępny; odśwież skan i sprawdź pilot","BLE-Assistent nicht verfügbar; Suche aktualisieren und Fernbedienung prüfen"],

 ["Wi-Fi AP • kreator na pilocie", "Wi-Fi AP • wizard on the remote", "WLAN-AP • Assistent auf der Fernbedienung"],
 ["Otwórz kreator na pilocie", "Open wizard on the remote", "Assistent auf der Fernbedienung öffnen"],
 ["Kreator AP wymaga dwóch potwierdzeń na ekranie pilota. Najpierw zatwierdzisz opuszczenie domowej sieci, a potem przekazanie danych Wi-Fi wybranemu Shelly.", "The AP wizard requires two confirmations on the remote. First approve leaving the home network, then sharing Wi-Fi details with the selected Shelly.", "Der AP-Assistent erfordert zwei Bestätigungen auf der Fernbedienung: zuerst das Verlassen des Heimnetzes, dann die Weitergabe der WLAN-Daten an das gewählte Shelly."],
 ["Podczas połączenia z AP panel utraci dostęp do pilota. Dokończ kroki na pilocie. Po jego powrocie do domu połącz się ponownie z panelem.", "While connected to the AP, the panel will lose access to the remote. Complete the steps on the remote. Reconnect to the panel after it returns home.", "Während der AP-Verbindung verliert das Panel den Zugriff auf die Fernbedienung. Schritte dort abschließen und das Panel nach der Rückkehr ins Heimnetz erneut verbinden."],
 ["Odczyt dostępności kreatora…", "Checking wizard availability…", "Verfügbarkeit des Assistenten wird geprüft…"],
 ["Kreator dostępny. Otwórz go i kontynuuj na ekranie pilota.", "Wizard available. Open it and continue on the remote screen.", "Assistent verfügbar. Öffnen und auf der Fernbedienung fortfahren."],
 ["Żądanie oczekuje na ekran pilota. Nie przekazano danych Wi-Fi.", "Request waiting for the remote screen. No Wi-Fi details have been shared.", "Anfrage wartet auf den Bildschirm der Fernbedienung. Es wurden keine WLAN-Daten weitergegeben."],
 ["Kreator działa na pilocie. Potwierdzaj kolejne kroki na jego ekranie.", "Wizard active on the remote. Confirm each step on its screen.", "Assistent auf der Fernbedienung aktiv. Schritte auf deren Bildschirm bestätigen."],
 ["Żądanie wygasło. Sprawdź ekran pilota i spróbuj ponownie.", "Request expired. Check the remote screen and try again.", "Anfrage abgelaufen. Bildschirm der Fernbedienung prüfen und erneut versuchen."],
 ["Kreator niedostępny. Sprawdź ekran pilota i domowe Wi-Fi.", "Wizard unavailable. Check the remote screen and home Wi-Fi.", "Assistent nicht verfügbar. Bildschirm der Fernbedienung und Heim-WLAN prüfen."],
 ["Nie potwierdzono otwarcia. Sprawdź ekran pilota; żądanie nie będzie ponawiane automatycznie.", "Opening was not confirmed. Check the remote screen; the request will not be retried automatically.", "Öffnen nicht bestätigt. Bildschirm der Fernbedienung prüfen; die Anfrage wird nicht automatisch wiederholt."],
 ["Brak połączenia z pilotem. Dokończ na jego ekranie; panel wróci po przywróceniu LAN.", "Remote disconnected. Continue on its screen; the panel will return when LAN is restored.", "Keine Verbindung zur Fernbedienung. Auf deren Bildschirm fortfahren; das Panel kehrt nach Wiederherstellung des LAN zurück."],
 ["AP handoff accepts no configuration or credentials", "Przekazanie do kreatora AP nie przyjmuje konfiguracji ani haseł", "Die Übergabe an den AP-Assistenten nimmt keine Konfiguration oder Passwörter an"],
 ["AP wizard unavailable; check the remote and home Wi-Fi", "Kreator AP niedostępny; sprawdź pilot i domowe Wi-Fi", "AP-Assistent nicht verfügbar; Fernbedienung und Heim-WLAN prüfen"],
 ["Unsupported AP handoff request", "Nieobsługiwane żądanie kreatora AP", "Nicht unterstützte Anfrage an den AP-Assistenten"],

 ["Każde powiązanie zachowuje pozostałe urządzenia. Odczyty i polecenia są przypisane do wybranego kanału. Czujniki i rolety wymagają dalszej obsługi protokołu.", "Each binding preserves the other devices. Readings and commands belong to the selected channel. Sensors and covers require further protocol support.", "Jede Verknüpfung erhält die anderen Geräte. Messwerte und Befehle gehören zum gewählten Kanal. Sensoren und Rollläden benötigen weitere Protokollunterstützung."],
 ["Dodanie zachowuje poprzednie urządzenia i ich kanały.", "Adding preserves previous devices and their channels.", "Beim Hinzufügen bleiben bisherige Geräte und deren Kanäle erhalten."],
 ["Konfiguracja zmieniła się. Wczytaj ją i ponów dodawanie urządzenia.","Configuration changed; reload and retry adding the device","Konfiguration geändert. Neu laden und Gerät erneut hinzufügen."],
 ["Już dodane", "Already added", "Bereits hinzugefügt"],
 ['Failed to fetch','Nie udało się połączyć z pilotem.','Verbindung zur Fernbedienung fehlgeschlagen.'],['Load failed','Nie udało się połączyć z pilotem.','Verbindung zur Fernbedienung fehlgeschlagen.'],['NetworkError when attempting to fetch resource.','Nie udało się połączyć z pilotem.','Verbindung zur Fernbedienung fehlgeschlagen.'],
 ['Przerwij','Cancel','Abbrechen'],['LIVE','LIVE','LIVE'],
 ['{0} · {1}','{0} · {1}','{0} · {1}'],['{0} · Gen {1} · {2}','{0} · Gen {1} · {2}','{0} · Gen {1} · {2}'],['Przesuń w górę','Move up','Nach oben verschieben'],['Przesuń w dół','Move down','Nach unten verschieben'],['Plik aplikacji BIN','Application BIN file','Anwendungs-BIN-Datei'],
 ['Pulpity','Dashboards','Dashboards'],['Pomieszczenia','Rooms','Räume'],['Urządzenia','Devices','Geräte'],['Sceny','Scenes','Szenen'],
 ['Urządzenia Shelly','Shelly devices','Shelly-Geräte'],['Dodaj urządzenie Shelly','Add a Shelly device','Shelly-Gerät hinzufügen'],['Wygląd i język','Appearance and language','Darstellung und Sprache'],['System i OTA','System and OTA','System und OTA'],['System i aktualizacje','System and updates','System und Updates'],
 ['Oświetlenie','Lighting','Beleuchtung'],['Przekaźnik','Relay','Relais'],['Gniazdko','Socket','Steckdose'],['Roleta','Cover','Rollladen'],['Czujnik otwarcia','Contact sensor','Öffnungssensor'],['Czujnik','Sensor','Sensor'],
 ['Niezapisane zmiany','Unsaved changes','Ungespeicherte Änderungen'],['Parowanie wymagane','Pairing required','Kopplung erforderlich'],['Błąd {0}','Error {0}','Fehler {0}'],['Wszystkie zmiany zapisane','All changes saved','Alle Änderungen gespeichert'],
 ['Najpierw usuń powiązania automatyzacji z urządzeniem.','Remove automation references to this device first.','Zuerst Automatisierungszuordnungen dieses Geräts entfernen.'],['Najpierw usuń działania scen odwołujące się do urządzenia.','Remove scene actions referencing this device first.','Zuerst Szenenaktionen mit Bezug auf dieses Gerät entfernen.'],['Najpierw przenieś urządzenia z tego pomieszczenia.','Move devices out of this room first.','Zuerst die Geräte aus diesem Raum verschieben.'],['Najpierw usuń powiązane karty z pulpitów.','Remove referencing dashboard tiles first.','Zuerst verknüpfte Dashboard-Kacheln entfernen.'],['Zachowaj przynajmniej jeden pulpit.','Keep at least one dashboard.','Mindestens ein Dashboard behalten.'],
 ['Bez adresu','No address','Keine Adresse'],['Edytuj','Edit','Bearbeiten'],['Usuń','Delete','Löschen'],['Brak wpisów. Dodaj pierwszy.','No entries. Add the first one.','Keine Einträge. Den ersten hinzufügen.'],['+ Dodaj','+ Add','+ Hinzufügen'],['Osiągnięto limit wpisów.','Entry limit reached.','Eintragslimit erreicht.'],
 ['Nowy pulpit','New dashboard','Neues Dashboard'],['Nowe pomieszczenie','New room','Neuer Raum'],['Nowe urządzenie','New device','Neues Gerät'],['Nowa scena','New scene','Neue Szene'],['Edycja: {0}','Editing: {0}','Bearbeiten: {0}'],['Nazwa','Name','Name'],['← Wróć do listy','← Back to list','← Zurück zur Liste'],
 ['Ikona','Icon','Symbol'],['Salon','Living room','Wohnzimmer'],['Kuchnia','Kitchen','Küche'],['Sypialnia','Bedroom','Schlafzimmer'],['Łazienka','Bathroom','Badezimmer'],['Zdjęcie jest zapisane w pilocie.','The photo is stored on the remote.','Das Foto ist auf der Fernbedienung gespeichert.'],['Możesz użyć własnego zdjęcia.','You can use your own photo.','Sie können ein eigenes Foto verwenden.'],['Zdjęcie pomieszczenia','Room photo','Raumfoto'],['Zdjęcie zoptymalizowane do 320 × 180. Zapisz konfigurację.','Photo optimized to 320 × 180. Save the configuration.','Foto auf 320 × 180 optimiert. Konfiguration speichern.'],['Usuń zdjęcie','Remove photo','Foto entfernen'],
 ['Typ urządzenia','Device type','Gerätetyp'],['Adres IPv4 Shelly','Shelly IPv4 address','Shelly-IPv4-Adresse'],['Kanał (0–15)','Channel (0–15)','Kanal (0–15)'],['Pomieszczenie','Room','Raum'],['Nieprzypisane','Unassigned','Nicht zugewiesen'],
 ['Karta i połączenie są zapisywane osobno. Obecny moduł komunikacji obsługuje jedno zweryfikowane połączenie z przekaźnikiem. Sprawdzenie poniżej zastąpi poprzednie połączenie dopiero po sukcesie. Czujniki i rolety wymagają dalszej obsługi protokołu.','The tile and connection are saved separately. The current transport supports one verified relay connection. The check below replaces the previous connection only after success. Sensors and covers require further protocol support.','Kachel und Verbindung werden getrennt gespeichert. Der aktuelle Transport unterstützt eine verifizierte Relaisverbindung. Die Prüfung unten ersetzt die bisherige Verbindung erst nach Erfolg. Sensoren und Rollläden benötigen weitere Protokollunterstützung.'],
 ['Login Shelly','Shelly username','Shelly-Benutzername'],['Hasło Shelly','Shelly password','Shelly-Passwort'],['Sprawdź i powiąż połączenie','Verify and bind connection','Verbindung prüfen und verknüpfen'],['Sprawdzanie tożsamości Shelly…','Verifying Shelly identity…','Shelly-Identität wird geprüft…'],['Sprawdź wynik na ekranie pilota.','Check the result on the remote.','Ergebnis auf der Fernbedienung prüfen.'],['Sprawdzanie trwa. Wynik pojawi się na pilocie.','Verification continues. The result will appear on the remote.','Die Prüfung läuft. Das Ergebnis erscheint auf der Fernbedienung.'],
 ['Jawne działania sceny. Wykonanie wymaga obsługi typu urządzenia przez moduł komunikacji.','Explicit scene actions. Execution requires transport support for the device type.','Explizite Szenenaktionen. Die Ausführung erfordert Transportunterstützung für den Gerätetyp.'],['Urządzenie','Device','Gerät'],['Działanie','Action','Aktion'],['Włącz','On','Ein'],['Wyłącz','Off','Aus'],['Otwórz','Open','Öffnen'],['Zamknij','Close','Schließen'],['Stop','Stop','Stopp'],['+ Działanie','+ Action','+ Aktion'],['Dodaj urządzenie; limit sceny to 8 działań.','Add a device; a scene supports up to 8 actions.','Gerät hinzufügen; eine Szene unterstützt bis zu 8 Aktionen.'],
 ['Pulpit startowy','Startup dashboard','Start-Dashboard'],['Tak','Yes','Ja'],['Nie','No','Nein'],['Przeciągaj karty albo użyj strzałek. Karty urządzeń zawsze zajmują pełną szerokość.','Drag tiles or use the arrows. Device tiles always use the full width.','Kacheln ziehen oder Pfeile verwenden. Gerätekacheln nutzen immer die volle Breite.'],['Brak obiektu','Missing object','Objekt fehlt'],['pełna szerokość','full width','volle Breite'],['½ szerokości','½ width','½ Breite'],['Ten pulpit jest pusty.','This dashboard is empty.','Dieses Dashboard ist leer.'],['+ Dodaj kartę','+ Add tile','+ Kachel hinzufügen'],['Limit: 24 karty na pulpit.','Limit: 24 tiles per dashboard.','Limit: 24 Kacheln pro Dashboard.'],['Najpierw dodaj pomieszczenie lub urządzenie.','Add a room or device first.','Zuerst einen Raum oder ein Gerät hinzufügen.'],
 ['Karta pulpitu','Dashboard tile','Dashboard-Kachel'],['Typ karty','Tile type','Kacheltyp'],['Scena','Scene','Szene'],['Najpierw dodaj wpis tego typu.','Add an entry of this type first.','Zuerst einen Eintrag dieses Typs hinzufügen.'],['Wyświetlany obiekt','Displayed object','Angezeigtes Objekt'],['Szerokość','Width','Breite'],['Połowa','Half','Halb'],['Pełna szerokość','Full width','Volle Breite'],['Wysokość','Height','Höhe'],['Standardowa','Standard','Standard'],['Duża','Large','Groß'],['Moc (W)','Power (W)','Leistung (W)'],['Napięcie (V)','Voltage (V)','Spannung (V)'],['Bateria','Battery','Batterie'],['Temperatura','Temperature','Temperatur'],['Wilgotność','Humidity','Luftfeuchtigkeit'],['Pozycja','Position','Position'],['Stan otwarcia','Contact state','Öffnungszustand'],['Dodatkowe parametry','Additional parameters','Weitere Parameter'],['← Wróć do pulpitu','← Back to dashboard','← Zurück zum Dashboard'],
 ['Język pilota','Remote language','Sprache der Fernbedienung'],['Język pilota jest wspólnym ustawieniem i wymaga zapisu w pilocie.','Remote language is a shared setting and requires saving to the remote.','Die Sprache der Fernbedienung ist eine gemeinsame Einstellung und muss gespeichert werden.'],['Język Web Panelu: preferencja tej przeglądarki, wybierana w nagłówku. Nie wymaga zapisu w pilocie.','Web Panel language: this browser’s preference, selected in the header. No save to the remote is required.','Sprache des Web Panels: Einstellung dieses Browsers, oben auswählbar. Kein Speichern auf der Fernbedienung erforderlich.'],['Motyw','Theme','Design'],['Grafit','Graphite','Graphit'],['Jasny','Light','Hell'],['RPC przez BLE: włączone','RPC over BLE: enabled','RPC über BLE: aktiviert'],['RPC przez BLE: wyłączone','RPC over BLE: disabled','RPC über BLE: deaktiviert'],['Kolor akcentu','Accent color','Akzentfarbe'],
 ['Aktualizacja OTA','OTA update','OTA-Update'],['Wybierz plik aplikacji BIN dla LCDWiki ES3C35P. Obraz USB łączący bootloader i partycje zostanie odrzucony. Konfiguracja i zdjęcia pozostają w oddzielnej partycji.','Choose an application BIN for LCDWiki ES3C35P. A merged USB image containing bootloader and partitions will be rejected. Configuration and photos remain in a separate partition.','Eine Anwendungs-BIN für LCDWiki ES3C35P wählen. Ein zusammengeführtes USB-Abbild mit Bootloader und Partitionen wird abgelehnt. Konfiguration und Fotos bleiben in einer separaten Partition.'],['Sprawdź i zainstaluj OTA','Verify and install OTA','OTA prüfen und installieren'],['Wysyłanie obrazu do nieaktywnej partycji…','Uploading image to the inactive partition…','Abbild wird in die inaktive Partition übertragen…'],['Obraz zweryfikowany. Uruchom ponownie pilot, aby go zastosować.','Image verified. Restart the remote to apply it.','Abbild geprüft. Zum Anwenden die Fernbedienung neu starten.'],['Uruchom ponownie','Restart','Neu starten'],['Pilot uruchamia się ponownie…','Remote is restarting…','Fernbedienung startet neu…'],['Najpierw zweryfikuj źródło pliku. Suma SHA256 sprawdza przesyłanie, a nie podpis wydawcy. To rozwojowy instalator lokalny.','Verify the file source first. SHA256 checks the transfer, not the publisher signature. This is a local development installer.','Zuerst die Dateiquelle prüfen. SHA256 prüft die Übertragung, nicht die Herausgebersignatur. Dies ist ein lokaler Entwicklungsinstaller.'],
 ['Bluetooth niedostępny','Bluetooth unavailable','Bluetooth nicht verfügbar'],['Skan przerwany','Scan cancelled','Scan abgebrochen'],['Wykryto — nie sparowano','Detected — not paired','Erkannt — nicht gekoppelt'],['Nie znaleziono urządzeń BLE','No BLE devices found','Keine BLE-Geräte gefunden'],['Shelly • RPC / konfiguracja','Shelly • RPC / configuration','Shelly • RPC / Konfiguration'],['Shelly BLU','Shelly BLU','Shelly BLU'],['Pozostałe reklamy','Other advertisements','Weitere Advertisements'],['Nieznany model','Unknown model','Unbekanntes Modell'],['Bluetooth LE • skan','Bluetooth LE • scan','Bluetooth LE • Scan'],['← Metody dodawania','← Adding methods','← Methoden zum Hinzufügen'],['Skan reklam BLE działa na pilocie. Nie paruje, nie przekazuje danych Wi-Fi i nie zapisuje fikcyjnych urządzeń. Nazwa i typ reklamy nie są uwierzytelnioną tożsamością.','BLE advertisements are scanned on the remote. This does not pair, transfer Wi-Fi credentials or save fictitious devices. Advertised names and types are not authenticated identities.','BLE-Advertisements werden auf der Fernbedienung gescannt. Dabei werden keine Geräte gekoppelt, WLAN-Daten übertragen oder fiktive Geräte gespeichert. Namen und Typen aus Advertisements sind keine authentifizierten Identitäten.'],['Szukaj BLE','Scan BLE','BLE suchen'],['Anuluj skan BLE','Cancel BLE scan','BLE-Scan abbrechen'],['Odczyt stanu skanera…','Reading scanner status…','Scannerstatus wird gelesen…'],['BLU może wymagać krótkiego naciśnięcia przycisku. Urządzenie Wi-Fi musi reklamować BLE; konfiguracja RPC może wymagać pairing i włączonego okna konfiguracji.','BLU may require a short button press. A Wi-Fi device must advertise BLE; RPC configuration may require pairing and an enabled configuration window.','BLU erfordert eventuell einen kurzen Tastendruck. Ein WLAN-Gerät muss BLE-Advertisements senden; die RPC-Konfiguration benötigt eventuell Kopplung und ein geöffnetes Konfigurationsfenster.'],['Odbieranie reklam: {0} / 20 s','Receiving advertisements: {0} / 20 s','Advertisements empfangen: {0} / 20 s'],['{0} dBm · Wykryto, nie sparowano','{0} dBm · Detected, not paired','{0} dBm · Erkannt, nicht gekoppelt'],
 ['Błąd logowania. Sprawdź dane.','Authentication failed. Check credentials.','Anmeldung fehlgeschlagen. Zugangsdaten prüfen.'],['Shelly offline lub brak Wi-Fi.','Shelly offline or Wi-Fi unavailable.','Shelly offline oder kein WLAN.'],['Tożsamość urządzenia się zmieniła. Wyszukaj ponownie.','Device identity changed. Scan again.','Geräteidentität geändert. Erneut suchen.'],['Limit logowań Shelly. Poczekaj przed ponowieniem.','Shelly authentication limit. Wait before retrying.','Shelly-Anmeldelimit erreicht. Vor erneutem Versuch warten.'],['Nie udało się zweryfikować urządzenia.','Device verification failed.','Geräteprüfung fehlgeschlagen.'],['Zapisz szkic konfiguracji przed dodaniem połączenia.','Save the configuration draft before adding a connection.','Konfigurationsentwurf vor dem Hinzufügen einer Verbindung speichern.'],['Wyszukiwanie urządzeń…','Searching for devices…','Geräte werden gesucht…'],['Sprawdzanie urządzenia…','Verifying device…','Gerät wird geprüft…'],['Odczyt możliwości urządzenia…','Reading device capabilities…','Gerätefunktionen werden gelesen…'],['Weryfikacja API: {0} / {1}','API verification: {0} / {1}','API-Prüfung: {0} / {1}'],['Najpierw połącz Wi-Fi.','Connect Wi-Fi first.','Zuerst WLAN verbinden.'],['Możliwości urządzenia potwierdzone odpowiedzią API.','Device capabilities confirmed by its API response.','Gerätefunktionen durch API-Antwort bestätigt.'],['Sprawdzanie tożsamości i zapisywanie połączenia…','Verifying identity and saving connection…','Identität wird geprüft und Verbindung gespeichert…'],['Połączenie, nazwa i pomieszczenie zapisane.','Connection, name and room saved.','Verbindung, Name und Raum gespeichert.'],['Połączenie zapisano. Konfiguracja pulpitu wymaga sprawdzenia.','Connection saved. Check the dashboard configuration.','Verbindung gespeichert. Dashboard-Konfiguration prüfen.'],['Nie potwierdzono zakończenia zapisu. Sprawdź stan na pilocie.','Save completion was not confirmed. Check the remote.','Speicherabschluss nicht bestätigt. Status auf der Fernbedienung prüfen.'],
 ['← Urządzenia','← Devices','← Geräte'],['Wyszukiwanie mDNS i weryfikacja API…','Searching mDNS and verifying API…','mDNS-Suche und API-Prüfung…'],['Sprawdzanie wybranego urządzenia…','Verifying selected device…','Ausgewähltes Gerät wird geprüft…'],['Wyszukaj urządzenia w tej samej sieci lokalnej. Lista zawiera wyłącznie tożsamości potwierdzone odpowiedzią API.','Search for devices on the same local network. The list includes only identities confirmed by API responses.','Geräte im selben lokalen Netzwerk suchen. Die Liste enthält nur durch API-Antworten bestätigte Identitäten.'],['Skanuj sieć Wi-Fi','Scan Wi-Fi network','WLAN-Netzwerk durchsuchen'],['Wi-Fi AP — niewdrożone','Wi-Fi AP — not implemented','WLAN-AP — nicht implementiert'],['Skanuj ponownie','Scan again','Erneut suchen'],['Nie znaleziono Shelly. Sprawdź Wi-Fi, mDNS i izolację klientów.','No Shelly found. Check Wi-Fi, mDNS and client isolation.','Kein Shelly gefunden. WLAN, mDNS und Client-Isolation prüfen.'],['Wymaga hasła','Password required','Passwort erforderlich'],['Bez hasła','No password','Ohne Passwort'],['Sprawdź urządzenie','Verify device','Gerät prüfen'],['Login','Username','Benutzername'],['Nazwa urządzenia','Device name','Gerätename'],['Kanał z odpowiedzi API','Channel from API response','Kanal aus API-Antwort'],['Brak obsługiwanego kanału','No supported channel','Kein unterstützter Kanal'],['Obecny transport obsługuje jedno aktywne połączenie przekaźnika. Zapis zastąpi wcześniejsze powiązanie dopiero po poprawnej weryfikacji.','The current transport supports one active relay connection. Saving replaces the previous binding only after successful verification.','Der aktuelle Transport unterstützt eine aktive Relaisverbindung. Speichern ersetzt die bisherige Verknüpfung erst nach erfolgreicher Prüfung.'],['Sprawdź i zapisz połączenie','Verify and save connection','Verbindung prüfen und speichern'],['Urządzenie rozpoznane. Sterowanie tym typem nie jest wdrożone.','Device recognized. Control of this type is not implemented.','Gerät erkannt. Steuerung dieses Typs ist nicht implementiert.'],['Wyszukaj Shelly w LAN','Find Shelly on LAN','Shelly im LAN suchen'],
 ['Zapisywanie konfiguracji…','Saving configuration…','Konfiguration wird gespeichert…'],['Zapis nie powiódł się. Zmiany pozostają w edytorze.','Save failed. Changes remain in the editor.','Speichern fehlgeschlagen. Änderungen bleiben im Editor.'],['Konfiguracja zmieniła się na pilocie. Wczytaj aktualną wersję.','Configuration changed on the remote. Reload the current version.','Konfiguration auf der Fernbedienung geändert. Aktuelle Version laden.'],['Zapisano. Ekran pilota korzysta z tej samej konfiguracji.','Saved. The remote uses the same configuration.','Gespeichert. Die Fernbedienung nutzt dieselbe Konfiguration.'],['Zapisano wcześniejszą wersję. Późniejsze zmiany pozostają niezapisane.','An earlier version was saved. Later changes remain unsaved.','Eine frühere Version wurde gespeichert. Spätere Änderungen bleiben ungespeichert.'],['Nie potwierdzono zapisu w wymaganym czasie. Zachowano zmiany w edytorze.','Save was not confirmed in time. Changes remain in the editor.','Speichern nicht rechtzeitig bestätigt. Änderungen bleiben im Editor.'],['● Połączono lokalnie','● Connected locally','● Lokal verbunden'],['Konfiguracja zmieniła się na pilocie. Wczytaj aktualną wersję, aby uniknąć nadpisania.','Configuration changed on the remote. Reload to avoid overwriting it.','Konfiguration auf der Fernbedienung geändert. Neu laden, um Überschreiben zu vermeiden.'],['Obraz pobrany z zadania LVGL pilota.','Frame received from the remote’s LVGL task.','Bild vom LVGL-Task der Fernbedienung empfangen.'],['Brak połączenia','Disconnected','Nicht verbunden'],['Wybierz JPG, PNG lub WebP do 15 MB.','Choose JPG, PNG or WebP up to 15 MB.','JPG, PNG oder WebP bis 15 MB wählen.'],['Zbyt duże zdjęcie.','Photo too large.','Foto zu groß.'],
 ['DIY Shelly Remote · Panel lokalny','DIY Shelly Remote · Local panel','DIY Shelly Remote · Lokales Panel'],['● Panel lokalny','● Local panel','● Lokales Panel'],['Konfiguracja','Configuration','Konfiguration'],['Konfiguracja zapisana w pilocie.','Configuration stored on the remote.','Konfiguration auf der Fernbedienung gespeichert.'],['Bez konta i bez chmury.','No account or cloud.','Ohne Konto und Cloud.'],['TWÓJ DOM · TWÓJ PILOT','YOUR HOME · YOUR REMOTE','DEIN ZUHAUSE · DEINE FERNBEDIENUNG'],['Łączenie…','Connecting…','Verbindung wird hergestellt…'],['Język panelu','Panel language','Panelsprache'],['Połącz tę przeglądarkę','Pair this browser','Diesen Browser koppeln'],['Na pilocie wybierz Ustawienia → Panel WWW → Wygeneruj kod parowania. Porównaj odcisk certyfikatu z ekranem pilota, a następnie wpisz jednorazowy kod. Kod wygasa po 2 minutach (maks. 5 prób).','On the remote, choose Settings → Web Panel → Generate pairing code. Compare the certificate fingerprint with the remote, then enter the one-time code. It expires after 2 minutes (max. 5 attempts).','Auf der Fernbedienung Einstellungen → Web Panel → Kopplungscode erzeugen wählen. Zertifikatsfingerabdruck mit der Fernbedienung vergleichen, dann den Einmalcode eingeben. Er gilt 2 Minuten (max. 5 Versuche).'],['Kod parowania','Pairing code','Kopplungscode'],['Połącz lokalnie','Connect locally','Lokal verbinden'],['Panel korzysta z HTTPS i indywidualnego certyfikatu pilota. Przy pierwszym połączeniu sprawdź jego zgodność z odciskiem na ekranie. Panel przeznaczony jest do sieci lokalnej.','The panel uses HTTPS and the remote’s individual certificate. On first connection, verify its fingerprint against the remote’s screen before accepting the self-signed certificate warning. This panel is for local networks.','Das Panel nutzt HTTPS und das individuelle Zertifikat der Fernbedienung. Beim ersten Verbinden den Fingerabdruck auf dem Display prüfen, bevor die Warnung zum selbstsignierten Zertifikat bestätigt wird. Das Panel ist für lokale Netzwerke bestimmt.'],['Ekran pilota','Remote screen','Bildschirm der Fernbedienung'],['Rzeczywisty obraz LVGL · 320 × 480','Actual LVGL frame · 320 × 480','Echtes LVGL-Bild · 320 × 480'],['Rzeczywisty podgląd ekranu pilota','Actual remote screen preview','Echte Bildschirmvorschau der Fernbedienung'],['Oczekiwanie na klatkę z pilota…','Waiting for a frame from the remote…','Warten auf ein Bild der Fernbedienung…'],['Rewizja','Revision','Revision'],['Wczytaj z pilota','Reload from remote','Von Fernbedienung laden'],['Zapisz w pilocie','Save to remote','Auf Fernbedienung speichern'],
 ['Enter the 16-character code shown on the remote','Wpisz 16-znakowy kod z ekranu pilota','Den 16-stelligen Code auf der Fernbedienung eingeben'],
 ['Restart pending; pairing not saved','Restart oczekuje; parowanie nie zostało zapisane','Neustart ausstehend; Kopplung nicht gespeichert'],['Pairing expired, locked or invalid; reopen pairing on the remote','Kod wygasł, został zablokowany lub jest niepoprawny; otwórz parowanie na pilocie','Code abgelaufen, gesperrt oder ungültig; Kopplung auf der Fernbedienung öffnen'],['Pairing could not be saved','Nie zapisano parowania','Kopplung konnte nicht gespeichert werden'],['Allow OTA on the remote first; approval expires after 2 minutes','Najpierw zezwól na OTA na pilocie; zgoda wygasa po 2 minutach','Zuerst OTA auf der Fernbedienung erlauben; Freigabe gilt 2 Minuten'],['Upload the application BIN, not a merged USB image','Wyślij BIN aplikacji, nie scalony obraz USB','Anwendungs-BIN senden, kein zusammengeführtes USB-Abbild'],['SHA256 required','Wymagana suma SHA256','SHA256 erforderlich'],['Restart pending; upload not started','Restart oczekuje; przesyłanie nie rozpoczęło się','Neustart ausstehend; Übertragung nicht gestartet'],['Image, board marker or checksum rejected; boot slot unchanged','Odrzucono obraz, znacznik płytki lub sumę; slot startowy bez zmian','Abbild, Board-Kennung oder Prüfsumme abgelehnt; Startslot unverändert'],['Boot slot not changed','Nie zmieniono slotu startowego','Startslot nicht geändert'],['Operation pending; retry restart after saving finishes','Operacja trwa; ponów restart po zakończeniu zapisu','Vorgang läuft; Neustart nach dem Speichern erneut versuchen'],['Restart service unavailable','Usługa restartu niedostępna','Neustartdienst nicht verfügbar'],
 ['Brak Wi-Fi lub wyszukiwanie w toku','No Wi-Fi or discovery is running','Kein WLAN oder Suche läuft'],['Urządzenie niedostępne lub operacja w toku','Device unavailable or operation running','Gerät nicht verfügbar oder Vorgang läuft'],['BLE niedostępne lub skan trwa','BLE unavailable or scan running','BLE nicht verfügbar oder Scan läuft'],['Local same-origin access only','Dozwolony tylko lokalny dostęp z tego samego origin','Nur lokaler Zugriff gleichen Ursprungs erlaubt'],['Startup incomplete or diagnostic mode; collect USB boot log','Start nieukończony lub tryb diagnostyczny; zbierz log startowy USB','Start unvollständig oder Diagnosemodus; USB-Startprotokoll erfassen'],['Pair this browser on the remote','Sparuj tę przeglądarkę na pilocie','Diesen Browser auf der Fernbedienung koppeln'],['Invalid configuration or revision conflict; reload','Nieprawidłowa konfiguracja lub konflikt rewizji; wczytaj ponownie','Ungültige Konfiguration oder Revisionskonflikt; neu laden'],['Busy or revision conflict','Zajęty lub konflikt rewizji','Beschäftigt oder Revisionskonflikt'],['Invalid details or another device check is running','Nieprawidłowe dane lub trwa inne sprawdzanie urządzenia','Ungültige Angaben oder eine Geräteprüfung läuft'],['Expected a 320x180 RGB565 room photo','Wymagane zdjęcie pokoju 320×180 RGB565','Raumfoto 320×180 RGB565 erforderlich'],['Nie zapisano zdjęcia: suma kontrolna, pamięć lub limit 16 zdjęć. Zapisz usunięcie nieużywanych zdjęć i uruchom pilot ponownie, aby odzyskać miejsce.','Photo not saved: checksum, memory or 16-photo limit. Save removal of unused photos and restart the remote to reclaim space.','Foto nicht gespeichert: Prüfsumme, Speicher oder Limit von 16 Fotos. Entfernen ungenutzter Fotos speichern und Fernbedienung neu starten, um Platz freizugeben.'],['Unknown API route','Nieznana ścieżka API','Unbekannter API-Pfad'],
 ['Configuration changed; reload and retry adding the device','Konfiguracja zmieniła się. Wczytaj ją i ponów dodawanie urządzenia.','Konfiguration geändert. Neu laden und Gerät erneut hinzufügen.'],
 ['Workspace saved; private binding unconfirmed; add again','Konfiguracja zapisana; powiązanie niepotwierdzone; dodaj ponownie','Konfiguration gespeichert; Verbindung unbestätigt; erneut hinzufügen'],['Shelly verified; workspace unconfirmed; add again','Shelly zweryfikowane; zapis konfiguracji niepotwierdzony; dodaj ponownie','Shelly geprüft; Konfigurationsspeicherung unbestätigt; erneut hinzufügen'],['Bramka BLU • przekaźnik nieobsługiwany.','BLU gateway • relay control unsupported.','BLU-Gateway • Relaissteuerung nicht unterstützt.'],
 ['Checking Shelly identity…','Sprawdzanie tożsamości Shelly…','Shelly-Identität wird geprüft…'],['Shelly verified and saved','Shelly zweryfikowane i zapisane','Shelly geprüft und gespeichert'],['Authentication failed; check Shelly credentials','Błąd logowania; sprawdź dane Shelly','Anmeldung fehlgeschlagen; Shelly-Zugangsdaten prüfen'],['Shelly offline / Wi-Fi disconnected','Shelly offline / Wi-Fi rozłączone','Shelly offline / WLAN getrennt'],['Shelly check failed; previous binding retained','Weryfikacja Shelly nieudana; poprzednie powiązanie zachowane','Shelly-Prüfung fehlgeschlagen; bisherige Verknüpfung beibehalten']
].map(([key,en,de])=>[key,{pl:key,en,de}]));
/* Backend diagnostics use English canonical keys. Their PL entries are given
 * separately above, rather than translating arbitrary user-controlled text. */
const SERVER_MESSAGE_KEYS=new Set(["Battery below 20%; recharge the remote and renew OTA consent","Battery check unavailable; renew OTA consent and retry","OTA staging or restart busy; upload not started","Boot selection unconfirmed; OTA blocked until restart","Disable the active automation before changing its contacts, targets or temperature","Workspace saved; private binding unconfirmed; add again","Shelly verified; workspace unconfirmed; add again","Expected current BLE scan serial and index only","BLE wizard unavailable; refresh the scan and check the remote","AP handoff accepts no configuration or credentials","AP wizard unavailable; check the remote and home Wi-Fi","Unsupported AP handoff request",'Configuration changed; reload and retry adding the device','Failed to fetch','Load failed','NetworkError when attempting to fetch resource.','Enter the 16-character code shown on the remote','Restart pending; pairing not saved','Pairing expired, locked or invalid; reopen pairing on the remote','Pairing could not be saved','Allow OTA on the remote first; approval expires after 2 minutes','Upload the application BIN, not a merged USB image','SHA256 required','Restart pending; upload not started','Image, board marker or checksum rejected; boot slot unchanged','Boot slot not changed','Operation pending; retry restart after saving finishes','Restart service unavailable','Local same-origin access only','Startup incomplete or diagnostic mode; collect USB boot log','Pair this browser on the remote','Invalid configuration or revision conflict; reload','Busy or revision conflict','Invalid details or another device check is running','Expected a 320x180 RGB565 room photo','Unknown API route','Checking Shelly identity…','Shelly verified and saved','Authentication failed; check Shelly credentials','Shelly offline / Wi-Fi disconnected','Shelly check failed; previous binding retained']);
for(const key of SERVER_MESSAGE_KEYS){const row=WEB_MESSAGES.get(key);const pl=row.en;row.en=key;row.pl=pl;}
const uiTextBindings=new WeakMap(),uiAttributeBindings=new WeakMap();
const supportedLocale=value=>typeof value==='string'&&['pl','en','de'].includes(value)?value:null;
function initialWebLocale(){
 try{const stored=supportedLocale(localStorage.getItem('dsr.web.locale'));if(stored)return stored;}catch{}
 for(const language of [...(navigator.languages||[]),navigator.language]){const value=supportedLocale(String(language||'').toLowerCase().split('-')[0]);if(value)return value;}
 return 'pl';
}
let webLocale=initialWebLocale();
function m(key,...args){return {uiMessage:true,key,args,toString(){return translate(this);}};}
function translate(value){if(!value?.uiMessage)return String(value??'');const text=WEB_MESSAGES.get(value.key)?.[webLocale]??value.key;return text.replace(/\{(\d+)\}/g,(_,index)=>translate(value.args[Number(index)]));}
function uiText(value){const node=document.createTextNode(translate(value));if(value?.uiMessage)uiTextBindings.set(node,value);return node;}
function setText(node,value){node.replaceChildren(uiText(value));}
function setUIAttribute(node,name,value){let bindings=uiAttributeBindings.get(node);if(!bindings){bindings=new Map();uiAttributeBindings.set(node,bindings);}if(value?.uiMessage)bindings.set(name,value);else bindings.delete(name);node.setAttribute(name,translate(value));}
function updateWebLocale(value,persist=true){
 if(!supportedLocale(value))return;webLocale=value;
 if(persist)try{localStorage.setItem('dsr.web.locale',value);}catch{}
 document.documentElement.lang=value;
 const walker=document.createTreeWalker(document.documentElement,NodeFilter.SHOW_TEXT);for(let node=walker.nextNode();node;node=walker.nextNode()){const message=uiTextBindings.get(node);if(message)node.data=translate(message);}
 for(const node of document.querySelectorAll('*')){const bindings=uiAttributeBindings.get(node);if(bindings)for(const [name,message]of bindings)node.setAttribute(name,translate(message));}
 document.querySelector('#web-locale').value=value;if(window.dsrActionsLocale)window.dsrActionsLocale();backupPanel?.refreshLocale();
}
function failure(message){const error=Error(translate(message));if(message?.uiMessage)error.uiMessage=message;return error;}
function serverMessage(message){return WEB_MESSAGES.has(message)?m(message):message;}
const $=s=>document.querySelector(s), el=(tag,text,cls)=>{const n=document.createElement(tag);if(text!==undefined)n.append(uiText(text));if(cls)n.className=cls;return n;};
let config=null,page='dashboards',dirty=false,saving=false,conflict=false,dragId=null,editSerial=0;
const titles={automations:m('Automatyzacje'),dashboards:m('Pulpity'),rooms:m('Pomieszczenia'),devices:m('Urządzenia Shelly'),discovery:m('Dodaj urządzenie Shelly'),scenes:m('Sceny'),groups:m('Grupy'),activity:m('Aktywność'),appearance:m('Wygląd i język'),system:m('System i aktualizacje')};
const kinds={light:m('Oświetlenie'),relay:m('Przekaźnik'),plug:m('Gniazdko'),cover:m('Roleta'),contact:m('Czujnik otwarcia'),sensor:m('Czujnik')};
function notice(s){const n=$('#notice');setText(n,s);n.classList.toggle('automation-warning',!!n.textContent);}
function changed(){editSerial++;dirty=true;setText($('#dirty'),m('Niezapisane zmiany'));$('#save').disabled=saving||conflict;}
function button(text,fn,cls){const b=el('button',text,cls);b.type='button';if(text==='×')setUIAttribute(b,'aria-label',m('Usuń'));else if(text==='↑')setUIAttribute(b,'aria-label',m('Przesuń w górę'));else if(text==='↓')setUIAttribute(b,'aria-label',m('Przesuń w dół'));b.addEventListener('click',fn);return b;}
function field(parent,label,value,fn,type='text'){const l=el('label',label),i=el('input');i.type=type;i.value=value;i.addEventListener('input',()=>{fn(type==='number'?Number(i.value):i.value);changed();});l.append(i);parent.append(l);return i;}
function select(parent,label,value,options,fn){const l=el('label',label),s=el('select');setUIAttribute(s,'aria-label',label);for(const [v,t]of options){const o=el('option',t);o.value=v;s.append(o);}s.value=String(value);s.addEventListener('change',()=>{fn(s.value);changed();});l.append(s);parent.append(l);return s;}
function nextId(items){return Math.max(0,...items.map(x=>x.id))+1;}
async function request(path,options={}){const r=await fetch('/api/v1/'+path,{...options,headers:{'X-DSR-Request':'1',...(options.headers||{})}});if(r.status===401){$('#workspace').hidden=true;$('#pairing').hidden=false;throw failure(m('Parowanie wymagane'));}if(!r.ok){let e;try{e=await r.json();}catch{}const error=failure(e?.error?serverMessage(e.error):m('Błąd {0}',r.status));error.httpStatus=r.status;error.restartRequired=e?.restart_required===true;throw error;}return r;}
// A read owns the document, edit/view and explicit-load generation that
// existed when it began. Save/BIND/new reload must invalidate older reads.
let configGeneration=0,configReadSerial=0,statusReadSerial=0;
function configReadOwner(){return{generation:configGeneration,read:configReadSerial,document:config,revision:config?.revision,edits:editSerial,view:page,lan:lanGeneration,editor:$('#editor').firstElementChild,hidden:$('#workspace').hidden,saving};}
function configReadCurrent(owner){return owner.generation===configGeneration&&owner.read===configReadSerial&&owner.document===config&&owner.revision===config?.revision&&owner.edits===editSerial&&owner.view===page&&owner.lan===lanGeneration&&owner.editor===$('#editor').firstElementChild&&owner.hidden===$('#workspace').hidden&&owner.saving===saving;}
function showConfig(next){actionSession=null;automationSession=null;autoView=null;autoCache=null;configGeneration++;config=next;dirty=conflict=false;$('#workspace').hidden=false;$('#pairing').hidden=true;setText($('#revision'),config.revision);setText($('#dirty'),m('Wszystkie zmiany zapisane'));$('#save').disabled=true;render();notice('');}
async function load(){
 if(saving)return; // Never replace the submitted document before its exact ACK.
 ++configReadSerial;const owner=configReadOwner();
 try{const response=await request('config');if(!configReadCurrent(owner))return;const next=await response.json();if(!configReadCurrent(owner))return;showConfig(next);}
 catch(e){if(configReadCurrent(owner))notice(e.uiMessage||serverMessage(e.message));}
}
function box(title){const n=el('div',undefined,'box');if(title)n.append(el('h2',title));return n;}
function empty(parent,text){parent.append(el('p',text,'empty'));}
function remove(kind,id){if(kind==='devices'&&config.automations.some(a=>a.sensors.includes(id)||a.targets.includes(id))){notice(m('Najpierw usuń powiązania automatyzacji z urządzeniem.'));return;}const refs=[];for(const d of config.dashboards)for(const t of d.tiles)if(t.type===kind.slice(0,-1)&&t.ref===id)refs.push(t);if(kind==='devices'&&(config.scenes.some(s=>s.actions.some(a=>a.device===id))||config.groups.some(g=>g.members.includes(id)))){notice(m('Najpierw usuń działania scen odwołujące się do urządzenia.'));return;}if(kind==='rooms'&&(config.devices.some(d=>d.room===id)||config.groups.some(g=>g.room===id)||config.scenes.some(s=>s.room===id))){notice(m('Najpierw przenieś urządzenia z tego pomieszczenia.'));return;}if(refs.length){notice(m('Najpierw usuń powiązane karty z pulpitów.'));return;}if(kind==='dashboards'&&config.dashboards.length===1){notice(m('Zachowaj przynajmniej jeden pulpit.'));return;}config[kind]=config[kind].filter(x=>x.id!==id);if(actionSession?.kind===kind&&actionSession.item.id===id)actionSession=null;if(config.active_dashboard===id&&kind==='dashboards')config.active_dashboard=config.dashboards[0].id;changed();render();}
function list(kind,edit){const n=box(titles[kind]);for(const item of config[kind]){const row=el('div',undefined,'row'),txt=el('div',undefined,'grow');const heading=el('h3',item.name||(kind==='devices'?window.dsrRuntimeDeviceName?.(item.id)||'Shelly':''));if(kind==='devices')heading.dataset.deviceNameId=String(item.id);txt.append(heading);txt.append(el('p',kind==='devices'?(m('{0} · {1}',kinds[item.kind],item.host||m('Bez adresu'))):'#'+item.id));row.append(txt,button(m('Edytuj'),()=>edit(item)),button(m('Usuń'),()=>remove(kind,item.id),'danger'));n.append(row);}if(!config[kind].length)empty(n,m('Brak wpisów. Dodaj pierwszy.'));n.append(button(m('+ Dodaj'),()=>{const id=nextId(config[kind]);if(config[kind].length>=({dashboards:4,rooms:8,devices:16,scenes:8,groups:8}[kind]))return notice(m('Osiągnięto limit wpisów.'));const item={id,name:translate(kind==='dashboards'?m('Nowy pulpit'):kind==='rooms'?m('Nowe pomieszczenie'):kind==='devices'?m('Nowe urządzenie'):kind==='groups'?m('Nowa grupa'):m('Nowa scena')),...(kind==='dashboards'?{tiles:[]}:kind==='rooms'?{icon:0,photo:''}:kind==='devices'?{host:'',channel:0,room:0,kind:'light'}:kind==='groups'?{members:[]}:{actions:[]})};config[kind].push(item);changed();edit(item);},'primary'));return n;}
function editBase(kind,item){const target=$('#editor');target.replaceChildren();const n=box(m('Edycja: {0}',item.name));const form=el('div',undefined,'fields');field(form,m('Nazwa'),item.name,v=>item.name=v).maxLength=47;n.append(form);target.append(n);n.append(button(m('← Wróć do listy'),render));return {n,form};}
function editRoom(item){
 const {n,form}=editBase('rooms',item);
 select(form,m('Ikona'),item.icon,[[0,m('Salon')],[1,m('Kuchnia')],[2,m('Sypialnia')],[3,m('Łazienka')]],v=>item.icon=Number(v));
 n.append(el('p',item.photo?m('Zdjęcie jest zapisane w pilocie.'):m('Możesz użyć własnego zdjęcia.'),'muted'));
 const f=field(form,m('Zdjęcie pomieszczenia'),'',()=>{},'file');f.accept='image/jpeg,image/png,image/webp';
 f.addEventListener('change',async()=>{if(!f.files[0])return;try{await uploadPhoto(f.files[0],item);notice(m('Zdjęcie zoptymalizowane do 320 × 180. Zapisz konfigurację.'));}catch(e){notice(e.uiMessage||serverMessage(e.message));}});
 n.append(button(m('Usuń zdjęcie'),()=>{item.photo='';changed();editRoom(item);}));
}
function editDevice(item){
 const {form,n}=editBase('devices',item);
 n.append(button(m('Użyj nazwy z Shelly'),()=>{item.name='';changed();editDevice(item);}));
 select(form,m('Typ urządzenia'),item.kind,Object.entries(kinds),v=>item.kind=v);
 field(form,m('Adres IPv4 Shelly'),item.host,v=>item.host=v).maxLength=15;
 field(form,m('Kanał (0–15)'),item.channel,v=>item.channel=v,'number');
 select(form,m('Pomieszczenie'),item.room,[[0,m('Nieprzypisane')],...config.rooms.map(r=>[r.id,r.name])],v=>item.room=Number(v));
 n.append(el('p',m('Każde powiązanie zachowuje pozostałe urządzenia. Odczyty i polecenia są przypisane do wybranego kanału. Czujniki i rolety wymagają dalszej obsługi protokołu.'),'muted'));
 const loginLabel=el('label',m('Login Shelly')),login=el('input');login.value='admin';login.autocomplete='off';login.maxLength=32;loginLabel.append(login);
 const passLabel=el('label',m('Hasło Shelly')),password=el('input');password.type='password';password.autocomplete='off';password.maxLength=64;passLabel.append(password);form.append(loginLabel,passLabel);
 n.append(button(m('Sprawdź i powiąż połączenie'),async()=>{try{notice(m('Sprawdzanie tożsamości Shelly…'));await request('devices/bind',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({host:item.host,name:item.name,channel:item.channel,username:login.value,password:password.value})});password.value='';for(let i=0;i<60;i++){await new Promise(r=>setTimeout(r,500));const status=await(await request('status')).json();if(!status.device_busy){notice(serverMessage(status.device_message)||m('Sprawdź wynik na ekranie pilota.'));return;}}notice(m('Sprawdzanie trwa. Wynik pojawi się na pilocie.'));}catch(error){password.value='';notice(error.uiMessage||serverMessage(error.message));}},'primary'));
}
// Three steps: name, devices/commands, review. Appearance is optional.
let actionSession=null;
const actionNameValid=item=>item.name.trim().length>0&&Array.from(item.name).reduce((n,c)=>n+(c.codePointAt(0)<128?1:c.codePointAt(0)<2048?2:c.codePointAt(0)<65536?3:4),0)<=47;
const actionDeviceName=d=>d?.name||window.dsrRuntimeDeviceName?.(d?.id)||'Shelly';
const actionTrv=d=>actionSession?.runtime?.find(r=>r.layout_id===d?.id&&r.endpoint_kind===5&&r.blu?.trv);
const actionCommands=d=>actionTrv(d)?[['target','Ustaw temperaturę']]:d?.kind==='cover'?[['open','Otwórz'],['close','Zamknij'],['stop','Stop']]:[['on','Włącz'],['off','Wyłącz']];
const actionSupported=(d,group)=>['relay','plug',...(group?[]:['cover'])].includes(d?.kind)||(!group&&!!actionTrv(d));
const actionStepLabel=(d,a)=>a.command==='target'?m('Nastawa {0} °C',(a.value/2).toFixed(1)):m(actionCommands(d).find(c=>c[0]===a.command)?.[1]||a.command);
function openAction(kind,item,fresh=false,before=null,wasDirty=dirty){
 actionSession={owner:config,kind,item,fresh,baseline:fresh?null:structuredClone(item),before:before||structuredClone(config),wasDirty};editAction(kind,item,0);
}
window.dsrEditAction=(kind,id)=>{const item=config?.[kind]?.find(o=>o.id===id);if(item)openAction(kind,item);};
function editGroup(item){openAction('groups',item);}
function editScene(item){openAction('scenes',item);}
function actionDiscard(kind,item){
 if(saving)return notice(m('Poczekaj na zakończenie zapisu.'));
 const session=actionSession,leave=()=>{actionSession=null;render();};
 if(!session||session.owner!==config||session.item!==item)return leave();
 const different=session.fresh||JSON.stringify(canonical(item))!==JSON.stringify(canonical(session.baseline));if(!different)return leave();
 const overlay=el('div',undefined,'action-modal'),dialog=el('section',undefined,'box');dialog.setAttribute('role','dialog');dialog.setAttribute('aria-modal','true');
 dialog.append(el('h2',m('Odrzucić zmiany?')),el('p',m('Niezapisane zmiany tego formularza zostaną utracone.')));
 const controls=el('div',undefined,'row-buttons');controls.append(button(m('Zostań'),()=>overlay.remove()),button(m('Odrzuć'),()=>{
  const at=config[kind].indexOf(item);if(at>=0){if(session.fresh)config[kind].splice(at,1);else config[kind][at]=structuredClone(session.baseline);}
  editSerial++;const before={...session.before,revision:config.revision};dirty=session.wasDirty||JSON.stringify(canonical(config))!==JSON.stringify(canonical(before));setText($('#dirty'),m(dirty?'Niezapisane zmiany':'Wszystkie zmiany zapisane'));$('#save').disabled=!dirty||conflict;overlay.remove();leave();
 },'danger'));dialog.append(controls);overlay.append(dialog);document.body.append(overlay);controls.firstElementChild.focus();
}
function actionList(kind){
 const n=box(titles[kind]),tools=el('div',undefined,'action-list-tools'),searchLabel=el('label',m('Szukaj')),search=el('input'),roomLabel=el('label',m('Pomieszczenie')),room=el('select');search.type='search';searchLabel.append(search);roomLabel.append(room);
 for(const [id,name]of [[0,m('Cały dom')],...config.rooms.map(r=>[r.id,r.name])]){const option=el('option',name);option.value=id;room.append(option);}
 const filter=()=>window.dsrActionsFilter?.(search.value,Number(room.value));search.addEventListener('input',filter);room.addEventListener('change',filter);tools.append(searchLabel,roomLabel);n.append(tools);n.append(button(m(kind==='groups'?'+ Nowa grupa':'+ Nowa scena'),()=>{
  if(config[kind].length>=8)return notice(m('Osiągnięto limit wpisów.'));
  const before=structuredClone(config),wasDirty=dirty,item={id:nextId(config[kind]),name:'',...(kind==='groups'?{members:[]}:{actions:[]})};config[kind].push(item);changed();openAction(kind,item,true,before,wasDirty);
  },'primary'));if(kind==='scenes')n.append(button(m('Automatyzacje'),()=>{page='automations';render();}));return n;
}
function editAction(kind,item,step,view={}){
 document.body.classList.add('editing-action');
 const group=kind==='groups',target=$('#editor'),n=box(m(actionSession?.fresh?(group?'Nowa grupa':'Nowa scena'):(group?'Edytuj grupę':'Edytuj scenę')));target.replaceChildren(n);
 const track=el('ol',undefined,'action-steps');for(const [i,label]of ['Nazwa',group?'Urządzenia':'Polecenia','Zapis'].entries()){const marker=el('li',m(label));if(i===(step===3?view.returnStep||0:step))marker.setAttribute('aria-current','step');track.append(marker);}n.append(track);
 const body=el('div',undefined,'action-form');n.append(body);let next;
 const complete=()=>actionNameValid(item)&&(group?item.members.length:item.actions.length)>0;
 const rerender=()=>editAction(kind,item,step,view);
 if(step===0){
  const name=field(body,m(group?'Nazwa grupy':'Nazwa sceny'),item.name,v=>item.name=v);name.maxLength=47;setUIAttribute(name,'placeholder',m(group?'np. Lampy na parterze':'np. Dobranoc'));name.addEventListener('input',()=>next.disabled=!actionNameValid(item)||saving||conflict);
  body.append(el('p',m(group?'Wspólne włączanie i wyłączanie wybranych przekaźników i gniazdek.':'Jedno dotknięcie uruchomi wybrane polecenia.'),'muted'));
  body.append(button(m('Pokój i zdjęcie (opcjonalnie)'),()=>editAction(kind,item,3,{returnStep:0}),'action-optional'));
 }else if(step===3){
  select(body,m('Pomieszczenie (opcjonalnie)'),item.room||0,[[0,m('Cały dom')],...config.rooms.map(r=>[r.id,r.name])],v=>item.room=Number(v));
  body.append(el('h3',m('Zdjęcie (opcjonalnie)')));const upload=field(body,m('Własne zdjęcie'),'',()=>{},'file');upload.accept='image/jpeg,image/png,image/webp';
  upload.addEventListener('change',async()=>{if(!upload.files[0])return;const owner=config;const current=()=>config===owner&&target.firstElementChild===n&&actionSession?.item===item;try{if(await uploadPhoto(upload.files[0],item,current))notice(m('Zdjęcie zoptymalizowane do 320 × 180. Zapisz konfigurację.'));}catch(e){if(current())notice(e.uiMessage||serverMessage(e.message));}});
  select(body,m('Zdjęcie z pomieszczenia'),item.photo||'',[['',m('Bez zdjęcia')],...config.rooms.filter(r=>r.photo).map(r=>[r.photo,r.name])],v=>item.photo=v);
  body.append(button(m('Bez zdjęcia'),()=>{item.photo='';changed();rerender();}));if(!actionSession?.fresh)body.append(button(m('Usuń wpis'),()=>remove(kind,item.id),'danger'));
 }else if(step===2){
  const summary=el('section',undefined,'action-summary');summary.append(el('h3',item.name),el('p',config.rooms.find(r=>r.id===item.room)?.name||m('Cały dom')));body.append(summary);
  const list=el(group?'ul':'ol');for(const value of group?item.members:item.actions){const d=config.devices.find(d=>d.id===(group?value:value.device)),row=el('li');row.append(el('strong',actionDeviceName(d)));if(!group)row.append(el('span',actionStepLabel(d,value),'action-command'));list.append(row);}summary.append(list);
  body.append(button(m('Zmień nazwę i opcje'),()=>editAction(kind,item,0)));body.append(el('p',m('Sprawdź listę i zapisz.'),'muted'));
 }else if(group||view.selecting){
  const count=el('p',group?m('Wybrano: {0}',item.members.length):m('Wybierz urządzenie'));body.append(count);
   if(actionSession?.runtimeError){body.append(el('p',m('Nie można odczytać typów urządzeń.'),'runtime-error'),button(m('Odśwież'),()=>{actionSession.runtime=null;actionSession.loading=false;rerender();}));}
  if(!group&&actionSession&&!actionSession.runtime){
   body.append(el('p',m('Odczyt typów urządzeń…'),'muted'));const session=actionSession;session.waitingTypes={root:n,redraw:rerender};
   if(!session.loading){session.loading=true;request('devices/runtime').then(r=>r.json()).then(snapshot=>{
    if(actionSession!==session||config!==session.owner)return;session.runtime=Array.isArray(snapshot.devices)?snapshot.devices:[];session.runtimeError=!Array.isArray(snapshot.devices);if(session.waitingTypes?.root.isConnected)session.waitingTypes.redraw();
   }).catch(()=>{if(actionSession!==session||config!==session.owner)return;session.runtime=[];session.runtimeError=true;if(session.waitingTypes?.root.isConnected)session.waitingTypes.redraw();});}
  }else if(view.device){
   const d=config.devices.find(d=>d.id===view.device);body.append(el('h3',actionDeviceName(d)),el('p',m('Wybierz polecenie'),'muted'));
   const trv=actionTrv(d),old=item.actions[view.editIndex];let temperature;
   if(trv){const saved=old?.device===d.id&&old.command==='target'?old.value/2:trv.blu.trv.target_c;temperature=field(body,m('Temperatura zadana'),Number.isFinite(saved)&&saved>=4&&saved<=30?saved:21,()=>{},'number');temperature.min='4';temperature.max='30';temperature.step='0.5';}
   for(const [command,label]of actionCommands(d)){const pick=button(m(label),()=>{
    if(!actionSupported(d,false))return;const half=temperature?.valueAsNumber*2;if(command==='target'&&(!Number.isInteger(half)||half<8||half>60))return;
    const entry={device:d.id,command,...(command==='target'?{value:half}:{})};if(view.editIndex!==undefined)item.actions[view.editIndex]=entry;else if(item.actions.length<8)item.actions.push(entry);else return;changed();editAction(kind,item,1);
   },'action-command-choice');const enabled=()=>{const half=temperature?.valueAsNumber*2;pick.disabled=!actionSupported(d,false)||(command==='target'&&(!Number.isInteger(half)||half<8||half>60));};enabled();temperature?.addEventListener('input',enabled);body.append(pick);}
   body.append(button(m('Zmień urządzenie'),()=>editAction(kind,item,1,{selecting:true,editIndex:view.editIndex})));
  }else{
   const searchLabel=el('label',m('Szukaj')),search=el('input');search.type='search';searchLabel.append(search);body.append(searchLabel);const rooms=el('div',undefined,'action-rooms');body.append(rooms);
   const draw=()=>{rooms.replaceChildren();let shown=0;for(const room of [{id:0,name:translate(m('Nieprzypisane'))},...config.rooms]){
    const devices=config.devices.filter(d=>d.room===room.id&&(actionSupported(d,group)||(group&&item.members.includes(d.id)))&&actionDeviceName(d).toLocaleLowerCase().includes(search.value.toLocaleLowerCase()));if(!devices.length)continue;shown+=devices.length;
    const section=el('section',undefined,'action-room');section.append(el('h3',room.name));
    for(const d of devices){if(group){const label=el('label',actionDeviceName(d)),check=el('input');check.type='checkbox';check.checked=item.members.includes(d.id);check.addEventListener('change',()=>{if(check.checked&&!actionSupported(d,true)){check.checked=false;return;}item.members=check.checked?[...new Set([...item.members,d.id])]:item.members.filter(id=>id!==d.id);label.classList.toggle('selected',check.checked);setText(count,m('Wybrano: {0}',item.members.length));changed();next.disabled=!item.members.length;});label.classList.toggle('selected',check.checked);label.prepend(check);section.append(label);}else section.append(button(actionDeviceName(d),()=>editAction(kind,item,1,{selecting:true,device:d.id,editIndex:view.editIndex}),'action-device-choice'));}rooms.append(section);
   }if(!shown)empty(rooms,m(search.value?'Brak wyników. Zmień wyszukiwanie.':'Brak zgodnych urządzeń. Dodaj je na stronie Urządzenia.'));};search.addEventListener('input',draw);draw();
  }
 }else{
  if(!item.actions.length)body.append(el('h3',m('Co ma zrobić scena?')),el('p',m('Dodaj urządzenie i wybierz dla niego polecenie.'),'muted'));
  for(const [index,a]of item.actions.entries()){
   const row=el('section',undefined,'action-edit-row'),d=config.devices.find(d=>d.id===a.device);row.append(el('h3',`${index+1}. ${actionDeviceName(d)}`));const controls=el('div',undefined,'action-row-controls');
   controls.append(button(actionStepLabel(d,a),()=>editAction(kind,item,1,{selecting:true,device:a.device,editIndex:index}),'action-command-choice'));
   for(const delta of [-1,1]){const move=button(delta<0?'↑':'↓',()=>{const to=index+delta;if(to<0||to>=item.actions.length)return;item.actions.splice(to,0,item.actions.splice(index,1)[0]);changed();rerender();});move.disabled=index+delta<0||index+delta>=item.actions.length;controls.append(move);}
   controls.append(button('×',()=>{item.actions.splice(index,1);changed();rerender();},'danger'));row.append(controls);body.append(row);
  }
  const add=button(m('+ Dodaj polecenie'),()=>editAction(kind,item,1,{selecting:true}),'primary');add.disabled=item.actions.length>=8;body.append(add);
 }
 const footer=el('div',undefined,'onboarding-footer');footer.append(button(m(step===0?'Anuluj':'Wstecz'),()=>{if(step===0){actionDiscard(kind,item);return;}if(step===3){editAction(kind,item,view.returnStep||0);return;}if(view.device){editAction(kind,item,1,view.editIndex===undefined?{selecting:true}:{});return;}if(view.selecting){editAction(kind,item,1);return;}editAction(kind,item,step-1);}));
 next=button(m(step===3?'Gotowe':step===2?(group?'Zapisz grupę':'Zapisz scenę'):'Dalej'),async()=>{if(next.disabled)return;if(step===3){editAction(kind,item,view.returnStep||0);return;}if(step<2){editAction(kind,item,step+1);return;}await save();if(!dirty&&!saving&&target.firstElementChild===n){actionSession=null;render();}},'primary');const refresh=()=>{if(target.firstElementChild===n)next.disabled=saving||conflict||(step===0&&!actionNameValid(item))||(step===1&&(!!view.selecting||!(group?item.members.length:item.actions.length)))||(step===2&&!complete());};if(actionSession?.item===item)actionSession.refresh=refresh;refresh();footer.append(next);n.append(footer);
}
function move(d,id,delta){const at=d.tiles.findIndex(t=>t.id===id),to=at+delta;if(to<0||to>=d.tiles.length)return;d.tiles.splice(to,0,d.tiles.splice(at,1)[0]);changed();editDashboard(d);}
function editDashboard(d){const {n,form}=editBase('dashboards',d);select(form,m('Pulpit startowy'),config.active_dashboard===d.id?'yes':'no',[['yes',m('Tak')],['no',m('Nie')]],v=>{if(v==='yes')config.active_dashboard=d.id;});n.append(el('p',m('Przeciągaj karty albo użyj strzałek. Karty urządzeń zawsze zajmują pełną szerokość.'),'muted'));for(const t of d.tiles){const row=el('div',undefined,'tile');row.draggable=true;row.dataset.id=t.id;row.addEventListener('dragstart',()=>{dragId=t.id;row.classList.add('dragging');});row.addEventListener('dragend',()=>row.classList.remove('dragging'));row.addEventListener('dragover',e=>e.preventDefault());row.addEventListener('drop',e=>{e.preventDefault();const a=d.tiles.findIndex(x=>x.id===dragId),b=d.tiles.findIndex(x=>x.id===t.id);if(a>=0&&b>=0){d.tiles.splice(b,0,d.tiles.splice(a,1)[0]);changed();editDashboard(d);}});row.append(el('span','⠿','drag'));const target=config[t.type==='device'?'devices':t.type==='room'?'rooms':t.type==='scene'?'scenes':'groups'].find(x=>x.id===t.ref);const text=el('div',undefined,'grow');text.append(el('h3',target?.name||m('Brak obiektu')),el('p',m('{0} · {1}',m({device:'Urządzenie',room:'Pomieszczenie',scene:'Scena',group:'Grupa'}[t.type]),t.span===2?m('pełna szerokość'):m('½ szerokości')),'muted'));row.append(text,button('↑',()=>move(d,t.id,-1)),button('↓',()=>move(d,t.id,1)),button(m('Edytuj'),()=>editTile(d,t)),button('×',()=>{d.tiles=d.tiles.filter(x=>x.id!==t.id);changed();editDashboard(d);}));n.append(row);}if(!d.tiles.length)empty(n,m('Ten pulpit jest pusty.'));n.append(button(m('+ Dodaj kartę'),()=>{if(d.tiles.length>=24)return notice(m('Limit: 24 karty na pulpit.'));if(!config.devices.length&&!config.rooms.length&&!config.scenes.length&&!config.groups.length)return notice(m('Najpierw dodaj pomieszczenie lub urządzenie.'));const type=config.devices.length?'device':config.rooms.length?'room':config.scenes.length?'scene':'group',a=config[type==='device'?'devices':type==='room'?'rooms':type==='scene'?'scenes':'groups'];const tile={id:nextId(d.tiles),type,ref:a[0].id,span:2,height:1,fields:255};d.tiles.push(tile);changed();editTile(d,tile);},'primary'));}
function editTile(d,t){
 const target=$('#editor');target.replaceChildren();const n=box(m('Karta pulpitu')),form=el('div',undefined,'fields');n.append(form);target.append(n);
 select(form,m('Typ karty'),t.type,[['device',m('Urządzenie')],['room',m('Pomieszczenie')],['scene',m('Scena')],['group',m('Grupa')]],v=>{
  const a=config[v==='device'?'devices':v==='room'?'rooms':v==='scene'?'scenes':'groups'];
  if(!a.length){notice(m('Najpierw dodaj wpis tego typu.'));editTile(d,t);return;}
  t.type=v;t.ref=a[0].id;if(v!=='room')t.span=2;editTile(d,t);
 });
 const a=config[t.type==='device'?'devices':t.type==='room'?'rooms':t.type==='scene'?'scenes':'groups'];
 select(form,m('Wyświetlany obiekt'),t.ref,a.map(x=>[x.id,x.name]),v=>t.ref=Number(v));
 select(form,m('Szerokość'),t.span,t.type==='room'?[[1,m('Połowa')],[2,m('Pełna szerokość')]]:[[2,m('Pełna szerokość')]],v=>t.span=Number(v));
 select(form,m('Wysokość'),t.height,[[1,m('Standardowa')],[2,m('Duża')]],v=>t.height=Number(v));
 const c=el('div',undefined,'checks');
 for(const [bit,name]of [[1,m('Moc (W)')],[2,m('Napięcie (V)')],[4,m('Bateria')],[8,m('Temperatura')],[16,m('Wilgotność')],[32,m('Pozycja')],[64,m('Stan otwarcia')],[128,m('Dodatkowe parametry')]]){
  const l=el('label',name),i=el('input');i.type='checkbox';i.checked=!!(t.fields&bit);
  i.addEventListener('change',()=>{t.fields=i.checked?t.fields|bit:t.fields&~bit;changed();});l.prepend(i);c.append(l);
 }
 n.append(c,button(m('← Wróć do pulpitu'),()=>editDashboard(d)));
}
function appearance(){const n=box(m('Wygląd i język')),f=el('div',undefined,'fields');n.append(f,el('p',m('Język pilota jest wspólnym ustawieniem i wymaga zapisu w pilocie.'),'muted'),el('p',m('Język Web Panelu: preferencja tej przeglądarki, wybierana w nagłówku. Nie wymaga zapisu w pilocie.'),'muted'));select(f,m('Język pilota'),config.language,[['pl','Polski'],['en','English'],['de','Deutsch']],v=>config.language=v);select(f,m('Motyw'),config.theme==='midnight'?'graphite':config.theme,[['graphite',m('Grafit')],['light',m('Jasny')],['cyber_blue','Cyber Blue']],v=>config.theme=v);field(f,m('Kolor akcentu'),'#'+config.accent.toString(16).padStart(6,'0'),v=>config.accent=parseInt(v.slice(1),16),'color');return n;}
// A pending upload survives navigation; no view may submit it twice.
const localOta={phase:'idle',message:null,restarting:false,filename:'',size:0};
let localOtaUpdate=()=>{},backupPanel=null;
function system(){
 const n=box(m('Aktualizacja OTA'));n.classList.add('ota-flow');
 n.append(el('p',m('Wybierz plik aplikacji BIN dla LCDWiki ES3C35P. Obraz USB łączący bootloader i partycje zostanie odrzucony. Konfiguracja i zdjęcia pozostają w oddzielnej partycji.')));
 n.append(el('p',m("Przed wysłaniem potwierdź stabilne USB na ekranie pilota. Zgoda jest jednorazowa i ważna 120 s. Znany poziom baterii poniżej 20% blokuje instalację."),'automation-warning'));
 const label=el('label',m('Plik aplikacji BIN')),f=el('input');f.type='file';f.accept='.bin';setUIAttribute(f,'aria-label',m('Plik aplikacji BIN'));label.append(f);n.append(label);
 const fileSummary=el('p',undefined,'ota-file-summary');n.append(fileSummary);
 const status=el('p',undefined,'ota-message');status.setAttribute('role','status');status.setAttribute('aria-live','polite');
 const progress=el('progress',undefined,'ota-progress');setUIAttribute(progress,'aria-label',m('Wysyłanie i weryfikacja obrazu…'));progress.hidden=true;
 const install=button(m('Instaluj'),async()=>{
  if(['pending','staged','uncertain','unconfirmed'].includes(localOta.phase)||!f.files[0])return;
  const file=f.files[0];let sent=false;
  if(!/\.bin$/i.test(file.name)||file.size<512||file.size>0x600000){localOta.phase='error';localOta.message=m('Wybierz BIN aplikacji o rozmiarze od 512 B do 6 MiB.');localOtaUpdate();return;}
  localOta.phase='pending';localOta.filename=file.name;localOta.size=file.size;localOta.message=m('Obliczanie sumy SHA-256…');localOtaUpdate();
  try{
   const data=await file.arrayBuffer();const hash=sha256(new Uint8Array(data));
   localOta.message=m('Wysyłanie i weryfikacja obrazu…');localOtaUpdate();sent=true;
   const r=await request('ota',{method:'POST',headers:{'Content-Type':'application/octet-stream','X-SHA256':hash},body:data});
   const result=await r.json();if(result.installed!==true)throw failure(m('Nie potwierdzono wyniku aktualizacji. Sprawdź pilot przed ponowieniem.'));
   localOta.phase='staged';localOta.message=m('Obraz zweryfikowany. Uruchom ponownie pilot, aby go zastosować.');
  }catch(e){
   localOta.phase=e.restartRequired?'uncertain':sent&&(!e.httpStatus||e.httpStatus>=500)?'unconfirmed':'error';
   localOta.message=localOta.phase==='unconfirmed'?m('Nie potwierdzono wyniku aktualizacji. Sprawdź pilot przed ponowieniem.'):e.uiMessage||serverMessage(e.message);
  }
  localOtaUpdate();
 },'primary ota-install');
 const restart=button(m('Uruchom ponownie'),async()=>{
  if(localOta.restarting||!['staged','uncertain'].includes(localOta.phase))return;
  localOta.restarting=true;localOtaUpdate();
  try{await request('restart',{method:'POST'});localOta.message=m('Pilot uruchamia się ponownie…');}
  catch(e){localOta.restarting=!e.httpStatus||e.httpStatus>=500;localOta.message=localOta.restarting?m('Nie potwierdzono restartu. Sprawdź pilot.'):(e.uiMessage||serverMessage(e.message));}
  localOtaUpdate();
 },'ota-restart');
 const actions=el('div',undefined,'actions');actions.append(install,restart);n.append(status,progress,actions);
 n.append(el('p',m('Najpierw zweryfikuj źródło pliku. Suma SHA256 sprawdza przesyłanie, a nie podpis wydawcy. To rozwojowy instalator lokalny.'),'automation-warning'));
 const update=()=>{
  const locked=['pending','staged','uncertain','unconfirmed'].includes(localOta.phase);
  install.disabled=locked||!f.files[0];f.disabled=locked;
  label.hidden=locked;fileSummary.hidden=!locked;
  if(locked)setText(fileSummary,m('Plik: {0} ({1} B)',localOta.filename,localOta.size));
  restart.hidden=!['staged','uncertain'].includes(localOta.phase);restart.disabled=localOta.restarting;
  progress.hidden=localOta.phase!=='pending';status.dataset.state=localOta.phase;
  status.classList.toggle('automation-warning',['error','uncertain','unconfirmed'].includes(localOta.phase));
  setText(status,localOta.message||m('Wybierz plik aplikacji BIN.'));
 };
 localOtaUpdate=()=>{if(n.isConnected)update();};f.addEventListener('change',update);update();
 if(window.createBackupPanel){backupPanel=window.createBackupPanel({request,translate:variants=>variants[webLocale==='de'?2:webLocale==='en'?1:0]});n.append(backupPanel.root);}return n;
}
/* Selection is local state. Only an explicit Next can enter a real method. */
let onboardingChoice=null,onboardingStarting=false,onboardingNext=null,onboardingCards=[],remoteReachable=true;
// Every async LAN owner keeps this epoch across all awaits. Reopening or
// leaving invalidates it before a late response can touch the current view.
function onboardingCurrent(generation){return page==='discovery'&&generation===lanGeneration;}
function invalidateOnboarding(){lanGeneration++;onboardingStarting=false;}
function onboardingSteps(parent,step){
 const head=el('div',undefined,'onboarding-steps');head.append(el('p',m('Krok {0} z 3',step),'muted'));
 const track=el('div',undefined,'onboarding-track');for(let i=0;i<3;i++)track.append(el('span',undefined,i<step?'reached':''));head.append(track);parent.append(head);
}
function updateOnboardingChoice(){
 if(page!=='discovery'||lanMode!=='start'||!onboardingNext)return;
 for(const row of onboardingCards){const selected=onboardingChoice===row.id,available=row.id==='ble'||remoteReachable;
  row.node.disabled=!available;row.node.classList.toggle('selected',selected);row.node.setAttribute('aria-pressed',String(selected));row.radio.textContent=selected?'●':'○';setText(row.detail,m(available?row.description:'Brak połączenia z pilotem. Odśwież panel.'));
 }
 onboardingNext.disabled=onboardingStarting||!onboardingChoice||(onboardingChoice!=='ble'&&!remoteReachable);
}
function onboardingMethods(parent){
 onboardingCards=[];onboardingSteps(parent,1);
 const list=el('div',undefined,'onboarding-methods');
 for(const [id,name,description,symbol]of [['ble','Bluetooth LE • skan','Reklamy BLE • także bez Wi-Fi','BLE'],['ap','Wi-Fi AP • kreator na pilocie','Otwarty punkt dostępowy Shelly','AP'],['lan','Sieć lokalna','Urządzenia w domowej sieci','LAN']]){
  const node=button(undefined,()=>{if(node.disabled||onboardingStarting)return;onboardingChoice=id;updateOnboardingChoice();},'onboarding-method');setUIAttribute(node,'aria-label',m(name));
  const body=el('span',undefined,'grow'),detail=el('span',m(description),'muted'),radio=el('span','○','onboarding-radio');body.append(el('strong',m(name)),detail);node.append(el('span',symbol,'onboarding-symbol'),body,radio);list.append(node);onboardingCards.push({id,node,detail,radio,description});
 }
 const advanced=button(m('Dodaj ręcznie (IP)'),()=>{if(advanced.disabled||onboardingStarting)return;onboardingChoice='manual';updateOnboardingChoice();},'onboarding-advanced');setUIAttribute(advanced,'aria-label',m('Dodaj ręcznie (IP)'));
 const advancedRadio=el('span','○','onboarding-radio');advanced.append(advancedRadio);list.append(advanced);onboardingCards.push({id:'manual',node:advanced,detail:el('span'),radio:advancedRadio,description:'Adres IP • zaawansowane'});parent.append(list);
 const footer=el('div',undefined,'onboarding-footer');onboardingNext=button(m('Dalej'),async()=>{
  if(onboardingStarting||!onboardingChoice||page!=='discovery'||lanMode!=='start'||(onboardingChoice!=='ble'&&!remoteReachable))return;
  onboardingStarting=true;updateOnboardingChoice();const method=onboardingChoice,generation=++lanGeneration;
  try{if(method==='lan')await scanLan(generation);else{lanCredentials=null;lanMode=method;render();}}
  finally{if(onboardingCurrent(generation)){onboardingStarting=false;updateOnboardingChoice();}}
 },'primary');footer.append(button(m('Anuluj'),()=>{invalidateOnboarding();onboardingChoice=null;onboardingCards=[];onboardingNext=null;page='devices';render();}),onboardingNext);parent.append(footer);updateOnboardingChoice();
}
let manualDraft={host:'',name:'',username:'admin',password:'',channel:0,room:0,family:''};
function manualOnboarding(parent){
 onboardingSteps(parent,3);parent.append(el('p',m('Zaawansowane: adres, tożsamość API i stan zostaną sprawdzone przed dodaniem.')));
 const form=el('div',undefined,'fields');parent.append(form);
 const host=lanInput(form,m('Adres IPv4 Shelly'),manualDraft.host),name=lanInput(form,m('Nazwa urządzenia'),manualDraft.name),room=lanSelect(form,m('Pomieszczenie'),[[0,m('Nieprzypisane')],...config.rooms.map(r=>[r.id,r.name])]),channel=lanInput(form,m('Kanał 0–15'),manualDraft.channel,'number'),user=lanInput(form,m('Login'),manualDraft.username),password=lanInput(form,m('Hasło Shelly'),manualDraft.password,'password');
 const family=lanSelect(form,m('Rodzina API'),[['','Switch / Relay'],['cover','Cover Gen2+'],['light','Light Gen2+'],['rgb','RGB Plus RGBW PM'],['sensor','H&T Gen3 • read only']]);family.value=manualDraft.family;
 room.value=String(manualDraft.room);channel.min=0;channel.max=15;channel.step=1;
 const inputs={host,name,room,channel,username:user,password,family};for(const [key,input]of Object.entries(inputs))input.addEventListener('input',()=>manualDraft[key]=input.value);
 const apply=button(m('Sprawdź i zapisz połączenie'),()=>{
  if(apply.disabled)return;
  if(!host.value.trim()||!name.value.trim()||!Number.isInteger(Number(channel.value))||Number(channel.value)<0||Number(channel.value)>15)return notice(m('Sprawdź adres, nazwę i kanał 0–15.'));
  lanSelected={host:host.value.trim(),id:'',model:m('Adres IP • zaawansowane'),name:name.value};lanCredentials={username:user.value,password:password.value};bindLan(name.value,room.value,family.value?`${family.value}:${channel.value}`:channel.value,apply,true);
 },'primary');const footer=el('div',undefined,'onboarding-footer');footer.append(apply);parent.append(footer);
}
let lanBound=[];
const alreadyBound=(d,channel)=>lanBound.some(b=>b.device_id===d.id&&b.generation===(d.generation===1?1:2)&&(channel===undefined||b.channel===channel));
let lanMode='start',lanSelected=null,lanCredentials=null,lanSnapshot=null,lanGeneration=0;
let bleGeneration=0,bleEpoch=0,blePosting=false,bleTimer,blePollSerial=0;
function scheduleBle(generation,delay){clearTimeout(bleTimer);bleTimer=setTimeout(()=>pollBle(generation),delay);}
async function pollBle(generation){
 if(page!=='discovery'||lanMode!=='ble'||generation!==bleGeneration)return;
 try{
  const epoch=bleEpoch,token=++blePollSerial,s=await(await request('ble')).json();if(generation!==bleGeneration||epoch!==bleEpoch||token!==blePollSerial)return;
  const status=$('#ble-status'),list=$('#ble-results');if(!status||!list)return;
  setText(status,s.busy?m('Odbieranie reklam: {0} / 20 s',Math.floor(s.elapsed_ms/1000)) : s.phase===5?m('Bluetooth niedostępny'):s.phase===4?m('Skan przerwany'):s.devices.length?m('Wykryto — nie sparowano'):m('Nie znaleziono urządzeń BLE'));
  if(s.handoff===2||s.handoff===3)setText(status,m('Dokończ kreator na LCD pilota. Obie zgody wymagają dotknięcia ekranu; samo żądanie WWW nie wysyła Wi-Fi.'));
  $('#ble-start').disabled=s.busy||blePosting||s.handoff===2||s.handoff===3;$('#ble-cancel').disabled=!s.busy;list.replaceChildren();
  for(const [kind,heading] of [[0,m('Shelly • RPC / konfiguracja')],[1,m('Shelly BLU')],[2,m('Pozostałe reklamy')]]){
   const rows=s.devices.map((d,index)=>({...d,index})).filter(d=>d.kind===kind);if(!rows.length)continue;list.append(el('h3',heading));
   for(const d of rows){const card=el('div',undefined,'row');card.append(el('strong',d.model||m(d.kind===0?'Shelly • reklamowane RPC':'Nierozpoznane urządzenie')),el('span',m('Reklama: {0}',d.name||m('bez nazwy'))),...(d.rpc_enabled===undefined?[]:[el('span',m(d.rpc_enabled?'RPC przez BLE: włączone':'RPC przez BLE: wyłączone'))]),el('span',m('{0} dBm · Wykryto, nie sparowano',d.rssi)));if(d.kind===0){const open=button(m('Otwórz kreator BLE na pilocie'),async()=>{
     if(open.disabled||blePosting||generation!==bleGeneration)return;
     blePosting=true;bleEpoch++;open.disabled=true;
     try{const result=await(await request('ble/open',{method:'POST',body:JSON.stringify({serial:s.serial,index:d.index})})).json();if(generation===bleGeneration&&result.queued)setText(status,m('Dokończ kreator na LCD pilota. Obie zgody wymagają dotknięcia ekranu; samo żądanie WWW nie wysyła Wi-Fi.'));}
     catch(e){if(generation===bleGeneration)notice(e.uiMessage||serverMessage(e.message));}
     finally{blePosting=false;bleEpoch++;if(generation===bleGeneration)scheduleBle(generation,0);}
    });open.disabled=blePosting||s.busy||![1,4].includes(s.handoff)||d.age_ms>30000;card.append(open);}
    list.append(card);}
  }
 }catch(e){notice(e.uiMessage||serverMessage(e.message));}
 if(generation===bleGeneration)scheduleBle(generation,800);
}
function bleView(){
 const n=box(m('Bluetooth LE • skan')),generation=++bleGeneration;
 n.append(button(m('← Metody dodawania'),()=>{bleGeneration++;lanMode='start';render();}),el('p',m('Skan działa na pilocie. Dla reklamowanego RPC możesz otworzyć prawdziwy kreator na LCD. Dopiero dwie fizyczne zgody pozwalają sprawdzić urządzenie i przekazać Wi-Fi. Reklama nie dowodzi autentyczności.')));
 onboardingSteps(n,2);
 const start=button(m('Szukaj BLE'),async()=>{try{await request('ble/scan',{method:'POST'});}catch(e){notice(e.uiMessage||serverMessage(e.message));}},'primary');start.id='ble-start';
 const cancel=button(m('Anuluj skan BLE'),async()=>{try{await request('ble/cancel',{method:'POST'});}catch(e){notice(e.uiMessage||serverMessage(e.message));}});cancel.id='ble-cancel';
 const status=el('p',m('Odczyt stanu skanera…'));status.id='ble-status';const list=el('div');list.id='ble-results';n.append(start,cancel,status,list,el('p',m('Tylko obsługiwany Gen2+ lub H&T Gen3 w otwartym setup, bez bondingu i z HTTP w LAN. Fabryczne HTTPS 2.0, BLU, BTHome i urządzenia zabezpieczone nie są obsługiwane tym kreatorem. Pilot pozostaje w LAN.'),'muted'));scheduleBle(generation,0);return n;
}
let apGeneration=0;
function apView(){
 const n=box(m('Wi-Fi AP • kreator na pilocie')),generation=++apGeneration;
 n.append(button(m('← Metody dodawania'),()=>{apGeneration++;lanMode='start';render();}),el('p',m('Kreator AP wymaga dwóch potwierdzeń na ekranie pilota. Najpierw zatwierdzisz opuszczenie domowej sieci, a potem przekazanie danych Wi-Fi wybranemu Shelly.')),el('p',m('Podczas połączenia z AP panel utraci dostęp do pilota. Dokończ kroki na pilocie. Po jego powrocie do domu połącz się ponownie z panelem.'),'muted'));
 onboardingSteps(n,2);
 const status=el('p',m('Odczyt dostępności kreatora…'));status.id='ap-status';
 let posting=false,serial=0;
 const current=()=>page==='discovery'&&lanMode==='ap'&&generation===apGeneration;
 const open=button(m('Otwórz kreator na pilocie'),async()=>{
  if(open.disabled||posting||!current())return;
  posting=true;open.disabled=true;serial++;setText(status,m('Żądanie oczekuje na ekran pilota. Nie przekazano danych Wi-Fi.'));
  const controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),5000);
  try{const result=await(await request('ap/open',{method:'POST',signal:controller.signal})).json();if(!result.queued)throw Error('invalid handoff response');}
  catch{if(current())setText(status,m('Nie potwierdzono otwarcia. Sprawdź ekran pilota; żądanie nie będzie ponawiane automatycznie.'));}
  finally{clearTimeout(timeout);serial++;posting=false;}
 },'primary');open.id='ap-open';open.disabled=true;n.append(open,status);
 async function poll(){
  if(!current())return;
  if(posting){setTimeout(poll,800);return;}
  const token=serial,controller=new AbortController(),timeout=setTimeout(()=>controller.abort(),5000);
  try{
   const result=await(await request('ap',{signal:controller.signal})).json();
   if(!current()||token!==serial||posting)return;
   const texts=['Kreator niedostępny. Sprawdź ekran pilota i domowe Wi-Fi.','Kreator dostępny. Otwórz go i kontynuuj na ekranie pilota.','Żądanie oczekuje na ekran pilota. Nie przekazano danych Wi-Fi.','Kreator działa na pilocie. Potwierdzaj kolejne kroki na jego ekranie.','Żądanie wygasło. Sprawdź ekran pilota i spróbuj ponownie.'];
   const state=Number.isInteger(result.handoff)&&result.handoff>=0&&result.handoff<texts.length?result.handoff:0;
   setText(status,m(texts[state]));open.disabled=state!==1&&state!==4;
  }catch{if(current()&&token===serial&&!posting){open.disabled=true;setText(status,m('Brak połączenia z pilotem. Dokończ na jego ekranie; panel wróci po przywróceniu LAN.'));}}
  finally{clearTimeout(timeout);if(current())setTimeout(poll,800);}
 }
 setTimeout(poll,0);return n;
}
const lanError=code=>({11:m('Konfiguracja zmieniła się. Wczytaj ją i ponów dodawanie urządzenia.'),4:m('Błąd logowania. Sprawdź dane.'),5:m('Shelly offline lub brak Wi-Fi.'),9:m('Tożsamość urządzenia się zmieniła. Wyszukaj ponownie.'),10:m('Limit logowań Shelly. Poczekaj przed ponowieniem.')}[code]||m('Nie udało się zweryfikować urządzenia.'));
function lanInput(parent,label,value='',type='text'){
 const l=el('label',label),i=el('input');i.type=type;i.value=value;setUIAttribute(i,'aria-label',label);i.maxLength=type==='password'?64:47;l.append(i);parent.append(l);return i;
}
function lanSelect(parent,label,items){const l=el('label',label),s=el('select');setUIAttribute(s,'aria-label',label);for(const [value,name]of items){const o=el('option',name);o.value=value;s.append(o);}l.append(s);parent.append(l);return s;}
function openDiscovery(){if(dirty||saving)return notice(m('Zapisz szkic konfiguracji przed dodaniem połączenia.'));invalidateOnboarding();onboardingChoice=null;manualDraft={host:'',name:'',username:'admin',password:'',channel:0,room:0,family:''};page='discovery';lanMode='start';lanCredentials=null;render();}
async function scanLan(generation){
 if(!Number.isSafeInteger(generation))generation=++lanGeneration; // DOM click passes an Event.
 if(!onboardingCurrent(generation))return;
 try{
  const response=await request('devices/runtime');if(!onboardingCurrent(generation))return;
  const snapshot=await response.json();if(!onboardingCurrent(generation))return;
  lanBound=snapshot.devices||[];await request('discovery/scan',{method:'POST'});if(!onboardingCurrent(generation))return;
  notice(m('Wyszukiwanie urządzeń…'));lanMode='scan';render();pollLan(generation);
 }catch(e){if(onboardingCurrent(generation))notice(e.uiMessage||serverMessage(e.message));}
}
async function inspectLan(device,credentials){
 if(page!=='discovery')return;const generation=++lanGeneration;
 lanSelected=device;
 if(device.auth_required&&!credentials){lanMode='credentials';render();return;}
 lanCredentials=credentials||{username:'',password:''};
 try{await request('discovery/inspect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({host:device.host,id:device.id,...lanCredentials})});if(!onboardingCurrent(generation))return;notice(m('Sprawdzanie urządzenia…'));lanMode='inspect';render();pollLan(generation);}catch(e){if(onboardingCurrent(generation))notice(e.uiMessage||serverMessage(e.message));}
}
async function pollLan(generation){
 if(!onboardingCurrent(generation))return;
 try{
  const response=await request('discovery');if(!onboardingCurrent(generation))return;
  const s=await response.json();if(!onboardingCurrent(generation))return;lanSnapshot=s;
  if(s.phase===1||s.phase===5){const progress=$('#lan-progress');if(progress)setText(progress,s.phase===1?m('Weryfikacja API: {0} / {1}',s.probed,s.total):m('Odczyt możliwości urządzenia…'));setTimeout(()=>pollLan(generation),800);return;}
  if(lanMode==='scan'){lanMode='results';render();if(s.phase===4)notice(m('Najpierw połącz Wi-Fi.'));}
  else if(lanMode==='inspect'){
   if(s.phase===6&&s.selected?.id===lanSelected.id&&s.selected.host===lanSelected.host&&s.selected.generation===lanSelected.generation){lanMode='components';render();notice(m('Możliwości urządzenia potwierdzone odpowiedzią API.'));}
   else{lanMode='credentials';render();notice(lanError(s.result));}
  }
 }catch(e){if(onboardingCurrent(generation))notice(e.uiMessage||serverMessage(e.message));}
}
async function bindLan(name,room,channel,button,manual=false){
 if(button.disabled||page!=='discovery')return;const generation=++lanGeneration;
 button.disabled=true;const cover=String(channel).startsWith('cover:'),light=String(channel).startsWith('light:'),rgb=String(channel).startsWith('rgb:'),sensor=String(channel).startsWith('sensor:');const body={host:lanSelected.host,...(lanSelected.id?{expected_id:lanSelected.id}:{}),name,room:Number(room),channel:Number(cover||light?String(channel).slice(6):rgb?String(channel).slice(4):sensor?String(channel).slice(7):channel),...lanCredentials};if(cover||light||rgb||sensor)body.endpoint_kind=sensor?4:cover?1:rgb?3:2;
 try{
  await request('devices/bind',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});if(!onboardingCurrent(generation))return;notice(m('Sprawdzanie tożsamości i zapisywanie połączenia…'));
  for(let i=0;i<100;i++){
   await new Promise(r=>setTimeout(r,250));if(!onboardingCurrent(generation))return;
   const response=await request('status');if(!onboardingCurrent(generation))return;
   const s=await response.json();if(!onboardingCurrent(generation))return;
   if(s.device_busy||s.busy)continue;
   if(s.device_result!==0){if(s.device_result===4&&!manual){lanMode='credentials';render();}throw failure(lanError(s.device_result));}
   const configResponse=await request('config');if(!onboardingCurrent(generation))return;
   const current=await configResponse.json();if(!onboardingCurrent(generation))return;const saved=current.devices.find(d=>d.host===body.host&&d.channel===body.channel&&(d.kind==='cover')===(body.endpoint_kind===1)&&(d.kind==='light')===([2,3].includes(body.endpoint_kind))&&(d.kind==='sensor')===(body.endpoint_kind===4)&&d.room===body.room&&d.name===body.name);
   page='devices';lanCredentials=null;invalidateOnboarding();showConfig(current);notice(saved?m('Połączenie, nazwa i pomieszczenie zapisane.'):m('Połączenie zapisano. Konfiguracja pulpitu wymaga sprawdzenia.'));return;
  }
  throw failure(m('Nie potwierdzono zakończenia zapisu. Sprawdź stan na pilocie.'));
 }catch(e){if(onboardingCurrent(generation))notice(e.uiMessage||serverMessage(e.message));}finally{body.password='';if(onboardingCurrent(generation))button.disabled=false;}
}
function discoveryView(){
 if(lanMode==='ble')return bleView();
 if(lanMode==='ap')return apView();
 const n=box(m('Dodaj urządzenie Shelly'));n.append(button(m('← Urządzenia'),()=>{invalidateOnboarding();lanCredentials=null;manualDraft.password='';onboardingChoice=null;onboardingCards=[];onboardingNext=null;page='devices';render();}));
 if(lanMode==='scan'||lanMode==='inspect'){
  onboardingSteps(n,2);
  const p=el('p',lanMode==='scan'?m('Wyszukiwanie mDNS i weryfikacja API…'):m('Sprawdzanie wybranego urządzenia…'));p.id='lan-progress';n.append(p,button(m('Przerwij'),async()=>{const generation=++lanGeneration;lanMode='scan';try{await request('discovery/cancel',{method:'POST'});if(onboardingCurrent(generation))pollLan(generation);}catch(e){if(onboardingCurrent(generation))notice(e.uiMessage||serverMessage(e.message));}}));return n;
 }
 if(lanMode==='start'){onboardingMethods(n);return n;}
 if(lanMode==='manual'){manualOnboarding(n);return n;}
 onboardingSteps(n,lanMode==='components'?3:2);
 if(lanMode==='results'){
  n.append(button(m('Skanuj ponownie'),scanLan));if(!lanSnapshot?.devices.length)empty(n,m('Nie znaleziono Shelly. Sprawdź Wi-Fi, mDNS i izolację klientów.'));
  for(const d of lanSnapshot?.devices||[]){const row=el('div',undefined,'row'),text=el('div',undefined,'grow');text.append(el('h3',d.name||d.id),el('p',m('{0} · Gen {1} · {2}',d.host,d.generation,d.auth_required?m('Wymaga hasła'):m('Bez hasła'))),el('p',d.id,'muted'));if(alreadyBound(d))text.append(el('p',m('Już dodane'),'tag'));row.append(text,button(m('Sprawdź urządzenie'),()=>inspectLan(d)));n.append(row);}return n;
 }
 n.append(el('h3',lanSelected.name||lanSelected.id),el('p',`${lanSelected.host} · Gen ${lanSelected.generation} · ${lanSelected.id}`,'muted'));
 const form=el('div',undefined,'fields');n.append(form);
 if(lanMode==='credentials'){
  const u=lanInput(form,m('Login'),'admin'),p=lanInput(form,m('Hasło Shelly'),'','password');n.append(button(m('Sprawdź urządzenie'),()=>inspectLan(lanSelected,{username:u.value,password:p.value}),'primary'));return n;
 }
 const name=lanInput(form,m('Nazwa urządzenia'),lanSelected.name),room=lanSelect(form,m('Pomieszczenie'),[[0,m('Nieprzypisane')],...config.rooms.map(r=>[r.id,r.name])]);
 const channels=[];for(let kind=0;kind<4;kind++)for(let i=0;i<16;i++)if(lanSelected.model!=='S3GW-1DBT001'&&((kind?(lanSelected.generation>=2?(kind===1?lanSnapshot.capabilities.covers:kind===2?lanSnapshot.capabilities.lights:lanSnapshot.capabilities.rgbs):0):lanSnapshot.capabilities.switches)&(1<<i)))channels.push([kind?`${kind===1?'cover':kind===2?'light':'rgb'}:${i}`:i,`${kind===1?'Cover':kind===2?'Light':kind===3?'RGB':'Switch'} ${i}`]);
 if(lanSnapshot.capabilities.ht===true&&lanSelected.generation===3&&lanSelected.model==='S3SN-0U12A')channels.push(['sensor:0','H&T Gen3 • read only']);
 const channel=lanSelect(form,m('Kanał z odpowiedzi API'),channels.length?channels:[['',m('Brak obsługiwanego kanału')]]);
 n.append(el('p',m('Dodanie zachowuje poprzednie urządzenia i ich kanały.')));
 const save=button(m('Sprawdź i zapisz połączenie'),()=>bindLan(name.value,room.value,channel.value,save),'primary');save.disabled=!channels.length;n.append(save);
 if(!channels.length)n.append(el('p',m(lanSelected.model==='S3GW-1DBT001'?'Bramka BLU • przekaźnik nieobsługiwany.':'Urządzenie rozpoznane. Sterowanie tym typem nie jest wdrożone.'),'muted'));return n;
}
function render(){if(!config)return;backupPanel?.dispose();backupPanel=null;$('#workspace .savebar').hidden=page==='system'&&!dirty;document.body.classList.toggle('showing-automations',page==='automations');document.body.classList.remove('editing-action');if(page!=='discovery'){invalidateOnboarding();lanCredentials=null;manualDraft.password='';onboardingCards=[];onboardingNext=null;}setText($('#title'),titles[page]);document.querySelectorAll('nav button').forEach(b=>b.setAttribute('aria-current',b.dataset.page===(page==='discovery'?'devices':page==='automations'?'scenes':page)?'page':'false'));const target=$('#editor');if(page==='discovery'){target.replaceChildren(discoveryView());return;}if(page==='automations'){target.replaceChildren(automationList());return;}const view=page==='activity'?box(m('Aktywność')):page==='appearance'?appearance():page==='system'?system():['groups','scenes'].includes(page)?actionList(page):list(page,({dashboards:editDashboard,rooms:editRoom,devices:editDevice,scenes:editScene,groups:editGroup})[page]);if(page==='devices')view.prepend(button(m('Wyszukaj Shelly w LAN'),openDiscovery,'primary'));target.replaceChildren(view);if(page==='devices'&&window.dsrRuntimeMount)window.dsrRuntimeMount(view);if(['groups','scenes','activity','dashboards'].includes(page)&&window.dsrActionsMount)window.dsrActionsMount(view,page);}
let automationSession=null,autoView=null,autoCache=null,autoRead=0;
const autoState=value=>m(['Wyłączona','Oczekiwanie na dane','Aktywna','Ręczna zmiana','Oczekiwanie na potwierdzenie','Błąd reguły'][value]||'Oczekiwanie na dane');
function autoValid(item){return typeof item.name==='string'&&item.name.trim().length>0&&new TextEncoder().encode(item.name).length<=47&&item.sensors.length>0&&item.targets.length>0&&new Set(item.sensors).size===item.sensors.length&&new Set(item.targets).size===item.targets.length&&Number.isInteger(item.target_half)&&item.target_half>=8&&item.target_half<=60;}
function autoWarning(text){return el('p',text,'automation-warning');}
function autoDiscard(){
 const s=automationSession;if(s?.owner===config){config=structuredClone(s.before);dirty=s.wasDirty;editSerial++;configGeneration++;setText($('#dirty'),m(dirty?'Niezapisane zmiany':'Wszystkie zmiany zapisane'));$('#save').disabled=!dirty||saving||conflict;}
 automationSession=null;autoView=null;notice('');
}
function autoTemperature(value){return Number.isInteger(value)&&value>=8&&value<=60?`${(value/2).toFixed(1)} °C`:'—';}
function autoReadReason(result){return m(({3:'Bezpośredni odczyt TRV lub nastawa nieobsługiwane.',4:'Wymagane logowanie do bramki.',9:'Powiązanie termostatu zmienione. Sprawdź konfigurację.',10:'Limit zapytań. Oczekiwanie przed ponowieniem.',8:'Oczekiwanie na dostęp do urządzenia.',5:'Brak odpowiedzi termostatu. Oczekiwanie na nowy odczyt.',7:'Błąd pamięci.'})[result]||'Niepoprawny odczyt termostatu.');}
function autoStatus(parent,item,data){
 parent.replaceChildren();const status=data?.rules?.find(r=>r.id===item.id);parent.append(el('p',autoState(status?.state??(item.enabled?1:0)),status?.state>=3?'automation-warning':'muted'));
 if(status?.missing_id)parent.append(autoWarning(m('Brak nowego raportu: {0}',config.devices.find(d=>d.id===status.missing_id)?.name||`#${status.missing_id}`)));
 const targetError=data?.targets?.some(t=>item.targets.includes(t.id)&&t.result&&t.result===status?.result);
 if(status?.result&&!targetError)parent.append(autoWarning(status.result===11?m('Przed zmianą aktywnej reguły wyłącz ją i zapisz.'):status.result===7?m('Błąd dziennika. Automatyzacja wstrzymana.'):autoReadReason(status.result)));
 for(const target of data?.targets||[])if(item.targets.includes(target.id)){
  const row=el('div',undefined,'automation-target');row.append(el('strong',config.devices.find(d=>d.id===target.id)?.name||`#${target.id}`),el('p',m('Poprzednia: {0} · Żądana: {1} · Odczytana: {2}',autoTemperature(target.baseline_half),autoTemperature(target.requested_half),autoTemperature(target.read_half))));if(target.overridden)row.append(autoWarning(m('Ręczna zmiana')));if(target.result)row.append(autoWarning(autoReadReason(target.result)));parent.append(row);
 }
}
async function autoRefresh(view){
 if(!view||autoView!==view||!view.root.isConnected||page!=='automations'||saving||$('#workspace').hidden)return;
 const owner=config,generation=configGeneration,revision=config.revision,serial=++autoRead;
 const current=()=>autoView===view&&view.root.isConnected&&page==='automations'&&owner===config&&generation===configGeneration&&revision===config.revision&&!saving&&!$('#workspace').hidden&&serial===autoRead;
 try{const response=await request('automations/status'),data=await response.json();if(!current())return;
  if(data.revision!==revision){view.message.replaceChildren(autoWarning(m('Status innej rewizji. Odśwież lub wczytaj konfigurację; szkic pozostaje.')));return;}
  if(!Array.isArray(data.devices)||!Array.isArray(data.rules)||!Array.isArray(data.targets))throw Error('Invalid automation status');
  autoCache=data;view.message.replaceChildren();view.update(data);
 }catch{if(current())view.message.replaceChildren(autoWarning(m('Stan reguł niedostępny. Odśwież, aby spróbować ponownie.')));}
}
function autoTools(root,update){
 const message=el('div'),view={root,message,update};autoView=view;root.append(button(m('Odśwież stan reguł'),()=>autoRefresh(view)),message);
 queueMicrotask(()=>autoRefresh(view));return view;
}
function automationList(){
 const root=box(m('Automatyzacje')),rows=[];root.classList.add('automations');
 root.append(button(m('← Wróć do listy'),()=>{page='scenes';render();}));
 root.append(button(m('+ Nowa reguła'),()=>{
  if(config.automations.length>=8)return notice(m('Osiągnięto limit wpisów.'));
  const before=structuredClone(config),wasDirty=dirty,item={id:nextId(config.automations),name:'',kind:'contact_heating',enabled:false,sensors:[],targets:[],target_half:16};config.automations.push(item);changed();automationEdit(item,before,wasDirty);
 },'primary'));
 for(const item of config.automations){const card=el('section',undefined,'automation-card'),status=el('div');card.append(el('h3',item.name),status,button(m('Edytuj regułę'),()=>automationEdit(item)));rows.push([status,item]);root.append(card);}
 const view=autoTools(root,data=>{for(const [row,item]of rows)autoStatus(row,item,data);});for(const [row,item]of rows)autoStatus(row,item,autoCache?.revision===config.revision?autoCache:null);return root;
}
function automationEdit(item,before=structuredClone(config),wasDirty=dirty){
 const session={owner:config,item,before,wasDirty};automationSession=session;
 const root=box(m('Automatyzacje')),body=el('fieldset',undefined,'automation-form'),status=el('div'),selections=[];document.body.classList.add('editing-action');root.classList.add('automations');$('#editor').replaceChildren(root);autoView=null;
 body.append(el('legend',m('Reguła ogrzewania')),el('p',m('Reakcja natychmiastowa. Powrót po zamknięciu wszystkich kontaktów. Ręczne zmiany zostają zachowane.')));root.append(body);
 const name=field(body,m('Nazwa reguły'),item.name,value=>{item.name=value;refresh();});name.maxLength=47;
 const temperature=field(body,m('Temperatura przy otwarciu'),(item.target_half??16)/2,value=>{item.target_half=value*2;refresh();},'number');temperature.min='4';temperature.max='30';temperature.step='0.5';
 const search=el('input'),searchLabel=el('label',m('Szukaj urządzeń'));search.type='search';searchLabel.append(search);body.append(searchLabel);
 const options=el('div',undefined,'automation-selections');body.append(options);
 const enableLabel=el('label',m('Włącz regułę'),'automation-choice'),enable=el('input');enable.type='checkbox';enable.checked=item.enabled;enableLabel.prepend(enable);enable.addEventListener('change',()=>{item.enabled=enable.checked;changed();refresh();});body.append(enableLabel);
 body.append(autoWarning(m('Wyłączenie lub usunięcie zatrzymuje przyszłe polecenia bez przywracania temperatury. Aktywną regułę wyłącz i zapisz przed zmianą czujników, celów lub nastawy.')));
 const statusHeading=el('h3',m('Stan reguły'));root.append(statusHeading,status,el('p',m('Nowe raporty wszystkich kontaktów i odczyty TRV są wymagane po przerwie. Tryb beacon można włączyć samodzielnie w Shelly.'),'muted'));
 function choices(data){
  const focused=document.activeElement?.dataset.autoDevice;options.replaceChildren();selections.length=0;
  for(const [key,role,title]of [['sensors',1,'Okna i drzwi'],['targets',2,'Termostaty']]){
   const group=el('fieldset'),legend=el('legend',m(title));group.append(legend);let count=0;
   for(const d of config.devices){const checked=item[key].includes(d.id),eligible=data?.devices?.some(v=>v.id===d.id&&v.role===role);if(!checked&&!eligible)continue;
    const label=el('label',d.name,'automation-choice'),input=el('input');input.type='checkbox';setUIAttribute(input,'aria-label',d.name);input.dataset.autoDevice=`${key}-${d.id}`;input.checked=checked;
    const used=key==='targets'&&config.automations.some(r=>r!==item&&r.targets.includes(d.id));input.disabled=!checked&&used;
    if(used)label.append(el('small',m('Zajęty przez inną regułę')));
    input.addEventListener('change',()=>{if(saving)return;if(input.checked&&(!eligible||used)){input.checked=false;return;}item[key]=input.checked?[...new Set([...item[key],d.id])]:item[key].filter(id=>id!==d.id);changed();refresh();});label.prepend(input);group.append(label);selections.push({label,name:d.name.toLocaleLowerCase()});count++;
   }
   if(!count)group.append(autoWarning(m('Brak zweryfikowanych urządzeń. Dodaj kontakt BLU lub termostat TRV przez bramkę.')));options.append(group);
  }
  filter();if(focused)options.querySelector(`[data-auto-device="${focused}"]`)?.focus();
 }
 function filter(){const q=search.value.toLocaleLowerCase().trim();for(const row of selections)row.label.hidden=!row.name.includes(q);}
 search.addEventListener('input',filter);
 const footer=el('div',undefined,'onboarding-footer'),cancel=button(m('Anuluj'),()=>{if(saving)return;autoDiscard();render();});
 const commit=button(m('Zapisz regułę'),async()=>{if(!autoValid(item)||saving)return;body.disabled=true;cancel.disabled=commit.disabled=true;await save();if(automationSession!==session||!root.isConnected)return;body.disabled=false;refresh();if(!dirty&&!saving){automationSession=null;render();}},'primary');footer.append(cancel,commit);root.insertBefore(footer,statusHeading);
 const remove=button(m('Usuń regułę'),async()=>{if(saving||!confirm(translate(m('Usunąć regułę? Przyszłe polecenia zostaną anulowane bez przywracania temperatury.'))))return;const position=config.automations.indexOf(item);if(position<0)return;config.automations.splice(position,1);changed();automationSession=null;await save();if(dirty){config.automations.splice(position,0,item);automationSession=session;refresh();return;}render();},'danger');body.append(remove);
 function refresh(){commit.disabled=saving||conflict||!autoValid(item);cancel.disabled=saving;}
 session.refresh=refresh;
 const view=autoTools(root,data=>{choices(data);autoStatus(status,item,data);});if(autoCache?.revision===config.revision)view.update(autoCache);else choices(null);refresh();
}
function canonical(value){if(Array.isArray(value))return value.map(canonical);if(value&&typeof value==='object')return Object.fromEntries(Object.keys(value).sort().map(k=>[k,canonical(value[k])]));return value;}
async function save(){
 if(!dirty||saving||conflict)return;
 if(automationSession?.owner===config&&!autoValid(automationSession.item))return notice(m('Przed zapisem podaj nazwę (maks. 47 bajtów), wybierz kontakty i termostaty oraz temperaturę 4–30°C co 0,5°C.'));
 if(actionSession?.owner===config&&(!actionNameValid(actionSession.item)||!(actionSession.kind==='groups'?actionSession.item.members.length:actionSession.item.actions.length))){notice(m('Uzupełnij nazwę i wybierz co najmniej jedno urządzenie lub polecenie.'));editAction(actionSession.kind,actionSession.item,actionNameValid(actionSession.item)?1:0);return;}
 const submitted=structuredClone(config),serial=editSerial;configGeneration++;saving=true;$('#save').disabled=true;
 try{
  await request('config',{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify(submitted)});notice(m('Zapisywanie konfiguracji…'));let acknowledged=false;
  for(let i=0;i<40;i++){
   await new Promise(r=>setTimeout(r,250));const s=await(await request('status')).json();if(s.busy)continue;
   if(s.revision!==submitted.revision+1)throw failure(m('Zapis nie powiódł się. Zmiany pozostają w edytorze.'));
   const published=await(await request('config')).json(),expected={...submitted,revision:s.revision};
   if(JSON.stringify(canonical(published))!==JSON.stringify(canonical(expected))){conflict=true;throw failure(m('Konfiguracja zmieniła się na pilocie. Wczytaj aktualną wersję.'));}
   config.revision=s.revision;
   if(actionSession?.owner===config){const committed=submitted[actionSession.kind].find(item=>item.id===actionSession.item.id);if(committed){actionSession.baseline=structuredClone(committed);actionSession.fresh=false;actionSession.before=structuredClone(expected);actionSession.wasDirty=false;}}
   setText($('#revision'),s.revision);acknowledged=true;
   if(editSerial===serial){dirty=false;setText($('#dirty'),m('Wszystkie zmiany zapisane'));notice(m('Zapisano. Ekran pilota korzysta z tej samej konfiguracji.'));}
   else{dirty=true;setText($('#dirty'),m('Niezapisane zmiany'));notice(m('Zapisano wcześniejszą wersję. Późniejsze zmiany pozostają niezapisane.'));}break;
  }
  if(!acknowledged)throw failure(m('Nie potwierdzono zapisu w wymaganym czasie. Zachowano zmiany w edytorze.'));
 }catch(e){notice(e.uiMessage||serverMessage(e.message));}finally{saving=false;$('#workspace .savebar').hidden=page==='system'&&!dirty;$('#save').disabled=!dirty||conflict;actionSession?.refresh?.();automationSession?.refresh?.();}
}
let statusPollBusy=false,previewBusy=false;
function previewMessage(text,warning=false){const n=$('#preview-status');setText(n,text);n.classList.toggle('automation-warning',warning);}
/* A 300 KiB snapshot is an explicit action, separate from lightweight status.
 * Keep one transfer in flight and discard a frame after navigation or hiding. */
async function refreshPreview(){
 if(!config||$('#workspace').hidden||document.hidden||previewBusy)return;
 const owner=configReadOwner(),current=()=>!document.hidden&&configReadCurrent(owner);
 previewBusy=true;$('#preview-refresh').disabled=true;
 previewMessage(m('Pobieranie obrazu z pilota…'));
 try{
  const ctx=$('#preview').getContext('2d');ctx.clearRect(0,0,320,480);
  const r=await request('preview.raw');if(!current())return;
  if(r.status===202){previewMessage(m('Pilot jeszcze nie udostępnił nowej klatki. Odśwież podgląd ponownie.'),true);return;}
  if(r.status!==200){previewMessage(m('Nieprawidłowy obraz z pilota. Odśwież podgląd ponownie.'),true);return;}
  const bytes=new Uint8Array(await r.arrayBuffer());if(!current())return;
  if(bytes.length!==320*480*2){previewMessage(m('Nieprawidłowy obraz z pilota. Odśwież podgląd ponownie.'),true);return;}
  const im=ctx.createImageData(320,480);
  for(let i=0;i<320*480;i++){const v=bytes[i*2]|bytes[i*2+1]<<8;im.data[i*4]=(v>>11)*255/31;im.data[i*4+1]=((v>>5)&63)*255/63;im.data[i*4+2]=(v&31)*255/31;im.data[i*4+3]=255;}
  ctx.putImageData(im,0,0);previewMessage(m('Obraz pobrany z zadania LVGL pilota.'));
 }catch(e){if(current())previewMessage(m('Nie udało się pobrać obrazu. Sprawdź połączenie i odśwież podgląd.'),true);}
 finally{previewBusy=false;$('#preview-refresh').disabled=false;if(!current())previewMessage(m('Oczekiwanie na klatkę z pilota…'));}
}
async function poll(){if(!config||$('#workspace').hidden||document.hidden||statusPollBusy)return;statusPollBusy=true;const serial=++statusReadSerial,owner=configReadOwner();const current=()=>!document.hidden&&serial===statusReadSerial&&configReadCurrent(owner);try{const response=await request('status');if(!current())return;const s=await response.json();if(!current())return;remoteReachable=true;updateOnboardingChoice();setText($('#connection'),m('● Połączono lokalnie'));setText($('#firmware'),s.firmware||'');if(!saving&&s.revision!==config.revision){if(dirty){conflict=true;$('#save').disabled=true;notice(m('Konfiguracja zmieniła się na pilocie. Wczytaj aktualną wersję, aby uniknąć nadpisania.'));}else{await load();return;}}}catch(e){if(!current())return;remoteReachable=false;updateOnboardingChoice();setText($('#connection'),m('Brak połączenia'));}finally{statusPollBusy=false;if(current()){if(window.dsrRuntimeRefresh)window.dsrRuntimeRefresh();if(window.dsrActionsRefresh)window.dsrActionsRefresh();}}}
async function uploadPhoto(file,room,current=()=>true){
 if(!['image/jpeg','image/png','image/webp'].includes(file.type)||file.size>15*1024*1024)throw failure(m('Wybierz JPG, PNG lub WebP do 15 MB.'));
 const bitmap=await createImageBitmap(file);if(bitmap.width*bitmap.height>40000000){bitmap.close();throw failure(m('Zbyt duże zdjęcie.'));}
 const c=document.createElement('canvas');c.width=320;c.height=180;
 const ctx=c.getContext('2d'),ratio=Math.max(320/bitmap.width,180/bitmap.height);
 ctx.drawImage(bitmap,(320-bitmap.width*ratio)/2,(180-bitmap.height*ratio)/2,bitmap.width*ratio,bitmap.height*ratio);bitmap.close();
 const rgba=ctx.getImageData(0,0,320,180).data,raw=new Uint8Array(115200);
 for(let i=0;i<57600;i++){const value=(rgba[i*4]>>3)<<11|(rgba[i*4+1]>>2)<<5|(rgba[i*4+2]>>3);raw[i*2]=value&255;raw[i*2+1]=value>>8;}
 const id=sha256(raw).slice(0,32);await request('photos/'+id,{method:'POST',headers:{'Content-Type':'application/octet-stream'},body:raw});if(!current())return false;room.photo=id;changed();return true;
}
/* SHA-256 is included locally because WebCrypto.digest is not available on
 * ordinary HTTP origins. No CDN, telemetry, external fonts or cloud calls. */
function sha256(input){const K=[0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2];let H=[0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19];const length=Math.ceil((input.length+9)/64)*64,p=new Uint8Array(length);p.set(input);p[input.length]=128;new DataView(p.buffer).setUint32(length-4,input.length*8);const w=new Uint32Array(64),ro=(v,n)=>v>>>n|v<<(32-n);for(let at=0;at<length;at+=64){const dv=new DataView(p.buffer,at,64);for(let i=0;i<16;i++)w[i]=dv.getUint32(i*4);for(let i=16;i<64;i++)w[i]=(w[i-16]+(ro(w[i-15],7)^ro(w[i-15],18)^w[i-15]>>>3)+w[i-7]+(ro(w[i-2],17)^ro(w[i-2],19)^w[i-2]>>>10))>>>0;let[a,b,c,d,e,f,g,h]=H;for(let i=0;i<64;i++){const t1=(h+(ro(e,6)^ro(e,11)^ro(e,25))+((e&f)^(~e&g))+K[i]+w[i])>>>0,t2=((ro(a,2)^ro(a,13)^ro(a,22))+((a&b)^(a&c)^(b&c)))>>>0;h=g;g=f;f=e;e=(d+t1)>>>0;d=c;c=b;b=a;a=(t1+t2)>>>0;}H=H.map((v,i)=>(v+[a,b,c,d,e,f,g,h][i])>>>0);}return H.map(v=>v.toString(16).padStart(8,'0')).join('');}
document.querySelectorAll('[data-ui]').forEach(node=>setText(node,m(node.dataset.ui)));
document.querySelectorAll('[data-ui-aria]').forEach(node=>setUIAttribute(node,'aria-label',m(node.dataset.uiAria)));
$('#web-locale').addEventListener('change',event=>updateWebLocale(event.target.value));
updateWebLocale(webLocale,false);
document.querySelectorAll('nav button').forEach(b=>b.addEventListener('click',()=>{if(automationSession?.owner===config){if(saving||!confirm(translate(m('Odrzucić niezapisane zmiany reguły?'))))return;autoDiscard();}page=b.dataset.page;render();}));$('#save').addEventListener('click',save);$('#reload').addEventListener('click',load);$('#preview-refresh').addEventListener('click',refreshPreview);$('#pair-form').addEventListener('submit',async e=>{e.preventDefault();try{await request('pair',{method:'POST',body:$('#pair-code').value.trim()});$('#pair-code').value='';await load();}catch(error){notice(error.uiMessage||serverMessage(error.message));}});window.addEventListener('beforeunload',e=>{if(dirty){e.preventDefault();e.returnValue='';}});load();
/* Schedule the next read only after the previous read and body have settled. */
async function pollLoop(){try{await poll();}finally{setTimeout(pollLoop,1500);}}
setTimeout(pollLoop,1500);
