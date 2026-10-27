# Changelog

## 3.5.4-free-route-fallback.1 – 2026-10-27

- kostenlosen ADSBDB-Dienst als automatische zweite Routenquelle ergänzt;
  der vorhandene adsb.im-Dienst bleibt die erste Quelle
- ADSBDB wird nur bei fehlender Primärroute abgefragt und benötigt weder Konto
  noch API-Schlüssel
- Fallbackrouten werden anhand von Callsign und geografischer Lage auf
  Plausibilität geprüft und maximal einen Tag ausschließlich im RAM gehalten;
  sie werden nicht in den permanenten Routencache kopiert
- Routenquelle in Status-JSON, Statusseite, Routentest und Cachetabelle sichtbar
- ADSBDB-Fallback in der Konfigurationsseite abschaltbar
- Airline-Codes `APA` (Air Park Aviation), `FRO` (Frost Air) und `XRO`
  (Exxaero) ergänzt

## 3.5.3-compact-config.1 – 2026-10-27

- Eingabefelder der Konfigurationsseite in ein responsives Raster eingeordnet
- kurze Zahlenwerte, Uhrzeiten und zusammengehörige Optionen stehen auf
  breiten Bildschirmen nebeneinander; lange Adressen und Topics bleiben breit
- Seitenaktivierung und Anzeigedauer werden kompakt als gemeinsame Zeilen
  dargestellt; sehr schmale iPhone-Ansichten bleiben gut bedienbar

## 3.5.2-aircraft-description.1 – 2026-10-27

- die Seite „CPA und Sondertyp“ zeigt in ihrer dritten Textzeile zusätzlich
  die ausführliche ADS-B-Flugzeugtypenbezeichnung an
- Sondertyp und Typenbezeichnung werden mit ` | ` kombiniert; zu lange Texte
  laufen vollständig durch und verlängern die Seitendauer automatisch

## 3.5.1-wifi-setup-display.1 – 2026-10-27

- bei fehlender WLAN-Verbindung zeigt die Matrix die Daten des eigenen
  Einrichtungs-WLANs mit `WLAN SETUP`, SSID, Passwort und
  `CONFIG IP 192.168.4.1` an
- lange SSIDs laufen automatisch durch die zweite Displayzeile
- Flughafencodes `ADL`, `AJR`, `EGC`, `ERC` und `SCN` sowie Betreiber-Codes
  `RAZ`, `RGN` und `VOE` aus der Merkliste ergänzt

## 3.4.0-3bit-dmasync.15-looproutes – 2026-10-25

- geschlossene Multi-Hop-Routen werden auf den Kompaktseiten ohne das
  redundante Endziel dargestellt, beispielsweise `DUS<>BUD` statt
  `DUS>BUD>DUS` beziehungsweise `Duesseldorf > Duesseldorf`
- die Seite „Kompakt + Flughäfen“ scrollt bei solchen Umläufen die sinnvolle
  Namensfolge, beispielsweise `Duesseldorf <> Budapest`
- die ausführliche Flughafenseite behält sämtliche Stationen und scrollt
  weiterhin die vollständige Route `DUS>BUD>DUS` samt aller Flughafennamen

## 3.4.0-3bit-dmasync.14-routecache – 2026-10-25

- Routencache-Verwaltung in der Konfigurationsseite mit Callsign, Route, Alter,
  Speicherort und Zustand ergänzt
- einzelne Einträge können gelöscht oder sofort neu abgefragt werden; zusätzlich
  lässt sich der gesamte RAM- und Flash-Routencache mit Bestätigung leeren
- gefundene Standardrouten gelten sieben Tage, negative Ergebnisse eine Stunde
  und erkannte militärische beziehungsweise behördliche Einsatzrufzeichen
  30 Minuten
- Flash-Dubletten desselben Callsigns werden beim Schreiben entfernt; beim
  Lesen gewinnt stets der jüngste gültige Eintrag
- Cacheänderungen erfolgen mit kontrolliert dunkel geschalteter Matrix; die
  automatische Statusaktualisierung liest die Flash-Cachetabelle nicht erneut

## 3.4.0-3bit-dmasync.13-adsbfi – 2026-10-25

- ADS-B-Abrufe auf HTTP/1.0 umgestellt, damit die von Cloudflare für adsb.fi
  verwendete HTTP/1.1-Chunk-Kodierung nicht als JSON-Inhalt gelesen wird
- erfolgreiche JSON-Antworten werden zusätzlich auf ein vorhandenes
  `aircraft`- oder `ac`-Array geprüft; ungültige Strukturen erzeugen nun einen
  sichtbaren Formatfehler statt eines Scheinerfolgs mit null Flugzeugen

## 3.4.0-3bit-dmasync.12-adsbfi – 2026-10-25

- adsb.fi als dritte, separat auswählbare ADS-B-Quelle ergänzt; die beiden frei
  konfigurierbaren lokalen Empfängerplätze bleiben erhalten
- HTTPS-Abruf mit eindeutiger FlightWall-Kennung und Unterstützung des
  adsb.fi-v3-Formats (`ac`, Zeitstempel in Millisekunden) ergänzt
- externe Abfrage-URL wird automatisch aus Standort und dem für Special Types
  benötigten doppelten Anzeigeradius erzeugt
- aktive Quelle und erzeugte adsb.fi-URL in Statusseite und Status-API ergänzt
- erforderliche adsb.fi-Quellenangabe in Webinterface und README aufgenommen
- Flughafencodes `DNZ` und `YYR` sowie Betreiber-Codes `AMB`, `DAG` und `PTN`
  aus der Merkliste ergänzt

## 3.4.0-3bit-dmasync.11 – 2026-10-25

- Flughafencodes `ALA`, `CIT`, `CKZ`, `CUN`, `KGF`, `MCT`, `PVR`, `TMP`,
  `TSR` und `TXL` ergänzt
- Betreiber-/Airline-Codes `AGR`, `ATJ`, `AUH`, `AWH`, `AXY`, `HGO`, `HOP`,
  `LHB`, `OMA`, `OSY`, `PWF`, `ROT`, `RSX`, `TAH`, `VIR` und `VND` ergänzt
- Generierte Firmwaretabellen aus den aktualisierten Quelldaten neu erstellt

## 3.4.0-3bit-dmasync.10 – 2026-10-22

- Airline-/Betreibercodes `AAB`, `AWU`, `DCS`, `LUX`, `MAS`, `MMD` und `RES`
  in die Übersetzungstabelle aufgenommen
- Die generierte Firmwaretabelle aus den geprüften Quelldaten aktualisiert

## 3.4.0-3bit-dmasync.9 – 2026-10-22

- Eindeutige Special-Flugzeuge seit dem letzten Neustart werden anhand ihrer
  ICAO-Hexadresse gezählt und nach der tatsächlich angezeigten Hauptkategorie
  aufgeschlüsselt
- Transpondernotfälle sind als eigene Kategorien `7500` (Hijack), `7600`
  (Radio) und `7700` (Emergency) enthalten und haben Vorrang vor sonstigen
  Special-Klassifizierungen
- Eingebaute Notfall-Testflugzeuge werden nicht gezählt; die Statistik steht
  in der regelmäßig aktualisierten Statusanzeige und in `/api/status` bereit
- Vor dem OTA-Update neu aufgelaufene Flughafencodes `DOV` und `HHN` sowie
  Operator-Code `GDK` ergänzt

## 3.4.0-3bit-dmasync.8 – 2026-10-22

- Rufzeichen nach dem Muster `FELON…` werden vorrangig als militärische Flüge
  klassifiziert und erhalten die Military-Anzeige
- Die vollständigen Codewörter werden erkannt, damit der zivile ICAO-Code
  `FEL` nicht pauschal als militärisch gilt
- Vor dem OTA-Update gesammelte Flughafencodes `STL`, `ABZ`, `PSR` und `JNH`
  sowie Operator-Codes `MCK`, `GFD`, `BAF`, `SCR`, `MPH`, `DUK` und `JCB`
  ergänzt

## 3.4.0-3bit-dmasync.7 – 2026-10-21

- CPU-0-Netzwerk-Task auf Idle-Priorität umgestellt, damit synchrone HTTP-,
  TLS- und Webserver-Pfade den vom Task-Watchdog überwachten Idle-Task nicht
  mehr vollständig verdrängen können
- kurze Scheduler-Pausen zwischen Webserver, OTA und MQTT ergänzt
- Netzwerkphasen Webserver, OTA, MQTT, WLAN, ADS-B, Route und Flash mit ihren
  maximalen Laufzeiten instrumentiert
- aktive Netzwerkphase im RTC-Speicher gesichert; nach einem Task-Watchdog-
  Neustart zeigt die Statusdiagnose den zuletzt aktiven Abschnitt
- Statusaktualisierung der Konfigurationsseite von einem vollständigen
  Seitenabruf auf ein kleineres Statusfragment umgestellt und auf zehn
  Sekunden entzerrt
- Heap-Neuzuweisungen beim Erzeugen der Konfigurationsseite durch eine zur
  tatsächlichen Seitengröße passende Reservierung reduziert
- Operator-Lookups `AOJ` (Avcon Jet) und `FHM` (Freebird Airlines Europe)
  ergänzt

## 3.4.0-3bit-dmasync.6 – 2026-10-20

- MQTT-Feld und Wetterstatus von „Regen 24 h“ auf „Regen heute“ umbenannt,
  passend zur Bedeutung des gelieferten MQTT-Werts
- Englische Oberfläche entsprechend auf „Rain today“ angepasst

## 3.4.0-3bit-dmasync.5 – 2026-10-20

- Spaltenbreiten der Statuskarten begrenzt, damit lange Beschreibungen die
  Messwertspalte nicht mehr auf wenige Zeichen zusammendrücken
- Erläuterung zur kritischen Oberflächentemperatur als zweispaltige Notiz
  dargestellt und fehlerhafte HTML-Verschachtelung im Wetterstatus korrigiert

## 3.4.0-3bit-dmasync.4 – 2026-10-20

- Rufzeichen nach dem Muster `SONIC…` werden vorrangig als militärische Flüge
  klassifiziert und erhalten die Military-Anzeige
- Der zivile ICAO-Operatorcode `SON` bleibt für andere Callsigns weiterhin als
  ACASS Ireland verfügbar

## 3.4.0-3bit-dmasync.3 – 2026-10-20

- Schimmelrisiko aus BME280-Temperatur und -Luftfeuchte ergänzt, einschließlich
  Taupunkt und kritischer Oberflächentemperatur für 80 % relative Feuchte
- Zeitlich geglättete Warnstufen verhindern Fehlalarme durch kurze
  Feuchtespitzen; hohe Feuchte wird erst nach 30 Minuten beziehungsweise ab
  75 % unmittelbar als hohes Risiko gewertet
- Konfigurationsstatus und Status-API um Risikostufe, Grenztemperatur und Dauer
  erhöhter Feuchte erweitert
- Luftfeuchte der Matrix-Wetterseite erscheint bei erhöhtem Risiko gelb und bei
  hohem Risiko rot
- Vor dem OTA-Update gesammelte Operator-Codes ergänzt: `AMQ` (AMC Aviation),
  `SAH` (Smart Jet) und `SON` (ACASS Ireland)

## 3.4.0-3bit-dmasync.2 – 2026-10-20

- Taktische German-Airforce-Rufzeichen ergänzt: `BURRO`, `HAMMER`, `SMASH`,
  `VADER`, `CONAN`, `FALCON`, `THOR` und `VIRUS`
- Die vollständigen Codewörter werden gezielt erkannt, damit gleichlautende
  zivile Drei-Buchstaben-Codes wie `HAM` und `BUR` korrekt bleiben
- German-Airforce-Erkennung für Auswahl, Special-Klassifizierung,
  Operatoranzeige und Unknown-Code-Erfassung zentral zusammengeführt
- Neue Operator-Codes aus dem Controllerstatus ergänzt: `AEG` (Airest),
  `AAN` (CTR Atmospherica Aviation), `GFA` (Gulf Air), `TMT` (Titan Airways
  Malta) und `BUR` (Air Bucharest)

## 3.4.0-3bit-dmasync.1 – 2026-10-19

- Experimenteller ESP32-S3-DMA-Fix aus Adafruit Protomatter übernommen:
  Vor Latch/OE-Änderungen wird jetzt der vollständige Abschluss der laufenden
  Zeilenübertragung abgewartet
- Farbtiefe und HUB75-Pixeltakt bleiben für einen klaren Vergleich zunächst
  unverändert bei 3 Bit beziehungsweise rund 17,8 MHz
- Fehlende Operator-Codes aus dem Controllerstatus ergänzt: `BLX` (TUIfly
  Nordic) und `UNI` (Unicair)

## 3.4.0-3bit-exp.8 – 2026-10-19

- Militärisches Callsign-Präfix `SHO` (beispielsweise `SHOCK11`) als German
  Airforce aufgenommen; die Kennzeichnung und das Military-Special-Icon
  funktionieren damit unabhängig von gelieferten ADS-B-Operator-Daten

## 3.4.0-3bit-exp.7 – 2026-10-19

- Militärische Callsigns mit Präfix `VAD` (zum Beispiel `Vader21`) werden wie
  `GAF…` eindeutig als German Airforce klassifiziert und erhalten das
  Military-Special-Icon
- Fehlende Codes aus dem Status übernommen: Flughafen `BQN` (Aguadilla) sowie
  Operator `VJH` (VistaJet Germany)

## 3.4.0-3bit-exp.6 – 2026-10-19

- Blickrichtungszusatz `weit` von einer festen Bodenentfernung auf den
  vertikalen Elevationswinkel über dem Horizont umgestellt
- Standardgrenze 30°, im Bereich 10–45° konfigurierbar und mit 2° Hysterese
- Konfigurationsseite, Statusseite und Status-API auf die Winkelangabe
  umgestellt; bestehende Installationen werden automatisch auf 30° migriert

## 3.4.0-3bit-exp.5 – 2026-10-19

- Steig- und Sinkanzeige der Seite „Flugdaten“ an die Kompaktseiten
  angeglichen: grünes Aufwärts- beziehungsweise oranges Abwärtsdreieck statt
  diagonaler Richtungspfeile

## 3.4.0-3bit-exp.4 – 2026-10-19

- schnelle Freitext-Laufgeschwindigkeit von 50 auf 25 ms pro Pixel verdoppelt
- Zeichenintervall der schnellen Freitextstufe für einen flüssigen Lauf auf
  25 ms verkürzt; mittlere und langsame Stufe bleiben unverändert

## 3.4.0-3bit-exp.3 – 2026-10-19

- einheitliche 14×7-Special-Type-Icons der Kompaktseiten auf Haupt-, Routen-,
  CPA-, Flughafen-, Freitext- und Notfallseite übernommen
- Kopfzeilenbreite für Special-Icon, Callsign und Flugzeugtyp dynamisch
  aufgeteilt

## 3.4.0-3bit-exp.2 – 2026-10-19

- Berechnung der von Protomatter seit dem letzten Messintervall gemeldeten
  Matrix-Refresh-Rate korrigiert

## 3.4.0-3bit-exp.1 – 2026-10-19

- experimentelle Matrix-Farbtiefe von 2 auf 3 Bit erhöht
- sieben native 3-Bit-Helligkeitsstufen und Migration der gespeicherten
  2-Bit-Helligkeit ergänzt
- Bildübergaben für statische Seiten und die Ruheanzeige deutlich reduziert;
  Lauftexte und Übergänge behalten ihre flüssige Aktualisierung
- Konfigurations- und Tastereinstellungen werden bei kontrolliert abgedunkelter
  Matrix in den Flash geschrieben
- Matrix-Refresh-Rate, Anzahl sowie aktuelle und maximale Dauer der
  Bildübergaben in Statusseite und Status-API ergänzt
- stabile 2-Bit-Version bleibt auf `main`; Experiment auf
  `experiment/3bit-clean-display`

## 3.4.0-ui-preview.3 – 2026-10-19

- Böenwert der Matrix-Wetterseite in der englischen Oberfläche mit `G` für
  *gusts* statt mit dem deutschen `B` gekennzeichnet
- Airline-Lookup für SAB (Schmiedel Air Service Tchad) ergänzt

## 3.4.0-ui-preview.2 – 2026-10-18

- verbliebene deutsche Begriffe in der englischen Konfigurations- und
  Statusseite übersetzt
- dynamische Statuswerte wie verbunden, ja/nein, Höhenfilter und
  Helligkeitsautomatik sprachabhängig dargestellt
- Übersetzungsreihenfolge für zusammengesetzte Hilfetexte korrigiert

## 3.4.0-ui-preview – 2026-10-18

- dauerhaft gespeicherte Sprachwahl Deutsch/Englisch in der Konfiguration
- zweisprachige Konfigurations-, Status-, Webdisplay-, Test-, Neustart- und
  OTA-Oberflächen
- sprachabhängige Matrixtexte, Blickrichtungen und Wetter-Himmelsrichtungen
- aktive Sprache in den JSON-APIs ergänzt, bestehende Datenfelder bleiben
  kompatibel

## 3.3.4 – 2026-10-17

- Airport-Lookups für RZE, YHZ, GDL, DEB, CND, TGM, SDQ, SJO und TRN ergänzt
- Airline-/Betreiber-Lookups für JOK, NJE, HAM, ADN, HKY, PAV, PLF, LXA,
  MMF, HYP, BCP, GAC, JFA, RRR, RWL, PJJ, FJO, MJF, AOM, SVI, UKN, BTX,
  HYS, LYF, EFD, TWI und TYW ergänzt

## 3.3.3 – 2026-10-13

- Airport-Lookups für SBZ (Sibiu), CBR (Canberra) und BNE (Brisbane) ergänzt
- Airline-/Betreiber-Lookups für RHH, MOG, NFA, PIA, FRF, LSM, STL, BOO,
  QAJ, JSD, VCJ, TAL, JEI, LEJ, SFG, PAC, JAR, NAT, QQE, LHA und PEX ergänzt

## 3.3.2 – 2026-10-11

- Gradzeichen der absoluten Blickrichtung in den Kompaktanzeigen korrekt
  hochgestellt
- Lookup-Ergänzung für SJI (Smart Jet International)

## 3.3.1 – 2026-10-11

- absolute Blickrichtung vom Standort zum Flugzeug in Grad auf beiden
  Kompaktseiten ergänzt; Norden entspricht 0°
- Lookup-Ergänzungen für TES, RCH, LUA und JDI

## 3.3.0 – 2026-10-11

- Squawk-Codes 7500, 7600 und 7700 werden direkt aus readsb/dump1090 gelesen
  und als Transponder-Notfälle klassifiziert
- Notfallflugzeuge umgehen Radius-, Höhen- und Kleinflugzeugfilter und erhalten
  höchste Auswahlpriorität
- zusätzliche, nicht abschaltbare Matrixseite mit Code und Bedeutung
  `Hijack`, `Radio` oder `Emergency`
- stabile Rotation mehrerer gleichzeitiger Notfallflugzeuge nach jeweils einem
  vollständigen konfigurierten Seitenzyklus
- Web-Display, Statusseite und JSON-APIs um Squawk, Notfallstatus und
  Rotationsposition erweitert
- zeitlich begrenzter Webtest mit einem bis drei simulierten Notfallflugzeugen

## 3.0.0 – 2026-10-08

- Airline-Code und Callsign-Präfix `NRW` als Polizeifliegerstaffel NRW bzw.
  Special Type `Police` ergänzt
- Callsigns `HBT…` als Polizeihubschrauber bzw. Special Type `Police` erkannt
  und Airline-Code HBT entsprechend übersetzt
- Betriebs-Pixel an die 2-Bit-Farbtiefe angepasst, damit weißes und rotes
  Blinken im Ruhezustand wieder sicher sichtbar sind
- Scrollgeschwindigkeit der Freitextseite als langsam, mittel oder schnell
  konfigurierbar und dauerhaft gespeichert; Standard ist mittel
- Webserver, mDNS und OTA werden nach einer WLAN-Unterbrechung automatisch
  sauber beendet und nach der Neuverbindung erneut gestartet; Diagnosezähler
  zeigen WLAN-Neuverbindungen und Webserver-Starts
- Web-Display-Abfrageintervall zur Entlastung bei mehreren Clients von einer
  auf zwei Sekunden erhöht
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
