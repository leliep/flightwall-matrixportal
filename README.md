# FlightWall MatrixPortal

FlightWall ist eine eigenständige Flugzeuganzeige für den ESP32-S3. Das Projekt
liefert zwei gleichwertig gepflegte Firmwarevarianten: Der **Matrixcontroller**
steuert mit einem Adafruit MatrixPortal S3 zwei HUB75-Panels an; der
**Headless-Controller** läuft auf einem ESP32-S3-N16R8 ohne LED-Matrix und nutzt
die responsive FlightWall-Webseite als Anzeige. Beide Varianten lesen
ADS-B-Daten direkt ein, wählen das relevanteste Flugzeug aus und ergänzen Route
sowie Klartexte. Ein Raspberry Pi oder separater FlightMatrixServer ist nicht
erforderlich.

![FlightWall-Webdisplay mit KLM877, Route Amsterdam–Mumbai und CPA-Countdown](docs/images/flightwall-web-display-2026-11-16.png)

*Webdisplay im laufenden Betrieb: Flugzeugtyp, Airline, Route, Entfernung,
Höhe, Geschwindigkeit, Blickrichtung und CPA-Countdown auf einen Blick.*

**Neu hier?** Die Anleitung [Getting Started](GETTING_STARTED.md) führt vom
Hardwareaufbau über die Ersteinrichtung bis zur ersten Flugzeuganzeige und
erklärt auch BME280, BH1750, MQTT, Routenauflösung und OTA.

Weitere Dokumentation:

- [Getting Started](GETTING_STARTED.md) – Aufbau und Ersteinrichtung
- [PlatformIO-Hinweise](PLATFORMIO.md) – Build, Upload und Entwicklung
- [Changelog](CHANGELOG.md) – Änderungen nach Firmwareversion
- [GitHub Releases](https://github.com/leliep/flightwall-matrixportal/releases) – fertige OTA-Images

## Schnellstart

1. Hardwarevariante auswählen: MatrixPortal mit 128×32-Matrix oder ESP32-S3
   N16R8 ohne Matrix.
2. Repository klonen und `include/secrets.h.example` nach
   `include/secrets.h` kopieren.
3. Erstmalige WLAN-Daten und optional ein OTA-Passwort eintragen.
4. Das zur Hardware passende PlatformIO-Environment per USB installieren.
5. `/config` öffnen und Standort, Blickrichtung, Radius sowie ADS-B-Quelle
   prüfen.
6. Unter `/status` den erfolgreichen ADS-B-Abruf kontrollieren und
   anschließend `/display` beziehungsweise die Matrix verwenden.

Die Kurzfassung ersetzt nicht die Hinweise zu Matrixstromversorgung,
unterschiedlichen Firmwareimages und WLAN-Rückfallmodus weiter unten.

## Inhalt

- [Hardware und Verkabelung](#hardware)
- [ADS-B-Daten und Flugzeugauswahl](#ads-b-daten-und-flugzeugauswahl)
- [Routen, Klartexte und Cache](#routen-klartexte-und-cache)
- [Matrixanzeige und Radar](#matrixanzeige)
- [Wetterseite und Schimmelrisiko](#wetterseite)
- [Weboberfläche](#weboberfläche)
- [Liste der Konfigurationsoptionen](#liste-der-konfigurationsoptionen)
- [Installation per USB](#installation-per-usb)
- [OTA-Updates](#ota-updates)
- [Übersetzungstabellen](#übersetzungstabellen-pflegen)
- [Diagnose und Tests](#diagnose-und-tests)
- [Betrieb und Fehlersuche](#betrieb-und-fehlersuche)
- [Sicherheit und Grenzen](#sicherheit-und-grenzen)

## Aktueller Stand

| Merkmal | Stand |
|---|---|
| Firmware | `3.6.1-dual-target.67` |
| Controller | Adafruit MatrixPortal S3 oder ESP32-S3 DevKitC-1 N16R8 |
| Anzeige Matrixcontroller | 2 × 64×32 HUB75 (128×32 Pixel) oder eigener 128×64-Build, optional 125 nutzbare Spalten, und Web-Display |
| Anzeige Headless-Controller | responsives Web-Display |
| Farbtiefe | 3 Bit je Farbkanal |
| Framework | Arduino über PlatformIO |
| Bedienoberfläche | Deutsch und Englisch |
| Firmwareaktualisierung | USB, Web-OTA oder PlatformIO-OTA |
| Lookup-Aktualisierung | gemeinsame JSON-Ergänzungen per Upload, ohne Firmwarebau oder Neustart |
| Gerätestatus | gemeinsame RGB-LED-Logik; zusätzlicher Betriebs-Pixel auf der Matrix |
| Lizenz | MIT |

Die aktuelle Firmware ist gezielt für **128×32 Pixel aus zwei horizontal
verketteten 64×32-Panels** ausgelegt. Zusätzlich gibt es einen eigenen Build für ein **128×64-HUB75E-Panel mit
1/32-Scan und binärer ABCDE-Adressierung**. Er zeigt oben die konfigurierten
Informationsseiten und unten dauerhaft das Radar. Andere Scanverfahren sind
weiterhin kein direkt unterstützter Ersatz.

## Eindrücke aus dem Betrieb

![FlightWall MatrixPortal mit aktuellen Flugdaten](IMG_4482.png)

*Der reale Aufbau mit zwei horizontal verketteten 64×32-HUB75-Panels.*

Die Screenshots in dieser README stammen vom 16. November 2026 und zeigen
Firmware `.53`. Flug- und Messwerte sind Momentaufnahmen; aktivierte Optionen
und vorhandene Sensoren richten sich nach dem jeweiligen Aufbau.

Die folgende Aufnahme zeigt den automatischen WLAN-Rückfallmodus direkt auf
der Matrix. SSID, Passwort und Konfigurationsadresse bleiben dadurch auch ohne
Zugriff auf den seriellen Monitor sichtbar.

![FlightWall MatrixPortal im WLAN-Einrichtungsmodus](IMG_4788.jpeg)

[▶ Video: Flugzeuganzeige im laufenden Betrieb (MOV, ca. 22 MB)](IMG_4794.mov)

Das Video zeigt den realen Aufbau mit Callsign, Flugzeugtyp, CPA sowie
Annäherungsinformation. GitHub öffnet die MOV-Datei je nach Browser direkt oder
bietet sie zum Herunterladen an.

Wesentliche Funktionen:

- direkte Verarbeitung lokaler readsb-/dump1090-Daten und der externen
  adsb.fi-API
- Auswahl des nächsten geeigneten Flugzeugs einschließlich CPA-Berechnung
  und sekundengenauem Countdown in den letzten drei Minuten
- konfigurierbare Filter für Kleinflugzeuge und Maximalhöhe
- bevorzugte Behandlung interessanter Flugzeuge und Transpondernotfälle
- große Notfallseite und automatische Rotation mehrerer Notfallflugzeuge in
  Matrix- und Webanzeige
- tägliche Statistik und herunterladbares Tagesjournal der angezeigten
  Flugzeuge, Specials und Transpondernotfälle
- tägliche Special-Chronik für alle erkannten Sonderflugzeuge sowie eine
  nachvollziehbare Auswahlbegründung
- Routenauflösung über adsb.im mit kostenlosem ADSBDB-Fallback und
  verwaltetem RAM-/Flash-Cache sowie sichtbarer Qualitätsbewertung
- automatisches Failover zwischen den beiden lokalen ADS-B-Quellen und
  adsb.fi mit regelmäßiger Rückkehrprüfung zur bevorzugten Quelle
- vollständige Multi-Hop-Routen und kompakte Darstellung geschlossener Umläufe
- integrierte Flughafen- und Airline-Klartexte
- acht frei auswählbare und sortierbare Matrixseiten
- optional 125 statt 128 nutzbare Matrixspalten bei beschädigtem rechten Panelrand
- separat aktualisierbare Lookup-Ergänzungen ohne Firmwareupdate
- maßstabstreue Radaransicht über die gesamte Matrixbreite mit
  Standortzentrum, Blickrichtung, unverzerrtem Radiuskreis und bis zu
  64 gleichzeitig sichtbaren Flugzeugpunkten; bei deaktivierter Wetterseite
  auch im Leerlauf sichtbar
- responsive Webanzeige für iPhone, Tablet und Desktop; auf der
  Matrix- und Headless-Variante einschließlich anklickbarem Live-Radar
- deutsch/englisch umschaltbare Matrix- und Weboberfläche
- BME280-Klimawerte, Schimmelrisiko und optionale MQTT-Wetterdaten
- BH1750-Helligkeitsautomatik mit sieben stabilen Helligkeitsstufen
- umfangreiche Status-, Test- und Diagnosefunktionen
- änderbare WLAN-Zugangsdaten mit geschütztem Einrichtungsnetz als Rückfallweg
- vollständige Konfigurationssicherung und -wiederherstellung als JSON-Datei
- Dual-Core-Betrieb mit eigener Web-, Daten- und MQTT-Verarbeitung

## Systemaufbau

```text
readsb / dump1090 ─┐
                   ├─> ADS-B-Auswahl ─> Route/Klartexte ─┬─> LED-Matrix (optional)
adsb.fi ───────────┘          │                  │       └─> Web-Display
                              ├─> RGB-Status-LED ├─> Routencache
                              └─> Status-API     └─> unbekannte Codes

BME280 ───────────────> Klima-/Schimmelbewertung ─┐
Mosquitto ────────────> Wind/Böen/Regen ──────────┴─> Wetterseite
BH1750 ───────────────> automatische Helligkeit
```

Die Arduino-Hauptschleife auf Core 1 bedient Matrix, Tasten und Sensoren.
Auf Core 0 arbeiten drei getrennte FreeRTOS-Tasks: Der Daten-Task übernimmt
WLAN und ADS-B-/Routenabfragen, der Web-Task bedient die HTTP-Oberfläche und
Web-OTA, der MQTT-Task hält die Wetterverbindung und führt MQTT-Tests aus.
Blockierende externe Abfragen geben die Sperre der gemeinsamen Daten frei;
dadurch bleiben die Webseiten während ADS-B- und Routenabfragen bedienbar.
Flugzeug- und Wetterdaten werden über geschützte Kopien übergeben.

## Hardware

### Unterstützte Varianten

| Variante | PlatformIO-Environment | Anzeige | optionale Sensoren | mDNS-Name |
|---|---|---|---|---|
| Matrixcontroller: Adafruit MatrixPortal S3 | `adafruit_matrixportal_esp32s3` | 128×32 HUB75-Matrix und Web | BH1750, BME280 | `matrixportal.local` |
| Headless-Controller: ESP32-S3 N16R8 | `esp32s3_n16r8_headless` | Web-FlightWall | BME280 | `flightwall-server.local` |

Beide Varianten verwenden denselben ADS-B-, Routen-, Klassifizierungs-,
Wetter-, Konfigurations-, Webserver- und RGB-Statuscode. Matrixansteuerung,
Taster, BH1750-Automatik und Matrixseiten werden nur im Matrix-Build
initialisiert. Ein optionaler BME280 wird beim Headless-Controller über SDA
GPIO 16 und SCL GPIO 17 angeschlossen. Einstellungen und Datenformate bleiben
zwischen beiden Varianten weitgehend identisch.

### Benötigte Komponenten: Matrixcontroller

- Adafruit MatrixPortal S3
- zwei kompatible 64×32-RGB-HUB75-Panels mit 1/16-Scan
- ausreichend dimensioniertes, stabilisiertes 5-V-Netzteil für die Panels
- zwei HUB75-Flachbandkabel
- WLAN mit Zugriff auf mindestens eine ADS-B-Quelle

### Benötigte Komponenten: Headless-Controller

- ESP32-S3-DevKitC-1-kompatibles Board mit ESP32-S3-N16R8-Modul
- USB-C-Datenkabel und geeignete USB-Stromversorgung
- WLAN mit Zugriff auf mindestens eine ADS-B-Quelle

Eine LED-Matrix, ein MatrixPortal, ein BH1750 und ein separates
5-V-Matrixnetzteil werden für diese Variante nicht benötigt. Das eigentliche
FlightWall-Display läuft im Browser auf iPhone, Tablet oder Desktop.

Die Fotos zeigen den Headless-Controller im 3D-gedruckten Gehäuse: links
mit Deckel, rechts geöffnet mit sichtbarem ESP32-S3-N16R8-Modul. Die
USB-Verbindung versorgt das Board; die RGB-Status-LED bleibt durch die
Gehäuseöffnung sichtbar. Die beschrifteten RESET- und BOOT-Tasten sind auch
bei geschlossenem Gehäuse zugänglich.

| Geschlossenes Gehäuse | Offener Aufbau |
|---|---|
| <img src="docs/images/flightwall-headless-case-2026-11-16.jpeg" alt="FlightWall-Headless-Controller im geschlossenen Gehäuse mit RESET-/BOOT-Tasten und sichtbarer Status-LED" width="320"> | <img src="docs/images/flightwall-headless-open-2026-11-16.jpeg" alt="Geöffneter FlightWall-Headless-Controller mit ESP32-S3-N16R8 und USB-Stromversorgung" width="320"> |


Optional:

- BH1750 zur automatischen Helligkeitsregelung
- BME280 für Temperatur, Luftfeuchte und Luftdruck
- Mosquitto-kompatibler MQTT-Broker für Wind und Regen

| Option | Matrixcontroller | Headless-Controller |
|---|---|---|
| BME280 | Status und Wetter-Ruheanzeige | Status/API; keine lokale Matrixseite |
| BH1750 | automatische Matrixhelligkeit | nicht verwendet |
| MQTT-Wetter | Wetter-Ruheanzeige und Status | Status/API; keine lokale Matrixseite |
| Browser | zusätzliche Anzeige und Bedienung | primäre Anzeige und Bedienung |
| adsb.im / ADSBDB | Routenauflösung | Routenauflösung |

### Eigenständige 128×64-Variante

Für das Panel **P2-1515-128X64-32S-S2** mit ICN2037BP ist das Environment
`adafruit_matrixportal_esp32s3_128x64` vorgesehen. Die Hardwarekennung lautet
`matrixportal_s3_128x64`; dieses Image gehört ausschließlich auf den Controller
mit dem 128×64-Panel, nicht auf die bisherige 128×32-Hardware.

- Ein Panel, 128×64 Pixel, 1/32-Scan, fünf binäre Adressleitungen ABCDE.
- Für das bestätigte ICN2037BP-Panel sind seit `.65` R1/B1 und R2/B2
  hardwareseitig in der Pinzuordnung getauscht; Grün und Webfarben bleiben gleich.
- E wird durch GPIO 21 angesteuert. Vor Installation die tatsächliche
  E-Belegung des Panels und des MatrixPortal-Nachbaus prüfen; für E auf
  HUB75-Pin 8 muss der entsprechende 8/16-Jumper auf 8 stehen.
- Unter `/config` → Radar ist „Radar ohne angezeigtes Flugzeug“ wählbar:
  **Aus** (leeres Panel mit Blink-Pixel unten links), **Untere Hälfte** (Standard)
  oder **Volle Höhe** (Radar über 64 Zeilen). Auslöser ist kein anzeigbares
  Flugzeug im eingestellten Anzeigeradius; weiter entfernte Radarziele verhindern
  den Leerlauf nicht. Aus und volle Höhe ersetzen die Wetter-Leerlaufanzeige.
  Sobald wieder ein Flugzeug angezeigt wird, gilt automatisch das normale
  Layout. Nachtruhe, Display-Aus, Start-/Fehler- und OTA-Anzeigen behalten Vorrang.
  Gespeichert als `matrix_idle_radar` (`off`, `lower`, `full`), nur beim 128×64-Build;
  Sicherung, Wiederherstellung und Status enthalten die Auswahl. Alte Sicherungen
  ohne diesen Wert erhalten die bestehende Auswahl.
- Oben 128×32 Pixel mit den bisherigen konfigurierten Informationsseiten;
  unten 128×32 Pixel mit dauerhaftem Radar, einschließlich Spuren, Punkt-/
  Strichumlauf und konfigurierbarer Geschwindigkeit. Die separate Radarseite
  wird oben übersprungen, ohne die gespeicherte Seitenauswahl zu verändern.
- Sind nur Radar oder inhaltslose Seiten ausgewählt, erscheinen bei einem
  aktuellen Flug oben die Flugdaten als Rückfallanzeige.
- Helligkeit, Nachtabschaltung, Ausschalten der Anzeige, OTA und Startanzeige
  behalten Vorrang. Ohne ausgewählten Flug zeigt der untere Bereich weiter
  Radar; oben erscheint gegebenenfalls die konfigurierte Wetteranzeige.
- Displaytest und die optionale Maskierung der rechten drei Spalten umfassen
  alle 64 Zeilen. `/api/status` meldet `matrix_height` und `matrix_scan_rows`.
- Drei Bit Farbtiefe und doppelte Pufferung bleiben erhalten; DMA-Bedarf und
  Bildpuffer wachsen. Reale Bildwiederholrate, Zeilen-/Farbreihenfolge und
  TLS-Speicherreserve müssen nach der Inbetriebnahme am Panel geprüft werden.

Die Quelltexte und Webanzeige bleiben gemeinsam mit 128×32 und Headless
gepflegt. Dieser Build wird zusätzlich im GitHub-Workflow geprüft und als
separates Artefakt bereitgestellt.

### Matrixverkabelung

Die Panels werden horizontal verkettet:

```text
MatrixPortal HUB75 ─> IN Panel 1
OUT Panel 1 ────────> IN Panel 2
```

Beide Panels werden parallel aus dem separaten 5-V-Netzteil versorgt. Das
MatrixPortal, das Netzteil und beide Panels benötigen eine gemeinsame Masse.
Die Panels dürfen nicht aus dem USB-Anschluss des MatrixPortal gespeist werden.
Vor Änderungen an Datenkabeln oder Stromversorgung ist die 5-V-Versorgung
abzuschalten.

Die Firmware verwendet die fest verdrahteten HUB75-Signale des MatrixPortal S3:

| HUB75 | GPIO | HUB75 | GPIO |
|---|---:|---|---:|
| R1 | 42 | R2 | 38 |
| G1 | 41 | G2 | 39 |
| B1 | 40 | B2 | 37 |
| A | 45 | CLK | 2 |
| B | 36 | LAT | 47 |
| C | 48 | OE | 14 |
| D | 35 | E | nicht verwendet |

Andere Panelauflösungen, Scanverfahren oder Panels mit E-Adressleitung
benötigen eine angepasste Protomatter-Konfiguration.

### Sensoranschlüsse

Der BH1750 wird im aktuellen Aufbau am gelöteten I²C-Bus betrieben:

| BH1750 | MatrixPortal S3 |
|---|---|
| VIN | 3,3 V |
| GND | GND |
| SDA | A1 / GPIO 3 |
| SCL | A2 / GPIO 9 |

Die Adressen `0x23` und `0x5C` werden automatisch erkannt. Fällt der Sensor
aus, verwendet die Firmware die gespeicherte manuelle Helligkeit und versucht
regelmäßig eine erneute Erkennung.

Der BME280 wird auf `0x76` und `0x77` gesucht – zuerst am Bus GPIO 3/9 und
anschließend am STEMMA-QT-Bus GPIO 16/17. Er liefert:

- Umgebungstemperatur in °C
- relative Luftfeuchte in Prozent
- gemessenen Stationsdruck
- anhand der konfigurierten Standorthöhe auf Meereshöhe reduzierten Luftdruck
- Taupunkt, kritische Oberflächentemperatur und Schimmelrisiko

Die interne ESP32-S3-Temperatur steht zusätzlich als Diagnosewert bereit. Sie
ist keine Messung der Raumtemperatur.

### RGB-Status-LED

Beide Controller-Varianten verwenden ihre integrierte adressierbare RGB-LED mit
derselben Logik. Die Helligkeit ist in der Konfiguration als **aus**,
**schwach** oder **normal** wählbar. Während der konfigurierten Nachtruhe wird
die LED zusätzlich gedimmt.

| Farbe und Verhalten | Bedeutung |
|---|---|
| Orange beim Start | Firmware startet |
| Türkis blinkend | WLAN-Einrichtungsnetz ist aktiv |
| Blau blinkend | Verbindung mit dem konfigurierten WLAN wird aufgebaut |
| Grün mit kurzem hellerem Herzschlag | Normalbetrieb; auch die Hauptschleife ist aktiv |
| Orange blinkend | ADS-B-Daten fehlen oder sind nicht aktuell |
| Violett | Firmwareupdate läuft |
| Rot blinkend | Fehler |
| Rot schnell blinkend | Transpondernotfall `7500`, `7600` oder `7700` |
| Gelb für etwa 180 ms | manueller HTTP-/Webzugriff |

Regelmäßige Statusabfragen der Weboberfläche lösen den gelben Impuls nicht aus,
damit die LED im normalen Betrieb ruhig bleibt. Update, Fehler und Notfall
haben Vorrang vor der HTTP-Anzeige. Der aktuelle logische Zustand steht auch
als `status_led_state` in `/api/status`.

Beim Matrixcontroller bleibt zusätzlich der kleine Betriebs-Pixel unten links
erhalten. Er gehört zur LED-Matrix und ist unabhängig von der RGB-LED: weiß
bedeutet gültige ADS-B-Daten, rot einen Abruffehler. Beim Headless-Controller
gibt es naturgemäß nur die RGB-Status-LED.

## ADS-B-Daten und Flugzeugauswahl

Es stehen drei auswählbare Quellen zur Verfügung:

1. frei konfigurierbare lokale URL, typischerweise `aircraft.json`
2. zweite frei konfigurierbare lokale URL
3. externe [adsb.fi Open Data API](https://adsb.fi/)

Nur die aktive Quelle wird im regulären Betrieb abgefragt. Die beiden lokalen
Quellen akzeptieren das readsb-/dump1090-Format mit `aircraft`; adsb.fi liefert
das v3-Format mit `ac`. Zeitstempel in Sekunden und Millisekunden werden intern
vereinheitlicht. Die Standardabfrage erfolgt alle fünf Sekunden.

Für adsb.fi erzeugt die Firmware die URL automatisch aus Standort und
Anzeigeradius. Der abgefragte Radius ist groß genug für den verdoppelten
Special-Type-Radius. Die öffentliche API ist für persönliche,
nichtkommerzielle Nutzung vorgesehen; FlightWall bleibt mit einer Abfrage alle
fünf Sekunden deutlich unter dem Limit von einer Anfrage pro Sekunde.

### Auswahlregeln

Im Normalfall wird das nächstgelegene geeignete Flugzeug gewählt. Dabei gelten:

- normaler Anzeigeradius: frei konfigurierbar
- Special Type: doppelter Anzeigeradius
- optionaler Filter für Sport- und Kleinflugzeuge
- optionaler Filter für Flugzeuge oberhalb einer Maximalhöhe in ft
- frische Position erforderlich

Der Kleinflugzeugfilter berücksichtigt unter anderem Kategorie `A1`, Typen wie
`PRIM` und `C152`, Segler-/Ultraleicht-Beschreibungen sowie deutsche
Kennzeichen nach den Mustern `D-E…`, `D-K…` und `D-M…`. Ein erkannter Special
Type wird nicht allein wegen des Kleinflugzeugfilters verworfen.

Für Flugzeuge mit Geschwindigkeit und Track berechnet der ESP32 den Closest
Point of Approach (CPA), die Zeit bis zum CPA und ob sich das Flugzeug nähert
oder entfernt. In den letzten drei Minuten vor dem CPA wird die Restzeit auf
Matrix, Webdisplay, Statusseite und in den APIs sekundengenau heruntergezählt.

### Bodenverkehr

ADS-B-Meldungen mit `alt_baro: "ground"`, `ground` oder `on_ground` behalten
ihren Bodenstatus. Die Anzeige unterscheidet:

- `steht` bis einschließlich 3 km/h
- `rollt` oberhalb von 3 km/h
- `am Boden`, wenn keine brauchbare Geschwindigkeit vorliegt

Stehende Flugzeuge werden nicht ausgewählt. Rollende Flugzeuge können
erscheinen, wenn kein geeignetes fliegendes Flugzeug im normalen Radius
vorhanden ist. Im Radar wird Bodenverkehr orange dargestellt.

### Special Types

Military, German Airforce, Bundespolizei, Polizei, Regierung, Hubschrauber,
Luftschiff und A380 werden besonders klassifiziert und auf allen
Flugzeugseiten mit eigenen Symbolen dargestellt. Die Erkennung verwendet
ADS-B-Metadaten und bekannte Callsign-Muster. Dazu gehören unter anderem
`GAF…`, `VAD…`/`VADER…`, `SHO…`/`SHOCK…`, `BURRO…`, `HAMMER…`, `SMASH…`,
`CONAN…`, `FALCON…`, `THOR…`, `VIRUS…`, `SONIC…`, `FELON…`, `IAM…`,
`BPO…`, `POL…`, `HBT…`, `HUM…` und `NRW…`. Die vollständige, tatsächlich
verwendete Regelbasis steht in `include/CallsignClassifier.hpp` und
`src/AircraftProcessor.cpp`.

Die Statusseite zählt pro Kalendertag eindeutige Flugzeuge und weist den
Anteil der Specials aus. Specials werden zusätzlich nach Kategorie aufgeteilt.
Bis zu 4096 Flugzeug- und 256 Special-Identitäten werden über ICAO-Hexadresse,
Registrierung oder Callsign unterschieden. Die flüchtige Statistik wird um
Mitternacht Ortszeit sowie bei einem Neustart zurückgesetzt.

### Transpondernotfälle

Die Squawk-Codes erhalten immer höchste Priorität:

| Code | Anzeige | Bedeutung |
|---|---|---|
| `7700` | Emergency | allgemeiner Notfall |
| `7600` | Radio | Funkausfall |
| `7500` | Hijack | Entführung |

Diese Flugzeuge umgehen Anzeigeradius, Maximalhöhe und Kleinflugzeugfilter.
Eine zusätzliche, nicht abschaltbare Alarmseite zeigt Identität, Code,
Bedeutung und bei mehreren Notfällen deren Position in der Auswahl. Sind
mehrere Notfälle vorhanden, wechselt der Server alle zehn Sekunden zum
nächsten Flugzeug. Matrix und Webdisplay verwenden dieselbe Auswahl; der
Wechsel funktioniert daher auch in der Headless-Variante.

Das Webinterface kann ein bis drei Testflugzeuge für höchstens zwei Minuten
simulieren. Der Test verändert weder ADS-B-Daten noch gespeicherte
Einstellungen.

## Routen, Klartexte und Cache

Der optionale Routendienst fragt standardmäßig `http://adsb.im/api/0/routeset`
mit Callsign und aktueller Position ab. Er ergänzt:

- Airline beziehungsweise Betreiber
- Start, Ziel und Zwischenstopps
- kurze Airport-Codes
- kompakte deutschsprachige Airport- und Airline-Bezeichnungen

Ab `.56` entspricht der Abruf den [aktuellen adsb.im-Vorgaben](https://adsb.im/api):
HTTP statt HTTPS, eine eigene FlightWall-Clientkennung mit öffentlicher
Kontaktmöglichkeit über die Projekt-Issues und gemeinsame POST-Abfragen nach
mindestens 30 Sekunden Sammelzeit. Zwischen zwei Batch-Anfragen liegen ebenfalls
mindestens 30 Sekunden. Bereits gespeicherte adsb.im-HTTPS-Adressen werden nur für
den tatsächlichen Abruf auf HTTP umgesetzt; die gespeicherte Konfiguration bleibt
erhalten. Andere Routendienst-Adressen werden nicht umgeschrieben.

Die Warteschlange fasst bis zu acht unterschiedliche ausgewählte Callsigns zusammen;
mehrfache Beobachtungen aktualisieren deren Position, statt weitere Einträge anzulegen.
Neue Routen erscheinen deshalb frühestens nach der Sammelzeit, normalerweise beim
nächsten ADS-B-Durchlauf. Währenddessen gilt `pending`; dieser Zustand wird nicht
im Routencache gespeichert. Bestehende gültige Cacheeinträge sind sofort nutzbar.
Bei voller Warteschlange wartet ein weiteres Callsign auf einen freien Platz.
Antworten werden anhand ihres Callsigns einschließlich normalisierter führender
Nullen zugeordnet und vor Übernahme erneut auf Plausibilität geprüft.
Nicht abgeholte Batch-Ergebnisse verfallen nach einer Minute. ADS-B-Verarbeitung,
Radar, MQTT und Webbedienung laufen während der Sammelzeit weiter. Erst nach einer
fertigen Primärantwort wird gegebenenfalls ADSBDB abgefragt.

Auch der manuelle Routentest hält diese Sammelzeit und den Mindestabstand ein;
die wartende Ergebnisseite weist darauf hin. Er verwendet weiter den
Controllerstandort als Prüfposition und übernimmt keine Route in den Cache.

Findet der Primärdienst keine Route, kann die Firmware automatisch den
kostenlosen, schlüsselfreien Dienst ADSBDB abfragen. Das Ergebnis wird nur
übernommen, wenn Callsign und Lage des Flugzeugs zur Verbindung zwischen Start
und Ziel plausibel sind. ADSBDB dient ausschließlich als Fallback; eine bereits
gefundene Primärroute wird nicht ersetzt. Die Funktion ist in der
Konfigurationsseite abschaltbar. ADSBDB-Ergebnisse bleiben ausschließlich im
flüchtigen RAM-Cache und werden nicht in den permanenten Routencache kopiert.

Multi-Hop-Routen bleiben vollständig erhalten. Die ausführliche Matrixseite
scrollt beispielsweise:

```text
DUS>BUD>DUS
Duesseldorf > Budapest > Duesseldorf
```

Auf den Kompaktseiten wird ein geschlossener Umlauf ohne redundantes Endziel
als `DUS<>BUD` beziehungsweise `Duesseldorf <> Budapest` dargestellt. Eine
normale Route wie `DUS-FRA-JFK` bleibt vollständig `DUS>FRA>JFK`.

Der Routencache besitzt je 32 Plätze im RAM und im nichtflüchtigen Speicher.
Routen werden zusätzlich anhand der aktuellen Flugposition, Höhe,
Vertikalbewegung und Flugrichtung geprüft. Die dafür benötigten
Flughafenkoordinaten werden aus dem täglich aktualisierten, gemeinfreien
[OurAirports-Datensatz](https://ourairports.com/data/) erzeugt und fest in die
Firmware eingebaut. Der Controller muss OurAirports nicht zur Laufzeit
erreichen. Eine unplausible Route wird weder angezeigt noch gespeichert.
Kehrt dasselbe Callsign nach mindestens 30 Minuten zurück oder wird es von
einem anderen Flugzeug verwendet, wird sein vorhandener Cacheeintrag vor der
Abfrage entfernt.
Bleibt dasselbe Flugzeug ausgewählt, werden Routen mit Status `unavailable`
oder `rejected` nach frühestens 60 Sekunden erneut abgefragt, sobald wieder
aktuelle ADS-B-Daten vorliegen. Dabei werden die aktuellen Positions-, Höhen-
und Bewegungsdaten verwendet. Weiterhin unplausible Cacheeinträge werden wie
bisher übersprungen; die Route wird erneut beim Dienst angefragt. Die
Plausibilitätsprüfung bleibt aktiv, eine wiederholt falsche Dienstzuordnung
wird deshalb weiterhin abgewiesen.

Gültigkeitszeiten:

| Ergebnis | Gültigkeit |
|---|---:|
| gefundene Route des Primärdienstes | 6 Stunden |
| gefundene ADSBDB-Fallbackroute (nur RAM) | maximal 6 Stunden |
| nicht gefundene Route | 1 Stunde |
| militärisches/behördliches Einsatzrufzeichen | 30 Minuten |

Neue Ergebnisse werden zunächst im RAM gesammelt. Der Flash-Schreibvorgang
erfolgt bevorzugt, wenn kein Flugzeug angezeigt wird oder die Nachtruhe aktiv
ist; die Matrix wird dafür kontrolliert dunkel geschaltet. Die
Konfigurationsseite zeigt Callsign, Route, Alter, Speicherort und Zustand.
Einträge können einzeln gelöscht, für das aktuelle Flugzeug neu abgefragt oder
vollständig geleert werden. Statusseite, Routentest und Cachetabelle zeigen
zusätzlich, aus welcher Quelle das Ergebnis stammt.

Ab Version `.36` liegen auch sämtliche dauerhaft gehaltenen Texte des
RAM-Routencaches, Tagesjournals und der Special-Chronik im externen PSRAM.
Das betrifft unter anderem Callsigns, Flugzeugbeschreibungen, Airline- und
Flughafennamen, Routen und Auswahlbegründungen. Eintragsmetadaten und Zeiger bleiben
intern; für die aktuelle Flugzeuganzeige und Downloads entstehen weiterhin
vorübergehende normale Textkopien. Es gibt keine globale Umleitung der
Speicherverwaltung und keine Änderung der TLS-Einstellungen.

Die PSRAM-Texte teilen beim Kopieren zunächst ihren unveränderlichen Puffer.
Nur geänderte Texte benötigen eine neue Reservierung; identische Daten bei
wiederholten ADS-B-Abfragen reservieren keinen neuen Textspeicher. Beim
Ersetzen, Entfernen oder Leeren werden nicht mehr benötigte Puffer tatsächlich
freigegeben. Abgelaufene Cacheeinträge werden weiterhin erst beim entsprechenden
Zugriff entfernt, nicht durch einen zusätzlichen Timer. Spurdauer, Journalgrenzen (256/64 Einträge), Routencachegrenzen
(32 RAM-/32 Flashplätze), Ablaufzeiten und JSON-/CSV-Downloadformate bleiben
erhalten. Der persistente Flash-Routencache verwendet sein bisheriges Format.

Schlägt eine PSRAM-Textreservierung fehl, wird der vorbereitete Journal- oder
Cacheeintrag nicht übernommen. Bestehende Texte, Metadaten, Eintragszähler und die
Cache-Ersetzungsposition bleiben erhalten; eine neue Beobachtung kann dann
fehlen, bis wieder Speicher verfügbar ist. Routen können weiterhin direkt
angezeigt werden, auch wenn sie gerade nicht in den RAM-Cache übernommen
werden können. Es gibt keinen Rückfall auf internen Textspeicher. Ohne
funktionierenden PSRAM sind entsprechend keine neuen Journal-/Cachetexte
speicherbar; Gerätekonfiguration und gespeicherte Flashdaten werden nicht
verändert.

Die Statusseite zeigt belegte Textbytes, Anzahl der Textpuffer und fehlgeschlagene
Reservierungen. `/api/status` liefert diese Werte unter
`persistent_text_memory` als `storage: "psram"`, `allocated_bytes`, `blocks`
und `allocation_failures`. Die Bytezahl enthält die Referenzverwaltungsdaten
der Textpuffer, jedoch nicht den zusätzlichen Verwaltungsaufwand des
Speicherallokators. Gemeinsame Puffer werden nur einmal gezählt. Die
Fehlerzahl ist seit Start kumulativ. Gegenüber `.35` benötigt der Matrix-Build
zusätzlich rund 40,5 KiB weniger statischen internen RAM; das weitere Wachstum
der gespeicherten Texte belastet den externen Speicher.

Lokaler Regressionstest der PSRAM-Textverwaltung einschließlich tatsächlicher
Journal-/Cache-Aktualisierung, vollständiger Belegung, absichtlich simulierter
Reservierungsfehler sowie Reset- und Freigabepfade:

```bash
python3 tools/test_psram_text.py
```

## Matrixanzeige

Nach dem Boot zeigt der Matrixcontroller einmalig **Software-Version,
Standortbezeichnung und IP-Adresse**. Während der WLAN-Anmeldung erscheint
zunächst „WLAN VERBINDET…“; nach erfolgreicher Verbindung bleibt die
Informationsseite sechs Sekunden sichtbar. Lange Versions- und Standorttexte
scrollen innerhalb dieses Zeitfensters vollständig durch. Ohne Bezeichnung
steht „Standort nicht benannt“. Die Darstellung berücksichtigt 128/125 Pixel.
Unter **Startanzeige → IP-Adresse beim Start anzeigen** lässt sich die IP-Zeile
ausblenden. Die Option ist standardmäßig eingeschaltet. Ausgeschaltet bleibt die
unterste Zeile leer; Version, Standort und die sechs Sekunden bleiben erhalten.
Die Einstellung wird pro Matrixcontroller gespeichert und als `boot_show_ip`
mitgesichert. Ältere Sicherungen ohne dieses Feld behalten den vorhandenen Wert.
Auf der Statusseite bleibt die IP-Adresse unabhängig davon verfügbar.

Die Anzeige blockiert weder Netzwerkverarbeitung noch Webbedienung. Ein
Transpondernotfall oder das WLAN-Einrichtungsnetz beendet sie; OTA und
Flash-Schreibschutz behalten Vorrang. Display-Aus und Nachtruhe werden
beachtet. Nach Verbindungsverlust oder späterer Wiederverbindung wird die
Startseite nicht erneut eingeblendet. Die Headless-Anzeige bleibt unverändert.


Alle acht Flugzeugseiten können einzeln aktiviert, frei angeordnet und mit
einer Dauer von 2 bis 60 Sekunden versehen werden:

| Seite | Inhalt |
|---|---|
| Flugdaten | Callsign, Entfernung, Richtung, Höhe und Steig-/Sinkdreieck |
| Airline und Route | Airline, vollständige Routencodes und Geschwindigkeit |
| CPA und Sondertyp | CPA, Annäherung/Entfernung, Klassifizierung und ausführliche Flugzeugtypenbezeichnung |
| Flughafennamen | Codes und Langnamen; Multi-Hop vollständig gescrollt |
| Freitext | eigener Text bis 128 Zeichen, langsam/mittel/schnell |
| Kompaktansicht | Typ, Kerndaten, Blickrichtung, Route und CPA |
| Kompakt + Flughäfen | Airline, Typ, Kerndaten und dynamische Namenszeile |
| Radaransicht | Maßstabstreuer rechteckiger Ausschnitt über die volle Matrixbreite, Standortzentrum, Blickrichtung nach oben, Radiuskreis und bis zu 64 Flugzeugpunkte |

Lange Texte verlängern die Seitendauer automatisch, bis der Lauf vollständig
sichtbar war. Seiten ohne verwertbaren Inhalt werden übersprungen. Sanfte
Übergänge sind optional.

Auf **CPA und Sondertyp** läuft eine zu lange Detailzeile wie auf
**Kompakt + Flughäfen**: 0,8 Sekunden Startpause, Vorlauf mit 80 ms pro Pixel,
0,8 Sekunden Pause am Textende, Rücklauf mit 20 ms pro Pixel und 0,8 Sekunden
Pause am Anfang. Das Zeichenintervall beträgt 50 ms. Die Seitendauer wird bei
Bedarf bis zum vollständigen Vor-/Rücklauf einschließlich Pausen verlängert;
kurze Texte stehen still. Auch die separate Seite **Flughafennamen** verwendet diesen Vor-/Rücklauf
für Start und Ziel sowie bei Multi-Hop-Routen für die vollständigen Codes und
Flughafennamen. Die jeweils längste Zeile bestimmt die Mindestdauer; kürzere
Zeilen warten nach ihrem Rücklauf am Anfang. Auch hier beträgt das
Zeichenintervall 50 ms.

Die konkrete Typenbeschreibung kommt bevorzugt aus der ADS-B-Quelle. Fehlt sie
oder enthält sie nur eine allgemeine Angabe wie „Helicopter“, ergänzt eine
kleine lokale Tabelle derzeit Robinson R22, R44 und R66. Unbekannte Modelle
zeigen zumindest ihren ICAO-Typcode; eine Variante wie Raven II wird nicht aus
dem allgemeinen Code R44 geraten. Eine tatsächlich gelieferte ausführliche
Beschreibung bleibt erhalten. Die Ergänzung erfolgt zentral für Matrix,
Webdisplay und Journal, nach der bestehenden Klassifizierung. Es entstehen
keine zusätzlichen Netzwerkabfragen. Quelldatei ist
`data/aircraft_type_names.json`; Generator und Validator heißen
`tools/generate_aircraft_type_names.py` und `tools/validate_aircraft_type_names.py`.
Die Robinson-Zuordnung folgt den
[FAA Aircraft Type Designators](https://www.faa.gov/documentLibrary/media/Order/JO_7360.1G_Aircraft_Type_Designators_FINAL.pdf).


In der Radaransicht sind normale Ziele cyan, das ausgewählte Ziel gelb,
Specials violett, Bodenverkehr orange und Transpondernotfälle blinkend rot.
Der Standort erscheint grün im Zentrum. Der unverzerrte Kreis kennzeichnet
den normalen Auswahlradius. Seitliche Ziele außerhalb des Kreises werden mit
demselben Entfernungsmaßstab bis zum Matrixrand dargestellt, ohne dadurch die
normale Flugzeugauswahl zu verändern. Ist die Wetterseite deaktiviert, bleibt
das Radar auch ohne ausgewähltes Flugzeug als Leerlaufanzeige sichtbar. Ein
heller Punkt umläuft den Radius ausgehend von 12 Uhr im Uhrzeigersinn.
Unter **Radar → Radar-Umlauf** ist ab `.57` wahlweise **Punkt** (Voreinstellung)
oder **Strich** verfügbar. Der Strich ist ein dezent grüner, ein Pixel breiter
Radarstrahl vom Standortzentrum bis zum Radiuskreis. Flugzeugpunkte und
Radarspuren werden darüber gezeichnet. Das funktioniert auf der Matrix mit
128 oder 125 nutzbaren Pixeln und im Webradar beider Varianten.

Das ebenfalls unter **Radar** angeordnete Umlaufintervall ist für beide Varianten
von 1 bis 10 Sekunden konfigurierbar; `0` schaltet Punkt beziehungsweise Strich
aus. Der Standardwert beträgt 5 Sekunden. Unter **Dauer eines Umlaufs** ist ab
`.58` zusätzlich die Geschwindigkeit einstellbar: **0,8 bis 4,0 Sekunden** in
0,1-Sekunden-Schritten, Standard 0,8 Sekunden. Jeder Umlauf endet sichtbar
wieder bei 12 Uhr. Das Intervall bezeichnet weiterhin den Startabstand. Ist die
Dauer länger als das Intervall, gilt die Dauer als Mindestabstand: Ein Umlauf
wird vollständig beendet und der nächste beginnt ohne Pause. Beispiel: Bei
2 Sekunden Intervall und 4 Sekunden Dauer läuft alle 4 Sekunden ein vollständiger
Umlauf. Langsame Umläufe erhalten mehr Winkelpositionen bei gleicher Bildfrequenz.
Der Browser animiert diesen Umlauf lokal, ohne zusätzliche Controllerabfragen; außerhalb des Umlaufs
wird seltener neu gezeichnet. Die Darstellung benötigt keinen neuen Bild- oder
Spurpuffer auf dem Controller.

Stil, Intervall und Dauer bleiben nach Updates erhalten und werden als
`radar_sweep_style` (`dot`/`line`), `radar_sweep_interval_seconds` und
`radar_sweep_duration_ms` (800–4000 Millisekunden) gesichert. Ältere Sicherungen
ohne das jeweilige neue Feld behalten eine vorhandene Auswahl beziehungsweise
Dauer bei; frisch eingerichtete Controller verwenden den Punkt. Status- und Display-API
liefern alle drei Werte. Die Matrix-Webanzeige erhält weiterhin kein zusätzliches Radar.

Radarspuren zeigen auf Matrix und Webradar beider Varianten die beobachteten Positionen
der letzten 60 Sekunden als einzelne, mit dem Alter dunkler werdende Punkte.
Sie sind standardmäßig aktiv und unter **Radar → Radarspuren (60 Sekunden)**
abschaltbar. Bei kleinster Matrixhelligkeit bleiben die Spuren auf der kleinsten
sichtbaren Stufe. Identische Bildpunkte werden zusammengefasst; Linien und
vorhergesagte Positionen werden nicht gezeichnet. Empfangslücken über 15 Sekunden,
unplausible Positionssprünge sowie Änderungen von Quelle, Standort, Radaransicht
oder Filtern beginnen eine neue Spur. Auch das Speichern der Konfiguration
setzt die Spuren zurück. Die Verläufe liegen ausschließlich im RAM
und sind nach einem Neustart leer. Nur aktuell im Radar enthaltene Ziele erhalten
eine Spur; bei Abruffehlern laufen vorhandene Spurpunkte trotzdem nach 60 Sekunden ab.
Die Einstellung ist in Konfigurationssicherung und -wiederherstellung enthalten.

Ab Version `.35` liegen Verlaufsspeicher und beide Spur-Anzeigekopien in einem
beim Start einmalig reservierten **externen PSRAM-Block** (24.064 Byte auf dem
ESP32-S3). Interner RAM enthält nur die aktuellen Radarpositionen. Die bereits
vorhandenen PSRAM-Bausteine beider unterstützter Controller reichen dafür aus;
Spurdauer, maximal 64 Ziele und Darstellung bleiben erhalten. Beim Matrix-Build
belegt die Firmware dadurch rund 23,5 KiB weniger statischen internen RAM als
`.34-diag`. Die zusätzliche Reserve steht insbesondere den internen
TLS-Speicherzuteilungen der Routendienste zur Verfügung.

Kann der externe Speicher beim Start nicht reserviert werden, bleibt das
Live-Radar verfügbar und nur die Spuren werden für diese Laufzeit ausgelassen.
Es gibt bewusst keinen Rückfall auf großen internen Verlaufsspeicher, der das
TLS-Speicherproblem wieder auslösen würde. Die gespeicherte Spuren-Einstellung
bleibt unverändert. Konfiguration, Status und Webradar beider Varianten zeigen die
fehlende Verfügbarkeit an. `/api/status` und `/api/display` liefern zusätzlich
`radar_trails_available`; `/api/status` zeigt `radar_trails_storage_bytes` und
`free_psram_bytes`. `radar_trails_enabled` beschreibt weiterhin die gespeicherte
Auswahl; tatsächlich aktiv sind die Spuren nur bei aktivierter Auswahl und
verfügbarem Speicher. Die Routen-Verbindungsdiagnose bleibt zur Überprüfung
der internen Speicherreserven und möglicher weiterer TLS-Fehler erhalten.


Die Blickrichtung bezieht sich auf den Standort des Betrachters, nicht auf den
Track des Flugzeugs. Aus Standort, Entfernung, Höhe und Standorthöhe berechnet
die Firmware den vertikalen Blickwinkel. Unterhalb des konfigurierbaren
Grenzwerts – standardmäßig 30° – erscheint `weit`. Ab etwa 60° wird `oben`
ergänzt, ab 80° gilt das Flugzeug als direkt über dem Betrachter. Eine
Hysterese beruhigt die Anzeige an den Winkelgrenzen.

Wenn kein Flugzeug im Anzeigeradius liegt, erscheint wahlweise die Wetterseite.
Nur bei ansonsten leerem sichtbarem Matrixbild blinkt der Betriebs-Pixel ganz
unten links: (0,31) bei 128×32, (0,63) bei 128×64. Er ist weiß bei gültigen
ADS-B-Daten und rot nach einem Abruffehler. Wetter, Radar und andere Inhalte
unterdrücken ihn vollständig, auch bei Fehlern. Bei Display-Aus oder Nachtruhe
bleibt er dunkel. Die separate NeoPixel-Status-LED bleibt unverändert.
Flash- und OTA-Zustände haben Vorrang vor der normalen Seitenausgabe.

### Matrixansteuerung und Helligkeit

Adafruit Protomatter betreibt die Matrix mit 3 Bit Farbtiefe, Doppelbuffer und
einem angepassten ESP32-S3-DMA-Timing. Vollständige Seiten werden im
unsichtbaren Puffer aufgebaut und mit `matrix.show()` umgeschaltet. Vor
Seitenwechseln und zeitkritischen Flashzugriffen werden beide Puffer gezielt
bereinigt. Diese Kombination reduziert sporadische Störpixel bei gleichzeitig
feineren Farben als die frühere 2-Bit-Version.

Sieben native Helligkeitsstufen von 14 bis 100 Prozent vermeiden zeitliches
Dithering. Die BH1750-Automatik verteilt sie zwischen den konfigurierten
Lux-Schwellen. Manuelle Bedienung:

- **UP:** manueller Modus, heller
- **DOWN:** manueller Modus, dunkler
- **UP + DOWN:** Automatik aktivieren

## Wetterseite

Ist „Wetterdaten anzeigen, wenn kein Flugzeug in der Nähe ist“ aktiviert,
erscheinen alle verfügbaren Messwerte gemeinsam auf einer ruhigen Seite:

- BME280: Temperatur, Feuchte und auf Meereshöhe reduzierter Luftdruck
- MQTT: Windrichtung, Windgeschwindigkeit, Böen und Regen heute

Die obere Zeile enthält Temperatur, mittig Feuchte und rechts Luftdruck. Die
untere Zeile enthält Windrichtung, Windgeschwindigkeit, mittig Böen und rechts
Regen. `B` steht in der deutschen Oberfläche für Böen, `G` in der englischen
für *gusts*. Regen wird über MQTT in Zentimetern empfangen und für die Anzeige
in Millimeter umgerechnet. MQTT-Werte verfallen nach 15 Minuten ohne Update.

Broker, Port, Benutzer, Passwort und alle vier Topics sind im Webinterface
konfigurierbar. Windrichtung kann numerisch oder als kurze Himmelsrichtung
geliefert werden; numerische Werte werden in eine von 16 Richtungen umgesetzt.
Die Option für die Wetter-Ruheanzeige schaltet die gesamte Matrix-Wetterseite
ein oder aus. Auf dem Headless-Controller werden BME280- und MQTT-Werte
weiterhin in Status und API verarbeitet, aber nicht als lokale Ruheanzeige
dargestellt.

### Schimmelrisiko

Aus Temperatur und relativer Luftfeuchtigkeit berechnet die Firmware Taupunkt
und die Oberflächentemperatur, bei der 80 % relative Feuchte erreicht würden.
Das Ergebnis ist eine Orientierungshilfe, keine bauphysikalische Messung:

- unter 60 %: normal
- ab 60 %: erhöht; Feuchte erscheint gelb
- ab 75 % sofort oder ab 65 % für 30 Minuten: hoch; Feuchte erscheint rot
- ein hoher Zustand wird nach 15 Minuten unter 60 % zurückgenommen

Statusseite und `/api/status` liefern Risikostufe, Taupunkt, kritische
Oberflächentemperatur und Dauer erhöhter Feuchtigkeit.

## Weboberfläche

Nach erfolgreicher WLAN-Verbindung antwortet der Server auf Port 80:

| Pfad | Matrixcontroller | Headless-Controller | Funktion |
|---|---|---|---|
| `/` | `http://matrixportal.local/` | `http://flightwall-server.local/` | Weiterleitung zum Status |
| `/status` | `http://matrixportal.local/status` | `http://flightwall-server.local/status` | automatisch aktualisierter Status und Diagnose |
| `/config` | `http://matrixportal.local/config` | `http://flightwall-server.local/config` | Konfiguration ohne automatische Seitenaktualisierung |
| `/display` | `http://matrixportal.local/display` | `http://flightwall-server.local/display` | responsive FlightWall-Anzeige |
| `/api/display` | verfügbar | verfügbar | aktuelle Displaydaten einschließlich Webradar und manueller Auswahl als JSON |
| `/api/site-airports?lat=…&lon=…` | verfügbar | verfügbar | Flughafen-Vorlagen innerhalb von 150 km um die angegebenen Koordinaten |
| `/api/status` | verfügbar | verfügbar | vollständige Statusdaten einschließlich RGB-LED als JSON |
| `/unknown-codes.json` | verfügbar | verfügbar | unbekannte Airport-/Airline-Codes herunterladen |
| `/configuration.json` | verfügbar | verfügbar | vollständige Konfiguration herunterladen |
| `/lookups` | verfügbar | verfügbar | Lookup-Ergänzungen verwalten und hochladen |
| `/lookups.json` | verfügbar | verfügbar | aktuelle Lookup-Ergänzungen herunterladen |
| `/journal.json` | verfügbar | verfügbar | heutiges Flugzeugjournal als JSON herunterladen |
| `/journal.csv` | verfügbar | verfügbar | heutiges Flugzeugjournal als CSV herunterladen |
| `/specials.json` | verfügbar | verfügbar | heutige Special-Chronik als JSON herunterladen |
| `/specials.csv` | verfügbar | verfügbar | heutige Special-Chronik als CSV herunterladen |
| `/update` | verfügbar | verfügbar | passwortgeschütztes Web-OTA |

Falls mDNS im Netz nicht funktioniert, kann statt des jeweiligen Namens die im
seriellen Monitor oder Router angezeigte IP-Adresse verwendet werden.
Werden mehrere Controller derselben Variante gleichzeitig betrieben, verwenden
sie denselben mDNS-Namen. In diesem Fall sollten die numerischen IP-Adressen oder
feste DHCP-Zuordnungen im Router verwendet werden.

Die FlightWall-Anzeige unter `/display` verwendet auf Tablet und Desktop ab 721 Pixel
Breite kleinere vertikale Abstände und benötigt dadurch ungefähr 10 % weniger Höhe.
Schriftgrößen und Radar-Seitenverhältnis bleiben erhalten. Auf schmalen Smartphones
bleiben die Karten untereinander angeordnet und die Seite scrollbar.

Die Konfigurationsseite ist für iPhone, Tablet und Desktop optimiert. Kurze
Werte und zusammengehörige Optionen werden in einem kompakten, responsiven
Raster angeordnet; auf schmalen Displays ordnen sich die Felder automatisch
untereinander an. Die Seite verwaltet unter anderem WLAN-Zugang, Sprache,
Display, Wetterseite, Standort, Blickrichtung, ADS-B-Quelle, Filter,
Routendienst, MQTT, Seitenreihenfolge, Freitext, Helligkeit und Nachtruhe.
Statuskarten werden regelmäßig aktualisiert, ohne geöffnete Bereiche zu
schließen oder ungespeicherte Formulareingaben zu überschreiben.

### Standortprofile

Matrix- und Headless-Variante verwalten bis zu **16 Standortprofile**. Jedes
Profil enthält Bezeichnung, Koordinaten, Höhe über NHN in Metern und Blickrichtung.
Der vorhandene Standort wird beim ersten Start unverändert als erstes Profil
übernommen; weitere Standorte lassen sich unter `/config` anlegen und per
Pulldown auswählen, bearbeiten oder löschen. Mindestens ein Profil bleibt erhalten.
**Änderungen übernehmen** aktualisiert die Liste im Formular; erst
**Einstellungen speichern** speichert und aktiviert die Auswahl auf dem Controller.

Der Pulldown bietet außerdem Flughafen-Vorlagen innerhalb von **150 km** um die
angezeigten Koordinaten an, nach Entfernung sortiert. Sie stammen aus den
verfügbaren Flughafen-Lookups einschließlich manuell hochgeladener Ergänzungen;
das ist kein vollständiges Verzeichnis aller Flugplätze. Die Auswahl einer
Vorlage erzeugt ein eigenes Profil. Flughafenhöhen werden von Fuß in Meter
umgerechnet; ohne Höhenangabe bleibt der bisherige Eingabewert stehen.
Höhe immer für den tatsächlichen Aufstellort prüfen und die passende Blickrichtung
eintragen. Vorlagen überschreiben keine eigenen Profile.

Profile und aktive Auswahl gehören zur Konfigurationssicherung. Alte Sicherungen
ohne Profile aktualisieren nur den aktiven Standort; andere Profile bleiben erhalten.
Jeder Controller verwaltet seine eigenen Profile. Beim Wechsel werden die manuelle
Flugzeugauswahl, Radarspuren und die bisherige Anzeige zurückgesetzt; der nächste
ADS-B-Abruf berechnet Entfernung, CPA und Routenplausibilität am neuen Standort.
Lookup-Ergänzungen und andere Einstellungen bleiben dabei erhalten.

### Flugzeug im Webradar auswählen

Das Radar unter `/display` ist bei beiden Hardwarevarianten sichtbar, unabhängig
von den aktivierten Matrixseiten. Ein Klick oder Tippen auf einen Flugzeugpunkt
fordert dieses Flugzeug zur Anzeige an. Die Auswahl wird mit dem nächsten
ADS-B-Abgleich übernommen und gilt auch für die LED-Matrix. Weiße Eckmarken im
Radar kennzeichnen das manuell gewählte Flugzeug; unter dem Radar stehen die
Restzeit und **Automatik fortsetzen**.

Nach 60 Sekunden ohne erneute Flugzeugauswahl übernimmt die normale Automatik.
Ein weiterer Klick – auch auf dasselbe Flugzeug – startet die Frist neu.
Automatische Browserabfragen und die Radaranimation verlängern sie nicht.
Fehlt das Ziel im nächsten erfolgreichen ADS-B-Abruf, endet die Auswahl vorzeitig.
Transpondernotfälle haben Vorrang; währenddessen läuft die Frist weiter.
Die Auswahl gilt für alle Browser desselben Controllers, wird nur im RAM gehalten
und verändert weder Konfiguration noch Lookup-Ergänzungen.

### Status und Diagnose

![Statusseite mit Netzwerk, aktuellem Flugzeug und Tagesstatistik](docs/images/flightwall-status-2026-11-16.jpeg)

*Statusansicht mit Standortbezeichnung im Titel. Die Aufnahme zeigt einen
Aufbau ohne BH1750; ein nicht vorhandener optionaler Sensor verhindert den
Flugbetrieb nicht.*

`/status` bündelt Netzwerkzustand, aktuelles Flugzeug, Tagesstatistik, Journal,
Special-Chronik und technische Diagnose. Dazu gehören MQTT-Verbindung,
Stackreserven, PSRAM-Belegung und die Speicherstände der Routenabfragen.
Geöffnete Diagnosebereiche bleiben bei der automatischen Aktualisierung offen.
Die Konfigurationsseite wird dagegen nicht automatisch neu geladen.

### Konfiguration und Verbindungstests

![Konfigurationsseite mit Standort, Routenoptionen und Verbindungstests](docs/images/flightwall-configuration-2026-11-16.png)

*Standort, Blickrichtung und Filter sowie ADS-B-, Routen- und Notfalltests.
Das Routentest-Feld wird beim Seitenaufruf mit dem aktuellen Callsign vorbelegt.*

### Liste der Konfigurationsoptionen

Stand: **`.62`**. Die Tabellen beschreiben die Bedienung unter `/config`.
**Beide** bedeutet Matrix- und Headless-Variante, **Matrix** nur den
Matrixcontroller. Angegebene Grenzen und Schrittweiten entsprechen dem Formular.

Änderungen werden mit **Einstellungen speichern** dauerhaft übernommen.
**Änderungen übernehmen** im Standorteditor aktualisiert zunächst nur das Formular.
Radius, Filter, Radar- und Netzwerkeinstellungen gelten für den Controller;
**nur Bezeichnung, Koordinaten, Höhe und Blickrichtung gehören zum Standortprofil**.
Ungültige Standort-Zahlenfelder werden markiert und verhindern das Speichern.
Auch `0°` Blickrichtung und `0 m` Höhe sind gültig; ein leeres Zahlenfeld ist kein Nullwert.

#### Standort und Flugzeugauswahl

| Option | Auswahl / Eingabe | Variante |
|---|---|---|
| Standortprofil | Pulldown mit bis zu 16 eigenen Profilen; vorhandener Standort wird übernommen | Beide |
| Flughafen-Vorlage | Flughäfen aus verfügbaren Lookup-Daten innerhalb von 150 km, nach Entfernung sortiert; legt ein eigenes Profil an | Beide |
| Standort bearbeiten | Neuer Standort, Änderungen übernehmen, Standort löschen; mindestens ein Profil bleibt erhalten | Beide |
| Standortbezeichnung | Optional, bis zu 48 Unicode-Zeichen | Beide |
| Breitengrad | −90 bis +90°, Schritt 0,000001° | Beide |
| Längengrad | −180 bis +180°, Schritt 0,000001° | Beide |
| Standorthöhe über NHN | −500 bis 9000 m, Schritt 1 m; Flughafenhöhe für den tatsächlichen Aufstellort prüfen | Beide |
| Blickrichtung | 0–359°, Schritt 1°; 0° = Norden, 90° = Osten | Beide |
| Anzeigeradius | 1–100 km, Schritt 0,5 km; Specials können im doppelten Radius ausgewählt werden | Beide |
| Blickwinkel für „weit“ | 10–45°, Schritt 1°; Grenze des vertikalen Elevationswinkels | Beide |
| Sport- und Kleinflugzeuge ausblenden | Ein / aus; erkannte Specials werden dadurch nicht ausgeschlossen | Beide |
| Maximalhöhe | 1000–100000 ft, Schritt 500 ft; wirkt bei aktivem Höhenfilter | Beide |
| Flugzeuge oberhalb der Maximalhöhe ausblenden | Höhenfilter ein / aus; Transpondernotfälle haben weiterhin Vorrang | Beide |

#### ADS-B und Routendaten

| Option | Auswahl / Eingabe | Variante |
|---|---|---|
| Aktive ADS-B-Quelle | Lokale Quelle 1, lokale Quelle 2 oder adsb.fi | Beide |
| ADS-B-Adresse Quelle 1 / 2 | HTTP-/HTTPS-Adresse, jeweils bis zu 180 Zeichen; Quelle 2 darf leer bleiben | Beide |
| Automatisches Failover | Bei Fehlern andere konfigurierte Quelle versuchen; bevorzugte Quelle jede Minute erneut prüfen | Beide |
| Routen über adsb.im ergänzen | Ein / aus | Beide |
| ADSBDB bei fehlender Route verwenden | Ein / aus | Beide |
| Routen-Dienst | HTTP-/HTTPS-Adresse, bis zu 180 Zeichen; adsb.im-Adressen werden beim Abruf gemäß Dienstvorgaben behandelt | Beide |
| Test-Callsign | Bis zu 10 alphanumerische Zeichen; beim Seitenaufruf aktuelles Callsign, sonst `DLH123` | Beide |

#### Radar

| Option | Auswahl / Eingabe | Variante |
|---|---|---|
| Radarspuren | Ein / aus; beobachtete Positionen der letzten 60 Sekunden | Beide |
| Radar-Umlauf | Punkt oder Strich | Beide |
| Umlaufintervall | Aus (`0`) oder 1–10 Sekunden zwischen Umlaufstarts | Beide |
| Dauer eines Umlaufs | 0,8–4,0 Sekunden, Schritt 0,1 Sekunden; bei längerer Dauer als dem Intervall läuft der Umlauf ohne Pause | Beide |

Das Webradar bleibt unabhängig von aktivierten Matrixseiten verfügbar.
Ein Klick oder Tippen auf einen Flugzeugpunkt wählt ihn vorübergehend für
60 Sekunden aus. Erneute Auswahl startet die Frist neu; **Automatik fortsetzen**
beendet sie. Dies ist eine vorübergehende Bedienaktion, keine gespeicherte Option.

#### Sprache, WLAN und Firmwarezugang

| Option | Auswahl / Eingabe | Variante |
|---|---|---|
| Sprache | Deutsch oder Englisch | Beide |
| WLAN-Name | SSID mit 1–32 Bytes; Netzwerkscan zur Auswahl verfügbar | Beide |
| Neues WLAN-Passwort | 8–63 Zeichen; leer lässt das gespeicherte Passwort unverändert | Beide |
| Offenes WLAN | Verbindung ohne WLAN-Passwort ausdrücklich aktivieren | Beide |
| Neues OTA-Passwort | 8–63 Zeichen; leer lässt das gespeicherte Passwort unverändert | Beide |
| OTA deaktivieren | Deaktiviert den passwortgeschützten Updatezugang | Beide |

WLAN- und OTA-Zugangsänderungen werden durch einen automatischen Neustart aktiv.
Ein normaler Wechsel von Standort, Radius oder Höhenfilter benötigt keinen Neustart.

#### Wetter und MQTT

| Option | Auswahl / Eingabe | Variante |
|---|---|---|
| Wetterdaten ohne Flugzeug anzeigen | BME280-Wetterseite im Leerlauf ein / aus | Matrix |
| MQTT-Wetter aktivieren | MQTT-Wetterdaten verwenden; lokale Wetter-Ruheanzeige nur beim Matrixcontroller | Beide |
| Mosquitto-Server | Hostname oder IP-Adresse, bis zu 96 Zeichen; MQTT ohne TLS | Beide |
| MQTT-Port | 1–65535 | Beide |
| MQTT-Benutzer | Bis zu 64 Zeichen | Beide |
| MQTT-Passwort | Bis zu 96 Zeichen; leer erhält das gespeicherte Passwort | Beide |
| Topic Windrichtung | Bis zu 128 Zeichen; Richtung in Grad oder unterstütztes Kompasskürzel | Beide |
| Topic Windgeschwindigkeit | Bis zu 128 Zeichen; Wert in km/h | Beide |
| Topic Windböen | Bis zu 128 Zeichen; Wert in km/h | Beide |
| Topic Regen heute | Bis zu 128 Zeichen; Eingang in cm, Anzeige in mm | Beide |

BME280 und BH1750 werden automatisch erkannt; ihre I²C-Adressen werden nicht
über die Konfiguration eingegeben. Sensoren und MQTT sind optional.

#### Matrixdarstellung, Helligkeit und Nachtruhe

| Option | Auswahl / Eingabe | Variante |
|---|---|---|
| Display eingeschaltet | Matrixausgabe ein / aus | Matrix |
| Status-LED | Aus, schwach oder normal | Beide |
| Nachtruhe / Nachtabsenkung | Ein / aus; Anfang und Ende als Uhrzeit, auch über Mitternacht; gleiche Zeiten deaktivieren die Absenkung | Beide |
| IP-Adresse beim Start anzeigen | Ein / aus; standardmäßig ein, Version und Standort bleiben Teil der Startanzeige | Matrix |
| Defekte rechte Spalten ausblenden | 125 statt 128 nutzbare Pixel; Inhalte werden entsprechend angeordnet | Matrix |
| Sanfte Übergänge | Ein / aus | Matrix |
| Anzeigeseiten | Jede der acht normalen Matrixseiten einzeln aktivieren oder deaktivieren | Matrix |
| Seitendauer | Je Seite 2–60 Sekunden; Lauftextseiten können für einen vollständigen Vor-/Rücklauf länger sichtbar bleiben | Matrix |
| Seitenreihenfolge | Auswahl der Seite für jede Position der Rotation | Matrix |
| Freitext | Bis zu 128 Zeichen; ein leeres Feld überspringt die Seite | Matrix |
| Freitext-Scrollgeschwindigkeit | Langsam, mittel oder schnell | Matrix |
| BH1750-Automatik | Automatische Matrixhelligkeit ein / aus | Matrix |
| Manuelle Helligkeitsstufe | 14, 29, 43, 57, 71, 86 oder 100 % | Matrix |
| Schwelle dunkel | 1–500 Lux, Schritt 1 Lux | Matrix |
| Schwelle hell | 50–50000 Lux, Schritt 10 Lux; muss über der Dunkelschwelle liegen | Matrix |

Die Nachtruhe dimmt beim Headless-Controller die Status-LED; eine lokale
LED-Matrix ist dort nicht vorhanden. Transpondernotfälle werden unabhängig von
der normalen Seitenauswahl angezeigt. Die 125-Pixel-Option ändert die Webanzeige nicht.

#### Tests, Pflege und Sicherung

Diese Schaltflächen sind Bedienaktionen und keine zusätzlichen Anzeigeoptionen.

| Aktion | Wirkung | Variante |
|---|---|---|
| WLANs suchen | Verfügbare Netze zur Auswahl anzeigen | Beide |
| ADS-B-Verbindung testen | Abruf im Hintergrund; übernimmt zuvor die Formulareinstellungen | Beide |
| Routendienst testen | Abfrage ohne Cache; zeigt auch verworfene Antworten und HTTP-Fehlertexte | Beide |
| MQTT-Verbindung testen | Hintergrundtest mit Formularwerten, auch bei deaktivierter Wetteranzeige; speichert keine Einstellungen | Beide |
| Notfallanzeige testen | 7500, 7600 oder 7700 mit 1–3 Testflugzeugen; höchstens zwei Minuten | Beide |
| Test beenden | Laufenden Notfalltest beenden | Beide |
| Displaytest | Matrix-Prüfbilder und Typensymbole starten oder beenden | Matrix |
| Lookup-Ergänzungen verwalten | JSON-Ergänzungen herunterladen oder separat hochladen, ohne Firmwarebau und Neustart | Beide |
| Konfiguration herunterladen | JSON-Sicherung einschließlich Standortprofilen und Zugangsdaten; geschützt aufbewahren | Beide |
| Sicherung wiederherstellen | Einstellungen aus JSON übernehmen; anschließend automatischer Neustart | Beide |
| Routen-Cache verwalten | Unter `/status` Einträge prüfen, erneut abfragen oder löschen; gesamten Cache leeren | Beide |
| Firmware-Update | Passendes Firmwareimage unter `/update` installieren | Beide |
| ESP neu starten | Neustart über die Statusseite | Beide |

Bei den Standortkoordinaten lässt sich eine optionale **Standortbezeichnung**
mit bis zu 48 Zeichen hinterlegen, etwa „Büro München“ oder „Dachstation 2“.
Sie erscheint dezent bei den Standortdaten auf `/status`, als Eingabefeld auf
`/config` und unter dem Kopfbereich von `/display`, auch ohne aktuelles Flugzeug.
Ein leeres Feld blendet die Beschriftung aus. Umlaute, Leerzeichen und Satzzeichen
sind möglich; Steuerzeichen und ungültiges UTF-8 werden abgewiesen.
Auf den regulären Matrix-Flugzeugseiten erscheint die Standortbezeichnung nicht;
die Startanzeige zeigt sie zusammen mit der Version und optional der IP-Adresse. Der Text wird gespeichert,
bei OTA erhalten und als `site_name` mit der Konfiguration gesichert.
Ältere Sicherungen ohne dieses Feld behalten eine vorhandene Bezeichnung bei;
eine ausdrücklich leere Bezeichnung löscht sie.

Die Standortbezeichnung erscheint auch im Browser-Titel und in der Überschrift
von Status und Konfiguration. Beim Öffnen der Konfiguration wird das aktuelle
Callsign als Routentest-Vorgabe verwendet; ohne Callsign gilt `DLH123`. Eigene
Eingaben bleiben bis zum erneuten Laden der Seite erhalten.

Bei beschädigten rechten Panelspalten lässt sich unter **Matrixbreite** die
Option **Defekte rechte Spalten ausblenden (125 Pixel)** einschalten. Sie ist
standardmäßig aus. Texte, Scrollwege, Radar und Zentrierungen verwenden dann
125 Pixel; die drei rechten physischen Spalten erhalten bei jeder Bildausgabe
schwarze Daten, einschließlich Displaytest und Übergängen. Das Panel bleibt
physisch 128×32, die Webanzeige unverändert. Die Einstellung wird gespeichert,
bei OTA erhalten und als `matrix_reduced_width` gesichert. Alte Sicherungen ohne
dieses Feld behalten die Einstellung bei. Der Headless-Build bietet sie nicht an.
Ein Hardwarefehler, der auch bei schwarzen Bilddaten leuchtet, lässt sich so
nicht abschalten.

Matrixbezogene Felder werden im Headless-Build ausgeblendet. Einstellungen
werden im NVS gespeichert und bleiben bei normalen USB- und OTA-Updates
erhalten.

Im Abschnitt **Konfiguration sichern** kann der gesamte Gerätestand als
JSON-Datei heruntergeladen und später auf demselben oder einem Ersatzgerät
wiederhergestellt werden. Die Datei enthält auch WLAN-, MQTT- und
OTA-Zugangsdaten **im Klartext** und muss deshalb geschützt aufbewahrt werden.
Ist OTA eingerichtet, verlangt der Download dieselben Zugangsdaten wie die
Update-Seite. Die Sicherungsdatei selbst ist weder verschlüsselt noch durch ein
zusätzliches Passwort geschützt. Eine Wiederherstellung übernimmt die
enthaltenen Einstellungen und startet den Controller anschließend neu.
Lookup-Ergänzungen werden separat als `lookups.json` gesichert und hochgeladen.

Ist das ADS-B-Failover aktiviert, verwendet die Firmware bei einem Fehler
sofort eine andere konfigurierte Quelle. Die bevorzugte Quelle wird danach
einmal pro Minute erneut geprüft. Statusseite und `/api/status` unterscheiden
zwischen bevorzugter und tatsächlich verwendeter Quelle.

Die Seite `/display` zeigt Callsign, Typ, Airline/Special Type, vollständige
Route, Entfernung, Höhe, Geschwindigkeit, CPA, Vertikalbewegung und
Blickrichtung. Sie aktualisiert sich automatisch und ist unabhängig vom
128×32-Layout gestaltet. Auf der Headless-Variante enthält sie zusätzlich ein
kompaktes Live-Radar im Seitenverhältnis der Matrix. Es verwendet dieselbe
Orientierung, Entfernungsskala und Farblogik wie das Matrixradar.

Deutsch/Englisch wirkt gemeinsam auf Matrix, Konfiguration, Diagnose,
Web-Display und Wetterkürzel. Airport-, Airline- und Freitextnamen werden nicht
automatisch übersetzt.

### Tagesjournal und Special-Chronik

![Special-Chronik mit verlinkten Flugkennungen, Klassifizierung und Auswahlgrund](docs/images/flightwall-special-chronicle-2026-11-16.png)

*Die Special-Chronik erfasst auch erkannte Sonderflugzeuge, die nicht auf der
Anzeige ausgewählt wurden. Türkise Flugkennungen öffnen FlightAware.*

Die Tagesstatistik zählt jedes frische Flugzeug anhand ICAO-Hexadresse,
Registrierung oder Callsign nur einmal. Bis zu 4096 Flugzeuge und 256 Specials
werden im RAM verfolgt. Der Reset erfolgt um Mitternacht lokaler NTP-Zeit und
zusätzlich bei jedem Neustart; die Statusseite zeigt den tatsächlichen
Erfassungsbeginn und beide Kapazitätsgrenzen.

Das Tagesjournal hält zusätzlich bis zu 256 tatsächlich zur Anzeige ausgewählte
Flugzeuge im RAM. Für jeden Eintrag werden erster und letzter Sichtzeitpunkt,
Flug, Typ, Route, Klassifizierung sowie kleinste Entfernung und kleinster CPA
fortgeschrieben. Die Statusseite zeigt aus Stabilitätsgründen die neuesten 64
Einträge in umgekehrter zeitlicher Reihenfolge; die JSON- und CSV-Downloads
enthalten das vollständige Journal. Journal und Tagesstatistik werden gemeinsam
bei lokalem Datumswechsel oder Neustart gelöscht.

Die Flugkennungen in Tagesjournal und Special-Chronik verlinken auf die
zugehörige FlightAware-Seite in einem neuen Tab. Ohne Flugkennung bleibt die
Hex-Kennung normaler Text. Der Link bezeichnet die Flugkennung, nicht zwingend
den historischen Flug zum Zeitpunkt des Eintrags; JSON-/CSV-Exporte bleiben unverändert.

Die Special-Chronik erfasst unabhängig von der Anzeige bis zu 64 Specials und
Transpondernotfälle, die der aktive ADS-B-Empfänger innerhalb des
Verarbeitungsbereichs liefert. Sie enthält Klassifizierung, ersten und letzten
Zeitpunkt, Typ, minimale Entfernung und CPA sowie die Information, ob das
Flugzeug tatsächlich angezeigt wurde. Für das aktuelle Flugzeug und jeden
angezeigten Journaleintrag nennt FlightWall außerdem den Auswahlgrund:
Transpondernotfall, nächstes Special im doppelten Radius oder geringste
Entfernung unter den geeigneten Kandidaten.


## Installation per USB

### Voraussetzungen

- Git
- VS Code mit PlatformIO-Erweiterung oder PlatformIO Core
- USB-C-Datenkabel
- Zugangsdaten für das WLAN

Repository laden:

```bash
git clone https://github.com/leliep/flightwall-matrixportal.git
cd flightwall-matrixportal
cp include/secrets.h.example include/secrets.h
```

In `include/secrets.h` mindestens WLAN-Name und Passwort eintragen:

```cpp
constexpr char WIFI_SSID[] = "mein-wlan";
constexpr char WIFI_PASSWORD[] = "mein-wlan-passwort";

#define OTA_PASSWORD "ein-langes-ota-passwort"
#define MQTT_PASSWORD ""
```

Die Werte `WIFI_SSID` und `WIFI_PASSWORD` dienen als erstmalige beziehungsweise
werkseitige Zugangsdaten. Danach können WLAN-Name und Passwort über die
Konfigurationsseite geändert werden; die gespeicherten Werte bleiben bei
Firmwareupdates erhalten.

`OTA_PASSWORD` benötigt mindestens acht Zeichen und dient nur noch als
Startwert. Das OTA-Passwort kann anschließend auf der Konfigurationsseite
gesetzt, geändert oder deaktiviert werden und bleibt in NVS gespeichert. Damit
können auch die öffentlichen GitHub-Builds ohne eingebettetes privates Passwort
nach der ersten USB-Installation für OTA eingerichtet werden.
`include/secrets.h` wird von Git ignoriert und darf nicht eingecheckt werden.

### WLAN später ändern

Unter **WLAN-Zugang** kann ein neuer WLAN-Name mit neuem Passwort gespeichert
werden. Ein leeres Passwortfeld behält das vorhandene Passwort bei; für ein
offenes Netz steht eine eigene Auswahl zur Verfügung. Nach einer Änderung
startet der Controller automatisch neu.

**WLANs suchen** startet auf Wunsch einen asynchronen Umgebungsscan. Die Liste
ist nach Signalstärke sortiert, fasst gleichnamige Netze zusammen und zeigt
SSID, Empfangspegel und Verschlüsselung. Eine Auswahl übernimmt die SSID in das
weiterhin frei editierbare Eingabefeld; bei einem offenen Netz wird die
entsprechende Option gesetzt. Versteckte oder während des Scans nicht sichtbare
Netze können unverändert von Hand eingetragen werden. Es finden keine
automatischen Dauerscans statt.

Kann der Controller das gespeicherte WLAN nicht erreichen, öffnet er ein
Einrichtungsnetz mit einem gerätespezifischen Namen nach dem Muster
`FlightWall-Setup-123ABC`:

| Einstellung | Wert |
|---|---|
| Passwort | `flightwall` |
| Konfigurationsadresse | `http://192.168.4.1/` |

Das Einrichtungsnetz leitet Browseraufrufe auf die Konfigurationsseite um und
wird nach einer erfolgreichen Verbindung mit dem normalen WLAN wieder beendet.
Solange es aktiv ist, zeigt die Matrix dauerhaft WLAN-Name, Passwort und
`CONFIG IP 192.168.4.1` an. Ein zu langer WLAN-Name läuft als Laufschrift.

MatrixPortal kompilieren und per USB übertragen:

```bash
pio run -e adafruit_matrixportal_esp32s3
pio run -e adafruit_matrixportal_esp32s3 -t upload
```

Headless-Server für das ESP32-S3-N16R8-Board kompilieren und übertragen:

```bash
pio run -e esp32s3_n16r8_headless
pio run -e esp32s3_n16r8_headless -t upload
```

Bei mehreren angeschlossenen seriellen Geräten zuerst den USB-Port ermitteln
und ausdrücklich angeben (hier als Beispiel unter macOS):

```bash
pio device list
pio run -e esp32s3_n16r8_headless -t upload \
  --upload-port /dev/cu.usbmodem101
```

Ein normaler USB-Upload ersetzt Bootloader, Partitionstabelle und Firmware,
lässt aber die im NVS gespeicherte WLAN- und Anwendungskonfiguration bestehen.
Das ist für Updates und Ersatzcontroller meist erwünscht. Soll ein bereits
benutztes Board vollständig neu eingerichtet werden, muss es vor dem Upload
gezielt gelöscht werden; dabei gehen alle gespeicherten Einstellungen verloren.

Die beiden Firmwaredateien liegen anschließend unter:

```text
.pio/build/adafruit_matrixportal_esp32s3/firmware.bin
.pio/build/esp32s3_n16r8_headless/firmware.bin
```

Die Images sind **nicht austauschbar**: Das Matrix-Image initialisiert HUB75,
Taster und die Matrixsensorik, das Headless-Image lässt diese Hardware bewusst
aus. Die Update-Seite nennt deshalb die erkannte Gerätevariante.

Beim ersten USB-Upload kann es je nach Board und USB-Modus erforderlich sein,
BOOT gedrückt zu halten, kurz RESET zu betätigen und BOOT wieder loszulassen.
Nach erfolgreicher Installation genügt normalerweise ein gewöhnlicher Upload
oder OTA. Der im Betrieb sichtbare USB-Port kann sich vom Bootloader-Port
unterscheiden.

Der serielle Monitor läuft mit 115200 Baud:

```bash
pio device monitor -b 115200
```

Nach dem ersten Start `http://matrixportal.local/config` öffnen und mindestens
Standort, Blickrichtung, aktive ADS-B-Quelle und Anzeigeradius kontrollieren.
Die mitgelieferten Standardwerte sind auf den ursprünglichen Aufbau
zugeschnitten und nicht als allgemeine Standortvorgabe gedacht.

GitHub Actions baut bei Push und Pull Request beide Firmwarevarianten mit den
Platzhalterwerten aus `include/secrets.h.example` und stellt beide BIN-Dateien
als getrennte Build-Artefakte bereit.

| GitHub-Actions-Artefakt | Zielgerät | enthaltene Datei |
|---|---|---|
| `flightwall-matrixportal-s3` | Adafruit MatrixPortal S3 mit HUB75-Matrix | `firmware.bin` |
| `flightwall-esp32s3-n16r8-headless` | ESP32-S3 DevKitC-1 N16R8 ohne Matrix | `firmware.bin` |

Zum Herunterladen unter **Actions → letzter erfolgreicher „PlatformIO
Build“-Lauf → Artifacts** das zum Controller passende ZIP auswählen. Die
Build-Artefakte sind nicht direkt als Binärdateien im Git-Repository
eingecheckt.

### Fertige Release-Images

Für stabile Versionen ist **Releases → Latest** der bevorzugte Downloadweg.
Ein Versionstag im Format `v<APP_VERSION>`, beispielsweise
`v3.6.1-dual-target.33`, veröffentlicht nach zwei erfolgreichen Builds
automatisch:

- `flightwall-matrixportal-<VERSION>.bin`
- `flightwall-headless-<VERSION>.bin`
- `SHA256SUMS.txt`
- `firmware-manifest.json`

Der Workflow bricht ab, wenn Tag und `APP_VERSION` nicht exakt übereinstimmen.
Das Manifest nennt für beide Hardwarevarianten Dateiname, Größe, SHA-256-Wert
und Quell-Commit. Dadurch lassen sich vertauschte oder beschädigte Images vor
dem Web-OTA erkennen. Normale Build-Artefakte bleiben zusätzlich für jeden
erfolgreichen Commit verfügbar.

`main` kann bereits eine neuere, erfolgreich gebaute Entwicklungsversion als
das jüngste getaggte Release enthalten. In diesem Fall stehen die aktuellen
Images zunächst als GitHub-Actions-Artefakte oder nach lokalem Build bereit.

## OTA-Updates

Das erste OTA-fähige Image muss per USB installiert werden. Danach gibt es
zwei Wege.

### Web-OTA

![Firmware-Update mit installierter Version und Hardwarevariante](docs/images/flightwall-firmware-update-2026-11-16.png)

*Die Update-Seite zeigt Version und Hardwarevariante vor der Dateiauswahl.
Für Matrix und Headless sind jeweils unterschiedliche Images erforderlich.*

1. `http://matrixportal.local/update` öffnen.
2. Mit Benutzer `admin` und dem `OTA_PASSWORD` anmelden.
3. Das MatrixPortal-Image aus dem aktuellen GitHub Release oder lokal
   `.pio/build/adafruit_matrixportal_esp32s3/firmware.bin` auswählen.
4. Upload bestätigen und den automatischen Neustart abwarten.

Die Seite prüft Dateiendung und ESP32-Dateikopf, zeigt den Fortschritt und
vergleicht nach dem Neustart alte und neue Firmwareversion.

Für den Headless-Server lautet die Adresse
`http://flightwall-server.local/update`; dort muss die Datei aus
`.pio/build/esp32s3_n16r8_headless/` beziehungsweise das mit `headless`
bezeichnete Release-Image verwendet werden. Die Update-Seite zeigt die
installierte Hardwarevariante ausdrücklich an. Images der beiden Varianten
dürfen nicht miteinander vertauscht werden.

### PlatformIO-OTA

```bash
export MATRIXPORTAL_OTA_PASSWORD='ein-langes-ota-passwort'
pio run -e adafruit_matrixportal_esp32s3_ota -t upload
```

Das Passwort steht bewusst nicht in `platformio.ini`. Die VS-Code-Aufgabe
**FlightWall: Git-Update + OTA** führt zunächst ein Fast-forward-Update von
`origin/main` aus, fragt das Passwort verdeckt ab und startet anschließend den
Upload.

Vor jedem geplanten Neustart oder OTA die unbekannten Airport- und Airline-Codes
auf allen betroffenen Controllern prüfen. Fehlende Namen und Airport-Koordinaten
zuerst in einer Lookup-Ergänzung oder im Firmware-Grundbestand sichern; die
Beobachtungslisten selbst liegen nur im RAM. Konfiguration und Lookup-Datei
getrennt sichern, das richtige Firmwareimage auswählen und nach dem Update
Version, Erreichbarkeit und Code-Listen kontrollieren.

## Übersetzungstabellen pflegen

### Lookup-Ergänzungen ohne Firmwareupdate

Unter `/config` → **Lookup-Ergänzungen verwalten** beziehungsweise `/lookups`
werden zusätzliche Airportnamen, Koordinaten und Airlines unabhängig von der
Firmware hochgeladen. Anmeldung: Benutzer `admin` mit dem bestehenden
OTA-Passwort; ohne eingerichtetes OTA-Passwort ist der Upload gesperrt.
Jeder Controller erhält die Datei separat. Es gibt keinen automatischen Download.

1. Bei der ersten Verwendung gegebenenfalls **Lookup-Speicher einrichten**
   wählen. Diese ausdrücklich bestätigte Aktion formatiert nur die vorhandene
   Datenpartition für die Lookup-Dateien; NVS-Konfiguration, OTA-Slots und
   Matrix-UF2 bleiben erhalten. Bei einem späteren Mountfehler nicht ungeprüft
   formatieren: Auch ein beschädigtes oder fremdes Dateisystem kann die Ursache sein.
2. Beispieldatei herunterladen, ergänzen und als JSON auswählen. Vorhandene
   Ergänzungen vorher herunterladen und in der neuen Datei beibehalten.
3. **Prüfen und übernehmen** ersetzt den gesamten Ergänzungsbestand. Es findet
   kein Neustart statt. Neue Namen werden beim nächsten Flugdatendurchlauf auch
   aus dem bestehenden Routencache aufgelöst; alte Journale werden nicht umgeschrieben.

Format (vollständiges Beispiel: `data/lookup_extensions.example.json`):

```json
{
  "format": "flightwall-lookups-v1",
  "version": "example-1",
  "airports": [
    {"code": "DUS", "name": "Düsseldorf", "latitude": 51.2895,
     "longitude": 6.76678, "elevation_ft": 147}
  ],
  "airlines": [{"code": "DLH", "name": "Lufthansa"}]
}
```

Alle vier Hauptfelder sind erforderlich. Flughafen-Codes haben 3–4,
Airline-Codes 2–4 Großbuchstaben/Ziffern; Codes müssen innerhalb ihrer Tabelle
eindeutig sein. Jeder Flughafen benötigt beide Koordinaten und `elevation_ft`
als ganze Zahl zwischen −1500 und 30000 oder `null`. Namen müssen gültiges UTF-8
ohne Steuerzeichen sein, maximal 28 Zeichen für Airports bzw. 64 für Airlines;
die Datenversion enthält 1–64 Zeichen. Grenzen: 64 KiB und je 256 Einträge.
Ergänzungen haben bei Lookups Vorrang vor dem eingebauten Grundbestand.
Eine Datei mit leeren Arrays entfernt alle Ergänzungen und stellt den Grundbestand her.

Eine Datei aus den gepflegten Quellen erzeugen:

```bash
python3 tools/generate_lookup_extensions.py --version local-1 \
  --airports DUS --airlines DLH BNJ JFL --output work/lookup-extensions.json
```

### Unbekannte Codes recherchieren und Bestände zusammenführen

`tools/research_lookup_extensions.py` fragt die angegebenen Controller
lesend ab, lädt ihre vorhandenen Ergänzungen und erstellt einen gemeinsamen
Bestand. Ohne `--controller` gelten die beiden im Werkzeug hinterlegten
Standardadressen; für drei oder mehr Geräte jede Adresse explizit angeben.
Namen/Koordinaten kommen zuerst aus den lokalen Quelltabellen;
fehlende Airports werden über OurAirports, fehlende ICAO-Airlines über die
aktuelle FAA-Designatortabelle recherchiert. Die Software setzt Python 3.9+
voraus und benötigt keine zusätzlichen Python-Pakete.

```bash
python3 tools/research_lookup_extensions.py \
  --version local-2 \
  --output work/lookup-extensions.json \
  --report work/lookup-review.json
```

Für eine gemeinsame Datei mehrerer Geräte alle Adressen im selben Aufruf
angeben, beispielsweise:

```bash
python3 tools/research_lookup_extensions.py \
  --controller http://controller-matrix-a.local \
  --controller http://controller-matrix-b.local \
  --controller http://controller-headless.local \
  --version local-3 \
  --output work/lookup-extensions.json \
  --report work/lookup-review.json
```

Die Namen sind Platzhalter; die tatsächlichen IP-Adressen oder Hostnamen einsetzen.
Nach konfliktfreier Prüfung dieselbe Datei auf jedes Gerät hochladen und
anschließend Lookup-Version sowie leere Code-Listen kontrollieren.
Das jeweilige OTA-Passwort wird verdeckt abgefragt. Es gibt keine Passwortoption
in der Kommandozeile; Zugangsdaten werden weder gespeichert noch im Bericht
veröffentlicht. Eine Eingabe ohne geschütztes Terminal wird abgewiesen.
Für die Geräte sind ausschließlich GET-Abfragen implementiert; Zugangsdaten
werden bei HTTP-Weiterleitungen nicht weitergereicht. Es gibt keinen Upload,
keine Änderung von Einstellungen und keinen Neustart.

Die Ausgabe umfasst die gemeinsame Lookup-Datei sowie einen JSON-Prüfbericht
und eine lesbare Markdown-Fassung gleichen Namens. Der Bericht enthält
Controller-/Datenversionen, erhaltene Einträge, neue Daten, Quellen mit
Abrufzeitpunkt und SHA-256, relevante Quellzeilen, Konflikte und offene Punkte.
Nur generische Flughafen-Endungen werden bei Bedarf aus zu langen Namen
entfernt; jede solche Anpassung erscheint im Bericht. Es wird kein
identifizierender Name blind abgeschnitten oder geraten. Die FAA-Quelle
liefert ICAO-Codes; fehlende zweibuchstabige Airline-Codes bleiben offen.

Vorhandene Ergänzungen haben Vorrang vor Recherchedaten. Identische Einträge
beider Geräte werden zusammengeführt. Unterschiedliche Angaben zum selben
Code erscheinen mit beiden Versionen im Konfliktbericht, ohne automatische
Auswahl. Ebenso blockieren nicht erreichbare Controller, unvollständige
Code-Listen nach Überlauf, mehrdeutige Quellen, fehlende Angaben und ungültige
oder zu große Daten die Freigabe einer Upload-Datei.

**Nur bei Status `ready` und Rückgabecode 0 wird die Upload-Datei geschrieben.**
Bei Status `blocked` (Rückgabecode 2) bleibt eine bereits vorhandene Datei am
Ausgabepfad unverändert; sie gehört dann nicht zum neuen Lauf. Der aktuelle
JSON-Bericht weist `output_written: false` aus. Bei Freigabe enthält er auch
die Prüfsumme der neu erzeugten Datei. Konflikte müssen vor dem manuellen
Upload geklärt werden. Anschließend dieselbe geprüfte Datei auf beiden
Controllern hochladen; das Recherchewerkzeug führt diesen Schritt nicht aus.

Mit `--airports-csv DATEI` und `--airlines-html DATEI` lassen sich lokale
Quellsnapshots verwenden; der Bericht kennzeichnet diese und dokumentiert
ihre Prüfsummen. Die Daten können veraltet sein und müssen dann geprüft werden.

Tests mit zwei simulierten Controllern, Bestands-/Konfliktfällen und Quellen:

```bash
python3 tools/test_research_lookup_extensions.py
```

### Speicher der Lookup-Ergänzungen

Die vorhandenen Partitionstabellen bleiben unverändert:

| Variante | Datenpartition | Dateisystem | Größe |
|---|---|---|---|
| MatrixPortal | `ffat`, Offset `0x450000` | FAT/FFat | 3.6875 MiB |
| Headless N16R8 | `spiffs`, Offset `0xc90000` | LittleFS | 3.375 MiB |

Die Tabelle beschreibt die Partitionen; nutzbarer Dateisystemplatz ist wegen
Verwaltungsdaten kleiner. Zwei abwechselnde Dateien mit Rückleseprüfung und
CRC sowie ein separat gespeicherter aktiver Dateizeiger schützen die vorherige
Version bei fehlgeschlagenen Updates. Der Zeiger wird erst nach Schließen und
Aushängen des Dateisystems in einem eigenen NVS-Namespace geändert.
Firmware-OTA überschreibt diese Dateien nicht. `uploadfs`, Neuformatierung oder
vollständiges Flash-Löschen können sie entfernen; deshalb separat herunterladen.
Die Konfigurationssicherung enthält die Ergänzungsdatei nicht.

Uploadpuffer und JSON-Daten liegen ausschließlich im PSRAM, zusammen auf 1 MiB
begrenzt. Dateisystemtreiber benötigen während des Zugriffs zusätzlich interne
Puffer; das Dateisystem bleibt im normalen Betrieb ausgehängt. Fehlerhafte oder
zu große Dateien, fehlender PSRAM sowie Schreib-/Prüffehler aktivieren keinen
neuen Bestand. Beim Start mit unlesbarer Datei bleibt der Grundbestand verfügbar.

Statusseite und `/api/status` zeigen unter `lookup_overrides` Datenversion,
Airport-/Airlineanzahl, Dateigröße, Partitionskapazität, reservierte PSRAM-Bytes,
Reservierungsfehler und den letzten Fehler. Aufgelöste Codes werden nach einem
Upload aus den unbekannten Listen entfernt; Überlaufzähler bleiben erhalten.

Lokale Tests mit realem Parser, beiden generierten Lookup-Varianten und
simulierten PSRAM-, Datei- und NVS-Fehlern:

```bash
python3 tools/test_lookup_overrides.py
python3 tools/test_lookup_upload.py
```

### Eingebauten Grundbestand pflegen

Die editierbaren Quellen liegen hier:

- `data/airport_names.de.json`
- `data/airline_names.de.json`
- `data/airport_coordinates.json` (aus OurAirports erzeugt)

Die C++-Dateien in `src/*.generated.cpp` werden daraus erzeugt und sollten
nicht direkt bearbeitet werden:

```bash
python3 tools/generate_airport_names.py
python3 tools/generate_airline_names.py
python3 tools/generate_airport_coordinates.py
python3 tools/validate_airport_names.py
python3 tools/validate_airline_names.py
python3 tools/validate_airport_coordinates.py
pio run
```

Nach dem Ergänzen neuer Flughafencodes wird die Koordinatentabelle mit
`python3 tools/update_airport_coordinates.py` aus dem aktuellen
OurAirports-Datensatz aktualisiert. Das optionale Argument ist der Pfad zu
einer bereits heruntergeladenen `airports.csv`.

Empfohlene Reihenfolge vor einem Controllerneustart:

1. unbekannte Codes auf **beiden** Controllern prüfen oder herunterladen,
2. Codes und Klartexte in die JSON-Quelldateien eintragen,
3. bei Flughäfen zusätzlich Koordinaten und Höhe ergänzen,
4. generierte C++-Tabellen neu erzeugen und alle Validatoren ausführen,
5. beide Firmwarevarianten bauen und erst danach OTA beziehungsweise Neustart
   durchführen.

Unbekannte Codes werden während des Betriebs flüchtig gesammelt – jeweils bis
zu 32 Airports und Airlines. Die Statusseite und `/api/status` zeigen die
gesammelten Listen und mögliche Überläufe. Über **Codes herunterladen (.json)**
oder `/unknown-codes.json` lässt sich jederzeit eine Datei für die spätere
Tabellenpflege sichern. Der Download löscht keine Einträge. Nach einem Neustart
sind diese Beobachtungen verloren.

## Diagnose und Tests

Die Statusseite zeigt unter anderem:

- Firmwareversion, Build, Laufzeit und Neustartgrund
- Hardwarevariante sowie Zustand und Helligkeitsmodus der RGB-Status-LED
- freien internen Heap, Minimum seit Start und größten zusammenhängenden
  freien internen 8-Bit-Speicherblock (vor Aufbau der Statusantwort gemessen)
- aktuelle IP, Hostname, verbundene und gespeicherte SSID,
  Signalqualität, Gateway, Subnetz, DNS, WLAN-Kanal, MAC-Adresse,
  Einrichtungsnetz und Wiederverbindungen
- aktive ADS-B-Quelle, Abrufalter, empfangene und geeignete Flugzeuge
- aktuelles Flugzeug, Route, CPA, Filter und Routencache
- unbekannte Airport-/Airline-Codes
- tägliche Gesamt-, Special- und Notfallstatistik mit Erfassungsbeginn
- BME280-, Schimmel-, MQTT- und BH1750-Werte
- Matrix-Farbtiefe, Bildwiederholrate sowie letzte/maximale `show()`-Dauer
- maximale Laufzeiten einzelner Netzwerkphasen und Taskdiagnose

### MQTT-Verbindung und Hintergrundtest

Ab `.51` bedient ein eigener MQTT-Task die Verbindung unabhängig von ADS-B und
Routendiensten. MQTT-I/O hält die gemeinsame Anwendungsdatensperre nicht fest.
Die Client-ID enthält alle sechs MAC-Bytes; der Test nutzt eine separate ID,
damit er die laufende Verbindung nicht verdrängt. Der Client verwendet MQTT
3.1.1 über unverschlüsseltes TCP wie bisher, eine TCP-Verbindungsfrist von
4 Sekunden, eine MQTT-Antwortfrist von 5 Sekunden und 45 Sekunden Keepalive.
Die Verarbeitung wird regelmäßig bedient statt auf den nächsten ADS-B-Zyklus
zu warten. Netzwerkfehler und Konfigurationswechsel werden getrennt behandelt.

Unter `/config` startet **MQTT-Verbindung testen** einen Hintergrundauftrag.
Die eingetragenen MQTT-Werte werden nur für den Test verwendet und nicht
persistiert; ein leeres Passwortfeld verwendet das gespeicherte Passwort.
Andere Formfelder werden dabei nicht übernommen. Der Test funktioniert auch
bei deaktivierter MQTT-Wetteranzeige und aktiviert diese nicht dauerhaft.
Er prüft DNS, TCP, MQTT-Anmeldung, die Bestätigung jedes konfigurierten
Abonnements (SUBACK) und bis zu acht Sekunden eingehende Wetterwerte. Er
veröffentlicht keine Testwerte und verwendet keine persistente Broker-Sitzung.

Das Ergebnis trennt Verbindungsfehler, fehlende/abgelehnte Abonnements und
fehlende/ungültige Daten. Sind Verbindung und Abonnements in Ordnung, aber
noch nicht alle Werte eingetroffen, ist das ein Hinweis und kein Beweis eines
Brokerfehlers: Ohne Retained-Werte kann die nächste Messung später kommen.
Ein empfangener, aber ungültiger letzter Wert wird gesondert als Datenfehler
gekennzeichnet. Passwort und Benutzername werden nicht ausgegeben.

Wetter-Topics sind konkrete Topicnamen mit höchstens 128 Bytes im Test;
Wildcard-Abonnements werden bei der Zuordnung zu den vier Wetterwerten nicht
unterstützt. Werte müssen Zahlen ohne Einheit sein; Windrichtung akzeptiert
auch die dokumentierten Kompasskürzel. Windrichtung wird normiert, negative
Wind-/Regenwerte sowie nicht endliche Zahlen werden verworfen. Regen wird
weiterhin von cm in mm umgerechnet. Ungültige Nachrichten setzen den
Frischezeitpunkt nicht mehr zurück.

`/api/status` ergänzt `mqtt_diagnostics` mit DNS-/TCP-/CONNACK-Laufzeiten,
MQTT-Code und Fehlergrund, aufgelöster IPv4-Adresse, Client-ID, PONG-Zähler und
je Topic SUBACK, Empfangs-/Gültigkeitszählern und Alter des letzten gültigen
Werts. `background_network` zeigt zusätzlich MQTT-I/O und die MQTT-Stackreserve.
Statusseite und serielle Diagnose enthalten die wichtigsten Angaben ebenfalls.
SUBACK: -1 nicht konfiguriert, -2 noch nicht bestätigt, -3 Senden fehlgeschlagen,
0–2 bestätigt, 128 vom Broker abgelehnt. Die zusätzliche Task benötigt 8 KiB
internen Stack; Session-/Diagnoseobjekte liegen im PSRAM. Nach Installation
sind MQTT-Funktion sowie verbleibende Stack-/TLS-Reserven am Gerät zu prüfen.

`python3 tools/test_mqtt_session.py` prüft die tatsächliche PubSubClient-2.8-
Implementierung mit kontrollierten Sockets: Anmeldung, Wiederanmeldung,
verzögerte Antworten, Fehlerphasen, SUBACK, Keepalive, Wettervalidierung,
parallelen Webzugriff, sichere Testergebnisse und unveränderte Einstellungen.

### Temporäre Puffer und TLS-Reserve

Ab `.50` hat die Webbedienung einen eigenen Task. Der Daten-Task bleibt allein
für ausgehende ADS-B-/Routenabfragen zuständig. Beide Tasks schützen die
gemeinsamen Anwendungsdaten mit einer Sperre; während blockierender HTTP-/TLS-
Zugriffe, Antwortlesen und Verbindungsabschluss arbeitet der Daten-Task nur mit
seinen lokalen Objekten und gibt die Sperre frei. Dadurch können die Webseiten
auch während eines langsamen Routendienstes oder ADS-B-Ersatzabrufs antworten.
Es werden keine zweiten gleichzeitigen TLS-Routenverbindungen gestartet.

Änderungen an Konfiguration, Lookups oder Routencache invalidieren laufende
Abfrageergebnisse. Nach Rückkehr aus dem blockierenden Zugriff werden diese
verworfen und die Verarbeitung mit aktuellem Stand erneut angefordert; eine
bereits laufende Socketoperation wird damit nicht sofort zwangsweise beendet.
Die Web- und Daten-Task-Stacks sind jeweils 12 KiB groß. Der zusätzliche
Web-Stack benötigt internen RAM; große Antwort-/JSON-Puffer bleiben im PSRAM.
Die technische Diagnose zeigt zusätzlich die **Web-Stackreserve**, die API
unter `background_network` auch laufende I/O und ausstehende manuelle Tests.
Die zusätzliche Stack-/TLS-Reserve muss nach einer Installation am Gerät
kontrolliert werden. Schwacher WLAN-Empfang wird durch diese Trennung nicht
behoben. MQTT läuft in einem eigenen Task und gibt bei Netzwerk-I/O ebenfalls
die Datensperre frei. Ausdrückliche Flashaktionen bleiben gesondert geschützte
Arbeitsabschnitte.

Manuelle ADS-B-/Routentests starten ab `.50` einen einzelnen Hintergrundauftrag.
Der Browser folgt automatisch zu `/request-result?id=…` und lädt die Seite
während der Bearbeitung erneut. Andere Webseiten bleiben während der externen
Abfragen verfügbar. Weitere Tests werden mit HTTP 409 abgewiesen, solange einer
läuft. Nach Abschluss erscheinen die bisherigen Ergebnisse einschließlich der
verworfenen Routenantworten. Nur das jüngste fertige Testergebnis wird bis zum
nächsten Test oder Neustart im PSRAM gehalten. Für Skripte: POST liefert eine
303-Weiterleitung, GET während der Bearbeitung HTTP 202, danach das fertige
Ergebnis mit HTTP 200 oder 502; ersetzte IDs liefern HTTP 410.

Die Tests `tools/test_adsb_background_io.py`,
`tools/test_route_test_responses.py` und `tools/test_background_jobs.py` prüfen
die tatsächlichen Abfrage-/Auftragspfade mit verzögerten HTTP-/JSON-Streams und
gleichzeitiger Webbedienung. Der PSRAM-Puffertest enthält zusätzlich parallele
Allokationen. Diese Prüfungen ersetzen keine spätere Laufzeitmessung auf den
Controllern.

Ab `.49` baut die Status-API ihr JSON in kurzen, aufeinanderfolgenden Schritten
auf. Große Verkettungen halten sonst viele temporäre Objekte gleichzeitig auf
dem Task-Stack, auch wenn deren Textdaten im PSRAM liegen. Der vom Zielcompiler
ausgewiesene Stackframe dieser Funktion beträgt auf beiden Varianten 528 statt
10.272 Bytes; der Netzwerk-Task behält seinen 12-KiB-Stack. Die Prüfung
`python3 tools/check_network_stack.py` kontrolliert nach dem Bau beider Varianten
die Handlerframes im tatsächlichen ESP32-Binärprogramm. Sie ersetzt keine
Laufzeitkontrolle der Stackreserve nach Installation.

Der manuelle **Routendienst-Test** zeigt Hauptdienst und ADSBDB getrennt:
HTTP-Status, zurückgegebenes Callsign, gelieferte Routencodes, Ergebnis und
Fehler sowie die dekodierte JSON-Antwort. Diese wird vor dem Verwerfen einer
Route ausschließlich für die Testseite erfasst und danach freigegeben.
Je Anbieter werden höchstens 8192 Bytes JSON im PSRAM erhalten; Kürzung oder
Speichermangel werden sichtbar gemeldet. Bei HTTP-Fehlern wird zusätzlich die
originale Fehlerantwort des Dienstes angezeigt, auch bei Text- oder HTML-Antworten.
Sie wird ebenfalls auf 8192 Bytes im PSRAM begrenzt und als sicher maskierter Text
angezeigt; enthaltenes HTML wird nicht ausgeführt. Das gilt für Hauptdienst und
ADSBDB. Ohne Antwortkörper bleibt der entsprechende Hinweis sichtbar. Abgelehnte
Antworten werden weiterhin weder als gültige Route angezeigt noch gespeichert.
Der Test nennt ausdrücklich den **Controllerstandort als Prüfposition**;
automatische Routenabfragen verwenden dagegen die aktuelle Flugposition.
`python3 tools/test_route_test_responses.py` prüft die tatsächlichen Abfrage-
und Ablehnungszweige sowie die sichere Darstellung beider Varianten.

Ab `.48` liegen große Status-/Konfigurations-/API-Antworten und die JSON-Dokumente
für ADS-B, Routendienste und Konfiguration ausschließlich in PSRAM. Die
Statusaktualisierung versendet direkt einen Ausschnitt des vorhandenen Puffers,
ohne eine zweite vollständige interne String-Kopie. Ein einzelner Antwortpuffer
ist auf 512 KiB begrenzt; bei fehlendem PSRAM wird keine unvollständige Seite
als erfolgreiche Antwort versendet, sondern HTTP 503. Bereits begonnene
Streaming-Downloads werden bei Pufferfehlern abgebrochen. Kleine Formatierungs-
und Frameworkpuffer können weiterhin internen RAM benötigen; TLS selbst bleibt
entsprechend der eingebundenen SDK-Konfiguration im internen Speicher.

`/api/status` enthält unter `temporary_memory` aktuelle und maximale
PSRAM-Pufferbelegung sowie `allocation_failures`. Die aktuelle Belegung umfasst
auch den Puffer der gerade aufgebauten Statusantwort und den Scan-Ergebniscache.
Der Spitzenwert gilt seit Start. Es gibt keinen Rückfall auf internen Speicher.

Die Routen-Verbindungsdiagnose enthält zusätzlich `after_cleanup`: interner
freier RAM, größter Block und Minimum **nach** Freigabe der lokalen HTTP-, TLS-
und JSON-Ressourcen. Dieser Messpunkt wird auch bei Fehlern und abgelehnten Routen
erfasst und beim letzten Fehler mitgeführt. `after_request` bleibt die Messung
unmittelbar nach GET/POST, bevor alle Ressourcen freigegeben wurden. Die
Differenz ist deshalb für sich kein Nachweis eines Speicherlecks. Die Werte
beim Funktionsende enthalten weiterhin andere laufende Anwendungsdaten und
messen nicht den Speicherbedarf mitten im TLS-Handshake.

### Automatische WLAN-Wiederherstellung

Ab `.45` wird das WLAN auch dann neu verbunden, wenn es weiterhin als verbunden
erscheint, aber mindestens fünf vollständige ADS-B-Abrufzyklen über mindestens
90 Sekunden ausschließlich mit Transportfehlern enden. Alle im jeweiligen Zyklus
versuchten Quellen müssen betroffen sein. Jede ADS-B-HTTP-Antwort, auch ein Fehlerstatus,
oder eine funktionierende MQTT-Verbindung unterbricht diese Fehlerfolge.
JSON-, Konfigurations- und explizite HTTPClient-Speicherfehler zählen nicht dazu.
Ein alleiniger Ausfall aller verwendeten Dienste kann dieselben Transportfehler
verursachen; deshalb gilt zwischen Wiederherstellungen eine Sperrzeit von fünf Minuten.

Dabei wird nur die WLAN-Verbindung erneuert, ohne Controller-Neustart und ohne
Änderung gespeicherter Einstellungen oder Lookup-Ergänzungen. Webserver, MQTT
und OTA werden über den bestehenden Wiederverbindungspfad wieder gestartet.
Während OTA, ausstehendem Neustart oder vorbereiteter Routencache-Schreibphase
wird keine solche Wiederherstellung ausgelöst. Schlägt die Anmeldung fehl,
bleiben das Einrichtungsnetz und die regulären WLAN-Wiederholungen verfügbar.

`/api/status` enthält `wifi_transport_failure_cycles`, `wifi_transport_recoveries`
und `last_wifi_transport_recovery_age_seconds`. Die serielle Ausgabe meldet den
Auslöser und anschließend den regulären Verbindungsaufbau. Zähler gelten seit Start.

### Routen-Verbindungsdiagnose

Seit Diagnoseversion `3.6.1-dual-target.34-diag` erfasst die Firmware echte HTTP-Abfragen
beim Hauptdienst und beim ADSBDB-Fallback getrennt. Unter **Status →
Routen-Verbindungsdiagnose** stehen Quelle, Callsign, Anzahl und Alter der
Anfragen, Dauer, HTTP-Code sowie der negative Fehlercode des Secure-Clients.
Bei einem konkreten mbedTLS-Fehler erscheinen zusätzlich Hexadezimalcode und
Fehlertext. adsb.im verwendet ab `.56` HTTP und benötigt dafür keinen TLS-Handshake;
ADSBDB und andere konfigurierte HTTPS-Dienste verwenden weiterhin TLS. Der allgemeine Secure-Client-Code `-1` und ein fehlender TLS-Code
werden ausdrücklich als **ohne TLS-Details** gekennzeichnet; daraus lässt sich
kein bestimmter DNS-, Socket- oder TLS-Fehler ableiten. Ein HTTP-Fehler nach
erfolgreichem TLS-Aufbau wird nicht als TLS-Fehler ausgegeben.

Der interne, byteweise adressierbare RAM wird an vier Grenzen gemessen:

1. vor dem Erzeugen des Secure-Clients und der Anfrage (`entry`),
2. unmittelbar vor `POST` beziehungsweise `GET` (`before_request`),
3. unmittelbar nach deren Rückkehr (`after_request`),
4. nach Freigabe der lokalen HTTP-, TLS- und JSON-Ressourcen (`after_cleanup`).

Jede Messung enthält freien RAM, den größten zusammenhängenden freien Block
und das Heap-Minimum seit dem Start. Die verwendete TLS-Bibliothek nutzt
internen Speicher; PSRAM wird diesen Werten nicht zugerechnet. Die Messungen
sind **keine Messung innerhalb des TLS-Handshakes**: Die Bibliothek kann ihre
TLS-Puffer bei einer fehlgeschlagenen Verbindung bereits vor der Rückkehr
freigegeben haben. Auch das Heap-Minimum ist ein Wert seit Gerätestart und
kein isoliertes Minimum der einzelnen Routenabfrage.

In `/api/status` stehen die Werte unter:

- `route_memory`: aktueller interner Heap vor Aufbau der API-Antwort
- `route_diagnostics.primary`: letzte HTTP-Anfrage des Hauptdienstes
- `route_diagnostics.fallback`: letzte HTTP-Anfrage des ADSBDB-Fallbacks
- `route_diagnostics.last_failure`: letzter Verbindungs-/HTTP-Fehler, auch wenn
  spätere Anfragen erfolgreich sind oder Routen aus dem Cache kommen

Noch nicht vorhandene Abfragedatensätze sind `null`. Die Datensätze enthalten
`http_attempted`, `http_code`, `tls_error_code`, `tls_error_hex`,
`tls_error_detail_available`, `tls_error_text`, `started_ms`, `age_seconds`,
`duration_ms` und die Speicherstände `entry`, `before_request`, `after_request`
und `after_cleanup`.
Die Speicherfelder heißen `free_internal_bytes`, `largest_internal_block_bytes`
und `minimum_internal_bytes`; alle Größen sind Bytes. Die Dauer beginnt vor
der Anfragevorbereitung und endet nach dem Empfang der HTTP-Antwortheader.

Cachetreffer lösen keine neue Messung aus; Callsign und Alter zeigen, zu welcher
Anfrage die Diagnose gehört. HTTP 404 beim ADSBDB-Dienst bedeutet keine gefundene
Route und ersetzt den gespeicherten Verbindungsfehler nicht. Die Aufzeichnung
verwendet feste Diagnosestrukturen im RAM, benötigt bei der Erfassung keine
zusätzlichen dynamischen Textpuffer und endet mit einem
Neustart. URLs, Positionen und Zugangsdaten werden nicht in die neuen
Diagnosedatensätze aufgenommen. Routenwahl, Cache-Regeln, TLS-Einstellungen und
Controllerkonfiguration bleiben unverändert.

Lokale Diagnoseprüfungen ohne Controller:

```bash
c++ -std=c++11 -Wall -Wextra -pedantic -Iinclude tools/test_route_diagnostics.cpp -o /tmp/flightwall-route-diagnostics
/tmp/flightwall-route-diagnostics
python3 tools/test_route_diagnostic_output.py
c++ -std=c++11 -Wall -Wextra -pedantic -Iinclude tools/test_route_callsign.cpp -o /tmp/flightwall-route-callsign
/tmp/flightwall-route-callsign
python3 tools/test_route_validation.py
```

### Verfügbare Tests

- ADS-B-Verbindungstest der ausgewählten Quelle
- Routendiensttest mit aktuellem oder frei eingegebenem Callsign, gelieferten
  Antworten und getrennten Callsign-/Positionsfehlern
- MQTT-Hintergrundtest mit DNS, TCP, Anmeldung, Abonnements und gültigen Daten;
  ohne Speichern der Formwerte und ohne Testnachrichten zu veröffentlichen
- Transpondernotfalltest mit einem bis drei Flugzeugen
- etwa 23 Sekunden langer Matrixtest mit Farben, Raster, Linien, Text,
  Farbbalken und allen Special-Type-Symbolen

Daten-, Web- und MQTT-Task laufen mit niedriger Priorität und kurzen
Scheduler-Pausen, damit synchrone Netzwerkaufrufe den ESP32-Task-Watchdog nicht
blockieren. Bei WLAN-Verlust werden mDNS, MQTT und OTA kontrolliert beendet;
der Webserver bleibt über das Einrichtungsnetz erreichbar und die normalen
Netzdienste werden nach der Wiederverbindung neu gestartet.

## Betrieb und Fehlersuche

Für den normalen Betrieb empfiehlt sich folgende kurze Kontrolle:

- `/status` zeigt eine aktuelle ADS-B-Antwort ohne Fehler,
- bevorzugte und tatsächlich aktive ADS-B-Quelle stimmen überein oder das
  Failover ist nachvollziehbar aktiv,
- unbekannte Codes und Überlaufzähler werden vor geplanten Neustarts geprüft,
- BME280, MQTT und BH1750 werden nur dann als Fehler bewertet, wenn sie im
  eigenen Aufbau tatsächlich verwendet werden,
- OTA-Updates werden ausschließlich mit dem zur Hardware passenden Image
  durchgeführt.

| Symptom | Prüfung |
|---|---|
| Gerät nicht per Namen erreichbar | numerische IP im Router oder seriellen Monitor verwenden; mDNS-Unterstützung des Clients prüfen |
| Keine Flugzeuge | aktive ADS-B-URL testen, Zeitstempel und Positionen in `aircraft.json` prüfen, Radius/Filter kontrollieren |
| Falsche oder fehlende Route | Routentest und Cachequelle ansehen, Eintrag gezielt löschen oder neu abfragen |
| Matrix dunkel | Displayfreigabe, Nachtruhe, Helligkeit, 5-V-Versorgung und HUB75-Richtung prüfen |
| Wetterseite fehlt | Wetter-Ruheanzeige aktivieren; BME280- und MQTT-Status kontrollieren |
| WLAN nicht erreichbar | mit `FlightWall-Setup-…` verbinden und `http://192.168.4.1/` öffnen |
| Webserver träge | Zahl gleichzeitiger Browser reduzieren und Netzwerkdiagnose auf `/status` prüfen |
| OTA schlägt fehl | Hardwarevariante, OTA-Passwort, Dateiendung, freien Netzwerkzugriff und ausreichend lange Uploadzeit prüfen |

## Wiederherstellung und Git

Der Stand vor der Integration der Serverfunktionen ist mit dem Tag
`pre-flightmatrix-integration-2026-10-04` gesichert:

```bash
git fetch --tags
git switch -c restore-pre-integration pre-flightmatrix-integration-2026-10-04
```

Aktualisierung eines bestehenden Checkouts ohne lokale Änderungen:

```bash
git switch main
git pull --ff-only origin main
```

## Projektstruktur

| Pfad | Inhalt |
|---|---|
| `src/main.cpp` | Display, Webserver, Sensorik, MQTT, OTA und Laufzeitlogik |
| `src/AdsbClient.cpp` | ADS-B-Abruf und JSON-Parsing |
| `src/MqttSession.cpp` | MQTT-Sitzungen, Protokollbeobachtung und Wetterdatenvalidierung |
| `src/AircraftProcessor.cpp` | Filter, Auswahl, CPA und Klassifizierung |
| `src/RouteClient.cpp` | Routendienst und Cache |
| `src/WebDisplay.cpp` | responsives FlightWall-Web-Display |
| `src/StatusLed.cpp` | gemeinsame RGB-Statuslogik beider Controller-Varianten |
| `data/` | Airport- und Airline-Quelldaten |
| `tools/` | Prüfung und Erzeugung der Lookup-Tabellen |
| `include/secrets.h.example` | Vorlage ohne echte Zugangsdaten |
| `platformio.ini` | Build- und OTA-Umgebungen |
| `CHANGELOG.md` | chronologische Änderungen |
| `GETTING_STARTED.md` | schrittweise Ersteinrichtung einschließlich optionaler Komponenten |
| `PLATFORMIO.md` | ergänzende Entwicklerhinweise |
| `docs/images/` | Fotos des Headless-Aufbaus und Screenshots von Webdisplay, Status, Konfiguration, Special-Chronik und Web-OTA |
| `IMG_4482.png` | Beispiel einer Flugdatenseite im realen Aufbau |
| `IMG_4788.jpeg` | WLAN-Einrichtungsanzeige im realen Aufbau |
| `IMG_4794.mov` | kurzes Video der laufenden Flugzeuganzeige |

## Sicherheit und Grenzen

- Konfiguration, Status und JSON-APIs besitzen keine eigene Anmeldung. Das
  Gerät ist für ein vertrauenswürdiges lokales Netz gedacht.
- Die OTA-Seite ist mit Benutzername und Passwort geschützt. Der Download der
  vollständigen Konfiguration verwendet bei aktiviertem OTA dieselbe
  Anmeldung; Status, normale Konfiguration und Wiederherstellungs-Endpunkt
  bleiben im lokalen Netz ohne eigene Anmeldung erreichbar.
- Die erzeugte Konfigurationsdatei enthält Geheimnisse unverschlüsselt.
- Das Einrichtungsnetz verwendet das dokumentierte Standardpasswort
  `flightwall`. Es sollte nur vorübergehend und nicht als dauerhaftes
  Betriebsnetz aktiv sein.
- HTTPS-Verbindungen sind verschlüsselt, die Firmware validiert die
  Serverzertifikate derzeit jedoch nicht, weil beim frühen Systemstart noch
  keine verlässliche Uhrzeit garantiert ist.
- adsb.im-Routenabfragen verwenden gemäß Dienstvorgabe unverschlüsseltes HTTP.
  Dabei sind Callsigns und Flugpositionen auf dem Übertragungsweg lesbar. Die
  Plausibilitätsprüfung bleibt aktiv. ADSBDB verwendet weiterhin HTTPS.
- Bei aktivierten externen Diensten werden Callsign und Flugzeugposition an
  den konfigurierten Routendienst beziehungsweise Standort und Radius an
  adsb.fi übertragen. MQTT-Zugangsdaten bleiben im Gerät, werden aber bei
  unverschlüsseltem Port 1883 im lokalen Netz ohne TLS verwendet.
- Routen und Klartexte hängen von Verfügbarkeit und Qualität externer Daten ab.
- CPA und aktueller Streckenabschnitt sind berechnete beziehungsweise
  abgeleitete Werte und keine Flugsicherungsinformationen.
- Wetter- und Schimmelanzeige ersetzen keine kalibrierten Messgeräte oder
  bauphysikalische Begutachtung.

## Lizenz

MIT – siehe [LICENSE](LICENSE).
