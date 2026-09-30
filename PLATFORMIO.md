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
IP-Adresse erreichbar. Einstellungen werden in NVS gespeichert. Dazu gehören
auch FlightWall-Host, Port und Pfad sowie Aktivierung, Reihenfolge und Dauer
der sieben Anzeigeseiten. Die Seite **Flughafennamen** zeigt Start-
und Zielflughafen ausgeschrieben und scrollt zu lange Namen horizontal.
Die Seite **Freitext** scrollt einen frei konfigurierbaren Text mit bis zu
128 Zeichen bei jedem Aufruf genau einmal vollständig über das Display.
Die **Kompaktansicht** fasst Flugnummer, Flugzeugtyp, Höhe, Höhenentwicklung,
Entfernung, Blickrichtung, Route und CPA auf einer Seite zusammen.
**Kompakt + Flughäfen** zeigt in der unteren Zeile sofort die getrennt
angeordneten Langnamen und keine Kurzcodes. Je nach Länge werden beide Namen
gemeinsam außenbündig dargestellt, nur der lange Name gescrollt oder beide
Namen in getrennten Hälften gescrollt.
Den Zenitwinkel berechnet der ESP aus `altitude_ft`, `distance_km` und der im
Webinterface hinterlegten Standorthöhe. Die FlightWall-Schnittstelle benötigt
dafür keine zusätzlichen Felder.

## OTA

OTA ist nur aktiv, wenn `OTA_PASSWORD` in `include/secrets.h` mindestens acht
Zeichen lang ist. Das Webinterface bietet dann einen passwortgeschützten
Upload für `firmware.bin`. Für PlatformIO steht zusätzlich das Environment
`adafruit_matrixportal_esp32s3_ota` bereit. Sein Passwort kommt aus der
Umgebungsvariable `MATRIXPORTAL_OTA_PASSWORD` und wird nicht eingecheckt.
Der Web-Upload besitzt Firmwareprüfung, Bestätigungsdialog, Fortschrittsanzeige
und einen Versionsvergleich nach dem automatischen Neustart.

Für den kombinierten Ablauf steht in VS Code die Aufgabe
**FlightWall: Git-Update + OTA** bereit. Sie führt zuerst
`git pull --ff-only origin main` aus. Nur nach einem erfolgreichen Fast-forward
wird die OTA-Umgebung gebaut und hochgeladen. Das OTA-Passwort wird bei jedem
Start verdeckt abgefragt und weder in einer Datei noch in Git gespeichert.

Start über **Terminal → Aufgabe ausführen… → FlightWall: Git-Update + OTA**.

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
