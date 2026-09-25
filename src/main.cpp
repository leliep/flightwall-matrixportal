// Version generated: 2026-09-25 19:19:20 CEST (Europe/Berlin)
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_Protomatter.h>
#include "secrets.h"

#ifndef OTA_PASSWORD
#define OTA_PASSWORD ""
#endif

constexpr uint16_t MATRIX_WIDTH = 128;
constexpr uint8_t BIT_DEPTH = 2;
constexpr uint8_t BUTTON_UP_PIN = 6;
constexpr uint8_t BUTTON_DOWN_PIN = 7;
constexpr uint32_t WIFI_RETRY_MS = 60000;
constexpr uint32_t POLL_INTERVAL_MS = 5000;
constexpr uint32_t DEFAULT_PAGE_INTERVAL_MS = 5000;
constexpr uint32_t STALE_AFTER_MS = 20000;
constexpr uint32_t BUTTON_DEBOUNCE_MS = 150;
constexpr uint32_t BRIGHTNESS_MESSAGE_MS = 1200;
constexpr uint32_t IDLE_BLINK_HALF_PERIOD_MS = 3000;
constexpr uint32_t LIGHT_SENSOR_INTERVAL_MS = 2000;
constexpr uint32_t LIGHT_SENSOR_RETRY_MS = 10000;
constexpr uint8_t LIGHT_SENSOR_FAILURE_LIMIT = 3;

// BH1750 soldered connection: A1/GPIO 3 = SDA, A2/GPIO 9 = SCL.
// STEMMA QT remains an alternative with SDA 16 and SCL 17.
constexpr int8_t LIGHT_SENSOR_SDA_PIN = 3;
constexpr int8_t LIGHT_SENSOR_SCL_PIN = 9;
// ADD open/LOW uses 0x23; ADD connected to 3.3 V uses 0x5C.
// Both addresses are probed automatically, starting with this one.
constexpr uint8_t BH1750_PREFERRED_ADDRESS = 0x23;
constexpr uint8_t BH1750_ALTERNATE_ADDRESS = 0x5C;

// Only aircraft at or inside this distance are shown. Change this value to
// adjust the local display radius without modifying the FlightWall server.
constexpr float DEFAULT_DISPLAY_RADIUS_KM = 10.0f;

constexpr char FLIGHTWALL_HOST[] = "raspi-flw";
constexpr uint16_t FLIGHTWALL_PORT = 8088;
constexpr char FLIGHTWALL_URL[] = "http://raspi-flw:8088/api/state";
// At 2-bit color depth, these are the three useful non-zero PWM levels.
constexpr uint8_t BRIGHTNESS_LEVELS[] = {30, 60, 100};
constexpr uint8_t BRIGHTNESS_LEVEL_COUNT =
    sizeof(BRIGHTNESS_LEVELS) / sizeof(BRIGHTNESS_LEVELS[0]);

uint8_t rgbPins[] = {42, 41, 40, 38, 39, 37};
uint8_t addrPins[] = {45, 36, 48, 35};
constexpr uint8_t clockPin = 2;
constexpr uint8_t latchPin = 47;
constexpr uint8_t oePin = 14;

Adafruit_Protomatter matrix(
    MATRIX_WIDTH, BIT_DEPTH, 1, rgbPins, 4, addrPins,
    clockPin, latchPin, oePin, true);

struct FlightData {
  bool available = false;
  String flight;
  String airline;
  String type;
  String specialType;
  String direction;
  String verticalMotion;
  String motion;
  String origin;
  String destination;
  float distanceKm = NAN;
  float altitudeFt = NAN;
  float speedKmh = NAN;
  float cpaKm = NAN;
  float cpaMinutes = NAN;
  float verticalRateFpm = NAN;
  float relativeBearing = NAN;
};

FlightData flight;
FlightData networkFlight;
uint32_t lastWiFiAttempt = 0;
uint32_t lastPoll = 0;
uint32_t lastSuccessfulPoll = 0;
uint32_t networkLastSuccessfulPoll = 0;
String lastError = "WARTE AUF DATEN";
String networkLastError = "WARTE AUF DATEN";
Preferences preferences;
WebServer webServer(80);
SemaphoreHandle_t flightMutex;
SemaphoreHandle_t preferencesMutex;
TaskHandle_t networkTaskHandle = nullptr;
bool webServerStarted = false;
bool otaStarted = false;
uint32_t otaRebootAt = 0;
uint8_t brightnessIndex = 1;
uint8_t manualBrightnessIndex = 1;
bool automaticBrightnessEnabled = true;
bool lightSensorAvailable = false;
uint8_t lightSensorAddress = 0;
uint8_t lightSensorFailures = 0;
float filteredLux = NAN;
uint32_t lastLightSensorRead = 0;
uint32_t lastLightSensorProbe = 0;
uint32_t lastButtonEvent = 0;
uint32_t brightnessMessageUntil = 0;
String brightnessMessageTitle = "HELLIGKEIT";
String brightnessMessageValue;
volatile uint8_t pendingButtonEvents = 0;
bool displayEnabled = true;
float displayRadiusKm = DEFAULT_DISPLAY_RADIUS_KM;
uint32_t pageIntervalMs = DEFAULT_PAGE_INTERVAL_MS;
float lowLuxThreshold = 20.0f;
float highLuxThreshold = 800.0f;

constexpr uint8_t BUTTON_EVENT_UP = 0x01;
constexpr uint8_t BUTTON_EVENT_DOWN = 0x02;

uint16_t red;
uint16_t blue;
uint16_t white;
uint16_t green;
uint16_t yellow;
uint16_t orange;
uint16_t cyan;
uint16_t violet;
uint16_t idlePixel;

enum class VerticalState { Unknown, Climbing, Level, Descending };

constexpr uint8_t ARROW_UP[7] = {
    0b0001000, 0b0011100, 0b0111110, 0b0001000,
    0b0001000, 0b0001000, 0b0001000};
constexpr uint8_t ARROW_UP_RIGHT[7] = {
    0b0001111, 0b0000111, 0b0001111, 0b0011100,
    0b0111000, 0b1110000, 0b1100000};
constexpr uint8_t ARROW_RIGHT[7] = {
    0b0001000, 0b0001100, 0b1111110, 0b1111111,
    0b1111110, 0b0001100, 0b0001000};
constexpr uint8_t ARROW_DOWN_RIGHT[7] = {
    0b1100000, 0b1110000, 0b0111000, 0b0011100,
    0b0001111, 0b0000111, 0b0001111};
constexpr uint8_t ARROW_DOWN[7] = {
    0b0001000, 0b0001000, 0b0001000, 0b0001000,
    0b0111110, 0b0011100, 0b0001000};
constexpr uint8_t ARROW_DOWN_LEFT[7] = {
    0b0000011, 0b0000111, 0b0001110, 0b0011100,
    0b1111000, 0b1110000, 0b1111000};
constexpr uint8_t ARROW_LEFT[7] = {
    0b0001000, 0b0011000, 0b0111111, 0b1111111,
    0b0111111, 0b0011000, 0b0001000};
constexpr uint8_t ARROW_UP_LEFT[7] = {
    0b1111000, 0b1110000, 0b1111000, 0b0011100,
    0b0001110, 0b0000111, 0b0000011};


uint8_t scaledChannel(uint8_t value) {
  return static_cast<uint16_t>(value) * BRIGHTNESS_LEVELS[brightnessIndex] / 100;
}


void updateColors() {
  red = matrix.color565(scaledChannel(255), 0, 0);
  blue = matrix.color565(0, scaledChannel(255), scaledChannel(255));
  white = matrix.color565(
      scaledChannel(255), scaledChannel(255), scaledChannel(255));
  green = matrix.color565(0, scaledChannel(255), 0);
  yellow = matrix.color565(scaledChannel(255), scaledChannel(220), 0);
  orange = matrix.color565(scaledChannel(255), scaledChannel(170), 0);
  cyan = matrix.color565(0, scaledChannel(255), scaledChannel(255));
  violet = matrix.color565(scaledChannel(180), 0, scaledChannel(255));
  // Faint but still visible at the 2-bit color depth.
  idlePixel = matrix.color565(85, 85, 85);
}


String displayAscii(String text) {
  text.replace("Ä", "AE");
  text.replace("Ö", "OE");
  text.replace("Ü", "UE");
  text.replace("ä", "ae");
  text.replace("ö", "oe");
  text.replace("ü", "ue");
  text.replace("ß", "SS");
  text.replace("é", "e");
  text.replace("è", "e");
  text.replace("ê", "e");
  text.replace("É", "E");
  text.replace("È", "E");
  text.replace("Ê", "E");
  text.replace("á", "a");
  text.replace("à", "a");
  text.replace("Á", "A");
  text.replace("À", "A");
  text.replace("ó", "o");
  text.replace("ò", "o");
  text.replace("Ó", "O");
  text.replace("Ò", "O");
  text.replace("ú", "u");
  text.replace("ù", "u");
  text.replace("Ú", "U");
  text.replace("Ù", "U");
  text.replace("ç", "c");
  text.replace("Ç", "C");

  String ascii;
  ascii.reserve(text.length());
  for (size_t index = 0; index < text.length(); ++index) {
    const uint8_t character = static_cast<uint8_t>(text[index]);
    if (character >= 32 && character <= 126) ascii += static_cast<char>(character);
  }
  return ascii;
}


String clipped(String text, size_t maximumLength) {
  text = displayAscii(text);
  text.trim();
  if (text.length() > maximumLength) {
    text.remove(maximumLength);
  }
  return text;
}


int jsonValueStart(const String &json, const char *key) {
  const String token = "\"" + String(key) + "\"";
  int position = json.indexOf(token);
  if (position < 0) return -1;
  position = json.indexOf(':', position + token.length());
  if (position < 0) return -1;
  do {
    ++position;
  } while (position < static_cast<int>(json.length()) &&
           (json[position] == ' ' || json[position] == '\r' ||
            json[position] == '\n' || json[position] == '\t'));
  return position;
}


String jsonString(const String &json, const char *key) {
  int position = jsonValueStart(json, key);
  if (position < 0 || json[position] != '"') return "";
  String result;
  for (++position; position < static_cast<int>(json.length()); ++position) {
    const char character = json[position];
    if (character == '"') break;
    if (character == '\\' && position + 1 < static_cast<int>(json.length())) {
      const char escaped = json[++position];
      if (escaped == 'n') result += ' ';
      else if (escaped == 't') result += ' ';
      else if (escaped == 'u') {
        position += 4;
        result += '?';
      } else result += escaped;
    } else {
      result += character;
    }
  }
  return result;
}


float jsonNumber(const String &json, const char *key) {
  const int position = jsonValueStart(json, key);
  if (position < 0 || json.startsWith("null", position)) return NAN;
  return json.substring(position).toFloat();
}


bool jsonBoolean(const String &json, const char *key) {
  const int position = jsonValueStart(json, key);
  return position >= 0 && json.startsWith("true", position);
}


String jsonFirstArrayString(const String &json, const char *key) {
  int position = jsonValueStart(json, key);
  if (position < 0 || json[position] != '[') return "";
  const int arrayEnd = json.indexOf(']', position + 1);
  if (arrayEnd < 0) return "";
  position = json.indexOf('"', position + 1);
  if (position < 0 || position > arrayEnd) return "";
  const int end = json.indexOf('"', position + 1);
  if (end < 0 || end > arrayEnd) return "";
  return json.substring(position + 1, end);
}


void drawLine(int16_t x, int16_t y, const String &text, uint16_t color,
              uint8_t size = 1) {
  matrix.setFont(nullptr);
  matrix.setTextSize(size);
  matrix.setTextWrap(false);
  matrix.setTextColor(color);
  matrix.setCursor(x, y);
  matrix.print(text);
}


uint16_t aircraftColor() {
  String category = flight.specialType;
  category.toUpperCase();
  if (category.indexOf("POLIZEI") >= 0) return cyan;
  if (category.indexOf("MILIT") >= 0) return red;
  if (category.indexOf("HUBSCHRAUBER") >= 0) return yellow;
  if (category.indexOf("A380") >= 0) return violet;
  if (category.indexOf("REGIERUNG") >= 0) return blue;
  return red;
}


void drawHeader() {
  String callsign = clipped(flight.flight.length() ? flight.flight : "UNBEKANNT", 8);
  drawLine(0, 0, callsign, aircraftColor(), callsign.length() <= 8 ? 2 : 1);
  drawLine(104, 0, clipped(flight.type, 4), white);
}


void drawArrowBitmap(int16_t x, int16_t y, const uint8_t bitmap[7],
                     uint16_t color) {
  for (uint8_t row = 0; row < 7; ++row) {
    for (uint8_t column = 0; column < 7; ++column) {
      if (bitmap[row] & (1 << (6 - column))) {
        matrix.drawPixel(x + column, y + row, color);
      }
    }
  }
}


VerticalState verticalState() {
  String motion = flight.verticalMotion;
  motion.toUpperCase();
  if (motion.startsWith("STEIG")) return VerticalState::Climbing;
  if (motion.startsWith("SINK")) return VerticalState::Descending;
  if (motion.startsWith("HAELT") || motion.startsWith("HALT")) {
    return VerticalState::Level;
  }
  if (!isnan(flight.verticalRateFpm)) {
    if (flight.verticalRateFpm > 64) return VerticalState::Climbing;
    if (flight.verticalRateFpm < -64) return VerticalState::Descending;
    return VerticalState::Level;
  }
  return VerticalState::Unknown;
}


void drawAltitudeArrow() {
  switch (verticalState()) {
    case VerticalState::Climbing:
      drawArrowBitmap(121, 16, ARROW_UP_RIGHT, green);
      break;
    case VerticalState::Descending:
      drawArrowBitmap(121, 16, ARROW_DOWN_RIGHT, orange);
      break;
    case VerticalState::Level:
      drawArrowBitmap(121, 16, ARROW_RIGHT, white);
      break;
    default:
      break;
  }
}


String compactLookDirection(float angle) {
  if (isnan(angle)) return "richtung ?";
  const float absolute = fabs(angle);
  if (absolute <= 12) return "vorne";
  if (absolute <= 50) return angle > 0 ? "re vorn" : "li vorn";
  if (absolute <= 105) return angle > 0 ? "rechts" : "links";
  if (absolute <= 145) return angle > 0 ? "re hinten" : "li hinten";
  if (absolute <= 170) return angle > 0 ? "re Schulter" : "li Schulter";
  return "hinten";
}


void drawLookArrow(float angle, uint16_t color) {
  if (isnan(angle)) return;
  const float absolute = fabs(angle);
  const uint8_t *bitmap = ARROW_UP;
  if (absolute > 170) bitmap = ARROW_DOWN;
  else if (absolute > 105) bitmap = angle > 0 ? ARROW_DOWN_RIGHT : ARROW_DOWN_LEFT;
  else if (absolute > 50) bitmap = angle > 0 ? ARROW_RIGHT : ARROW_LEFT;
  else if (absolute > 12) bitmap = angle > 0 ? ARROW_UP_RIGHT : ARROW_UP_LEFT;
  drawArrowBitmap(0, 24, bitmap, color);
}


void drawMainPage() {
  drawHeader();

  String metrics;
  if (!isnan(flight.distanceKm)) metrics += String(flight.distanceKm, 1) + "km ";
  metrics += clipped(flight.direction, 2);
  if (!isnan(flight.altitudeFt)) {
    metrics += " " + String(static_cast<int>(round(flight.altitudeFt))) + "ft";
  }
  const String shownMetrics = clipped(metrics, 19);
  drawLine(0, 16, shownMetrics, white);
  drawAltitudeArrow();

  drawLookArrow(flight.relativeBearing, yellow);
  drawLine(10, 24, compactLookDirection(flight.relativeBearing), yellow);
}


void drawRoutePage() {
  drawHeader();
  drawLine(0, 16, clipped(flight.airline, 21), blue);

  const String origin = clipped(flight.origin, 4);
  const String destination = clipped(flight.destination, 4);
  String route;
  if (origin.length() && destination.length()) route = origin + ">" + destination;
  else if (origin.length()) route = origin;
  else if (destination.length()) route = destination;
  if (!isnan(flight.speedKmh)) {
    if (route.length()) route += " ";
    route += String(static_cast<int>(round(flight.speedKmh))) + "km/h";
  }
  drawLine(0, 24, clipped(route, 21), white);
}


void drawCpaPage() {
  drawHeader();
  String cpa = "CPA ";
  cpa += isnan(flight.cpaKm) ? "?" : String(flight.cpaKm, 1) + "km";
  const bool departing = flight.motion == "departing";
  if (departing) {
    cpa += " GEHT";
  } else if (!isnan(flight.cpaMinutes)) {
    cpa += " in " + String(static_cast<int>(round(flight.cpaMinutes))) + "min";
  }
  drawLine(0, 16, clipped(cpa, 21), departing ? orange : green);

  String details = flight.specialType;
  drawLine(0, 24, clipped(details, 21), aircraftColor());
}


void drawStatus(const String &line1, const String &line2) {
  matrix.fillScreen(0);
  drawLine(0, 8, clipped(line1, 21), yellow);
  drawLine(0, 18, clipped(line2, 21), blue);
  matrix.show();
}


bool aircraftIsInsideDisplayRadius() {
  return flight.available && !isnan(flight.distanceKm) &&
         flight.distanceKm <= displayRadiusKm;
}


void drawIdleDisplay() {
  matrix.fillScreen(0);
  if ((millis() / IDLE_BLINK_HALF_PERIOD_MS) % 2 == 0) {
    matrix.drawPixel(0, 31, idlePixel);
  }
  matrix.show();
}


void drawDisplay() {
  if (static_cast<int32_t>(brightnessMessageUntil - millis()) > 0) {
    drawStatus(brightnessMessageTitle, brightnessMessageValue);
    return;
  }
  if (!displayEnabled) {
    matrix.fillScreen(0);
    matrix.show();
    return;
  }
  if (!aircraftIsInsideDisplayRadius()) {
    drawIdleDisplay();
    return;
  }
  if (lastSuccessfulPoll == 0 || millis() - lastSuccessfulPoll > STALE_AFTER_MS) {
    drawStatus("DATEN VERALTET", lastError);
    return;
  }

  matrix.fillScreen(0);
  switch ((millis() / pageIntervalMs) % 3) {
    case 0: drawMainPage(); break;
    case 1: drawRoutePage(); break;
    default: drawCpaPage(); break;
  }
  matrix.show();
}


void applyBrightnessIndex(uint8_t newIndex, bool showMessage) {
  if (newIndex >= BRIGHTNESS_LEVEL_COUNT) return;
  if (newIndex != brightnessIndex) {
    brightnessIndex = newIndex;
    updateColors();
  }
  if (showMessage) {
    brightnessMessageTitle = automaticBrightnessEnabled ? "AUTO LICHT" : "HELLIGKEIT";
    brightnessMessageValue = String(BRIGHTNESS_LEVELS[brightnessIndex]) + "%";
    brightnessMessageUntil = millis() + BRIGHTNESS_MESSAGE_MS;
  }
  Serial.print("Brightness: ");
  Serial.print(BRIGHTNESS_LEVELS[brightnessIndex]);
  Serial.println('%');
}


bool sendBh1750Command(uint8_t address, uint8_t command) {
  Wire.beginTransmission(address);
  Wire.write(command);
  return Wire.endTransmission() == 0;
}


bool beginBh1750At(uint8_t address) {
  Wire.beginTransmission(address);
  if (Wire.endTransmission() != 0) return false;
  return sendBh1750Command(address, 0x01) &&  // Power on
         sendBh1750Command(address, 0x07) &&  // Reset measurement register
         sendBh1750Command(address, 0x10);    // Continuous high resolution
}


bool findLightSensor() {
  lastLightSensorProbe = millis();
  const uint8_t addresses[] = {
      BH1750_PREFERRED_ADDRESS, BH1750_ALTERNATE_ADDRESS};
  for (const uint8_t address : addresses) {
    if (beginBh1750At(address)) {
      lightSensorAddress = address;
      lightSensorAvailable = true;
      lightSensorFailures = 0;
      filteredLux = NAN;
      Serial.printf("BH1750 found at 0x%02X\n", address);
      return true;
    }
  }
  lightSensorAvailable = false;
  lightSensorAddress = 0;
  Serial.println("BH1750 not found; using manual brightness");
  return false;
}


bool readBh1750(float &lux) {
  if (Wire.requestFrom(lightSensorAddress, static_cast<uint8_t>(2)) != 2) {
    return false;
  }
  const uint16_t raw = (static_cast<uint16_t>(Wire.read()) << 8) | Wire.read();
  lux = raw / 1.2f;
  return isfinite(lux) && lux >= 0.0f;
}


uint8_t automaticBrightnessIndex(float lux) {
  const float lowDown = lowLuxThreshold * 0.75f;
  const float lowUp = lowLuxThreshold * 1.25f;
  const float highDown = highLuxThreshold * 0.875f;
  const float highUp = highLuxThreshold * 1.125f;
  if (brightnessIndex == 0) return lux > lowUp ? 1 : 0;
  if (brightnessIndex == 2) return lux < highDown ? 1 : 2;
  if (lux < lowDown) return 0;
  if (lux > highUp) return 2;
  return 1;
}


void serviceLightSensor() {
  const uint32_t now = millis();
  if (!lightSensorAvailable) {
    if (now - lastLightSensorProbe >= LIGHT_SENSOR_RETRY_MS) findLightSensor();
    return;
  }
  if (now - lastLightSensorRead < LIGHT_SENSOR_INTERVAL_MS) return;
  lastLightSensorRead = now;

  float lux;
  if (!readBh1750(lux)) {
    if (++lightSensorFailures >= LIGHT_SENSOR_FAILURE_LIMIT) {
      lightSensorAvailable = false;
      applyBrightnessIndex(manualBrightnessIndex, false);
      Serial.println("BH1750 stopped responding; using manual brightness");
    }
    return;
  }

  lightSensorFailures = 0;
  filteredLux = isnan(filteredLux) ? lux : filteredLux * 0.8f + lux * 0.2f;
  if (automaticBrightnessEnabled) {
    applyBrightnessIndex(automaticBrightnessIndex(filteredLux), false);
  }
}


void IRAM_ATTR onButtonUpPressed() {
  pendingButtonEvents |= BUTTON_EVENT_UP;
}


void IRAM_ATTR onButtonDownPressed() {
  pendingButtonEvents |= BUTTON_EVENT_DOWN;
}


void handleBrightnessButtons() {
  noInterrupts();
  const uint8_t events = pendingButtonEvents;
  pendingButtonEvents = 0;
  interrupts();

  if (events == 0) return;
  const uint32_t now = millis();

  if (digitalRead(BUTTON_UP_PIN) == LOW &&
      digitalRead(BUTTON_DOWN_PIN) == LOW) {
    lastButtonEvent = now;
    automaticBrightnessEnabled = true;
    if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      preferences.putBool("autoLight", true);
      xSemaphoreGive(preferencesMutex);
    }
    if (lightSensorAvailable && !isnan(filteredLux)) {
      applyBrightnessIndex(automaticBrightnessIndex(filteredLux), false);
      brightnessMessageTitle = "AUTO LICHT";
      brightnessMessageValue = "AKTIV";
    } else {
      applyBrightnessIndex(manualBrightnessIndex, false);
      brightnessMessageTitle = "MANUELL";
      brightnessMessageValue = "SENSOR FEHLT";
    }
    brightnessMessageUntil = now + BRIGHTNESS_MESSAGE_MS;
    return;
  }
  if (now - lastButtonEvent < BUTTON_DEBOUNCE_MS) return;

  lastButtonEvent = now;
  if (automaticBrightnessEnabled) {
    automaticBrightnessEnabled = false;
    manualBrightnessIndex = brightnessIndex;
    if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      preferences.putBool("autoLight", false);
      xSemaphoreGive(preferencesMutex);
    }
  }
  if ((events & BUTTON_EVENT_UP) &&
      manualBrightnessIndex + 1 < BRIGHTNESS_LEVEL_COUNT) {
    ++manualBrightnessIndex;
  } else if ((events & BUTTON_EVENT_DOWN) && manualBrightnessIndex > 0) {
    --manualBrightnessIndex;
  }
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    preferences.putUChar("brightness", manualBrightnessIndex);
    xSemaphoreGive(preferencesMutex);
  }
  applyBrightnessIndex(manualBrightnessIndex, true);
}


void setNetworkError(const String &message) {
  if (xSemaphoreTake(flightMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    networkLastError = message;
    xSemaphoreGive(flightMutex);
  }
}


void syncNetworkState() {
  if (xSemaphoreTake(flightMutex, 0) == pdTRUE) {
    flight = networkFlight;
    lastSuccessfulPoll = networkLastSuccessfulPoll;
    lastError = networkLastError;
    xSemaphoreGive(flightMutex);
  }
}


bool connectWiFi(uint32_t timeoutMs) {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < timeoutMs) {
    delay(250);
  }
  if (WiFi.status() != WL_CONNECTED) {
    setNetworkError("WLAN FEHLER");
    return false;
  }
  Serial.print("Wi-Fi connected: ");
  Serial.println(WiFi.localIP());
  return true;
}


String httpFailureText(int status) {
  if (WiFi.status() != WL_CONNECTED) return "WLAN GETRENNT";
  switch (status) {
    case -1: return "TCP ABGELEHNT";
    case -2: return "HEADER FEHLER";
    case -3: return "SENDEN FEHLER";
    case -4: return "NICHT VERBUNDEN";
    case -5: return "VERBINDUNG VERLOREN";
    case -6: return "KEIN DATENSTROM";
    case -7: return "KEIN HTTP SERVER";
    case -8: return "SPEICHER FEHLER";
    case -9: return "ENCODING FEHLER";
    case -10: return "SCHREIBFEHLER";
    case -11: return "LESE TIMEOUT";
    case -12: return "TCP TIMEOUT";
    default: return "HTTP FEHLER " + String(status);
  }
}


bool fetchFlightWall() {
  if (WiFi.status() != WL_CONNECTED) {
    setNetworkError("WLAN GETRENNT");
    Serial.println("FlightWall: Wi-Fi is disconnected");
    return false;
  }

  IPAddress serverAddress;
  if (WiFi.hostByName(FLIGHTWALL_HOST, serverAddress) != 1) {
    setNetworkError("DNS FEHLER");
    Serial.print("FlightWall: DNS lookup failed for ");
    Serial.println(FLIGHTWALL_HOST);
    return false;
  }
  Serial.print("FlightWall: ");
  Serial.print(FLIGHTWALL_HOST);
  Serial.print(" resolved to ");
  Serial.println(serverAddress);

  WiFiClient client;
  HTTPClient http;
  http.useHTTP10(true);
  http.setConnectTimeout(3000);
  http.setTimeout(4000);

  if (!http.begin(client, FLIGHTWALL_URL)) {
    setNetworkError("URL FEHLER");
    return false;
  }

  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    if (status < 0) {
      setNetworkError(httpFailureText(status));
      Serial.print("FlightWall transport error ");
      Serial.print(status);
      Serial.print(": ");
      Serial.println(HTTPClient::errorToString(status));
    } else {
      setNetworkError("HTTP " + String(status));
      Serial.print("FlightWall HTTP status: ");
      Serial.println(status);
    }
    http.end();
    return false;
  }

  const String json = http.getString();
  if (json.indexOf("\"available\"") < 0) {
    setNetworkError("JSON FEHLER");
    Serial.println("FlightWall: response does not contain 'available'");
    http.end();
    return false;
  }
  FlightData next;
  next.available = jsonBoolean(json, "available");
  next.flight = jsonString(json, "flight");
  next.airline = jsonString(json, "airline");
  next.type = jsonString(json, "type");
  next.direction = jsonString(json, "direction");
  next.verticalMotion = jsonString(json, "vertical_motion");
  next.motion = jsonString(json, "motion");
  next.origin = jsonString(json, "origin");
  next.destination = jsonString(json, "destination");
  next.distanceKm = jsonNumber(json, "distance_km");
  next.altitudeFt = jsonNumber(json, "altitude_ft");
  next.speedKmh = jsonNumber(json, "speed_kmh");
  next.cpaKm = jsonNumber(json, "cpa_km");
  next.cpaMinutes = jsonNumber(json, "cpa_minutes");
  next.verticalRateFpm = jsonNumber(json, "vertical_rate_fpm");
  next.relativeBearing = jsonNumber(json, "relative_bearing");
  next.specialType = jsonFirstArrayString(json, "special_types");

  if (xSemaphoreTake(flightMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    networkFlight = next;
    networkLastSuccessfulPoll = millis();
    networkLastError = "";
    xSemaphoreGive(flightMutex);
  }
  http.end();
  return true;
}


String checked(bool value) {
  return value ? " checked" : "";
}


String webPage() {
  FlightData statusFlight;
  String statusError;
  uint32_t statusUpdated = 0;
  if (xSemaphoreTake(flightMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    statusFlight = networkFlight;
    statusError = networkLastError;
    statusUpdated = networkLastSuccessfulPoll;
    xSemaphoreGive(flightMutex);
  }

  String html;
  html.reserve(6000);
  html += F("<!doctype html><html lang='de'><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>FlightWall MatrixPortal</title><style>");
  html += F("body{font:16px system-ui;background:#10131a;color:#eef2ff;margin:auto;max-width:720px;padding:20px}");
  html += F("h1{font-size:1.5rem}fieldset{border:1px solid #445;border-radius:10px;margin:14px 0;padding:14px}");
  html += F("label{display:block;margin:12px 0}input[type=number],select{width:100%;box-sizing:border-box;padding:9px}");
  html += F("button{background:#00bcd4;color:#001018;border:0;border-radius:8px;padding:12px 18px;font-weight:bold}");
  html += F(".status{display:grid;grid-template-columns:auto 1fr;gap:6px 14px}.muted{color:#aab}</style></head><body>");
  html += F("<h1>FlightWall MatrixPortal 2.0</h1><form method='post' action='/save'>");
  html += F("<fieldset><legend>Display</legend><label><input type='checkbox' name='display' value='1'");
  html += checked(displayEnabled);
  html += F("> Display eingeschaltet</label><label>Anzeigeradius (km)<input type='number' name='radius' min='1' max='100' step='0.5' value='");
  html += String(displayRadiusKm, 1);
  html += F("'></label><label>Seitenwechsel (Sekunden)<input type='number' name='pageSeconds' min='2' max='60' value='");
  html += String(pageIntervalMs / 1000);
  html += F("'></label></fieldset><fieldset><legend>Helligkeit</legend>");
  html += F("<label><input type='checkbox' name='autoLight' value='1'");
  html += checked(automaticBrightnessEnabled);
  html += F("> BH1750-Automatik</label><label>Manuelle Stufe<select name='brightness'>");
  for (uint8_t i = 0; i < BRIGHTNESS_LEVEL_COUNT; ++i) {
    html += "<option value='" + String(i) + "'";
    if (i == manualBrightnessIndex) html += " selected";
    html += ">" + String(BRIGHTNESS_LEVELS[i]) + "%</option>";
  }
  html += F("</select></label><label>Schwelle dunkel (Lux)<input type='number' name='lowLux' min='1' max='500' step='1' value='");
  html += String(lowLuxThreshold, 0);
  html += F("'></label><label>Schwelle hell (Lux)<input type='number' name='highLux' min='50' max='50000' step='10' value='");
  html += String(highLuxThreshold, 0);
  html += F("'></label></fieldset><button type='submit'>Einstellungen speichern</button></form>");
  html += F("<fieldset><legend>Status</legend><div class='status'><span>Adresse</span><b>http://matrixportal.local</b><span>IP</span><b>");
  html += WiFi.localIP().toString();
  html += F("</b><span>WLAN</span><b>");
  html += String(WiFi.RSSI()) + " dBm";
  html += F("</b><span>BH1750</span><b>");
  html += lightSensorAvailable ? "verbunden" : "nicht gefunden";
  html += F("</b><span>Lux</span><b>");
  html += isnan(filteredLux) ? "-" : String(filteredLux, 1);
  html += F("</b><span>Flug</span><b>");
  html += statusFlight.available ? displayAscii(statusFlight.flight) : "keiner";
  html += F("</b><span>FlightWall</span><b>");
  html += statusError.length() ? displayAscii(statusError) : "OK";
  html += F("</b><span>Letzter Abruf</span><b>");
  html += statusUpdated ? String((millis() - statusUpdated) / 1000) + " s" : "-";
  html += F("</b></div></fieldset>");
  if (strlen(OTA_PASSWORD) >= 8) {
    html += F("<p><a href='/update'>Firmware drahtlos aktualisieren</a></p>");
  }
  html += F("<p class='muted'>UP/DOWN: manuell; beide Tasten: Automatik.</p></body></html>");
  return html;
}


void handleWebSave() {
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
    displayEnabled = webServer.hasArg("display");
    automaticBrightnessEnabled = webServer.hasArg("autoLight");
    displayRadiusKm = constrain(webServer.arg("radius").toFloat(), 1.0f, 100.0f);
    pageIntervalMs = constrain(webServer.arg("pageSeconds").toInt(), 2L, 60L) * 1000UL;
    lowLuxThreshold = constrain(webServer.arg("lowLux").toFloat(), 1.0f, 500.0f);
    highLuxThreshold = constrain(webServer.arg("highLux").toFloat(), 50.0f, 50000.0f);
    if (highLuxThreshold <= lowLuxThreshold) highLuxThreshold = lowLuxThreshold + 50.0f;
    const int requestedBrightness = webServer.arg("brightness").toInt();
    if (requestedBrightness >= 0 && requestedBrightness < BRIGHTNESS_LEVEL_COUNT) {
      manualBrightnessIndex = requestedBrightness;
    }
    preferences.putBool("displayOn", displayEnabled);
    preferences.putBool("autoLight", automaticBrightnessEnabled);
    preferences.putUChar("brightness", manualBrightnessIndex);
    preferences.putFloat("radiusKm", displayRadiusKm);
    preferences.putUInt("pageMs", pageIntervalMs);
    preferences.putFloat("lowLux", lowLuxThreshold);
    preferences.putFloat("highLux", highLuxThreshold);
    xSemaphoreGive(preferencesMutex);
  }
  webServer.sendHeader("Location", "/");
  webServer.send(303, "text/plain", "Gespeichert");
}


bool otaAuthenticated() {
  if (strlen(OTA_PASSWORD) < 8) {
    webServer.send(503, "text/plain; charset=utf-8", "OTA ist nicht konfiguriert.");
    return false;
  }
  if (!webServer.authenticate("admin", OTA_PASSWORD)) {
    webServer.requestAuthentication();
    return false;
  }
  return true;
}


void handleOtaPage() {
  if (!otaAuthenticated()) return;
  webServer.send(200, "text/html; charset=utf-8",
      "<!doctype html><html lang='de'><meta name='viewport' content='width=device-width'>"
      "<title>Firmware-Update</title><body style='font:16px system-ui;max-width:600px;margin:40px auto'>"
      "<h1>Firmware-Update</h1><p>PlatformIO-Datei <b>firmware.bin</b> auswählen.</p>"
      "<form method='post' action='/update' enctype='multipart/form-data'>"
      "<input type='file' name='firmware' accept='.bin' required> "
      "<button type='submit'>Installieren</button></form>"
      "<p>Während des Updates die Stromversorgung nicht trennen.</p></body></html>");
}


void handleOtaUpload() {
  if (strlen(OTA_PASSWORD) < 8 ||
      !webServer.authenticate("admin", OTA_PASSWORD)) return;
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Web OTA start: %s\n", upload.filename.c_str());
    Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!Update.hasError()) Update.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (!Update.hasError() && Update.end(true)) {
      Serial.printf("Web OTA complete: %u bytes\n", upload.totalSize);
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    Serial.println("Web OTA aborted");
  }
}


void finishOtaUpload() {
  if (!otaAuthenticated()) return;
  const bool success = !Update.hasError() && Update.isFinished();
  webServer.send(success ? 200 : 500, "text/plain; charset=utf-8",
                 success ? "Update erfolgreich. Neustart..." : "Update fehlgeschlagen.");
  if (success) otaRebootAt = millis() + 1000;
}


void startArduinoOta() {
  if (otaStarted || strlen(OTA_PASSWORD) < 8) return;
  ArduinoOTA.setHostname("matrixportal");
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    Serial.println("PlatformIO OTA start");
    setNetworkError("OTA UPDATE");
  });
  ArduinoOTA.onEnd([]() { Serial.println("PlatformIO OTA complete"); });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("PlatformIO OTA error: %u\n", error);
    setNetworkError("OTA FEHLER");
  });
  ArduinoOTA.begin();
  otaStarted = true;
  Serial.println("PlatformIO OTA enabled for matrixportal.local");
}


void startWebServer() {
  if (webServerStarted) return;
  webServer.on("/", HTTP_GET, []() { webServer.send(200, "text/html; charset=utf-8", webPage()); });
  webServer.on("/save", HTTP_POST, handleWebSave);
  webServer.on("/update", HTTP_GET, handleOtaPage);
  webServer.on("/update", HTTP_POST, finishOtaUpload, handleOtaUpload);
  webServer.onNotFound([]() { webServer.send(404, "text/plain; charset=utf-8", "Nicht gefunden"); });
  webServer.begin();
  if (MDNS.begin("matrixportal")) MDNS.addService("http", "tcp", 80);
  webServerStarted = true;
  startArduinoOta();
  Serial.println("Web interface: http://matrixportal.local");
}


void networkTask(void *) {
  lastWiFiAttempt = millis();
  if (connectWiFi(20000)) {
    startWebServer();
    fetchFlightWall();
    lastPoll = millis();
  }
  for (;;) {
    const uint32_t now = millis();
    if (WiFi.status() == WL_CONNECTED) {
      webServer.handleClient();
      if (otaStarted) ArduinoOTA.handle();
    }
    if (otaRebootAt && static_cast<int32_t>(now - otaRebootAt) >= 0) ESP.restart();
    if (WiFi.status() != WL_CONNECTED && now - lastWiFiAttempt >= WIFI_RETRY_MS) {
      lastWiFiAttempt = now;
      if (connectWiFi(10000)) startWebServer();
    }
    if (WiFi.status() == WL_CONNECTED && now - lastPoll >= POLL_INTERVAL_MS) {
      lastPoll = now;
      fetchFlightWall();
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}


void setup() {
  Serial.begin(115200);
  delay(250);
  flightMutex = xSemaphoreCreateMutex();
  preferencesMutex = xSemaphoreCreateMutex();
  if (flightMutex == nullptr || preferencesMutex == nullptr) {
    Serial.println("Cannot create FreeRTOS mutexes");
    while (true) delay(1000);
  }
  const ProtomatterStatus status = matrix.begin();
  if (status != PROTOMATTER_OK) {
    Serial.printf("Protomatter error: %d\n", static_cast<int>(status));
    while (true) delay(1000);
  }

  pinMode(BUTTON_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_DOWN_PIN, INPUT_PULLUP);
  attachInterrupt(
      digitalPinToInterrupt(BUTTON_UP_PIN), onButtonUpPressed, FALLING);
  attachInterrupt(
      digitalPinToInterrupt(BUTTON_DOWN_PIN), onButtonDownPressed, FALLING);

  preferences.begin("flightwall", false);
  manualBrightnessIndex = preferences.getUChar("brightness", 1);
  if (manualBrightnessIndex >= BRIGHTNESS_LEVEL_COUNT) manualBrightnessIndex = 1;
  brightnessIndex = manualBrightnessIndex;
  automaticBrightnessEnabled = preferences.getBool("autoLight", true);
  displayEnabled = preferences.getBool("displayOn", true);
  displayRadiusKm = preferences.getFloat("radiusKm", DEFAULT_DISPLAY_RADIUS_KM);
  pageIntervalMs = preferences.getUInt("pageMs", DEFAULT_PAGE_INTERVAL_MS);
  lowLuxThreshold = preferences.getFloat("lowLux", 20.0f);
  highLuxThreshold = preferences.getFloat("highLux", 800.0f);
  displayRadiusKm = constrain(displayRadiusKm, 1.0f, 100.0f);
  pageIntervalMs = constrain(pageIntervalMs, 2000UL, 60000UL);
  lowLuxThreshold = constrain(lowLuxThreshold, 1.0f, 500.0f);
  highLuxThreshold = constrain(highLuxThreshold, 50.0f, 50000.0f);
  if (highLuxThreshold <= lowLuxThreshold) highLuxThreshold = lowLuxThreshold + 50.0f;
  updateColors();

  Wire.begin(LIGHT_SENSOR_SDA_PIN, LIGHT_SENSOR_SCL_PIN);
  Wire.setTimeOut(50);
  findLightSensor();

  drawStatus("FLIGHTWALL", "VERBINDET...");
  if (xTaskCreatePinnedToCore(
          networkTask, "network", 12288, nullptr, 1,
          &networkTaskHandle, 0) != pdPASS) {
    drawStatus("NETZWERK", "TASK FEHLER");
    while (true) delay(1000);
  }
}


void loop() {
  const uint32_t now = millis();
  syncNetworkState();
  handleBrightnessButtons();
  serviceLightSensor();
  if (!automaticBrightnessEnabled && brightnessIndex != manualBrightnessIndex) {
    applyBrightnessIndex(manualBrightnessIndex, false);
  }

  static uint32_t lastDraw = 0;
  if (now - lastDraw >= 250) {
    lastDraw = now;
    drawDisplay();
  }
  delay(10);
}
