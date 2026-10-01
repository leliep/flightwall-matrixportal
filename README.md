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
Lux-Schwellen, lokaler Anzeigeradius und eine tägliche Nachtruhe. Der Zeitraum
darf über Mitternacht reichen, beispielsweise von 23:55 bis 05:55. Während
dieser Zeit bleibt die Matrix einschließlich Betriebs-Pixel vollständig dunkel;
Netzwerk, Datenabruf, Webinterface und OTA bleiben aktiv. Die lokale Uhrzeit
wird per NTP bezogen und automatisch zwischen MEZ und MESZ umgestellt.
Wenn kein Flugzeug angezeigt wird, blinkt der Betriebs-Pixel unten links bei
erfolgreicher FlightWall-Verbindung weiß. Nach einem fehlgeschlagenen
Serverabruf blinkt er rot, bis wieder gültige Daten empfangen wurden.
Für die Blickrichtung werden außerdem die Standorthöhe über NHN und die
Entfernungsgrenze für `weit` gespeichert.
Host, Port und API-Pfad des
FlightWall-Servers lassen sich ändern und direkt testen. Die sieben
Anzeigeseiten können einzeln aktiviert, frei sortiert und mit einer eigenen
Dauer versehen werden. Sanfte Übergänge sind optional. Das Webinterface ist
für ein vertrauenswürdiges lokales WLAN gedacht und hat derzeit keine eigene
Anmeldung.

Der Displaytest im Webinterface prüft die gesamte Matrix für jeweils kurze
Zeit mit Rot, Grün, Blau, Weiß, einem Pixelraster, geometrischen Linien, Text
und Farbbalken. Anschließend zeigt er alle sieben 14×7-Typensymbole jeweils
zusammen mit ihrer Bedeutung. Nach etwa 23 Sekunden endet er automatisch;
über denselben Schalter kann er vorher beendet werden.

Die Seite **Flughafennamen** zeigt – sofern vom FlightWall-Server geliefert –
die Langnamen des Start- und Zielflughafens. Namen, die breiter als das Display
sind, werden vollständig horizontal gescrollt; dafür verlängert die Firmware
die konfigurierte Seitendauer bei Bedarf automatisch. Fehlt ein Langname, wird
der Flughafencode angezeigt. Sind Start und Ziel beide unbekannt, wird die
gesamte Seite automatisch übersprungen.

Die Seite **Freitext** zeigt einen im Webinterface eingegebenen Text mit bis
zu 128 darstellbaren Zeichen. Bei jedem Seitenaufruf läuft der Text genau
einmal vollständig von rechts nach links über die Anzeige. Die Firmware
verlängert die konfigurierte Seitendauer automatisch, wenn der Durchlauf mehr
Zeit benötigt. Ein leerer Freitext überspringt die Seite.

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
`900kmh`, bei weniger Platz als `900k` und wird andernfalls weggelassen.
Fehlende Routenteile erzeugen kein einzelnes Trennzeichen. Alle Werte werden
mit den laufenden FlightWall-Abrufen aktualisiert. Für Hubschrauber, A380,
Militär, Polizei, Regierung und Luftschiff (`SHIP`) verwendet sie eigene
14×7-Symbole; unbekannte Special Types erhalten ein allgemeines Symbol.

Die Seite **Kompakt + Flughäfen** zeigt in der ersten Zeile statt der Flugnummer
die Fluggesellschaft in Rot. Ist der verfügbare Platz vor dem unverändert
rechts stehenden Typensymbol und Flugzeugtyp zu kurz, scrollt ihr vollständiger
Name innerhalb dieses Bereichs. In der unteren Zeile erscheinen Start- und
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
Texte an den Grenzen. Die FlightWall-API bleibt unverändert.

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
