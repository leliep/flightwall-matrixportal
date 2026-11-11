# Changelog

## 3.6.1-dual-target.39 – 2026-11-11

- Routen-Callsigns mit dreibuchstabigem Airline-Präfix und rein numerischer
  Flugnummer tolerieren führende Nullen, z. B. DLH072 und DLH72; andere
  Präfixe, Flugnummern und Buchstabensuffixe bleiben strikt getrennt
- Vergleich gilt für Hauptdienst und ADSBDB; Cache-Schlüssel bleiben unverändert
- ADSBDB meldet abweichendes Callsign und unplausible Position getrennt;
  geografische Plausibilitätsprüfung bleibt erhalten
- Unbekannte Codes beider Controller geprüft: NHT (London-Northolt) samt
  OurAirports-Koordinaten sowie GUN (Grant Aviation) und IXR (iXAir) ergänzt;
  Lookup-Tabellen neu erzeugt und validiert
- Regressionstests für führende Nullen, falsche Präfixe/Nummern, Suffixe und
  getrennte Fehlerpfade; beide Firmwarevarianten gebaut, kein OTA

## 3.6.1-dual-target.38 – 2026-11-10

- Separate Seite „Flughafennamen“ auf den gemeinsamen Vor-/Rücklauf von CPA
  und Kompaktseite umgestellt: 800-ms-Pausen, 80 ms/Pixel vorwärts, 20 ms/Pixel zurück
- Start-/Zielzeilen sowie vollständige Multi-Hop-Codes und -Namen berücksichtigt;
  die längste Zeile bestimmt die vollständige Seitendauer einschließlich Rücklauf
- Bestehendes 50-ms-Zeichenintervall, Konfiguration und WLAN-Setup-Lauftext erhalten
- Regressionstests zeichnen normale und Multi-Hop-Flughafenseiten über den
  vollständigen Ablauf auf; kurze Zeilen bleiben stehen bzw. warten am Anfang
- Beide Controller vorab auf unbekannte Codes geprüft: Listen leer

## 3.6.1-dual-target.37 – 2026-11-10

- CPA-/Sondertyp-Detailzeile nutzt den Vor-/Rücklauf der Kompakt-Flughafenanzeige:
  800-ms-Pausen, 80 ms pro Pixel vorwärts und 20 ms pro Pixel zurück
- Zeichenintervall der CPA-Seite auf 50 ms gesetzt; Seitendauer berücksichtigt
  den vollständigen Hin-/Rücklauf einschließlich Schluss-Pause
- Fehlende/allgemeine Typenbeschreibungen anhand einer lokalen generierten
  Tabelle für Robinson R22/R44/R66 ergänzt; echte Quellbeschreibungen behalten
  Vorrang, unbekannte Typen fallen auf ihren ICAO-Code zurück
- Ursache anhand der Controller-Chroniken eingegrenzt: R44 wurde ohne
  Beschreibung geliefert; Modellergänzung erfolgt nach der Klassifizierung
  gemeinsam für Matrix, Webdisplay und Journal, ohne weitere Netzwerkabfragen
- Operator-Codes MYJ (My Jet) und AME (spanische Militärluftfahrt) aus den
  vorab geprüften Code-Listen ergänzt; Lookup-Tabellen neu erzeugt/validiert
- Regressionstests für Scrollphasen, Dauer, Zeichenintervall und Modell-Fallback
  für beide Varianten ergänzt

## 3.6.1-dual-target.36 – 2026-11-10

- Länger gespeicherte Texte von Tagesjournal, Special-Chronik und RAM-Routencache
  gezielt in externem PSRAM statt Arduino-String-Puffern im internen Heap gehalten
- Unveränderliche, referenzgezählte Textpuffer: identische Inhalte ohne neue
  Reservierung, tatsächliche Freigabe beim Ersetzen, Entfernen und Leeren
- Vorbereitete Einträge werden nur bei vollständig erfolgreicher Textreservierung
  übernommen; bei PSRAM-Mangel bleiben bestehende Einträge und Eintragszähler unverändert
- Kein Rückfall auf internen Textspeicher; neue Speicherdiagnose unter
  `persistent_text_memory` mit belegten Bytes, Puffern und Reservierungsfehlern
- Matrix-Build benötigt gegenüber `.35` etwa 40,5 KiB weniger statischen internen RAM
- Radarspuren, Journal-/Cachegrenzen, Routenlogik, Downloadformate, Flashdaten und
  Controllerkonfigurationen erhalten
- EDWF (Leer-Papenburg) und BMA (Stockholm-Bromma) samt OurAirports-Koordinaten
  aus dem Matrixstatus ergänzt; beide Controller vorab auf Codes geprüft
- Vor OTA zusätzlich aufgelaufenen Operator RFR (Royal Air Force) ergänzt
- Hosttests mit Speicherfehler-Simulation und Address-/UndefinedBehaviorSanitizer
  für Textverwaltung, volle Journale/Caches sowie echte Aktualisierungs-/Resetpfade

## 3.6.1-dual-target.35 – 2026-11-10

- TLS-Speicherreserve wiederhergestellt: Radarverläufe und Spur-Anzeigekopien
  einmalig in externem PSRAM statt statischem internem RAM reserviert
- Matrix-Build benötigt 24.056 Byte weniger statischen internen RAM als
  `.34-diag`; Headless-Build 30.464 Byte weniger; externer Block 24.064 Byte
- Punktierte 60-Sekunden-Spuren für bis zu 64 Ziele auf beiden Varianten
  einschließlich getrennter Netzwerk-/Anzeigekopien erhalten
- Bei fehlendem externem Speicher bleiben Live-Radar und Konfiguration
  erhalten; nur Spuren werden ohne internen Speicher-Fallback ausgelassen
- Speicherverfügbarkeit in Konfiguration, Status, Webradar und APIs sichtbar;
  Routen-/TLS-Diagnose bleibt erhalten
- Tests für fehlgeschlagene Reservierung, Initialisierung, volle Kapazität,
  unabhängige Anzeigekopien und Webanzeige ohne Spurenspeicher ergänzt
- Unbekannte Codes beider Controller vorab geprüft: beide Listen leer

## 3.6.1-dual-target.34-diag – 2026-11-10

- Diagnoseversion für Routenverbindungsfehler: interner freier Heap, größter
  zusammenhängender Block und Heap-Minimum vor Vorbereitung, vor HTTP sowie
  unmittelbar nach HTTP für Hauptdienst und ADSBDB getrennt erfasst
- Negativen Secure-Client-Fehler vor dem Aufräumen ausgelesen; konkrete
  mbedTLS-Fehler mit Dezimal-/Hexadezimalcode und Text, allgemeine
  Verbindungsfehler ausdrücklich ohne erfundene TLS-Details
- Letzten Verbindungs-/HTTP-Fehler über erfolgreiche Anfragen und Cachetreffer
  hinweg bis zum nächsten Fehler oder Neustart im RAM erhalten
- Diagnose in Statusseite und `/api/status`; aktuelle Speicherwerte vor
  Aufbau der Statusantwort gemessen; feste Diagnosedaten belegen 732 Byte
- Airline-Codes HER (Hera Flight) und BRJ (Bairline) aus dem Matrixstatus
  anhand des aktuellen FAA-Verzeichnisses ergänzt, beide Controller geprüft
- Lookup-Tabellen neu erzeugt/validiert; Tests für Fehlererfassung,
  Speichergrenzen, Zeitüberlauf, JSON-Ausgabe und Fehlererhalt ergänzt
- Routenlogik und Konfiguration unverändert; kein OTA als Teil dieser Änderung

## 3.6.1-dual-target.33 – 2026-11-09

- Punktierte Radarspuren über 60 Sekunden für Matrix und Headless-Webradar;
  standardmäßig aktiv und auf beiden Varianten unter Radar abschaltbar
- Fester RAM-Verlauf für bis zu 64 Ziele, altersabhängige Abdunklung,
  Zusammenfassung gleicher Bildpunkte und Ablauf auch bei Empfangsausfällen
- Spurunterbrechung bei Empfangslücken und Positionssprüngen; Zurücksetzen
  bei Änderungen von Quelle, Standort, Radaransicht oder Filtern
- Einstellung in NVS, Konfigurationssicherung/-wiederherstellung und APIs integriert
- EGSY (St Athan) mit Koordinaten aus den Statusmeldungen beider Controller
  ergänzt; Lookup-Tabellen neu erzeugt und validiert, einschließlich des
  bereits in den Quelldaten vorhandenen OBF-Koordinateneintrags
- Lokale Tests für Verlauf, Zeitüberlauf, Kapazität und Webdarstellung ergänzt

## 3.6.1-dual-target.32 – 2026-11-09

- Radar-Umlauf geschlossen: Der helle Punkt endet nun sichtbar wieder auf
  dem 12-Uhr-Pixel, auf dem der Umlauf begonnen hat
- Flughafen OBF (Oberpfaffenhofen) einschließlich Koordinaten sowie Airline
  MAC (Air Arabia Maroc) aus den vor dem Update beobachteten Codes ergänzt

## 3.6.1-dual-target.31 – 2026-11-09

- Radar-Umlauf auf einen kurzen Umlauf von etwa 800 ms pro konfiguriertem
  Intervall begrenzt; kein kontinuierlicher Umlauf mehr
- Umlaufintervall in der Matrix-Konfiguration von 0 bis 10 Sekunden
  einstellbar; `0` schaltet den Umlaufpunkt aus, Standardwert ist 5 Sekunden
- Zeichenlast des Matrix-Radars während der Umlaufpausen reduziert
- Einstellung in Status-API sowie Konfigurationssicherung und
  -wiederherstellung aufgenommen

## 3.6.1-dual-target.30 – 2026-11-09

- testweise heller Radar-Umlaufpunkt auf der Matrix: Start bei 12 Uhr,
  Bewegung im Uhrzeigersinn und vollständige Runde in 800 ms
- Matrix-Radar während der sichtbaren Seite mit 25 ms Zeichenintervall
  aktualisiert; Webradar und übrige Seiten bleiben unverändert
- Airport ERZ (Erzurum) sowie Airlines ARC, AUD, BEA, IAM, JBD, NTF und WLF
  ergänzt; IAM zusätzlich als militärischer Flug klassifiziert

## 3.6.1-dual-target.29 – 2026-11-09

- Headless-Webdisplay um ein responsives Radarbild im 128×32-Seitenverhältnis
  mit gepunktetem Radius, Standortzentrum und bis zu 64 Zielen erweitert
- Webradar übernimmt Orientierung, maßstabstreuen rechteckigen Ausschnitt,
  Farblogik, ausgewähltes Ziel und blinkende Transpondernotfälle der Matrix
- Headless-Controller sammelt den erweiterten Radarbereich unabhängig von den
  gemeinsam gespeicherten Matrixseiten-Einstellungen

## 3.6.1-dual-target.28 – 2026-11-08

- Suchradius der Radaransicht als dezenter Ring aus 32 gleichmäßig verteilten
  Einzelpunkten statt als geschlossene, dominante Kreislinie dargestellt

## 3.6.1-dual-target.27 – 2026-11-08

- Betriebsanzeige-Pixel während der Radaransicht im fehlerfreien Betrieb
  abgeschaltet; bei einer aktuellen Störung blinkt es weiterhin rot
- Airline ATV (Avanti Air) ergänzt

## 3.6.1-dual-target.26 – 2026-11-08

- redundante 12-Uhr-Markierung aus der Radaransicht entfernt; die
  konfigurierte Blickrichtung bleibt definitionsgemäß nach oben ausgerichtet
- Airport ANR (Antwerpen) einschließlich offizieller AIP-Koordinaten ergänzt

## 3.6.1-dual-target.25 – 2026-11-08

- Radaransicht bei ausgeschalteter Wetterseite auch ohne aktuell ausgewähltes
  Flugzeug als dauerhafte Leerlaufanzeige verfügbar
- Radarbereich maßstabstreu über die gesamte Matrixbreite erweitert; der
  konfigurierte Auswahlradius bleibt als unverzerrter Kreis erhalten
- ADS-B-Erfassung für die Radaransicht vom Auswahlradius getrennt, sodass
  seitliche Ziele außerhalb des Kreises die normale Flugzeugwahl nicht ändern

## 3.6.1-dual-target.24 – 2026-11-08

- neue konfigurierbare Radaransicht für die 128×32-Matrix: eigener Standort
  im Zentrum, Blickrichtung nach oben und Anzeigeradius als Kreis
- geeignete Flugzeuge als farbige Punkte; Auswahl, Specials, Bodenverkehr und
  Transpondernotfälle werden hervorgehoben
- Specials außerhalb des normalen Radius werden richtungsgetreu dargestellt;
  maximal 64 Radarziele pro ADS-B-Momentaufnahme
- Airports EDAZ (Schönhagen), HAU (Haugesund) und JJG (Jaguaruna)
  einschließlich Koordinaten sowie Airlines ALX (Alpi Jets), AZE (Arcus Air)
  und UNC (Uni-Fly) ergänzt

## 3.6.1-dual-target.23 – 2026-11-06

- Special-Chronik der Statusseite nach letzter Beobachtung absteigend sortiert
  und Zeitspalte eindeutig als „Zuletzt gesehen“ bezeichnet
- Airports EDLA (Arnsberg-Menden) und EDRJ (Saarlouis-Düren) einschließlich
  Koordinaten sowie Airlines JMP (Business Wings) und WOL (Wings Aviation)
  ergänzt

## 3.6.1-dual-target.22 – 2026-11-05

- vollständige Konfiguration einschließlich Zugangsdaten als JSON sichern und
  über die Konfigurationsseite wiederherstellen
- automatisches ADS-B-Failover auf die zweite lokale beziehungsweise externe
  Quelle mit minütlicher Rückkehrprüfung zur bevorzugten Quelle
- Routen erhalten eine sichtbare Qualitätsbewertung mit Quelle,
  Positionsprüfung und Cache-Hinweis
- Airports BZG (Bydgoszcz) und WIE (Wiesbaden Army Airfield) einschließlich
  Koordinaten ergänzt

## 3.6.1-dual-target.21 – 2026-11-05

- Airports MEL (Melbourne) und SNR (Saint-Nazaire-Montoir) einschließlich
  Koordinaten ergänzt
- Airlines AWC (Titan Airways), CMB (U.S. Transportation Command), ITL
  (ItalFly), JVW (Jetview), MMZ (EuroAtlantic Airways), NVD (Avion Express)
  und VOS (Volaris El Salvador) ergänzt

## 3.6.1-dual-target.20 – 2026-11-04

- ADS-B-Bodenstatus aus `alt_baro: "ground"`, `ground` und `on_ground`
  bleibt bei der Verarbeitung erhalten
- Flugzustand unterscheidet am Boden zwischen `steht` bis 3 km/h, `rollt`
  oberhalb dieser Schwelle und `am Boden` bei fehlender Geschwindigkeit
- stehende Flugzeuge werden nicht ausgewählt; rollende Flugzeuge erscheinen
  nur, wenn kein geeignetes fliegendes Flugzeug im Anzeigeradius vorhanden ist
- Bodenstatus auf Matrix, Webdisplay, Statusseite und in beiden Status-APIs
  ergänzt
- Airport SPX (Kairo-Sphinx) einschließlich Koordinaten sowie Airline FCK
  (FCS Flight Calibration Services) ergänzt

## 3.6.1-dual-target.19 – 2026-11-04

- Special-Chronik auf der Statusseite vor das Tagesjournal verschoben
- Tagesjournal von 64 auf 256 Einträge erweitert; die Statusseite zeigt für
  eine stabile und schnelle Darstellung die neuesten 64 Einträge
- JSON- und CSV-Downloads liefern weiterhin das vollständige Journal und
  werden nun speicherschonend in Blöcken übertragen
- Airport EDLF (Grefrath-Niershorst) einschließlich Koordinaten sowie Airlines
  AAO, BGF, FYL, RYS und TOY ergänzt

## 3.6.1-dual-target.18 – 2026-11-04

- Status und Konfiguration auf eigenständige Seiten `/status` und `/config`
  verteilt; `/` führt jetzt zur Statusseite
- gemeinsame responsive Navigation zu Status, Konfiguration, FlightWall-
  Webdisplay und Firmwareupdate ergänzt
- automatische Aktualisierung läuft nur noch auf der Statusseite; Eingaben
  und geöffnete Bereiche der Konfigurationsseite bleiben unberührt
- Airports AQJ (Aqaba) und HAV (Havanna) einschließlich Koordinaten ergänzt

## 3.6.1-dual-target.17 – 2026-11-04

- Routenprüfung toleriert Anflugschleifen, Warteschleifen, Gegenanflug und
  Radarvektoren innerhalb von 100 km um den Zielflughafen
- die momentane Flugrichtung wird nicht mehr als hartes Ausschlusskriterium
  verwendet; die geografische Korridorprüfung bleibt aktiv
- unplausible Momentaufnahmen löschen einen vorhandenen persistenten
  Routencache-Eintrag nicht mehr automatisch
- Airports ADA (Adana-Şakirpaşa) und GOX (Goa-Manohar) einschließlich
  Koordinaten sowie Airlines BQB (Axis Aviation Austria), EVA (EVA Air), MBK
  (Chrono Jet) und NCR (National Airlines) ergänzt

## 3.6.1-dual-target.16 – 2026-11-03

- tägliche Special-Chronik für bis zu 64 erkannte Specials und
  Transpondernotfälle, unabhängig davon, ob sie zur Anzeige ausgewählt wurden
- Chronik mit Zeit, Klassifizierung, Typ, Minimalentfernung, Minimal-CPA und
  Anzeigestatus auf der Statusseite sowie als `/specials.json` und
  `/specials.csv`
- Auswahlbegründung für das aktuelle Flugzeug im Webdisplay, Status und in den
  APIs; Begründung zusätzlich im Tagesjournal und in beiden Downloadformaten
- Auswahlgrund unterscheidet Transponderpriorität, Special im doppelten Radius
  und geringste Entfernung unter allen geeigneten Kandidaten
- taggesteuerte GitHub Releases mit eindeutig benannten Matrix-/Headless-
  Images, SHA-256-Prüfsummen, Firmwaremanifest und generierten Release Notes

## 3.6.1-dual-target.15 – 2026-11-03

- sekundengenauer CPA-Countdown in den letzten drei Minuten auf Matrix,
  Webdisplay, Statusseite sowie in `/api/display` und `/api/status`
- flüchtiges Tagesjournal für bis zu 64 tatsächlich angezeigte Flugzeuge mit
  Zeit, Typ, Route, Klassifizierung, Minimalentfernung und Minimal-CPA
- Journalanzeige im Status und Download über `/journal.json` und
  `/journal.csv`; automatischer Reset bei lokalem Datumswechsel
- Flughäfen EDLS (Stadtlohn-Vreden), JER (Jersey) und SJD (Los Cabos) sowie
  Airlines JSF (Jet Stream Charter), KLJ (KlasJet), NIT (Middle Georgia State
  University) und SPA (Sierra Pacific Airlines) ergänzt

## 3.6.1-dual-target.14 – 2026-11-02

- aktuelle IP-Adresse und Verbindungszustand im WLAN-Konfigurationsbereich
  hervorgehoben
- Netzwerkstatus um tatsächliche und gespeicherte SSID, Signalbewertung,
  Gateway, Subnetzmaske, DNS-Server, WLAN-Kanal und MAC-Adresse erweitert
- `/api/status` liefert die zusätzlichen Netzwerk- und Setup-WLAN-Daten auch
  für externe Diagnosewerkzeuge

## 3.6.1-dual-target.13 – 2026-11-02

- Headless-Status zeigt die nicht vorhandene Matrix-Wetteranzeige nicht mehr
  fälschlich als aktiv an
- `environment_idle_display_enabled` ist beim Headless-Controller immer
  `false`; BME280- und MQTT-Wetterstatus bleiben unverändert verfügbar

## 3.6.1-dual-target.12 – 2026-11-02

- Airports `LIL` (Lille), `LSY` (Lismore) und `NYO` (Stockholm-Skavsta)
  einschließlich Routenkoordinaten ergänzt
- Airlines `ABF` (Scanwings), `BSF` (Blue Sky Airways), `CLU`
  (CargoLogicAir), `LOG` (Loganair) und `POF` (Police Aux Frontières) ergänzt

## 3.6.1-dual-target.11 – 2026-11-02

- Airline-Code `ABP` (ABS Jets) ergänzt
- README, Getting-Started- und PlatformIO-Anleitung auf den aktuellen Stand
  der beiden Controller-Varianten, Notfallrotation und Tagesstatistik gebracht

## 3.6.1-dual-target.10 – 2026-11-02

- mehrere Transpondernotfälle wechseln alle zehn Sekunden unabhängig vom
  Matrixseitenzyklus; Webdisplay und Headless-Controller wechseln nun ebenfalls
- tägliche Zählung von bis zu 4096 eindeutigen, frischen ADS-B-Flugzeugen
- Special- und Notfallstatistik wird bei lokalem Datumswechsel zurückgesetzt
- Statusseite zeigt Gesamtzahl, Special-Anteil, Erfassungsbeginn und beide
  Kapazitätsgrenzen
- additive Tagesstatistik in `/api/status` ergänzt

## 3.6.1-dual-target.9 – 2026-11-02

- Transpondercode auf der Matrix-Notfallseite groß und mittig dargestellt
- eindeutige Notfallbedeutung in der unteren Zeile; `7600` gelb sowie `7500`
  und `7700` rot hervorgehoben
- Positionsanzeige bei mehreren gleichzeitigen Transpondernotfällen ergänzt
- sichere Ersatzanzeige `7???` bei fehlendem Notfallcode
- Airports `CHR` (Châteauroux) und `TLN` (Toulon-Hyères) ergänzt
- Airlines `AJD`, `EMC`, `PEA` und `TEU` ergänzt

## 3.6.1-dual-target.8 – 2026-11-01

- Gültigkeit regulärer Routen von sieben Tagen auf sechs Stunden reduziert,
  damit wiederverwendete Callsigns keine alten Strecken übernehmen
- 468 Flughafenkoordinaten aus dem gemeinfreien OurAirports-Datensatz als
  kompakte, offline verwendete Firmwaretabelle ergänzt
- Primär- und ADSBDB-Routen werden gegen aktuelle Position, Höhe,
  Flugrichtung und Steig-/Sinkbewegung geprüft
- Rückkehr eines Callsigns nach 30 Minuten Abwesenheit sowie ein Wechsel der
  Flugzeug-Hex-ID erzwingen eine frische Routenabfrage
- unplausible Cache- und Dienstergebnisse werden verworfen, nicht angezeigt
  und nicht erneut im Cache gespeichert
- reproduzierbare Aktualisierungs-, Generator- und Validierungsskripte für die
  Koordinatentabelle ergänzt

## 3.6.0-dual-target.2 – 2026-10-28

- Webdisplay zeigt zusätzlich zur Typkennung die ausführliche
  Flugzeugtypenbezeichnung; fehlende oder identische Angaben werden verborgen
- additive API-Eigenschaft `aircraft_type_description` in `/api/display` und
  `/api/status`
- Airline-Codes `DWW` (DAS Private Jets) und `TLI` (TL Aviation) ergänzt
- Radius, Standorthöhe und Blickwinkel werden auch in der Headless-Variante
  dauerhaft gespeichert und bleiben bei OTA-Neustarts erhalten

## 3.6.0-dual-target.1 – 2026-10-28

- zweites PlatformIO-Buildziel für ESP32-S3-DevKitC-1-N16R8 ohne LED-Matrix
- gemeinsamer ADS-B-, Routen-, Wetter-, Web-, Diagnose- und OTA-Kern für
  MatrixPortal- und Headless-Variante
- Protomatter, HUB75-DMA, Matrixseiten, Helligkeitstaster und BH1750 werden im
  Headless-Build nicht initialisiert
- eigene Gerätekennung und Adresse `flightwall-server.local`; Hardwarevariante
  zusätzlich in Status-API und OTA-Seite sichtbar
- matrixbezogene Eingabefelder werden in der Headless-Konfiguration entfernt
- BME280 am Headless-Board optional über GPIO 16/17
- GitHub Actions baut beide Firmwaredateien und stellt sie getrennt bereit

## 3.5.5-code-download.1 – 2026-10-28

- unbekannte Airport- und Airline-Codes können über die Statusseite als
  `flightwall-unknown-codes.json` heruntergeladen werden
- Download enthält Firmwareversion, Laufzeit, beide Codelisten, Kapazität und
  Überlaufzähler; die gesammelten Codes werden dabei nicht gelöscht
- Überlaufwerte zusätzlich auf der Statusseite und in `/api/status` sichtbar
- Flughäfen `RMS` (Ramstein), `BNJ` (Bonn-Hangelar) und `BES` (Brest) sowie
  Airline `GFM` (Grafair) ergänzt

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
