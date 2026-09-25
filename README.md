# FlightWall MatrixPortal

ESP32-S3-Firmware für zwei horizontal verkettete 64×32-HUB75-Panels. Das
Adafruit MatrixPortal S3 zeigt das vom FlightWall-Server ausgewählte Flugzeug
auf einer 128×32-Pixel-Anzeige an.

## Version 2.0

- FlightWall-Daten von `http://raspi-flw:8088/api/state`
- stabile HUB75-Ausgabe mit 2 Bit Farbtiefe
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
Lux-Schwellen, lokaler Anzeigeradius und Dauer einer Anzeigeseite. Das
Webinterface ist für ein vertrauenswürdiges lokales WLAN gedacht und hat
derzeit keine eigene Anmeldung.

## OTA-Updates

In `include/secrets.h` muss ein eigenes Passwort mit mindestens acht Zeichen
gesetzt werden:

```cpp
#define OTA_PASSWORD "ein-langes-eigenes-passwort"
```

Danach erscheint im Webinterface der Link **Firmware drahtlos aktualisieren**.
Die Anmeldung verwendet den Benutzernamen `admin` und das gesetzte
OTA-Passwort. Hochgeladen wird die von PlatformIO erzeugte `firmware.bin`.

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

## Hardware

Die Panels benötigen ein ausreichend dimensioniertes separates 5-V-Netzteil.
Die Masse des Netzteils und des MatrixPortal muss gemeinsam verbunden sein.

## Lizenz

MIT – siehe [LICENSE](LICENSE).
