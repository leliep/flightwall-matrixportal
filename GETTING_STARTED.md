# FlightWall – Getting Started

Diese Anleitung beschreibt die grundlegende Einrichtung von FlightWall in der
Firmwareversion `3.6.0-dual-target.1`. Sie führt vom Hardwareaufbau bis zur
ersten Flugzeuganzeige und erklärt auch alle optionalen Komponenten.

FlightWall arbeitet eigenständig auf dem ESP32-S3. Ein Raspberry Pi oder
FlightMatrixServer wird nicht benötigt. Die Firmware steht als MatrixPortal-
Variante mit HUB75-Anzeige und als Headless-Variante für ein ESP32-S3-N16R8-
DevKit ohne LED-Matrix zur Verfügung. Erforderlich ist lediglich eine
erreichbare ADS-B-Datenquelle im lokalen Netz oder die öffentliche
adsb.fi-Schnittstelle.

## 1. Was wird benötigt?

### Pflichtausstattung

Für die Matrixvariante:

| Komponente | Anforderung |
|---|---|
| Controller | Adafruit MatrixPortal S3 |
| LED-Matrix | zwei 64×32-HUB75-Panels, 1/16-Scan |
| Gesamtanzeige | 128×32 Pixel, beide Panels horizontal verkettet |
| Stromversorgung | stabilisiertes 5-V-Netzteil mit ausreichender Stromreserve |
| Datenkabel | zwei HUB75-Flachbandkabel |
| Einrichtung | USB-C-Datenkabel und Rechner mit PlatformIO |
| Netzwerk | 2,4-GHz-WLAN mit Zugriff auf eine ADS-B-Quelle oder das Internet |

Für die Headless-Variante genügen ein ESP32-S3-DevKitC-1 mit N16R8-Modul,
USB-C-Datenkabel und WLAN. Die Flugzeuganzeige erfolgt unter
`http://flightwall-server.local/display`; LED-Matrix, HUB75-Kabel und
Panelnetzteil entfallen.

Das LED-Netzteil muss zur tatsächlichen Stromaufnahme der Panels passen. Eine
großzügige Reserve ist sinnvoll. Die Panels dürfen nicht über den
USB-Anschluss des MatrixPortal versorgt werden.

### Optionale Komponenten und Dienste

| Komponente | Nutzen | Ohne diese Komponente |
|---|---|---|
| BH1750 | automatische Helligkeitsregelung | manuelle Helligkeit bleibt verfügbar |
| BME280 | Raumtemperatur, Feuchte, Luftdruck und Schimmelindikator | keine lokalen Klimawerte |
| MQTT-Broker | Windrichtung, Wind, Böen und Regen heute | Wetterseite zeigt nur BME280-Werte |
| zweite lokale ADS-B-Quelle | schneller Wechsel zwischen zwei Empfängern | eine Quelle genügt vollständig |
| adsb.fi | ADS-B-Empfang ohne eigenen Empfänger | lokale Quelle kann allein verwendet werden |
| adsb.im/ADSBDB | Route, Airline sowie Start und Ziel | Flugzeugdaten funktionieren ohne Route |
| OTA-Passwort | spätere Updates über WLAN | Updates erfolgen weiterhin per USB |
| Smartphone/Browser | Konfiguration, Status und Web-Display | Matrix arbeitet selbstständig weiter |

## 2. Matrix anschließen

Die beiden Panels werden von links nach rechts verkettet:

```text
MatrixPortal HUB75 ──> IN Panel 1
OUT Panel 1 ─────────> IN Panel 2
```

Beide Panels werden direkt und parallel aus dem separaten 5-V-Netzteil
versorgt. MatrixPortal, Netzteil und Panels benötigen eine gemeinsame Masse.

Vor dem Einschalten prüfen:

1. Das Datenkabel steckt am ersten Panel in **IN**, nicht in **OUT**.
2. Das zweite Kabel führt von **OUT** des ersten zu **IN** des zweiten Panels.
3. Plus und Minus der 5-V-Versorgung sind nicht vertauscht.
4. Es liegen keine 12 V oder eine andere höhere Spannung an.
5. Kabel werden nur bei ausgeschalteter Panelversorgung umgesteckt.

Die aktuelle Firmware ist für zwei 64×32-Panels mit vier Adressleitungen A–D
ausgelegt. Andere Auflösungen und Scanverfahren benötigen eine angepasste
Firmwarekonfiguration.

## 3. Firmware erstmals installieren

### PlatformIO vorbereiten

Benötigt werden:

- Git
- Visual Studio Code mit PlatformIO-Erweiterung oder PlatformIO Core
- ein USB-C-Kabel, das Daten übertragen kann

Repository laden und die lokale Zugangsdaten-Datei anlegen:

```bash
git clone https://github.com/leliep/flightwall-matrixportal.git
cd flightwall-matrixportal
cp include/secrets.h.example include/secrets.h
```

In `include/secrets.h` die erstmaligen WLAN-Daten eintragen:

```cpp
constexpr char WIFI_SSID[] = "mein-wlan";
constexpr char WIFI_PASSWORD[] = "mein-wlan-passwort";

#define OTA_PASSWORD "mein-sicheres-ota-passwort"
#define MQTT_PASSWORD ""
```

Hinweise:

- Das OTA-Passwort ist optional, muss bei Verwendung aber mindestens acht
  Zeichen lang sein.
- Das MQTT-Passwort ist nur erforderlich, wenn ein geschützter MQTT-Broker
  verwendet wird.
- `include/secrets.h` wird von Git ignoriert und darf nicht veröffentlicht
  werden.
- WLAN-Zugangsdaten lassen sich später über die Webseite ändern und bleiben
  bei Firmwareupdates erhalten.

MatrixPortal-Firmware bauen und per USB übertragen:

```bash
pio run
pio run -t upload
```

Headless-Firmware für das ESP32-S3-N16R8-Board bauen und übertragen:

```bash
pio run -e esp32s3_n16r8_headless
pio run -e esp32s3_n16r8_headless -t upload
```

Bei Problemen liefert der serielle Monitor weitere Informationen:

```bash
pio device monitor -b 115200
```

## 4. WLAN-Ersteinrichtung

Sind die in `secrets.h` eingetragenen WLAN-Daten erreichbar, verbindet sich
der Controller automatisch. Die Konfiguration ist anschließend normalerweise
unter folgender Adresse erreichbar:

```text
http://matrixportal.local/
```

Beim Headless-Server lautet die Adresse entsprechend
`http://flightwall-server.local/`.

Funktioniert der Name im eigenen Netz nicht, kann die im Router oder seriellen
Monitor angezeigte IP-Adresse verwendet werden.

### Einrichtungsnetz als Rückfallweg

Kann keine Verbindung zum gespeicherten WLAN hergestellt werden, öffnet der
Controller sein eigenes WLAN. Die Matrix zeigt dessen Namen, Passwort und die
Konfigurationsadresse an.

| Einstellung | Wert |
|---|---|
| WLAN-Name | `FlightWall-Setup-…` |
| Passwort | `flightwall` |
| Adresse | `http://192.168.4.1/` |

Mit diesem WLAN verbinden, die Adresse öffnen und unter **WLAN-Zugang** das
gewünschte Heimnetz eintragen. Nach dem Speichern startet der Controller neu
und verbindet sich mit dem neuen WLAN.

## 5. Grundkonfiguration

Nach dem ersten erfolgreichen Aufruf der Konfigurationsseite sollten die
folgenden Bereiche in dieser Reihenfolge geprüft werden.

### Sprache

Deutsch oder Englisch auswählen. Die Einstellung gilt für Matrix,
Konfigurationsseite, Status, Diagnose und Web-Display.

### Standort und Ausrichtung

Eintragen beziehungsweise kontrollieren:

- Breitengrad des Aufstellorts
- Längengrad des Aufstellorts
- Höhe über Normalnull in Metern
- Blickrichtung der Matrix in Grad, wobei Norden `0°` entspricht
- gewünschter normaler Anzeigeradius in Kilometern

Die Standortdaten werden für Entfernung, Blickrichtung, vertikalen Blickwinkel,
CPA, adsb.fi-Abfragen und die Luftdruckkorrektur benötigt. Die mitgelieferten
Standardwerte gehören zum ursprünglichen Aufbau und sollten an einem anderen
Standort geändert werden.

### ADS-B-Quelle

Es stehen drei Quellen zur Auswahl:

1. erste frei konfigurierbare lokale Quelle
2. zweite frei konfigurierbare lokale Quelle
3. öffentliche adsb.fi-Quelle

Für einen lokalen readsb-/dump1090-Empfänger wird dessen `aircraft.json`
eingetragen, beispielsweise:

```text
http://mein-adsb-empfaenger/data/aircraft.json
```

Port und Pfad hängen von der Installation des Empfängers ab. Die gewünschte
Quelle als **aktiv** auswählen und den **ADS-B-Verbindungstest** ausführen.
Empfangene Flugzeuge und geeignete Kandidaten sollten danach im Status
erscheinen.

Bei adsb.fi erzeugt FlightWall die konkrete URL automatisch aus Standort und
Radius. Dafür benötigt der Controller Internetzugang.

### Filter

Für den Einstieg empfiehlt sich:

- Sport- und Kleinflugzeugfilter zunächst ausgeschaltet lassen, wenn die
  korrekte Datenverarbeitung kontrolliert werden soll.
- Maximalhöhenfilter zunächst ausschalten oder großzügig einstellen.
- Nach erfolgreichem Test beide Filter nach Wunsch aktivieren.

Transpondernotfälle mit `7500`, `7600` oder `7700` umgehen Radius und Filter
immer. Erkannte Special Types werden innerhalb des doppelten normalen Radius
bevorzugt berücksichtigt.

### Matrixseiten

Die sieben Flugzeugseiten können einzeln aktiviert, sortiert und mit einer
Anzeigedauer versehen werden. Für einen ersten Test genügen beispielsweise:

- Kompaktansicht
- Airline und Route
- CPA und Sondertyp

Danach können Flughafennamen, Freitext und weitere Ansichten ergänzt werden.
Mit dem integrierten Matrixtest lassen sich Farben, Pixel, Linien, Schriften
und Special-Symbole unabhängig von einem realen Flugzeug kontrollieren.

### Helligkeit und Nachtruhe

Ohne BH1750 wird eine feste Helligkeit eingestellt. Zusätzlich können
Nachtruhe, Ein-/Ausschaltzeit und sanfte Seitenübergänge konfiguriert werden.

Die Tasten am MatrixPortal haben folgende Funktion:

- **UP:** manueller Modus und heller
- **DOWN:** manueller Modus und dunkler
- **UP + DOWN:** Helligkeitsautomatik aktivieren

## 6. Optionale Routenauflösung

Die Flugzeuganzeige benötigt keinen Routendienst. Wenn **Routen ergänzen**
aktiviert ist, fragt FlightWall zuerst den konfigurierten Primärdienst ab –
standardmäßig adsb.im. Wird dort keine Route gefunden, kann ADSBDB als
kostenloser Fallback verwendet werden.

Der Fallback:

- benötigt kein Konto und keinen API-Schlüssel,
- wird nur bei fehlender Primärroute abgefragt,
- prüft Callsign und geografische Plausibilität,
- hält seine Ergebnisse höchstens einen Tag im RAM,
- schreibt ADSBDB-Routen nicht in den permanenten Cache.

Mit **Routendienst testen** kann ein Callsign direkt geprüft werden. Die
Statusseite zeigt anschließend auch die verwendete Routenquelle.

## 7. Optionaler BH1750

Der BH1750 regelt die Matrixhelligkeit anhand des Umgebungslichts. Im aktuellen
Aufbau wird er am zusätzlichen I²C-Bus angeschlossen:

| BH1750 | MatrixPortal S3 |
|---|---|
| VIN | 3,3 V |
| GND | GND |
| SDA | A1 / GPIO 3 |
| SCL | A2 / GPIO 9 |

Die Adressen `0x23` und `0x5C` werden automatisch erkannt. Nach dem Anschluss:

1. Controller neu starten.
2. Im Status prüfen, ob Luxwerte erscheinen.
3. Unter **Helligkeit** die Lux-Schwellen und zugehörigen Stufen einstellen.
4. Die Automatik auf der Webseite oder mit **UP + DOWN** aktivieren.

Fehlt der Sensor oder fällt er aus, verwendet FlightWall weiterhin die
gespeicherte manuelle Helligkeit.

## 8. Optionaler BME280

Der BME280 ergänzt lokale Klimadaten:

- Umgebungstemperatur in °C
- relative Luftfeuchte
- auf Meereshöhe umgerechneter Luftdruck
- Taupunkt und Schimmelindikator

Er wird automatisch unter `0x76` und `0x77` gesucht. Unterstützt werden:

- derselbe I²C-Bus GPIO 3/9 wie beim BH1750 oder
- der STEMMA-QT-Bus GPIO 16/17

Bei einer losen Breakout-Platine 3,3 V verwenden, sofern deren Hersteller
nicht ausdrücklich eine eigene 5-V-taugliche Spannungsregelung angibt. BH1750
und BME280 können gemeinsam am I²C-Bus betrieben werden, solange ihre Adressen
nicht kollidieren.

Nach dem Anschluss im Status kontrollieren, ob Temperatur, Feuchte,
Stationsdruck und reduzierter Luftdruck plausible Werte liefern. Für die
Umrechnung auf Meereshöhe muss die konfigurierte Standorthöhe stimmen.

Die Option **Wetterdaten anzeigen, wenn kein Flugzeug in der Nähe ist** schaltet
die gesamte Wetterseite ein oder aus. Ohne MQTT zeigt sie nur die verfügbaren
BME280-Werte.

## 9. Optionale MQTT-Wetterdaten

Über einen Mosquitto-kompatiblen Broker kann die Wetterseite zusätzlich
anzeigen:

- Windrichtung
- Windgeschwindigkeit in km/h
- Böengeschwindigkeit in km/h
- Regen heute

In der Konfigurationsseite werden Brokername oder IP-Adresse, Port, Benutzer,
Passwort und vier Topics eingetragen. Typische Topic-Namen könnten sein:

```text
wetter/windrichtung
wetter/wind_kmh
wetter/boeen_kmh
wetter/regen_heute_cm
```

Die Payloads sollen jeweils nur den aktuellen Wert enthalten. Die
Windrichtung darf als Gradzahl oder kurze Himmelsrichtung geliefert werden.
Wind und Böen werden als km/h erwartet. Der Regenwert wird in Zentimetern
empfangen und auf der Matrix in Millimeter umgerechnet.

MQTT ist vollständig optional. Bei fehlender Verbindung laufen ADS-B,
Routenauflösung, Matrix und BME280 normal weiter. Wetterwerte gelten nach
15 Minuten ohne neue Nachricht als veraltet und werden dann nicht mehr
angezeigt.

## 10. Weboberfläche und Web-Display

Die wichtigsten Adressen sind:

| Adresse | Funktion |
|---|---|
| `http://matrixportal.local/` | Konfiguration und Status |
| `http://matrixportal.local/display` | FlightWall-Web-Display für iPhone, Tablet und Desktop |
| `http://matrixportal.local/api/display` | aktuelle Anzeigedaten als JSON |
| `http://matrixportal.local/api/status` | vollständige Diagnose als JSON |
| `http://matrixportal.local/unknown-codes.json` | unbekannte Airport-/Airline-Codes herunterladen |
| `http://matrixportal.local/update` | Web-OTA, sofern aktiviert |

Das Web-Display ist optional. Es zeigt dieselben aktuellen Flugdaten in einem
responsiven Browserlayout; die LED-Matrix funktioniert unabhängig davon.

## 11. OTA optional aktivieren

Das erste Image muss per USB installiert werden. Für spätere Updates kann ein
mindestens acht Zeichen langes `OTA_PASSWORD` in `include/secrets.h` gesetzt
werden.

### Update über die Webseite

1. Neue Firmware bauen: `pio run`
2. `http://matrixportal.local/update` öffnen.
3. Mit Benutzer `admin` und dem OTA-Passwort anmelden.
4. `.pio/build/adafruit_matrixportal_esp32s3/firmware.bin` auswählen.
5. Upload und automatischen Neustart abwarten.

### Update direkt mit PlatformIO

```bash
export MATRIXPORTAL_OTA_PASSWORD='mein-sicheres-ota-passwort'
pio run -e adafruit_matrixportal_esp32s3_ota -t upload
```

Vor jedem Neustart oder OTA die Listen **Unbekannte Airports** und
**Unbekannte Airlines** auf der Statusseite mit **Codes herunterladen (.json)**
sichern. Die Datei enthält beide Listen und mögliche Überlaufzähler; der
Download löscht keine Codes. Die Beobachtungen befinden sich nur im flüchtigen
Speicher und gehen beim Neustart verloren.

## 12. Erster Funktionstest

Eine erfolgreiche Grundeinrichtung ist an folgenden Punkten erkennbar:

- Matrix startet ohne dauerhaftes Fehlerbild.
- WLAN-Status zeigt **verbunden**.
- Konfigurationsseite ist über Namen oder IP-Adresse erreichbar.
- ADS-B-Verbindungstest ist erfolgreich.
- Status zeigt empfangene Flugzeuge und – abhängig von Radius und Filtern –
  geeignete Kandidaten.
- Bei einem Flugzeug im Radius erscheinen Callsign und Flugdaten auf der
  Matrix.
- Der Routentest liefert entweder eine Route oder die klare Meldung, dass für
  dieses Callsign keine Route gefunden wurde.
- Der Matrixtest zeigt Farben, Linien, Text und Symbole ohne auffällige
  Störpixel.

## 13. Häufige Probleme

### Konfigurationsseite nicht erreichbar

- IP-Adresse im Router nachsehen.
- `http://matrixportal.local/` und die numerische IP-Adresse testen.
- Prüfen, ob das Einrichtungsnetz `FlightWall-Setup-…` aktiv ist.
- Bei weiterhin laufender Matrix nicht sofort mehrfach zurücksetzen; zunächst
  offene Browserclients schließen und erneut versuchen.

### ADS-B verbunden, aber kein Flugzeug sichtbar

- Anzeigeradius vergrößern.
- Kleinflugzeug- und Maximalhöhenfilter vorübergehend ausschalten.
- Standortkoordinaten prüfen.
- Im Status **Empfangen / Kandidaten** vergleichen.
- Kontrollieren, ob die aktive ADS-B-Quelle die gewünschte ist.

### Falsche Richtung oder falsche Angabe „weit/oben“

- Blickrichtung der Matrix kontrollieren.
- Standortkoordinaten und Höhe über Normalnull korrigieren.
- Grenzwert für den vertikalen Blickwinkel prüfen.

### BME280 oder BH1750 fehlt im Status

- Versorgungsspannung und gemeinsame Masse prüfen.
- SDA und SCL nicht vertauschen.
- Sensoradresse prüfen.
- Controller nach dem Verkabeln neu starten.

### Flackern, falsche Farben oder Störpixel

- stabiles 5-V-Netzteil und kurze, sichere Stromleitungen verwenden,
- Masseverbindung kontrollieren,
- HUB75-Kabel und IN/OUT-Richtung prüfen,
- Helligkeit reduzieren,
- integrierten Matrixtest ausführen.

## 14. Abschluss-Checkliste

- [ ] Panels korrekt verkettet und separat mit 5 V versorgt
- [ ] Firmware erfolgreich per USB installiert
- [ ] WLAN verbunden und Konfigurationsseite erreichbar
- [ ] Standort, Höhe und Blickrichtung angepasst
- [ ] ADS-B-Quelle ausgewählt und getestet
- [ ] Radius und Filter sinnvoll eingestellt
- [ ] gewünschte Matrixseiten aktiviert und sortiert
- [ ] Helligkeit und gegebenenfalls Nachtruhe eingestellt
- [ ] optionale Sensoren im Status erkannt
- [ ] optionales MQTT auf aktuelle Werte geprüft
- [ ] OTA-Passwort sicher gesetzt oder OTA bewusst deaktiviert
- [ ] Matrix- und Routentest ausgeführt

Weitere technische Details, Diagnosemöglichkeiten und Pflegehinweise enthält
die [vollständige README](README.md).
