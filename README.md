# FlightWall MatrixPortal

ESP32-S3-Firmware für zwei horizontal verkettete 64×32-HUB75-Panels. Das
Adafruit MatrixPortal S3 zeigt das vom FlightWall-Server ausgewählte Flugzeug
auf einer 128×32-Pixel-Anzeige an.

## Version 2.0

- FlightWall-Daten von `http://raspi-flw:8088/api/state`
- HUB75-Ausgabe mit 3 Bit Farbtiefe und sieben Helligkeitsstufen
- Dual-Core-Betrieb: Netzwerk und Webserver auf Core 0, Display und Sensorik
  auf Core 1
- Webinterface unter `http://matrixportal.local`
- BH1750-Helligkeitsautomatik mit manuellem Fallback
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

## Webinterface

Nach erfolgreicher WLAN-Verbindung ist die Konfiguration normalerweise unter
`http://matrixportal.local` erreichbar. Falls mDNS im lokalen Netz nicht
funktioniert, zeigt der serielle Monitor die IP-Adresse an.

Konfigurierbar sind Display ein/aus, automatische oder manuelle Helligkeit,
Lux-Schwellen und lokaler Anzeigeradius. Host, Port und API-Pfad des
FlightWall-Servers lassen sich ändern und direkt testen. Die vier
Anzeigeseiten können einzeln aktiviert, frei sortiert und mit einer eigenen
Dauer versehen werden. Sanfte Übergänge sind optional. Das Webinterface ist
für ein vertrauenswürdiges lokales WLAN gedacht und hat derzeit keine eigene
Anmeldung.

Die Seite **Flughafennamen** zeigt – sofern vom FlightWall-Server geliefert –
die Langnamen des Start- und Zielflughafens. Namen, die breiter als das Display
sind, werden horizontal gescrollt. Fehlt ein Langname, wird zunächst der
Flughafencode und andernfalls `unbekannt` angezeigt.

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

## Diagnose

Die Statusseite zeigt Firmwareversion und Erstellungszeitpunkt, Laufzeit,
freien und minimal freien RAM, WLAN-Signal, Luxwert, Helligkeitsstufe,
Zeitpunkt des letzten FlightWall-Abrufs, Core-Zuordnung, Stackreserven und den
Grund des letzten Neustarts. Über eine bestätigungspflichtige Schaltfläche kann
der ESP neu gestartet werden. Maschinenlesbare Basiswerte stehen zusätzlich
unter `/api/status` als JSON bereit.

## Hardware

Die Panels benötigen ein ausreichend dimensioniertes separates 5-V-Netzteil.
Die Masse des Netzteils und des MatrixPortal muss gemeinsam verbunden sein.

## Lizenz

MIT – siehe [LICENSE](LICENSE).
