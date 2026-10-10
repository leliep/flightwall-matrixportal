# FlightWall MatrixPortal

ESP32-S3-Firmware für zwei horizontal verkettete 64×32-HUB75-Panels. Das
Adafruit MatrixPortal S3 liest ADS-B-Daten direkt ein, wählt das relevante
Flugzeug aus, ergänzt Route und Namen und steuert die 128×32-Pixel-Anzeige.
Ein separater FlightMatrixServer ist nicht mehr erforderlich.

## Anzeige im Betrieb

![FlightWall MatrixPortal mit aktuellen Flugdaten](IMG_4482.png)

*Die montierte 128×32-Pixel-Matrix mit Flugnummer, Flugzeugtyp, Airline und
aktuellen Flugdaten.*

## Version 3.2.5

- direkte readsb-/dump1090-Datenquelle (`aircraft.json`)
- lokale Flugzeugauswahl mit CPA, Kleinflugzeug- und Höhenfilter
- doppelter Erfassungsradius für Special Types
- Routenabfrage über adsb.im mit lokalem Cache
- integrierte Flughafen- und Airline-Übersetzungstabellen
- HUB75-Ausgabe mit 2 Bit Farbtiefe und drei Helligkeitsstufen
- Dual-Core-Betrieb: Netzwerk und Webserver auf Core 0, Display und Sensorik
  auf Core 1
- Webinterface unter `http://matrixportal.local`
- BH1750-Helligkeitsautomatik mit manuellem Fallback
- MQTT-Wetterseite mit Windrichtung, Windgeschwindigkeit, Böen und Regenmenge
- Bedienung über die UP- und DOWN-Tasten
- dauerhaft gespeicherte Einstellungen
- Diagnose für WLAN, DNS und HTTP

## Installation

1. VS Code und die Erweiterung **PlatformIO IDE** installieren.
2. Dieses Verzeichnis in VS Code öffnen.
3. `include/secrets.h.example` nach `include/secrets.h` kopieren.
4. WLAN-Name und Passwort in `include/secrets.h` eintragen.
5. **PlatformIO: Upload** ausführen.
6. Optional den seriellen Monitor mit 115200 Baud öffnen.

`include/secrets.h` wird durch `.gitignore` vom Repository ausgeschlossen.

## LED-Matrix und Ansteuerung

### Verwendeter Aufbau

| Komponente | Ausführung |
|---|---|
| Controller | Adafruit MatrixPortal S3 mit ESP32-S3 |
| LED-Panels | 2 × RGB-HUB75, jeweils 64 × 32 Pixel |
| Gesamtauflösung | 128 × 32 Pixel |
| Anordnung | horizontal, Panel 1 links und Panel 2 rechts |
| Verkettung | HUB75-OUT des ersten Panels an HUB75-IN des zweiten Panels |
| Scanverfahren | 1/16-Scan mit vier Adressleitungen A bis D |
| Bibliothek | Adafruit Protomatter 1.7.1 und Adafruit GFX 1.12.6 |
| Farbtiefe | 2 Bit pro Farbkanal |
| Bildausgabe | doppelter Bildpuffer; sichtbarer Pufferwechsel mit `matrix.show()` |

Die beiden Panels verhalten sich für die Firmware wie eine einzige
128 × 32-Pixel-Fläche. Der erste Pixelbereich reicht von `x=0` bis `x=63`,
der zweite von `x=64` bis `x=127`. Falls die Panelreihenfolge nach dem Aufbau
vertauscht erscheint, müssen die beiden HUB75-Datenkabel beziehungsweise die
Reihenfolge der Panels getauscht werden.

### HUB75-Signale des MatrixPortal S3

Die Pinbelegung entspricht dem fest verdrahteten HUB75-Anschluss des
MatrixPortal S3 und ist am Anfang von `src/main.cpp` hinterlegt:

| HUB75-Signal | ESP32-S3 GPIO |
|---|---:|
| R1 | 42 |
| G1 | 41 |
| B1 | 40 |
| R2 | 38 |
| G2 | 39 |
| B2 | 37 |
| A | 45 |
| B | 36 |
| C | 48 |
| D | 35 |
| CLK | 2 |
| LAT | 47 |
| OE | 14 |

Ein E-Adresssignal wird nicht verwendet. Der unveränderte Aufbau ist daher
für 64 × 32-Panels mit 1/16-Scan vorgesehen. Panels mit anderer Auflösung,
anderem Scanverfahren oder fünf Adressleitungen benötigen eine angepasste
Protomatter-Konfiguration und sind nicht ohne Weiteres austauschbar.

### Technische Ansteuerung

Die Initialisierung erfolgt als 128 Pixel breite, einfach verkettete Matrix:

```cpp
Adafruit_Protomatter matrix(
    128, 2, 1, rgbPins, 4, addrPins,
    clockPin, latchPin, oePin, true);
```

Der letzte Parameter aktiviert den Doppelbuffer. Die Firmware zeichnet eine
vollständige Seite zunächst unsichtbar in den rückwärtigen Puffer und schaltet
sie anschließend mit `matrix.show()` auf die Anzeige. Vor Seitenwechseln und
Flash-Schreibvorgängen werden beide Puffer gezielt gelöscht. Zusammen mit der
auf 2 Bit reduzierten Farbtiefe verschafft dies der zeitkritischen
Protomatter-Ansteuerung mehr Reserve und reduziert die zuvor beobachteten
einzelnen Störpixel deutlich.

Die drei Helligkeitsstufen 33, 66 und 100 Prozent werden vor dem Zeichnen auf
die RGB-Farbwerte angewendet. Sie können manuell mit den beiden Tasten oder
automatisch über den BH1750 gewählt werden. Die geringe Farbtiefe ist hier
bewusst gewählt: Für die klaren Signalfarben und große Schrift der FlightWall
ist sie ausreichend und verbessert gleichzeitig die Stabilität der Ausgabe.

### Stromversorgung und Verkabelung

Die LED-Panels werden **nicht** aus dem USB-Anschluss des MatrixPortal gespeist.
Sie benötigen ein eigenes, ausreichend dimensioniertes und stabilisiertes
5-V-Netzteil. Beide Panels werden parallel mit 5 V versorgt; der genaue
Strombedarf hängt von Paneltyp, Helligkeit und dargestelltem Bild ab. Netzteil,
beide Panels und MatrixPortal müssen eine gemeinsame Masse besitzen.

Für die Datenverbindung werden kurze, sauber sitzende HUB75-Flachbandkabel
empfohlen. Die Einspeisung erfolgt am `IN`-Anschluss des ersten Panels, dessen
`OUT`-Anschluss mit `IN` des zweiten Panels verbunden wird. Vor Änderungen an
Verkabelung oder Panelreihenfolge sollte die 5-V-Versorgung ausgeschaltet
werden.

## Wiederherstellungspunkt

Der letzte bekannte Stand vor der Server-Integration ist in GitHub mit dem
Tag `pre-flightmatrix-integration-2026-10-04` gesichert. Er kann jederzeit in
einen eigenen Wiederherstellungszweig ausgecheckt werden:

```bash
git fetch --tags
git switch -c restore-pre-integration pre-flightmatrix-integration-2026-10-04
```

## Webinterface

Nach erfolgreicher WLAN-Verbindung ist die Konfiguration normalerweise unter
`http://matrixportal.local` erreichbar. Falls mDNS im lokalen Netz nicht
funktioniert, zeigt der serielle Monitor die IP-Adresse an.

Unter `http://matrixportal.local/display` steht zusätzlich ein responsives
FlightWall-Dashboard für iPhone, Tablet und Desktop bereit. Es zeigt das
aktuelle Flugzeug in einer eigenständigen, für Webbrowser optimierten Ansicht:
Callsign, Flugzeugtyp, Airline, Route mit Flughafennamen, Entfernung, Höhe,
Geschwindigkeit, CPA, Steig-/Sinktendenz und Blickrichtung. Die Daten werden
automatisch aktualisiert.

Konfigurierbar sind Display ein/aus, automatische oder manuelle Helligkeit,
Lux-Schwellen, lokaler Anzeigeradius, Standort, Blickrichtung, zwei ADS-B-Quellen
mit Auswahl der aktiven Quelle,
Routendienst, Kleinflugzeug- und Höhenfilter sowie eine tägliche Nachtruhe. Der Zeitraum
darf über Mitternacht reichen, beispielsweise von 23:55 bis 05:55. Während
dieser Zeit bleibt die Matrix einschließlich Betriebs-Pixel vollständig dunkel;
Netzwerk, Datenabruf, Webinterface und OTA bleiben aktiv. Die lokale Uhrzeit
wird per NTP bezogen und automatisch zwischen MEZ und MESZ umgestellt.
Der Kleinflugzeugfilter erkennt unter anderem deutsche `D-E…`-, `D-K…`- und
`D-M…`-Kennzeichen; damit werden auch Motorsegler erfasst.
Wenn kein Flugzeug angezeigt wird, blinkt der Betriebs-Pixel unten links bei
erfolgreichem ADS-B-Abruf weiß. Nach einem fehlgeschlagenen
Datenabruf blinkt er rot, bis wieder gültige Daten empfangen wurden.
Die Statusanzeige und `/api/status` melden außerdem die interne
ESP32-S3-Chiptemperatur. Sie dient zur thermischen Überwachung und entspricht
nicht der Umgebungstemperatur.
Der Statusbereich im Webinterface aktualisiert sich bei sichtbarer Seite alle
fünf Sekunden, ohne ungespeicherte Eingaben im Konfigurationsformular zu ändern.
Vier farbige Zustandsanzeigen geben dort sofort Auskunft über System, ADS-B,
MQTT und Sensoren. Die Detailwerte sind in responsive Karten für Netzwerk,
aktuelles Flugzeug, ADS-B und Route, Konfiguration, Übersetzungstabellen,
Wetter/Sensoren und Display gegliedert. Lange URLs, Speicher-, Task- und
Resetinformationen bleiben in standardmäßig geschlossenen Diagnosebereichen;
auf dem iPhone ordnen sich alle Karten automatisch einspaltig an.
Für die Blickrichtung werden außerdem die Standorthöhe über NHN und die
Entfernungsgrenze für `weit` gespeichert.
Routen mit Zwischenlandungen bleiben vollständig erhalten, beispielsweise
`CGN-BER-CGN`. Matrix, Web-Display, Status-API und Routentest zeigen dabei alle
Flughäfen in der korrekten Reihenfolge; Start und Endziel werden zusätzlich
weiterhin separat bereitgestellt.
Neue Routenergebnisse werden beim Flugzeugwechsel ausschließlich im RAM
zwischengespeichert. Die persistente Speicherung erfolgt gesammelt, sobald
kein Flugzeug angezeigt wird oder die Nachtruhe aktiv ist. Während des kurzen
Flash-Schreibvorgangs wird die Matrix kontrolliert dunkel geschaltet, um
zufällige Lichtpunkte durch Flashzugriffe des ESP32-S3 zu vermeiden.
Die beiden vollständigen ADS-B-URLs und der Routendienst lassen sich ändern.
Nur die ausgewählte ADS-B-Quelle wird abgefragt und direkt getestet. Die sieben
Anzeigeseiten können einzeln aktiviert, frei sortiert und mit einer eigenen
Dauer versehen werden. Sanfte Übergänge sind optional. Das Webinterface ist
für ein vertrauenswürdiges lokales WLAN gedacht und hat derzeit keine eigene
Anmeldung.

Der Displaytest im Webinterface prüft die gesamte Matrix für jeweils kurze
Zeit mit Rot, Grün, Blau, Weiß, einem Pixelraster, geometrischen Linien, Text
und Farbbalken. Anschließend zeigt er alle sieben 14×7-Typensymbole jeweils
zusammen mit ihrer Bedeutung. Nach etwa 23 Sekunden endet er automatisch;
über denselben Schalter kann er vorher beendet werden.

Die Seite **Flughafennamen** zeigt – sofern eine Route ermittelt wurde –
die Langnamen des Start- und Zielflughafens. Namen, die breiter als das Display
sind, werden vollständig horizontal gescrollt; dafür verlängert die Firmware
die konfigurierte Seitendauer bei Bedarf automatisch. Fehlt ein Langname, wird
der Flughafencode angezeigt. Sind Start und Ziel beide unbekannt, wird die
gesamte Seite automatisch übersprungen.

Die Seite **Freitext** zeigt einen im Webinterface eingegebenen Text mit bis
zu 128 darstellbaren Zeichen. Bei jedem Seitenaufruf läuft der Text genau
einmal vollständig von rechts nach links über die Anzeige. Die Firmware
verlängert die konfigurierte Seitendauer automatisch, wenn der Durchlauf mehr
Zeit benötigt. Die Scrollgeschwindigkeit ist unabhängig davon als langsam,
mittel oder schnell wählbar; bestehende Konfigurationen verwenden mittel.
Ein leerer Freitext überspringt die Seite.

Vor jeder Seitenaktivierung löscht die Firmware beide Matrixpuffer vollständig
und setzt den Seitenzeitpunkt neu. Dadurch erscheinen weder Bildreste noch alte
Scrollpositionen der zuvor angezeigten Seite.

Die alternative **Kompaktansicht** bündelt Flugnummer, kurzen ICAO-Flugzeugtyp,
Entfernung, Höhe in Fuß mit direkt folgendem farbigem Steig-/Sinkdreieck und
Fluggeschwindigkeit,
Blickrichtung mit
Richtungspfeil sowie Route und CPA auf einer Seite. Entfernt sich das Flugzeug
nach dem CPA, erscheint bei ausreichendem Platz `entfernt sich`, andernfalls
die Kurzform `geht`.
Bei gleichbleibender Höhe wird kein Dreieck angezeigt. Alle Blickrichtungspfeile
verwenden auf sämtlichen Seiten dieselbe gleichmäßig kräftige 7×7-Gestaltung.
Die Geschwindigkeit erscheint bei ausreichendem Platz als beispielsweise
`900kmh`, bei weniger Platz als `900k` und wird andernfalls weggelassen. Fehlt
das Steig-/Sinkdreieck, trennt nach Möglichkeit ein Leerzeichen Höhe und
Geschwindigkeit; nur bei Platzmangel wird der Abstand verkleinert.
Fehlende Routenteile erzeugen kein einzelnes Trennzeichen. Alle Werte werden
mit den laufenden ADS-B-Abrufen aktualisiert. Für Hubschrauber, A380,
Militär, Polizei, Regierung und Luftschiff (`SHIP`) verwendet sie eigene
14×7-Symbole; unbekannte Special Types erhalten ein allgemeines Symbol.

Die Seite **Kompakt + Flughäfen** zeigt in der ersten Zeile statt der Flugnummer
die Fluggesellschaft in Rot. Ist der verfügbare Platz vor dem unverändert
rechts stehenden Typensymbol und Flugzeugtyp zu kurz, scrollt ihr vollständiger
Name innerhalb dieses Bereichs. Der Serverwert `UNKNOWN` wird als `Airline N/A`
angezeigt. In der unteren Zeile erscheinen Start- und
Zielflughafen sofort ausgeschrieben in
einem dynamischen Layout. Passen beide Namen gemeinsam in die Zeile, stehen sie
links- und rechtsbündig und das `>` mittig im freien Raum. Passt nur ein Name
in weniger als eine halbe Zeile, bleibt er außen bündig stehen und das `>`
rückt an ihn heran, während der lange Name im übrigen Bereich scrollt. Sind
beide Namen länger, scrollen beide in getrennten Hälften mit festem `>` in der
Mitte. Kurze Namen bleiben mindestens fünf Sekunden sichtbar; die konfigurierte
Seitendauer wird bei Bedarf bis zum Ende aller Lauftexte verlängert. Jeder
Lauftext pausiert kurz am vollständig gescrollten Ende, läuft anschließend mit
vierfacher Geschwindigkeit zum Anfang zurück und pausiert dort nochmals kurz,
bevor ein Seitenwechsel erfolgen kann.

Die Matrix berechnet den Zenitwinkel selbst aus horizontaler Entfernung,
Flugzeughöhe und konfigurierter Standorthöhe. Bis 10° vom Zenit erscheint
`oben`, bis 30° `oben vorne/rechts/hinten/links`. Unterhalb dieses Bereichs
gelten `vorne`, `links`, `rechts`, `hinten` und die beiden Schulterangaben;
ab der konfigurierten Entfernung werden die vier groben Richtungen mit `weit`
gekennzeichnet. Eine Winkel- und Entfernungshysterese verhindert springende
Texte an den Grenzen.

Special Types erhalten ein kleines farbiges Symbol neben der Flugzeugkennung.
Vor einem OTA-Flashvorgang erscheint kurz `OTA UPDATE / BITTE WARTEN`; danach
bleibt das Display bis zum Neustart schwarz.

## OTA-Updates

In `include/secrets.h` muss ein eigenes Passwort mit mindestens acht Zeichen
gesetzt werden:

```cpp
#define OTA_PASSWORD "ein-langes-eigenes-passwort"
```

Danach erscheint im Webinterface der Link **Firmware drahtlos aktualisieren**.
Die Anmeldung verwendet den Benutzernamen `admin` und das gesetzte
OTA-Passwort. Hochgeladen wird die von PlatformIO erzeugte `firmware.bin`.
Die Updateseite prüft Dateityp und ESP32-Dateikopf, verlangt eine Bestätigung,
zeigt den Uploadfortschritt und vergleicht nach dem Neustart die Firmwareversion
vor und nach dem Update. Flash- und Prüfungsfehler werden einzeln gemeldet.

Alternativ kann das OTA-Environment von PlatformIO verwendet werden. Das
Passwort wird dabei bewusst nicht in `platformio.ini` gespeichert:

```bash
export MATRIXPORTAL_OTA_PASSWORD='ein-langes-eigenes-passwort'
pio run -e adafruit_matrixportal_esp32s3_ota -t upload
```

In VS Code fasst die Aufgabe **FlightWall: Git-Update + OTA** das vorherige
Fast-forward-Update von `origin/main`, die verdeckte Passwortabfrage und den
OTA-Upload zusammen. Sie ist über **Terminal → Aufgabe ausführen…** erreichbar.

Das erste Aufspielen einer OTA-fähigen Firmware muss weiterhin per USB
erfolgen. Ohne gültiges `OTA_PASSWORD` sind beide OTA-Wege deaktiviert.

## BH1750

| BH1750 | Standard: gelötet | Alternative: STEMMA QT |
|---|---|---|
| VIN | 3,3 V | 3,3 V |
| GND | GND | GND |
| SDA | A1 / GPIO 3 | GPIO 16 |
| SCL | A2 / GPIO 9 | GPIO 17 |

Die Pins stehen gut sichtbar am Anfang von `src/main.cpp`. Die Adressen `0x23`
und `0x5C` werden automatisch erkannt. Antwortet der Sensor nicht, verwendet
die Firmware die gespeicherte manuelle Helligkeit und sucht ihn regelmäßig
erneut.

## Tasten

- **UP:** manueller Modus, Helligkeit erhöhen
- **DOWN:** manueller Modus, Helligkeit verringern
- **UP + DOWN:** Helligkeitsautomatik aktivieren

## BME280

Ein BME280 wird an den I²C-Adressen `0x76` und `0x77` automatisch gesucht.
Die Firmware prüft sowohl den mit dem BH1750 geteilten Bus auf GPIO 3/9 als
auch den STEMMA-QT-Bus auf GPIO 16/17. Umgebungstemperatur, relative
Luftfeuchtigkeit und der anhand der konfigurierten Standorthöhe auf Meereshöhe
reduzierte Luftdruck erscheinen in der Statusseite und unter `/api/status`.
Der direkt gemessene Stationsdruck bleibt dort zusätzlich als Diagnosewert
verfügbar. In der Webkonfiguration lässt sich außerdem einschalten, dass
die komplette Wetterseite auf der Matrix erscheint, solange kein Flugzeug innerhalb
des Anzeigeradius liegt. Dieser gemeinsame Schalter steuert sowohl die
BME280- als auch die MQTT-Werte. Ein erkanntes Flugzeug übernimmt die Anzeige sofort;
Nachtruhe und ausgeschaltetes Display bleiben vollständig dunkel.

## MQTT-Wetter

Die Firmware kann Windrichtung, Windgeschwindigkeit, Windböen und die
Regenmenge der letzten 24 Stunden von einem Mosquitto-Server im lokalen Netz
beziehen. Server, Port, Benutzer, Passwort und alle vier Topics werden im
Webinterface konfiguriert; das Passwort erscheint weder dort noch in der
Status-API im Klartext. Die gelieferten Zentimeter der Regenmenge werden für
die Anzeige in Millimeter umgerechnet.

Wenn kein Flugzeug angezeigt wird, erscheinen sämtliche Klima- und Wetterwerte
ruhig auf einer gemeinsamen Kompaktseite. Die obere Zeile zeigt Temperatur,
Feuchte und Luftdruck, die untere Windrichtung, Windgeschwindigkeit, Böen (`B`)
und Regenmenge (`R`). Wind und Böen werden platzsparend als ganze km/h
dargestellt. Feuchte und Böenwert teilen sich eine gemeinsame Spaltenmitte;
Luftdruck und Regenmenge sind am rechten Rand bündig ausgerichtet. Eine
numerische Windrichtung wird automatisch in eine von 16 deutschen
Himmelsrichtungen umgesetzt. Werte, die länger als 15 Minuten nicht erneuert
wurden, werden nicht mehr angezeigt. Verbindungszustand, letzter Empfang und
Messwerte stehen zusätzlich in der Statusseite und unter `/api/status`.

## Diagnose

Die Statusseite zeigt Firmwareversion und Erstellungszeitpunkt, Laufzeit,
freien und minimal freien RAM, WLAN-Signal, Luxwert, BME280-Umgebungswerte, Helligkeitsstufe,
Zeitpunkt des letzten ADS-B-Abrufs, empfangene und geeignete Flugzeuge,
unbekannte Flughafen- und Airline-Codes, Core-Zuordnung, Stackreserven und den
Grund des letzten Neustarts. Über eine bestätigungspflichtige Schaltfläche kann
der ESP neu gestartet werden. Maschinenlesbare Basiswerte stehen zusätzlich
unter `/api/status` als JSON bereit.

## Hardware

Die Panels benötigen ein ausreichend dimensioniertes separates 5-V-Netzteil.
Die Masse des Netzteils und des MatrixPortal muss gemeinsam verbunden sein.

## Lizenz

MIT – siehe [LICENSE](LICENSE).
