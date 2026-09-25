# PlatformIO und VS Code

## Build und Upload

1. Projektordner in VS Code öffnen.
2. `include/secrets.h.example` nach `include/secrets.h` kopieren.
3. WLAN-Zugangsdaten eintragen.
4. **PlatformIO: Build** wählen.
5. MatrixPortal S3 per USB verbinden und **PlatformIO: Upload** wählen.

Der serielle Monitor verwendet 115200 Baud.

## Laufzeitarchitektur

- Arduino `loop()` läuft auf Core 1 und bedient Display, Taster und BH1750.
- Eine eigene FreeRTOS-Task läuft auf Core 0 und bedient WLAN, DNS, HTTP,
  FlightWall und das Webinterface.
- Eine Mutex-geschützte vollständige `FlightData`-Kopie übergibt neue Daten an
  die Anzeige. Der Netzwerk-Task greift nie direkt auf die Matrix zu.

## Webinterface

Nach dem WLAN-Verbindungsaufbau startet der HTTP-Server auf Port 80. Er ist
über `http://matrixportal.local` oder die im seriellen Monitor genannte
IP-Adresse erreichbar. Einstellungen werden in NVS gespeichert.

## OTA

OTA ist nur aktiv, wenn `OTA_PASSWORD` in `include/secrets.h` mindestens acht
Zeichen lang ist. Das Webinterface bietet dann einen passwortgeschützten
Upload für `firmware.bin`. Für PlatformIO steht zusätzlich das Environment
`adafruit_matrixportal_esp32s3_ota` bereit. Sein Passwort kommt aus der
Umgebungsvariable `MATRIXPORTAL_OTA_PASSWORD` und wird nicht eingecheckt.

## Stabilität

Der aktuelle Teststand verwendet 3 Bit Farbtiefe. Damit stehen sieben
Helligkeitsstufen von 15 % bis 100 % zur Verfügung. Die automatische Regelung
verteilt diese logarithmisch zwischen den eingestellten Lux-Schwellen. Falls
auf der 128×32-Matrix Bildstörungen auftreten, kann `BIT_DEPTH` in
`src/main.cpp` wieder auf 2 gesetzt werden.

## Datenschutz

`include/secrets.h` ist ignoriert und darf nicht eingecheckt werden. Der
GitHub-Build erzeugt ausschließlich eine Build-Datei aus
`include/secrets.h.example` mit Platzhalterwerten.
