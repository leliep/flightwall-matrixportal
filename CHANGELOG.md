# Changelog

## Unreleased

- Callsign-Airlinecodes haben Vorrang vor Routendienst-Platzhaltern; der Wert
  `unknown` von adsb.im kann BPO/GAF und andere erkennbare Präfixe nicht mehr
  zu `Airline N/A` überschreiben
- fehlender Airline-/Betreibername wird bei Special Types durch eine lesbare
  Klassifizierung wie `Military`, `Police` oder `Helicopter` ersetzt
- Lookup-Ergänzungen für TAS, ZYL, AXS und BBC
- `GAF…` wird unabhängig von den ADS-B-Daten eindeutig als German Airforce
  erkannt, sichtbar so bezeichnet und mit dem Militärsymbol dargestellt
- zwei dauerhaft gespeicherte ADS-B-Quellen mit frei wählbarer aktiver Quelle;
  bestehende Konfigurationen werden als Quelle 1 übernommen
- `BPO…` wird unabhängig vom restlichen Callsign eindeutig als Bundespolizei
  erkannt und sichtbar so bezeichnet
- Lookup-Ergänzungen für NVI (Navoiy) und RXI (Riyadh Air)
- Matrix-Farbtiefe zur weiteren Unterdrückung sporadischer Störpixel von 3 auf
  2 Bit reduziert; Helligkeitsautomatik und gespeicherte Auswahl auf die drei
  nativen Stufen 33 %, 66 % und 100 % migriert
- Routencache beim Flugzeugwechsel in RAM verlagert; verzögerte persistente
  Speicherung bei kontrolliert dunkler Matrix im flugzeugfreien Zustand
- vollständige Verarbeitung, Zwischenspeicherung und Darstellung von
  Multihop-Routen wie `CGN-BER-CGN`
- Airline-/Operator-Code `CHX` als Luftrettung ergänzt; ADS-B-Platzhalter
  `UNKNOWN` wird durch vorhandene Lookup-Namen ersetzt
- Airline-/Operator-Code `BPO` als Bundespolizei ergänzt
- gut lesbares Gradzeichen bei der Temperatur der BME280-Matrixanzeige
- BME280-Luftdruck anhand der konfigurierten Standorthöhe auf Meereshöhe
  reduziert; Stationsdruck bleibt als Diagnosewert erhalten
- konfigurierbare BME280-Ruheanzeige auf der Matrix, wenn kein Flugzeug in der
  Nähe ist
- BME280-Autoerkennung auf beiden I²C-Bussen und Umgebungswerte in Webstatus
  und Status-API
- automatische Aktualisierung des Web-Statusbereichs alle fünf Sekunden
- Lookup-Ergänzungen für MCO, ERF, PEG, GEC und NBT
- FlightMatrixServer-Funktionen direkt in den MatrixPortal ESP32-S3 integriert
- direkter ADS-B-Abruf und lokale Auswahl des anzuzeigenden Flugzeugs
- lokale CPA-, Richtungs-, Special-Type-, Kleinflugzeug- und Höhenauswertung
- Routenabfrage über adsb.im mit NVS-Cache
- Flughafen- und Airline-Übersetzungstabellen mit Sammlung unbekannter Codes
- konfigurierbarer Standort und Standard-Blickrichtung 322°
- definierter Git-Wiederherstellungspunkt `pre-flightmatrix-integration-2026-10-04`
- ausführliche Web- und JSON-Diagnose für ADS-B-Abrufe, Routenfehler,
  Flugzeugdaten, CPA sowie normalen und effektiven Special-Type-Radius
- direkter Routendienst-Test ohne Cache mit frei eingebbarem Callsign
- Callsigns mit Präfix `GAF` werden als `German Airforce` erkannt
- eigenständiges responsives FlightWall-Dashboard unter `/display` mit
  Callsign, Airline, Flugzeugtyp, Route, Flugdaten, CPA und Blickrichtung;
  für iPhone, Tablet und Desktop optimiert
- interne ESP32-S3-Chiptemperatur in Statusseite und Status-API
- passwortgeschütztes Firmware-Update über das Webinterface
- passwortgeschützter OTA-Upload aus PlatformIO
- 2 Bit Farbtiefe mit drei Helligkeitsstufen und logarithmischer Lux-Regelung
- schwarzes Display während OTA-Flashzugriffen zur Unterdrückung sichtbarer Störungen
- konfigurierbare ADS-B- und Routen-Adressen mit Verbindungstest
- einzeln aktivierbare und frei sortierbare Seiten mit eigener Anzeigedauer
- optionale sanfte Seitenübergänge und Symbole für Special Types
- sichtbare OTA-Vorankündigung vor dem Abdunkeln
- bei jeder Helligkeitsstufe sichtbarer Betriebs-Pixel im Ruhezustand
- roter statt weißer Betriebs-Pixel bei fehlender FlightWall-Serververbindung
- erweiterte Diagnose mit Laufzeit, RAM, WLAN, Lux, Task-Stacks und Resetgrund
- Status-API und bestätigungspflichtiger Neustart über das Webinterface
- OTA-Fortschritt, Firmwareprüfung, verständliche Fehler und Versionsvergleich
- abwählbare Flughafennamen-Seite mit horizontalem Lauftext für lange Start- und Zielnamen
- Flughafennamen-Seite wird ohne Routendaten übersprungen und bleibt für einen vollständigen Lauftext sichtbar
- korrekte ASCII-Umschrift von `š` und `Š`, beispielsweise `Priština` als `Pristina`
- ASCII-Umschrift von `â`, `ā` und `ă`, beispielsweise `Coandă` als `Coanda`
- über das Webinterface start- und stoppbarer Displaytest
- konfigurierbare tägliche Nachtruhe mit NTP-Zeit und automatischer MEZ-/MESZ-Umschaltung
- frei sortier- und abwählbare Freitextseite mit einmaligem horizontalem Lauftext bis 128 Zeichen
- vollständiges Löschen beider Matrixpuffer und Rücksetzen der Laufzeit vor jeder Seitenaktivierung
- wählbare Kompaktansicht mit Flugnummer, Typ, Höhe, Entfernung, Blickrichtung, Route und CPA
- rote Flugnummer in der Kompaktansicht
- zenit- und entfernungsabhängige Blickrichtungen mit konfigurierbarer Standorthöhe und Hysterese
- zusätzliche Kompaktseite mit sofort dargestellten, getrennt scrollenden Flughafen-Langnamen
- Flugzeugtyp rechtsbündig und Special-Type-Symbol unmittelbar davor auf beiden Kompaktseiten
- Leerzeichen zwischen `CPA` und Entfernungswert auf der Kompaktseite
- Abfluganzeige `geht` einheitlich in Kleinschreibung
- dynamisches Flughafennamen-Layout mit außenbündigen Kurznamen und bedarfsgerechtem Lauftext
- aussagekräftige 14×7-Symbole auf beiden Kompaktseiten, einschließlich Luftschiff (`SHIP`)
- alle Typensymbole mit Bedeutung als zusätzliche Stufen im Displaytest
- rote, bei Bedarf scrollende Fluggesellschaft statt Flugnummer auf der Kompaktseite mit Flughäfen
- Kompaktseiten mit Entfernung vor Höhe und direkt folgendem Steig-/Sinkdreieck
- einheitlich kräftige 7×7-Blickrichtungspfeile auf allen Seiten
- Abfluganzeige `entfernt sich`, wenn die Zeile genügend Platz bietet, sonst `geht`
- Lauftexte der Kompaktseite mit Flughäfen scrollen nach Endpause schnell zurück und pausieren vor dem Seitenwechsel
- vollständige und rotationssymmetrische Pfeilspitzen für alle diagonalen Blickrichtungen
- platzabhängige Geschwindigkeitsanzeige `kmh`/`k` in der zweiten Zeile beider Kompaktseiten
- diagonale Pfeilspitzen mit exakt vier Pixeln langen, gleich großen Schenkeln
- Leerzeichen zwischen Höhe und Geschwindigkeit bei fehlendem Steig-/Sinkindikator, sofern Platz vorhanden
- Airline-Serverwert `UNKNOWN` wird als `Airline N/A` angezeigt

## 2.0.0 – 2026-09-25

- Dual-Core-Aufteilung für stabile Anzeige während blockierender Netzwerkzugriffe
- integriertes, mobil nutzbares Webinterface
- dauerhaft gespeicherte Display-, Radius- und Helligkeitseinstellungen
- BH1750-Automatik mit Glättung, Hysterese und manuellem Fallback
- interruptgestützte Tastererfassung
- FlightWall-Anzeige mit Richtungspfeilen, CPA, Flugroute und Sondertypen
- Höhe in Fuß und Cyan für Fluggesellschaften
- verbesserte WLAN-, DNS- und HTTP-Diagnose
