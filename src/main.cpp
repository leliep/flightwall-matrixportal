// Version generated: 2026-10-03 12:17:49 CEST (Europe/Berlin)
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_system.h>
#include <time.h>

#include <Adafruit_GFX.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Protomatter.h>
#include <PubSubClient.h>
#include "AdsbClient.hpp"
#include "AircraftProcessor.hpp"
#include "AirlineNames.hpp"
#include "Geo.hpp"
#include "RouteClient.hpp"
#include "UnknownCodes.hpp"
#include "WebDisplay.hpp"
#include "secrets.h"

#ifndef OTA_PASSWORD
#define OTA_PASSWORD ""
#endif
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif

constexpr uint16_t MATRIX_WIDTH = 128;
constexpr uint8_t BIT_DEPTH = 3;
constexpr char APP_VERSION[] = "3.4.0-3bit-exp.4";
constexpr char BUILD_TIMESTAMP[] = "2026-10-19 3-bit experiment";
constexpr uint8_t PAGE_COUNT = 7;
constexpr uint8_t BUTTON_UP_PIN = 6;
constexpr uint8_t BUTTON_DOWN_PIN = 7;
constexpr uint32_t WIFI_RETRY_MS = 60000;
constexpr uint32_t POLL_INTERVAL_MS = 5000;
constexpr uint32_t DEFAULT_PAGE_INTERVAL_MS = 5000;
constexpr uint32_t PAGE_TRANSITION_MS = 350;
constexpr uint32_t STALE_AFTER_MS = 20000;
constexpr uint32_t EMERGENCY_PAGE_DURATION_MS = 5000;
constexpr uint32_t EMERGENCY_TEST_DURATION_MS = 120000;
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
constexpr int8_t STEMMA_SDA_PIN = 16;
constexpr int8_t STEMMA_SCL_PIN = 17;
constexpr uint8_t BME280_PREFERRED_ADDRESS = 0x76;
constexpr uint8_t BME280_ALTERNATE_ADDRESS = 0x77;
constexpr uint32_t ENVIRONMENT_SENSOR_INTERVAL_MS = 5000;
constexpr uint32_t ENVIRONMENT_SENSOR_RETRY_MS = 15000;
constexpr uint8_t ENVIRONMENT_SENSOR_FAILURE_LIMIT = 3;
constexpr uint32_t MQTT_RETRY_MS = 15000;
constexpr uint32_t MQTT_VALUE_STALE_MS = 15UL * 60UL * 1000UL;
constexpr char DEFAULT_MQTT_HOST[] = "srv-mqtt.fritz.box";
constexpr uint16_t DEFAULT_MQTT_PORT = 1883;
constexpr char DEFAULT_MQTT_USER[] = "peter";
constexpr char DEFAULT_WIND_DIRECTION_TOPIC[] = "zons/weather/windDir";
constexpr char DEFAULT_WIND_SPEED_TOPIC[] = "zons/weather/windSpeed_kph";
constexpr char DEFAULT_WIND_GUST_TOPIC[] = "zons/weather/windGust_kph";
constexpr char DEFAULT_RAIN_TOPIC[] = "zons/weather/rain24_cm";

// Only aircraft at or inside this distance are shown. Change this value to
// adjust the local display radius without modifying the FlightWall server.
constexpr float DEFAULT_DISPLAY_RADIUS_KM = 10.0f;
// Viewing-direction geometry. The zenith angle is measured from straight up.
constexpr float DIRECTLY_ABOVE_ZENITH_DEG = 10.0f;
constexpr float ABOVE_ZENITH_DEG = 30.0f;
constexpr float ZENITH_HYSTERESIS_DEG = 2.0f;
constexpr float FAR_DISTANCE_HYSTERESIS_KM = 0.3f;

constexpr char DEFAULT_ADSB_URL[] =
    "http://t-eddl146.fritz.box:8080/data/aircraft.json";
constexpr char DEFAULT_ROUTE_URL[] = "https://adsb.im/api/0/routeset";
constexpr double DEFAULT_SITE_LATITUDE = 51.122500;
constexpr double DEFAULT_SITE_LONGITUDE = 6.839444;
constexpr float DEFAULT_VIEW_HEADING = 322.0f;
constexpr int DEFAULT_MAXIMUM_ALTITUDE_FT = 15000;
constexpr char LOCAL_TIME_ZONE[] = "CET-1CEST,M3.5.0,M10.5.0/3";
constexpr char NTP_SERVER_PRIMARY[] = "pool.ntp.org";
constexpr char NTP_SERVER_SECONDARY[] = "time.cloudflare.com";
// Seven native non-zero PWM levels are available at 3-bit color depth.
// Keeping the values aligned to those native steps avoids temporal dithering.
constexpr uint8_t BRIGHTNESS_LEVELS[] = {14, 29, 43, 57, 71, 86, 100};
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
constexpr uint16_t AIRPORT_TEXT_CANVAS_WIDTH = 128 * 6;
GFXcanvas1 airportTextCanvas(AIRPORT_TEXT_CANVAS_WIDTH, 8);
TwoWire environmentWire(1);
Adafruit_BME280 environmentSensor;

struct FlightData {
  bool available = false;
  bool emergency = false;
  bool hasPosition = false;
  String hex;
  String flight;
  String squawk;
  String airline;
  String type;
  String specialType;
  String direction;
  String verticalMotion;
  String motion;
  String origin;
  String destination;
  String routeCodes;
  String routeNames;
  String originName;
  String destinationName;
  float distanceKm = NAN;
  float altitudeFt = NAN;
  float speedKmh = NAN;
  float cpaKm = NAN;
  float cpaMinutes = NAN;
  float verticalRateFpm = NAN;
  float bearing = NAN;
  float relativeBearing = NAN;
  uint8_t emergencyCount = 0;
  uint8_t emergencyIndex = 0;
};

FlightData flight;
FlightData networkFlight;
uint32_t lastWiFiAttempt = 0;
uint32_t lastPoll = 0;
uint32_t lastSuccessfulPoll = 0;
uint32_t networkLastSuccessfulPoll = 0;
uint32_t lastAdsbAttempt = 0;
uint32_t lastAdsbSuccess = 0;
uint32_t lastFeedTimestamp = 0;
String lastError = "WARTE AUF DATEN";
String networkLastError = "WARTE AUF DATEN";
String lastAdsbError = "WARTE AUF DATEN";
String lastRouteError;
Preferences preferences;
WebServer webServer(80);
WiFiClient mqttTransport;
PubSubClient mqttClient(mqttTransport);
SemaphoreHandle_t flightMutex;
SemaphoreHandle_t preferencesMutex;
SemaphoreHandle_t weatherMutex;
TaskHandle_t networkTaskHandle = nullptr;
TaskHandle_t displayTaskHandle = nullptr;
bool webServerStarted = false;
bool webServerConfigured = false;
bool otaStarted = false;
bool wifiWasConnected = false;
uint32_t wifiReconnectCount = 0;
uint32_t webServerStartCount = 0;
uint32_t lastWifiDisconnect = 0;
uint32_t otaRebootAt = 0;
uint32_t manualRebootAt = 0;
volatile bool otaInProgress = false;
volatile bool otaPreparing = false;
volatile bool routeCacheWritePreparing = false;
volatile bool routeCacheWriteInProgress = false;
volatile bool settingsWritePreparing = false;
volatile bool settingsWriteInProgress = false;
volatile bool buttonSettingsWritePending = false;
volatile uint8_t displayTestCommand = 0;
volatile bool displayTestActive = false;
uint32_t displayTestStarted = 0;
bool otaUploadValid = false;
size_t otaBytesWritten = 0;
String otaErrorMessage;
uint8_t brightnessIndex = 1;
uint8_t manualBrightnessIndex = 1;
bool automaticBrightnessEnabled = true;
bool lightSensorAvailable = false;
uint8_t lightSensorAddress = 0;
uint8_t lightSensorFailures = 0;
float filteredLux = NAN;
uint32_t lastLightSensorRead = 0;
uint32_t lastLightSensorProbe = 0;
bool environmentSensorAvailable = false;
bool environmentSensorOnStemma = false;
uint8_t environmentSensorAddress = 0;
uint8_t environmentSensorFailures = 0;
float ambientTemperatureC = NAN;
float relativeHumidityPercent = NAN;
float stationPressureHpa = NAN;
float airPressureHpa = NAN;
uint32_t lastEnvironmentSensorRead = 0;
uint32_t lastEnvironmentSensorProbe = 0;
uint32_t lastButtonEvent = 0;
uint32_t brightnessMessageUntil = 0;
String brightnessMessageTitle = "HELLIGKEIT";
String brightnessMessageValue;
volatile uint8_t pendingButtonEvents = 0;
bool displayEnabled = true;
bool uiEnglish = false;
bool environmentIdleDisplayEnabled = true;
bool mqttWeatherEnabled = true;
String mqttHost = DEFAULT_MQTT_HOST;
uint16_t mqttPort = DEFAULT_MQTT_PORT;
String mqttUser = DEFAULT_MQTT_USER;
String mqttPassword = MQTT_PASSWORD;
String windDirectionTopic = DEFAULT_WIND_DIRECTION_TOPIC;
String windSpeedTopic = DEFAULT_WIND_SPEED_TOPIC;
String windGustTopic = DEFAULT_WIND_GUST_TOPIC;
String rainTopic = DEFAULT_RAIN_TOPIC;
bool mqttConnected = false;
uint32_t lastMqttAttempt = 0;
uint32_t lastMqttMessage = 0;
uint32_t mqttReconnectCount = 0;
String mqttLastError = "NOCH NICHT VERBUNDEN";
String windDirection;
float windSpeedKph = NAN;
float windGustKph = NAN;
float rain24Mm = NAN;
uint32_t windDirectionUpdated = 0;
uint32_t windSpeedUpdated = 0;
uint32_t windGustUpdated = 0;
uint32_t rainUpdated = 0;
bool quietHoursEnabled = false;
uint16_t quietHoursStartMinutes = 23 * 60 + 55;
uint16_t quietHoursEndMinutes = 5 * 60 + 55;
float displayRadiusKm = DEFAULT_DISPLAY_RADIUS_KM;
float observerElevationM = 0.0f;
float farDirectionDistanceKm = 5.0f;
uint32_t pageIntervalMs = DEFAULT_PAGE_INTERVAL_MS;
float lowLuxThreshold = 20.0f;
float highLuxThreshold = 800.0f;
String adsbUrl1 = DEFAULT_ADSB_URL;
String adsbUrl2;
uint8_t activeAdsbSource = 0;
String routeUrl = DEFAULT_ROUTE_URL;
double siteLatitude = DEFAULT_SITE_LATITUDE;
double siteLongitude = DEFAULT_SITE_LONGITUDE;
float viewHeading = DEFAULT_VIEW_HEADING;
bool routesEnabled = true;
bool hideSportAircraft = true;
bool altitudeFilterEnabled = false;


const char *uiText(const char *german, const char *english) {
  return uiEnglish ? english : german;
}
int maximumAltitudeFt = DEFAULT_MAXIMUM_ALTITUDE_FT;
AdsbClient adsbClient("");
RouteClient routeClient;
String routedFlight;
RouteResult currentRoute;
String selectedEmergencyHex;
volatile bool emergencyAdvanceRequested = false;
String emergencyTestCode;
uint8_t emergencyTestCount = 0;
uint32_t emergencyTestUntil = 0;
uint32_t lastRouteAttempt = 0;
size_t lastReceivedAircraft = 0;
size_t lastCandidateAircraft = 0;
String customPageText;
uint8_t customScrollSpeed = 1;
bool pageEnabled[] = {true, true, true, true, true, true, true};
uint8_t pageOrder[] = {0, 1, 2, 3, 4, 5, 6};
uint32_t pageDurationMs[] = {
    DEFAULT_PAGE_INTERVAL_MS, DEFAULT_PAGE_INTERVAL_MS,
    DEFAULT_PAGE_INTERVAL_MS, DEFAULT_PAGE_INTERVAL_MS,
    DEFAULT_PAGE_INTERVAL_MS, DEFAULT_PAGE_INTERVAL_MS,
    DEFAULT_PAGE_INTERVAL_MS};
bool smoothTransitions = true;
uint8_t currentPageSlot = 0;
uint32_t currentPageStarted = 0;
int8_t activeRenderedPage = -1;
uint32_t transitionStarted = 0;
uint8_t transitionPercent = 100;
enum class TransitionPhase { Normal, FadeOut, FadeIn };
TransitionPhase transitionPhase = TransitionPhase::Normal;
bool emergencyPagePending = false;
String displayedEmergencyKey;
uint32_t matrixRefreshHz = 0;
uint32_t matrixShowCount = 0;
uint32_t matrixShowMaxUs = 0;
uint32_t matrixShowLastUs = 0;

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
uint16_t idleErrorPixel;

enum class VerticalState { Unknown, Climbing, Level, Descending };
enum class LookRange { Unknown, Above, Near, Far };
enum class LookDirection {
  Unknown, Above, AboveFront, AboveRight, AboveBack, AboveLeft,
  Front, Right, RightShoulder, Back, LeftShoulder, Left,
  FarFront, FarRight, FarBack, FarLeft
};

LookRange lastLookRange = LookRange::Unknown;
bool lastDirectlyAbove = false;
String lastLookFlight;

constexpr uint8_t ARROW_UP[7] = {
    0b0001000, 0b0011100, 0b0111110, 0b0001000,
    0b0001000, 0b0001000, 0b0001000};
constexpr uint8_t ARROW_UP_RIGHT[7] = {
    0b0001111, 0b0000111, 0b0001111, 0b0011101,
    0b0111000, 0b1110000, 0b1100000};
constexpr uint8_t ARROW_RIGHT[7] = {
    0b0001000, 0b0001100, 0b1111110, 0b1111111,
    0b1111110, 0b0001100, 0b0001000};
constexpr uint8_t ARROW_DOWN_RIGHT[7] = {
    0b1100000, 0b1110000, 0b0111000, 0b0011101,
    0b0001111, 0b0000111, 0b0001111};
constexpr uint8_t ARROW_DOWN[7] = {
    0b0001000, 0b0001000, 0b0001000, 0b0001000,
    0b0111110, 0b0011100, 0b0001000};
constexpr uint8_t ARROW_DOWN_LEFT[7] = {
    0b0000011, 0b0000111, 0b0001110, 0b1011100,
    0b1111000, 0b1110000, 0b1111000};
constexpr uint8_t ARROW_LEFT[7] = {
    0b0001000, 0b0011000, 0b0111111, 0b1111111,
    0b0111111, 0b0011000, 0b0001000};
constexpr uint8_t ARROW_UP_LEFT[7] = {
    0b1111000, 0b1110000, 0b1111000, 0b1011100,
    0b0001110, 0b0000111, 0b0000011};
constexpr uint8_t ABOVE_MARKER[7] = {
    0b0011100, 0b0100010, 0b1000001, 0b1001001,
    0b1000001, 0b0100010, 0b0011100};
// Uniform, equally weighted arrows used exclusively for viewing directions.
constexpr uint8_t LOOK_ARROW_UP[7] = {
    0b0001000, 0b0011100, 0b0111110, 0b1111111,
    0b0011100, 0b0011100, 0b0011100};
constexpr uint8_t LOOK_ARROW_RIGHT[7] = {
    0b0001000, 0b0001100, 0b1111110, 0b1111111,
    0b1111110, 0b0001100, 0b0001000};
constexpr uint8_t LOOK_ARROW_DOWN[7] = {
    0b0011100, 0b0011100, 0b0011100, 0b1111111,
    0b0111110, 0b0011100, 0b0001000};
constexpr uint8_t LOOK_ARROW_LEFT[7] = {
    0b0001000, 0b0011000, 0b0111111, 0b1111111,
    0b0111111, 0b0011000, 0b0001000};
constexpr uint8_t ALTITUDE_TRIANGLE_UP[7] = {
    0b0001000, 0b0011100, 0b0011100, 0b0111110,
    0b0111110, 0b1111111, 0b1111111};
constexpr uint8_t ALTITUDE_TRIANGLE_DOWN[7] = {
    0b1111111, 0b1111111, 0b0111110, 0b0111110,
    0b0011100, 0b0011100, 0b0001000};


uint8_t scaledChannel(uint8_t value) {
  return static_cast<uint32_t>(value) * BRIGHTNESS_LEVELS[brightnessIndex] *
         transitionPercent / 10000;
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
  // Fixed first reliably visible 3-bit level, independent of automatic
  // brightness. Lower values are rounded down to black by Protomatter.
  idlePixel = matrix.color565(36, 36, 36);
  idleErrorPixel = matrix.color565(36, 0, 0);
}


void showMatrix() {
  const uint32_t started = micros();
  matrix.show();
  matrixShowLastUs = micros() - started;
  matrixShowMaxUs = max(matrixShowMaxUs, matrixShowLastUs);
  ++matrixShowCount;
}


void setTransitionPercent(uint8_t percent) {
  percent = min<uint8_t>(percent, 100);
  if (percent == transitionPercent) return;
  transitionPercent = percent;
  updateColors();
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
  text.replace("â", "a");
  text.replace("ā", "a");
  text.replace("ă", "a");
  text.replace("Á", "A");
  text.replace("À", "A");
  text.replace("Â", "A");
  text.replace("Ā", "A");
  text.replace("Ă", "A");
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
  text.replace("š", "s");
  text.replace("Š", "S");
  text.replace("ş", "s");
  text.replace("Ş", "S");

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
  if (category.indexOf("POLICE") >= 0 || category.indexOf("POLIZEI") >= 0) return cyan;
  if (category.indexOf("MILIT") >= 0 ||
      category.indexOf("AIRFORCE") >= 0) return red;
  if (category.indexOf("HELICOPTER") >= 0 || category.indexOf("HUBSCHRAUBER") >= 0) return yellow;
  if (category.indexOf("A380") >= 0) return violet;
  if (category.indexOf("GOVERNMENT") >= 0 || category.indexOf("REGIERUNG") >= 0) return blue;
  return red;
}


enum class CompactIconKind : uint8_t {
  Helicopter,
  A380,
  Military,
  Police,
  Government,
  Ship,
  Generic
};


CompactIconKind compactIconKind(const String &specialType) {
  String category = specialType;
  category.toUpperCase();
  if (category.indexOf("HELICOPTER") >= 0 ||
      category.indexOf("HUBSCHRAUBER") >= 0) {
    return CompactIconKind::Helicopter;
  }
  if (category.indexOf("A380") >= 0) return CompactIconKind::A380;
  if (category.indexOf("MILIT") >= 0 ||
      category.indexOf("AIRFORCE") >= 0) return CompactIconKind::Military;
  if (category.indexOf("POLICE") >= 0 ||
      category.indexOf("POLIZEI") >= 0) {
    return CompactIconKind::Police;
  }
  if (category.indexOf("GOVERNMENT") >= 0 ||
      category.indexOf("REGIERUNG") >= 0) {
    return CompactIconKind::Government;
  }
  if (category.indexOf("SHIP") >= 0 ||
      category.indexOf("LUFTSCHIFF") >= 0) {
    return CompactIconKind::Ship;
  }
  return CompactIconKind::Generic;
}


constexpr uint16_t COMPACT_ICON_HELICOPTER[7] = {
    0b00000011000000, 0b00111111111100, 0b00000011000000,
    0b00001111110000, 0b00011111111000, 0b00000111100000,
    0b00001000010000};
constexpr uint16_t COMPACT_ICON_A380[7] = {
    0b00000011000000, 0b00000111100000, 0b11000111100011,
    0b01111111111110, 0b00001111110000, 0b00000111100000,
    0b00000011000000};
constexpr uint16_t COMPACT_ICON_MILITARY[7] = {
    0b00000011000000, 0b00000111100000, 0b00011111111000,
    0b11111111111111, 0b00001111110000, 0b00000111100000,
    0b00001000010000};
constexpr uint16_t COMPACT_ICON_POLICE[7] = {
    0b00011111111000, 0b00110000001100, 0b01100011000110,
    0b01100111100110, 0b01100011000110, 0b00110000001100,
    0b00011111111000};
constexpr uint16_t COMPACT_ICON_GOVERNMENT[7] = {
    0b00000111100000, 0b00001111110000, 0b00011111111000,
    0b00111111111100, 0b00011011011000, 0b00011011011000,
    0b00111111111100};
constexpr uint16_t COMPACT_ICON_SHIP[7] = {
    0b00011111111000, 0b01111111111110, 0b11111111111111,
    0b01111111111110, 0b00011111111000, 0b00000111100000,
    0b00001111110000};
constexpr uint16_t COMPACT_ICON_GENERIC[7] = {
    0b00000011000000, 0b00000111100000, 0b00110011001100,
    0b00011111111000, 0b00001111110000, 0b00000111100000,
    0b00000011000000};


const uint16_t *compactIconBitmap(CompactIconKind kind) {
  switch (kind) {
    case CompactIconKind::Helicopter: return COMPACT_ICON_HELICOPTER;
    case CompactIconKind::A380: return COMPACT_ICON_A380;
    case CompactIconKind::Military: return COMPACT_ICON_MILITARY;
    case CompactIconKind::Police: return COMPACT_ICON_POLICE;
    case CompactIconKind::Government: return COMPACT_ICON_GOVERNMENT;
    case CompactIconKind::Ship: return COMPACT_ICON_SHIP;
    default: return COMPACT_ICON_GENERIC;
  }
}


uint16_t compactIconColor(CompactIconKind kind) {
  switch (kind) {
    case CompactIconKind::Helicopter: return yellow;
    case CompactIconKind::A380: return violet;
    case CompactIconKind::Military: return red;
    case CompactIconKind::Police: return cyan;
    case CompactIconKind::Government: return blue;
    case CompactIconKind::Ship: return red;
    default: return red;
  }
}


void drawCompactSpecialIcon(CompactIconKind kind, int16_t x, int16_t y,
                            uint16_t color) {
  const uint16_t *bitmap = compactIconBitmap(kind);
  for (uint8_t row = 0; row < 7; ++row) {
    for (uint8_t column = 0; column < 14; ++column) {
      if (bitmap[row] & (1U << (13 - column))) {
        matrix.drawPixel(x + column, y + row, color);
      }
    }
  }
}


void drawCurrentSpecialIcon(int16_t x, int16_t y = 0) {
  if (!flight.specialType.length()) return;
  const CompactIconKind kind = compactIconKind(flight.specialType);
  drawCompactSpecialIcon(kind, x, y, compactIconColor(kind));
}


void drawHeader() {
  String callsign = clipped(flight.flight.length() ? flight.flight : "UNBEKANNT", 8);
  const String aircraftType = clipped(flight.type, 4);
  const int16_t typeX = MATRIX_WIDTH - aircraftType.length() * 6;
  const bool hasSpecial = flight.specialType.length();
  const int16_t iconX = typeX - 16;
  drawLine(0, 0, callsign, aircraftColor(),
           callsign.length() <= (hasSpecial ? 7 : 8) ? 2 : 1);
  if (hasSpecial) drawCurrentSpecialIcon(iconX);
  drawLine(typeX, 0, aircraftType, white);
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


void drawAltitudeArrowAt(int16_t x, int16_t y) {
  switch (verticalState()) {
    case VerticalState::Climbing:
      drawArrowBitmap(x, y, ARROW_UP_RIGHT, green);
      break;
    case VerticalState::Descending:
      drawArrowBitmap(x, y, ARROW_DOWN_RIGHT, orange);
      break;
    case VerticalState::Level:
      drawArrowBitmap(x, y, ARROW_RIGHT, white);
      break;
    default:
      break;
  }
}


void drawCompactAltitudeIndicatorAt(int16_t x, int16_t y) {
  switch (verticalState()) {
    case VerticalState::Climbing:
      drawArrowBitmap(x, y, ALTITUDE_TRIANGLE_UP, green);
      break;
    case VerticalState::Descending:
      drawArrowBitmap(x, y, ALTITUDE_TRIANGLE_DOWN, orange);
      break;
    default:
      break;
  }
}


float zenithAngleDegrees() {
  if (isnan(flight.altitudeFt) || isnan(flight.distanceKm)) return NAN;
  const float relativeHeightKm =
      (flight.altitudeFt * 0.3048f - observerElevationM) / 1000.0f;
  if (relativeHeightKm <= 0.0f) return 90.0f;
  return atan2f(max(0.0f, flight.distanceKm), relativeHeightKm) *
         180.0f / PI;
}


LookRange currentLookRange() {
  if (lastLookFlight != flight.flight) {
    lastLookFlight = flight.flight;
    lastLookRange = LookRange::Unknown;
    lastDirectlyAbove = false;
  }

  const float zenith = zenithAngleDegrees();
  if (!isnan(zenith)) {
    const float aboveLimit = ABOVE_ZENITH_DEG +
        (lastLookRange == LookRange::Above ? ZENITH_HYSTERESIS_DEG : 0.0f);
    if (zenith <= aboveLimit) {
      lastLookRange = LookRange::Above;
      const float directLimit = DIRECTLY_ABOVE_ZENITH_DEG +
          (lastDirectlyAbove ? ZENITH_HYSTERESIS_DEG : 0.0f);
      lastDirectlyAbove = zenith <= directLimit;
      return lastLookRange;
    }
  }
  lastDirectlyAbove = false;

  if (!isnan(flight.distanceKm)) {
    const float farLimit = farDirectionDistanceKm -
        (lastLookRange == LookRange::Far ? FAR_DISTANCE_HYSTERESIS_KM : 0.0f);
    if (flight.distanceKm >= max(0.0f, farLimit)) {
      lastLookRange = LookRange::Far;
      return lastLookRange;
    }
  }
  lastLookRange = LookRange::Near;
  return lastLookRange;
}


LookDirection lookDirection(float angle) {
  if (isnan(angle)) return LookDirection::Unknown;
  const float absolute = fabs(angle);
  const LookRange range = currentLookRange();
  if (range == LookRange::Above) {
    if (lastDirectlyAbove) return LookDirection::Above;
    if (absolute <= 45) return LookDirection::AboveFront;
    if (absolute >= 135) return LookDirection::AboveBack;
    return angle > 0 ? LookDirection::AboveRight : LookDirection::AboveLeft;
  }
  if (range == LookRange::Far) {
    if (absolute <= 45) return LookDirection::FarFront;
    if (absolute >= 135) return LookDirection::FarBack;
    return angle > 0 ? LookDirection::FarRight : LookDirection::FarLeft;
  }
  if (absolute <= 30) return LookDirection::Front;
  if (absolute <= 100) return angle > 0 ? LookDirection::Right : LookDirection::Left;
  if (absolute <= 150) {
    return angle > 0 ? LookDirection::RightShoulder : LookDirection::LeftShoulder;
  }
  return LookDirection::Back;
}


String compactLookDirection(float angle) {
  switch (lookDirection(angle)) {
    case LookDirection::Above: return uiText("oben", "above");
    case LookDirection::AboveFront: return uiText("oben vorne", "above front");
    case LookDirection::AboveRight: return uiText("oben rechts", "above right");
    case LookDirection::AboveBack: return uiText("oben hinten", "above rear");
    case LookDirection::AboveLeft: return uiText("oben links", "above left");
    case LookDirection::Front: return uiText("vorne", "front");
    case LookDirection::Right: return uiText("rechts", "right");
    case LookDirection::RightShoulder: return uiText("rechte Schulter", "right rear");
    case LookDirection::Back: return uiText("hinten", "rear");
    case LookDirection::LeftShoulder: return uiText("linke Schulter", "left rear");
    case LookDirection::Left: return uiText("links", "left");
    case LookDirection::FarFront: return uiText("weit vorne", "far front");
    case LookDirection::FarRight: return uiText("weit rechts", "far right");
    case LookDirection::FarBack: return uiText("weit hinten", "far rear");
    case LookDirection::FarLeft: return uiText("weit links", "far left");
    default: return uiText("richtung ?", "direction ?");
  }
}


void drawLookArrowAt(float angle, int16_t x, int16_t y, uint16_t color) {
  const LookDirection direction = lookDirection(angle);
  const uint8_t *bitmap = nullptr;
  switch (direction) {
    case LookDirection::Above: bitmap = ABOVE_MARKER; break;
    case LookDirection::AboveFront:
    case LookDirection::Front:
    case LookDirection::FarFront: bitmap = LOOK_ARROW_UP; break;
    case LookDirection::AboveRight: bitmap = ARROW_UP_RIGHT; break;
    case LookDirection::Right:
    case LookDirection::FarRight: bitmap = LOOK_ARROW_RIGHT; break;
    case LookDirection::RightShoulder: bitmap = ARROW_DOWN_RIGHT; break;
    case LookDirection::AboveBack:
    case LookDirection::Back:
    case LookDirection::FarBack: bitmap = LOOK_ARROW_DOWN; break;
    case LookDirection::LeftShoulder: bitmap = ARROW_DOWN_LEFT; break;
    case LookDirection::AboveLeft: bitmap = ARROW_UP_LEFT; break;
    case LookDirection::Left:
    case LookDirection::FarLeft: bitmap = LOOK_ARROW_LEFT; break;
    default: break;
  }
  if (bitmap != nullptr) drawArrowBitmap(x, y, bitmap, color);
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
  drawAltitudeArrowAt(121, 16);

  drawLookArrowAt(flight.relativeBearing, 0, 24, yellow);
  drawLine(10, 24, compactLookDirection(flight.relativeBearing), yellow);
}


String compactRoute() {
  if (flight.routeCodes.length()) {
    String route = clipped(flight.routeCodes, 21);
    route.replace("-", ">");
    return route;
  }
  const String origin = clipped(flight.origin, 4);
  const String destination = clipped(flight.destination, 4);
  if (origin.length() && destination.length()) return origin + ">" + destination;
  if (origin.length()) return origin;
  return destination;
}


String displayedAirline() {
  String airline = flight.airline;
  airline.trim();
  String comparison = airline;
  comparison.toUpperCase();
  return comparison == "UNKNOWN" ? uiText("Airline unbekannt", "Airline N/A") : airline;
}


void drawRoutePage() {
  drawHeader();
  drawLine(0, 16, clipped(displayedAirline(), 21), blue);

  String route = compactRoute();
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
    const String departingText = uiText("entfernt sich", "departing");
    if (cpa.length() + 1 + departingText.length() <= 21) {
      cpa += " " + departingText;
    } else {
      cpa += uiText(" geht", " away");
    }
  } else if (!isnan(flight.cpaMinutes)) {
    cpa += " in " + String(static_cast<int>(round(flight.cpaMinutes))) + "min";
  }
  drawLine(0, 16, clipped(cpa, 21), departing ? orange : green);

  String details = flight.specialType;
  drawLine(0, 24, clipped(details, 21), aircraftColor());
}


String compactAircraftType() {
  return clipped(flight.type.length() ? flight.type : "Typ ?", 8);
}


int16_t compactHeaderLeftRegionWidth() {
  const int16_t typeX = MATRIX_WIDTH - compactAircraftType().length() * 6;
  return typeX - (flight.specialType.length() ? 18 : 2);
}


void drawCompactTypeHeader() {
  const String aircraftType = clipped(
      flight.type.length() ? flight.type : "Typ ?", 8);
  const int16_t typeX = MATRIX_WIDTH - aircraftType.length() * 6;
  if (flight.specialType.length()) {
    const CompactIconKind kind = compactIconKind(flight.specialType);
    drawCompactSpecialIcon(kind, typeX - 16, 0, compactIconColor(kind));
  }
  drawLine(typeX, 0, aircraftType, white);
}


void drawCompactDetails() {
  const String distance = isnan(flight.distanceKm) ?
      "?km" : String(flight.distanceKm, 1) + "km";
  const String altitude = isnan(flight.altitudeFt) ?
      "?ft" : String(static_cast<int>(round(flight.altitudeFt))) + "ft";
  const String metrics = distance + " " + altitude;
  drawLine(0, 8, metrics, yellow);
  const int16_t indicatorX = metrics.length() * 6 + 1;
  const VerticalState state = verticalState();
  const bool hasIndicator = state == VerticalState::Climbing ||
      state == VerticalState::Descending;
  const int16_t tightSpeedX = metrics.length() * 6 + 1;
  int16_t preferredSpeedX = metrics.length() * 6 + 6;
  if (hasIndicator && indicatorX + 7 <= MATRIX_WIDTH) {
    drawCompactAltitudeIndicatorAt(indicatorX, 8);
    preferredSpeedX = indicatorX + 8;
  }

  if (!isnan(flight.speedKmh)) {
    const String speedValue = String(static_cast<int>(round(flight.speedKmh)));
    const String fullSpeed = speedValue + "kmh";
    const String shortSpeed = speedValue + "k";
    const int16_t fullWidth = fullSpeed.length() * 6 - 1;
    const int16_t shortWidth = shortSpeed.length() * 6 - 1;
    if (preferredSpeedX + fullWidth <= MATRIX_WIDTH) {
      drawLine(preferredSpeedX, 8, fullSpeed, yellow);
    } else if (preferredSpeedX + shortWidth <= MATRIX_WIDTH) {
      drawLine(preferredSpeedX, 8, shortSpeed, yellow);
    } else if (!hasIndicator && tightSpeedX + fullWidth <= MATRIX_WIDTH) {
      drawLine(tightSpeedX, 8, fullSpeed, yellow);
    } else if (!hasIndicator && tightSpeedX + shortWidth <= MATRIX_WIDTH) {
      drawLine(tightSpeedX, 8, shortSpeed, yellow);
    }
  }

  const int absoluteBearing = isnan(flight.bearing)
      ? -1 : static_cast<int>(round(flight.bearing)) % 360;
  const String bearingText = absoluteBearing < 0
      ? "?" : String(absoluteBearing);
  constexpr int16_t degreeX = 114;
  const int16_t bearingX = degreeX - 1 - bearingText.length() * 6;
  const uint8_t lookTextLength = static_cast<uint8_t>(max(1, bearingX / 6));
  drawLine(0, 16,
           clipped(compactLookDirection(flight.relativeBearing), lookTextLength),
           cyan);
  drawLine(bearingX, 16, bearingText, cyan);
  matrix.drawCircle(degreeX + 1, 18, 1, cyan);
  drawLookArrowAt(flight.relativeBearing, 121, 16, yellow);
}


void drawCompactTop() {
  const String flightNumber = clipped(
      flight.flight.length() ? flight.flight : uiText("Flug ?", "Flight ?"), 10);
  drawLine(0, 0, flightNumber, red);
  drawCompactTypeHeader();

  drawCompactDetails();
}


void drawCompactPage() {
  drawCompactTop();

  String bottom = compactRoute();
  const bool departing = flight.motion == "departing";
  String cpa;
  if (departing) {
    const uint8_t separatorLength = bottom.length() ? 1 : 0;
    const String departingText = uiText("entfernt sich", "departing");
    cpa = bottom.length() + separatorLength + departingText.length() <= 21 ?
        departingText : uiText("geht", "away");
  } else if (!isnan(flight.cpaKm)) {
    cpa = "CPA " + String(flight.cpaKm, 1);
    if (!isnan(flight.cpaMinutes)) {
      const String withMinutes = cpa + "/" +
          String(static_cast<int>(round(flight.cpaMinutes))) + "m";
      if (bottom.length() + 1 + withMinutes.length() <= 21) cpa = withMinutes;
    }
  }
  if (cpa.length()) {
    if (bottom.length()) bottom += " ";
    bottom += cpa;
  }
  drawLine(0, 24, clipped(bottom, 21), departing ? orange : green);
}


constexpr int16_t AIRPORT_HALF_WIDTH = 60;
constexpr int16_t AIRPORT_SEPARATOR_X = 61;
constexpr int16_t AIRPORT_DESTINATION_X = 68;
constexpr uint32_t AIRPORT_NAMES_MINIMUM_MS = 5000;
constexpr uint32_t AIRPORT_HALF_PAUSE_MS = 800;
constexpr uint32_t AIRPORT_HALF_PIXEL_MS = 80;
constexpr uint32_t AIRPORT_RETURN_PIXEL_MS = 20;


String limitedAirportText(const String &name) {
  String text = displayAscii(name);
  text.trim();
  if (text.length() > 128) text.remove(128);
  return text;
}


String limitedAirlineText() {
  String airline = displayedAirline();
  String text = displayAscii(airline.length() ? airline : "Airline ?");
  text.trim();
  if (text.length() > 128) text.remove(128);
  return text;
}


uint32_t airportScrollDuration(const String &text, int16_t regionWidth) {
  const uint32_t textWidth = text.length() * 6UL;
  if (textWidth <= regionWidth) return 0;
  const uint32_t travelPixels = textWidth - regionWidth;
  return AIRPORT_HALF_PAUSE_MS +
         travelPixels * AIRPORT_HALF_PIXEL_MS +
         AIRPORT_HALF_PAUSE_MS +
         travelPixels * AIRPORT_RETURN_PIXEL_MS +
         AIRPORT_HALF_PAUSE_MS;
}


uint32_t compactAirportsNamePhaseDuration() {
  const String origin = limitedAirportText(flight.originName);
  const String destination = limitedAirportText(flight.destinationName);
  const int16_t originWidth = origin.length() * 6;
  const int16_t destinationWidth = destination.length() * 6;
  uint32_t scrollDuration = 0;

  if (!origin.length()) {
    scrollDuration = airportScrollDuration(destination, MATRIX_WIDTH);
  } else if (!destination.length()) {
    scrollDuration = airportScrollDuration(origin, MATRIX_WIDTH);
  } else if (originWidth + 6 + destinationWidth <= MATRIX_WIDTH) {
    scrollDuration = 0;
  } else if (originWidth < MATRIX_WIDTH / 2) {
    scrollDuration = airportScrollDuration(
        destination, MATRIX_WIDTH - originWidth - 6);
  } else if (destinationWidth < MATRIX_WIDTH / 2) {
    scrollDuration = airportScrollDuration(
        origin, MATRIX_WIDTH - destinationWidth - 6);
  } else {
    scrollDuration = max(
        airportScrollDuration(origin, AIRPORT_HALF_WIDTH),
        airportScrollDuration(destination, AIRPORT_HALF_WIDTH));
  }
  return max(scrollDuration, AIRPORT_NAMES_MINIMUM_MS);
}


uint32_t compactAirportsMinimumDuration() {
  const uint32_t airlineDuration = airportScrollDuration(
      limitedAirlineText(), compactHeaderLeftRegionWidth());
  return max(compactAirportsNamePhaseDuration(), airlineDuration);
}


int16_t airportTextX(const String &text, int16_t regionWidth,
                     uint32_t elapsedMs) {
  const int16_t textWidth = text.length() * 6;
  if (textWidth <= regionWidth) return 0;

  const int16_t travelPixels = textWidth - regionWidth;
  const int16_t finalX = -travelPixels;
  if (elapsedMs < AIRPORT_HALF_PAUSE_MS) return 0;
  elapsedMs -= AIRPORT_HALF_PAUSE_MS;

  const uint32_t forwardMs = travelPixels * AIRPORT_HALF_PIXEL_MS;
  if (elapsedMs < forwardMs) {
    return -static_cast<int16_t>(elapsedMs / AIRPORT_HALF_PIXEL_MS);
  }
  elapsedMs -= forwardMs;
  if (elapsedMs < AIRPORT_HALF_PAUSE_MS) return finalX;
  elapsedMs -= AIRPORT_HALF_PAUSE_MS;

  const uint32_t returnMs = travelPixels * AIRPORT_RETURN_PIXEL_MS;
  if (elapsedMs < returnMs) {
    return finalX + static_cast<int16_t>(elapsedMs / AIRPORT_RETURN_PIXEL_MS);
  }
  return 0;
}


void drawAirportRegion(const String &text, int16_t regionX,
                       int16_t regionY, int16_t regionWidth,
                       int16_t textX, uint16_t color) {
  if (!text.length() || airportTextCanvas.getBuffer() == nullptr) return;
  airportTextCanvas.fillScreen(0);
  airportTextCanvas.setFont(nullptr);
  airportTextCanvas.setTextSize(1);
  airportTextCanvas.setTextWrap(false);
  airportTextCanvas.setTextColor(1);
  airportTextCanvas.setCursor(0, 0);
  airportTextCanvas.print(text);
  for (int16_t x = 0; x < regionWidth; ++x) {
    const int16_t sourceX = x - textX;
    if (sourceX < 0 || sourceX >= AIRPORT_TEXT_CANVAS_WIDTH) continue;
    for (int16_t y = 0; y < 8; ++y) {
      if (airportTextCanvas.getPixel(sourceX, y)) {
        matrix.drawPixel(regionX + x, regionY + y, color);
      }
    }
  }
}


void drawCompactAirportsPage() {
  const uint32_t elapsed = millis() - currentPageStarted;
  const String airline = limitedAirlineText();
  const int16_t airlineRegionWidth = compactHeaderLeftRegionWidth();
  drawAirportRegion(airline, 0, 0, airlineRegionWidth,
                    airportTextX(airline, airlineRegionWidth, elapsed), red);
  drawCompactTypeHeader();
  drawCompactDetails();

  const String origin = limitedAirportText(flight.originName);
  const String destination = limitedAirportText(flight.destinationName);
  const int16_t originWidth = origin.length() * 6;
  const int16_t destinationWidth = destination.length() * 6;

  if (!origin.length()) {
    const int16_t x = destinationWidth <= MATRIX_WIDTH ?
        MATRIX_WIDTH - destinationWidth :
        airportTextX(destination, MATRIX_WIDTH, elapsed);
    drawAirportRegion(destination, 0, 24, MATRIX_WIDTH, x, yellow);
    return;
  }
  if (!destination.length()) {
    drawAirportRegion(origin, 0, 24, MATRIX_WIDTH,
                      airportTextX(origin, MATRIX_WIDTH, elapsed), white);
    return;
  }

  if (originWidth + 6 + destinationWidth <= MATRIX_WIDTH) {
    const int16_t destinationX = MATRIX_WIDTH - destinationWidth;
    const int16_t separatorX = originWidth +
        (destinationX - originWidth - 6) / 2;
    drawAirportRegion(origin, 0, 24, originWidth, 0, white);
    drawLine(separatorX, 24, ">", cyan);
    drawAirportRegion(destination, destinationX, 24, destinationWidth, 0, yellow);
  } else if (originWidth < MATRIX_WIDTH / 2) {
    const int16_t separatorX = originWidth;
    const int16_t destinationX = separatorX + 6;
    const int16_t destinationRegionWidth = MATRIX_WIDTH - destinationX;
    drawAirportRegion(origin, 0, 24, originWidth, 0, white);
    drawLine(separatorX, 24, ">", cyan);
    drawAirportRegion(destination, destinationX, 24, destinationRegionWidth,
                      airportTextX(destination, destinationRegionWidth, elapsed),
                      yellow);
  } else if (destinationWidth < MATRIX_WIDTH / 2) {
    const int16_t destinationX = MATRIX_WIDTH - destinationWidth;
    const int16_t separatorX = destinationX - 6;
    drawAirportRegion(origin, 0, 24, separatorX,
                      airportTextX(origin, separatorX, elapsed), white);
    drawLine(separatorX, 24, ">", cyan);
    drawAirportRegion(destination, destinationX, 24, destinationWidth, 0, yellow);
  } else {
    drawAirportRegion(origin, 0, 24, AIRPORT_HALF_WIDTH,
                      airportTextX(origin, AIRPORT_HALF_WIDTH, elapsed), white);
    drawLine(AIRPORT_SEPARATOR_X, 24, ">", cyan);
    drawAirportRegion(destination, AIRPORT_DESTINATION_X, 24, AIRPORT_HALF_WIDTH,
                      airportTextX(destination, AIRPORT_HALF_WIDTH, elapsed),
                      yellow);
  }
}


constexpr uint32_t SCROLL_PAUSE_MS = 1500;
constexpr uint32_t SCROLL_PIXEL_MS = 80;


uint32_t scrollingTextDuration(const String &text) {
  const int16_t textWidth = text.length() * 6;
  if (textWidth <= MATRIX_WIDTH) return 0;
  return SCROLL_PAUSE_MS +
         (textWidth - MATRIX_WIDTH) * SCROLL_PIXEL_MS +
         SCROLL_PAUSE_MS;
}


int16_t scrollingTextX(const String &text, uint32_t elapsedMs) {
  const int16_t textWidth = text.length() * 6;
  if (textWidth <= MATRIX_WIDTH || elapsedMs < SCROLL_PAUSE_MS) return 0;
  const uint32_t travelMs = (textWidth - MATRIX_WIDTH) * SCROLL_PIXEL_MS;
  if (elapsedMs < SCROLL_PAUSE_MS + travelMs) {
    return -static_cast<int16_t>(
        (elapsedMs - SCROLL_PAUSE_MS) / SCROLL_PIXEL_MS);
  }
  return MATRIX_WIDTH - textWidth;
}


bool flightHasMultiHopRoute() {
  const int firstSeparator = flight.routeCodes.indexOf('-');
  return firstSeparator >= 0 &&
         flight.routeCodes.indexOf('-', firstSeparator + 1) >= 0;
}


void drawAirportNamesPage() {
  drawLine(0, 0, uiText("Flughaefen", "Airports"), cyan);
  drawCurrentSpecialIcon(MATRIX_WIDTH - 14);
  const uint32_t elapsed = millis() - currentPageStarted;
  if (flightHasMultiHopRoute()) {
    String codes = displayAscii(flight.routeCodes);
    codes.replace("-", ">");
    const String names = displayAscii(flight.routeNames);
    drawLine(scrollingTextX(codes, elapsed), 11, codes, white);
    drawLine(scrollingTextX(names, elapsed), 22, names, yellow);
    return;
  }
  String originText = "S " + (flight.originName.length() ?
      flight.originName : (flight.origin.length() ? flight.origin : uiText("unbekannt", "unknown")));
  String destinationText = "Z " + (flight.destinationName.length() ?
      flight.destinationName : (flight.destination.length() ? flight.destination : uiText("unbekannt", "unknown")));
  originText = displayAscii(originText);
  destinationText = displayAscii(destinationText);
  drawLine(scrollingTextX(originText, elapsed), 11, originText, white);
  drawLine(scrollingTextX(destinationText, elapsed), 22, destinationText, yellow);
}


bool airportNamesPageHasData() {
  return flight.routeCodes.length() || flight.originName.length() || flight.origin.length() ||
         flight.destinationName.length() || flight.destination.length();
}


uint32_t airportNamesPageDuration() {
  if (flightHasMultiHopRoute()) {
    String codes = displayAscii(flight.routeCodes);
    codes.replace("-", ">");
    return max(scrollingTextDuration(codes),
               scrollingTextDuration(displayAscii(flight.routeNames)));
  }
  String originText = displayAscii("S " + (flight.originName.length() ?
      flight.originName : flight.origin));
  String destinationText = displayAscii("Z " + (flight.destinationName.length() ?
      flight.destinationName : flight.destination));
  return max(scrollingTextDuration(originText),
             scrollingTextDuration(destinationText));
}


constexpr uint32_t CUSTOM_SCROLL_PAUSE_MS = 500;
constexpr uint16_t CUSTOM_SCROLL_PIXEL_MS[] = {120, 80, 25};


uint16_t customScrollPixelMs() {
  return CUSTOM_SCROLL_PIXEL_MS[customScrollSpeed];
}


const char *customScrollSpeedName() {
  switch (customScrollSpeed) {
    case 0: return "langsam";
    case 2: return "schnell";
    default: return "mittel";
  }
}


uint32_t customPageScrollDuration() {
  const uint32_t textWidth = customPageText.length() * 6UL;
  return CUSTOM_SCROLL_PAUSE_MS +
         (MATRIX_WIDTH + textWidth) * customScrollPixelMs() +
         CUSTOM_SCROLL_PAUSE_MS;
}


void drawCustomTextPage() {
  const uint32_t elapsed = millis() - currentPageStarted;
  const uint32_t textWidth = customPageText.length() * 6UL;
  int32_t x = MATRIX_WIDTH;
  if (elapsed > CUSTOM_SCROLL_PAUSE_MS) {
    x -= (elapsed - CUSTOM_SCROLL_PAUSE_MS) / customScrollPixelMs();
  }
  const int32_t finalX = -static_cast<int32_t>(textWidth);
  if (x < finalX) x = finalX;
  drawLine(static_cast<int16_t>(x), 13, customPageText, cyan);
  drawCurrentSpecialIcon(MATRIX_WIDTH - 14);
}


void drawStatus(const String &line1, const String &line2) {
  matrix.fillScreen(0);
  drawLine(0, 8, clipped(line1, 21), yellow);
  drawLine(0, 18, clipped(line2, 21), blue);
  showMatrix();
}


String emergencyMeaning(const String &squawk) {
  if (squawk == "7500") return "HIJACK";
  if (squawk == "7600") return "RADIO";
  return "EMERGENCY";
}


void drawEmergencyPage() {
  matrix.fillScreen(0);
  const String identity = flight.flight.length() ? flight.flight : flight.hex;
  drawLine(0, 0, clipped(identity, 13), white);
  drawCurrentSpecialIcon(80);
  drawLine(98, 0, flight.squawk, red);
  const String meaning = emergencyMeaning(flight.squawk);
  int16_t meaningX = (MATRIX_WIDTH - meaning.length() * 6) / 2;
  if (meaningX < 0) meaningX = 0;
  drawLine(meaningX, 11, meaning, flight.squawk == "7600" ? yellow : red);
  String details;
  if (flight.hasPosition && !isnan(flight.distanceKm))
    details = String(flight.distanceKm, 1) + "km";
  else details = "POSITION?";
  if (!isnan(flight.altitudeFt)) details += " " + String(flight.altitudeFt, 0) + "ft";
  drawLine(0, 22, clipped(details, 21), orange);
}


bool aircraftIsInsideDisplayRadius() {
  if (flight.available && flight.emergency) return true;
  const float allowedRadius = flight.specialType.length()
                                  ? displayRadiusKm * 2.0f
                                  : displayRadiusKm;
  return flight.available && !isnan(flight.distanceKm) &&
         flight.distanceKm <= allowedRadius;
}


bool weatherValueFresh(uint32_t updated) {
  return updated && millis() - updated <= MQTT_VALUE_STALE_MS;
}


String weatherCompassDirection(float degrees) {
  static const char *directionsDe[] = {
      "N", "NNO", "NO", "ONO", "O", "OSO", "SO", "SSO",
      "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  static const char *directionsEn[] = {
      "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
      "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  while (degrees < 0.0f) degrees += 360.0f;
  while (degrees >= 360.0f) degrees -= 360.0f;
  const uint8_t index = static_cast<uint8_t>((degrees + 11.25f) / 22.5f) % 16;
  return uiEnglish ? directionsEn[index] : directionsDe[index];
}


bool mqttWeatherAvailable() {
  if (!mqttWeatherEnabled || weatherMutex == nullptr) return false;
  bool available = false;
  if (xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    available = weatherValueFresh(windDirectionUpdated) ||
                weatherValueFresh(windSpeedUpdated) ||
                weatherValueFresh(windGustUpdated) ||
                weatherValueFresh(rainUpdated);
    xSemaphoreGive(weatherMutex);
  }
  return available;
}


void drawCompactClimateDisplay(bool bmeAvailable, bool mqttAvailable) {
  String direction;
  float speed = NAN;
  float gust = NAN;
  float rain = NAN;
  if (mqttAvailable &&
      xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
    if (weatherValueFresh(windDirectionUpdated)) direction = windDirection;
    if (weatherValueFresh(windSpeedUpdated)) speed = windSpeedKph;
    if (weatherValueFresh(windGustUpdated)) gust = windGustKph;
    if (weatherValueFresh(rainUpdated)) rain = rain24Mm;
    xSemaphoreGive(weatherMutex);
  }
  const String temperatureText = bmeAvailable
      ? String(ambientTemperatureC, 1) : "--";
  drawLine(0, 5, temperatureText, yellow);
  const int16_t degreeX = temperatureText.length() * 6 + 1;
  matrix.drawCircle(degreeX + 1, 6, 1, yellow);
  drawLine(degreeX + 4, 5, "C", yellow);
  const String humidityText = bmeAvailable
      ? String(relativeHumidityPercent, 0) + "%" : "--%";
  const String pressureText = bmeAvailable
      ? String(airPressureHpa, 0) + "hPa" : "----hPa";
  constexpr int16_t middleColumnCenterX = 64;
  const int16_t humidityX = middleColumnCenterX -
      static_cast<int16_t>(humidityText.length() * 3);
  const int16_t pressureX = MATRIX_WIDTH -
      static_cast<int16_t>(pressureText.length() * 6);
  drawLine(humidityX, 5, humidityText, cyan);
  drawLine(pressureX, 5, pressureText, blue);

  drawLine(0, 18, direction.length() ? direction : "--", cyan);
  drawLine(24, 18, isfinite(speed) ? String(speed, 0) : "--", yellow);
  const String gustText = isfinite(gust) ? String(gust, 0) : "--";
  const int16_t gustX = middleColumnCenterX -
      static_cast<int16_t>(gustText.length() * 3);
  drawLine(gustX - 12, 18, uiText("B", "G"), orange);
  drawLine(gustX, 18, gustText, orange);
  String rainText = "--mm";
  if (isfinite(rain))
    rainText = String(rain, rain < 100.0f ? 1 : 0) + "mm";
  const int16_t rainX = MATRIX_WIDTH -
      static_cast<int16_t>(rainText.length() * 6);
  drawLine(rainX - 12, 18, "R", blue);
  drawLine(rainX, 18, rainText, blue);
}


void drawIdleDisplay() {
  matrix.fillScreen(0);
  const bool bmeAvailable = environmentIdleDisplayEnabled &&
      environmentSensorAvailable && isfinite(ambientTemperatureC) &&
      isfinite(relativeHumidityPercent) && isfinite(airPressureHpa);
  const bool mqttAvailable = environmentIdleDisplayEnabled &&
      mqttWeatherAvailable();
  if (bmeAvailable || mqttAvailable)
    drawCompactClimateDisplay(bmeAvailable, mqttAvailable);
  if ((millis() / IDLE_BLINK_HALF_PERIOD_MS) % 2 == 0) {
    matrix.drawPixel(0, 31, lastError.length() ? idleErrorPixel : idlePixel);
  }
  showMatrix();
}


bool selectEnabledPage() {
  for (uint8_t offset = 0; offset < PAGE_COUNT; ++offset) {
    const uint8_t slot = (currentPageSlot + offset) % PAGE_COUNT;
    const uint8_t page = pageOrder[slot];
    const bool pageHasContent =
        (page != 3 || airportNamesPageHasData()) &&
        (page != 4 || customPageText.length());
    if (page < PAGE_COUNT && pageEnabled[page] && pageHasContent) {
      currentPageSlot = slot;
      return true;
    }
  }
  return false;
}


void advancePage() {
  const uint8_t previousSlot = currentPageSlot;
  currentPageSlot = (currentPageSlot + 1) % PAGE_COUNT;
  selectEnabledPage();
  if (flight.emergency && flight.emergencyCount > 1 &&
      currentPageSlot <= previousSlot)
    emergencyAdvanceRequested = true;
}


void drawConfiguredPage(uint8_t page) {
  switch (page) {
    case 0: drawMainPage(); break;
    case 1: drawRoutePage(); break;
    case 2: drawCpaPage(); break;
    case 3: drawAirportNamesPage(); break;
    case 4: drawCustomTextPage(); break;
    case 5: drawCompactPage(); break;
    default: drawCompactAirportsPage(); break;
  }
}


void clearMatrixBuffers() {
  // Protomatter uses two frame buffers. Clear and swap twice so neither one
  // can expose pixels left by the previously active page.
  matrix.fillScreen(0);
  showMatrix();
  matrix.fillScreen(0);
  showMatrix();
}


void activatePage(uint8_t page, uint32_t now) {
  if (activeRenderedPage == static_cast<int8_t>(page)) return;
  clearMatrixBuffers();
  activeRenderedPage = page;
  currentPageStarted = now;
}


void resetPageTransition() {
  transitionPhase = TransitionPhase::Normal;
  currentPageStarted = 0;
  activeRenderedPage = -1;
  setTransitionPercent(100);
}


bool serviceDisplayTest() {
  const uint8_t command = displayTestCommand;
  displayTestCommand = 0;
  if (command == 1) {
    displayTestActive = true;
    displayTestStarted = millis();
    resetPageTransition();
  } else if (command == 2) {
    displayTestActive = false;
    resetPageTransition();
  }
  if (!displayTestActive) return false;

  constexpr uint32_t STAGE_MS = 1500;
  constexpr uint8_t STAGE_COUNT = 15;
  const uint32_t elapsed = millis() - displayTestStarted;
  if (elapsed >= STAGE_MS * STAGE_COUNT) {
    displayTestActive = false;
    resetPageTransition();
    return false;
  }

  // Use the configured brightness to avoid an unexpected full-current test.
  const uint16_t testRed = red;
  const uint16_t testGreen = green;
  const uint16_t testBlue = blue;
  const uint16_t testWhite = white;
  const uint8_t stage = elapsed / STAGE_MS;
  matrix.fillScreen(0);
  if (stage == 0) matrix.fillScreen(testRed);
  else if (stage == 1) matrix.fillScreen(testGreen);
  else if (stage == 2) matrix.fillScreen(testBlue);
  else if (stage == 3) matrix.fillScreen(testWhite);
  else if (stage == 4) {
    for (int16_t y = 0; y < 32; y += 4) {
      for (int16_t x = 0; x < MATRIX_WIDTH; x += 4) {
        if (((x / 4) + (y / 4)) % 2 == 0) {
          matrix.fillRect(x, y, 4, 4, testWhite);
        }
      }
    }
  } else if (stage == 5) {
    matrix.drawRect(0, 0, MATRIX_WIDTH, 32, testWhite);
    matrix.drawLine(0, 0, MATRIX_WIDTH - 1, 31, testRed);
    matrix.drawLine(MATRIX_WIDTH - 1, 0, 0, 31, testGreen);
    matrix.drawFastVLine(63, 0, 32, testBlue);
    matrix.drawFastVLine(64, 0, 32, testBlue);
  } else if (stage == 6) {
    drawLine(0, 0, "DISPLAYTEST", testWhite);
    drawLine(0, 11, "0123456789", testGreen);
    drawLine(0, 22, "RGB 128x32", testBlue);
  } else if (stage == 7) {
    const int16_t bandWidth = MATRIX_WIDTH / 4;
    matrix.fillRect(0, 0, bandWidth, 32, testRed);
    matrix.fillRect(bandWidth, 0, bandWidth, 32, testGreen);
    matrix.fillRect(bandWidth * 2, 0, bandWidth, 32, testBlue);
    matrix.fillRect(bandWidth * 3, 0,
                    MATRIX_WIDTH - bandWidth * 3, 32, testWhite);
  } else {
    constexpr CompactIconKind TEST_ICONS[7] = {
        CompactIconKind::Helicopter, CompactIconKind::A380,
        CompactIconKind::Military, CompactIconKind::Police,
        CompactIconKind::Government, CompactIconKind::Ship,
        CompactIconKind::Generic};
    constexpr const char *TEST_LABELS[7] = {
        "Hubschrauber", "A380", "Militaer", "Polizei",
        "Regierung", "Luftschiff", "Sonstiger Typ"};
    const uint8_t iconIndex = stage - 8;
    const CompactIconKind kind = TEST_ICONS[iconIndex];
    drawLine(0, 1, "TYPEN-SYMBOL", testWhite);
    drawCompactSpecialIcon(kind, 0, 17, compactIconColor(kind));
    drawLine(20, 17, TEST_LABELS[iconIndex], compactIconColor(kind));
  }
  showMatrix();
  return true;
}


bool readLocalClock(struct tm &localClock) {
  const time_t now = time(nullptr);
  if (now < 1700000000) return false;
  localtime_r(&now, &localClock);
  return true;
}


bool quietHoursAreActive() {
  if (!quietHoursEnabled || quietHoursStartMinutes == quietHoursEndMinutes) {
    return false;
  }
  struct tm localClock;
  if (!readLocalClock(localClock)) return false;
  const uint16_t currentMinutes = localClock.tm_hour * 60 + localClock.tm_min;
  if (quietHoursStartMinutes < quietHoursEndMinutes) {
    return currentMinutes >= quietHoursStartMinutes &&
           currentMinutes < quietHoursEndMinutes;
  }
  return currentMinutes >= quietHoursStartMinutes ||
         currentMinutes < quietHoursEndMinutes;
}


void drawDisplay() {
  if (otaPreparing) {
    setTransitionPercent(100);
    drawStatus("OTA UPDATE", uiText("BITTE WARTEN", "PLEASE WAIT"));
    return;
  }
  if (otaInProgress) {
    matrix.fillScreen(0);
    showMatrix();
    return;
  }
  if (routeCacheWritePreparing || routeCacheWriteInProgress ||
      settingsWritePreparing || settingsWriteInProgress) {
    resetPageTransition();
    matrix.fillScreen(0);
    showMatrix();
    return;
  }
  if (serviceDisplayTest()) return;
  if (quietHoursAreActive()) {
    resetPageTransition();
    matrix.fillScreen(0);
    showMatrix();
    return;
  }
  if (static_cast<int32_t>(brightnessMessageUntil - millis()) > 0) {
    resetPageTransition();
    drawStatus(brightnessMessageTitle, brightnessMessageValue);
    return;
  }
  if (!displayEnabled) {
    resetPageTransition();
    matrix.fillScreen(0);
    showMatrix();
    return;
  }
  if (!aircraftIsInsideDisplayRadius()) {
    resetPageTransition();
    drawIdleDisplay();
    return;
  }
  if (lastSuccessfulPoll == 0 || millis() - lastSuccessfulPoll > STALE_AFTER_MS) {
    resetPageTransition();
    drawStatus(uiText("DATEN VERALTET", "STALE DATA"), lastError);
    return;
  }

  if (flight.emergency && emergencyPagePending) {
    const uint32_t now = millis();
    activatePage(127, now);
    if (now - currentPageStarted < EMERGENCY_PAGE_DURATION_MS) {
      matrix.fillScreen(0);
      drawEmergencyPage();
      showMatrix();
      return;
    }
    emergencyPagePending = false;
    currentPageSlot = 0;
    currentPageStarted = 0;
    activeRenderedPage = -1;
    transitionPhase = TransitionPhase::Normal;
    setTransitionPercent(100);
  }

  if (!selectEnabledPage()) {
    resetPageTransition();
    matrix.fillScreen(0);
    if (flight.emergency) {
      drawEmergencyPage();
      if (flight.emergencyCount > 1) emergencyAdvanceRequested = true;
    }
    showMatrix();
    return;
  }

  const uint32_t now = millis();
  if (currentPageStarted == 0) currentPageStarted = now;
  const uint8_t page = pageOrder[currentPageSlot];
  uint32_t duration = pageDurationMs[page];
  if (page == 3) duration = max(duration, airportNamesPageDuration());
  if (page == 4) duration = max(duration, customPageScrollDuration());
  if (page == 6) duration = max(duration, compactAirportsMinimumDuration());

  if (transitionPhase == TransitionPhase::Normal &&
      now - currentPageStarted >= duration) {
    if (smoothTransitions) {
      transitionPhase = TransitionPhase::FadeOut;
      transitionStarted = now;
    } else {
      advancePage();
      currentPageStarted = now;
    }
  }
  if (transitionPhase == TransitionPhase::FadeOut) {
    const uint32_t elapsed = now - transitionStarted;
    if (elapsed >= PAGE_TRANSITION_MS) {
      setTransitionPercent(0);
      advancePage();
      transitionPhase = TransitionPhase::FadeIn;
      transitionStarted = now;
    } else {
      setTransitionPercent(100 - elapsed * 100 / PAGE_TRANSITION_MS);
    }
  } else if (transitionPhase == TransitionPhase::FadeIn) {
    const uint32_t elapsed = now - transitionStarted;
    if (elapsed >= PAGE_TRANSITION_MS) {
      setTransitionPercent(100);
      transitionPhase = TransitionPhase::Normal;
    } else {
      setTransitionPercent(elapsed * 100 / PAGE_TRANSITION_MS);
    }
  }

  const uint8_t pageToDraw = pageOrder[currentPageSlot];
  activatePage(pageToDraw, now);
  matrix.fillScreen(0);
  drawConfiguredPage(pageToDraw);
  showMatrix();
}


void applyBrightnessIndex(uint8_t newIndex, bool showMessage) {
  if (newIndex >= BRIGHTNESS_LEVEL_COUNT) return;
  if (newIndex != brightnessIndex) {
    brightnessIndex = newIndex;
    updateColors();
  }
  if (showMessage) {
    brightnessMessageTitle = automaticBrightnessEnabled
                                 ? uiText("AUTO LICHT", "AUTO LIGHT")
                                 : uiText("HELLIGKEIT", "BRIGHTNESS");
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


bool beginEnvironmentSensorAt(TwoWire &bus, uint8_t address, bool onStemma) {
  if (!environmentSensor.begin(address, &bus)) return false;
  environmentSensorAvailable = true;
  environmentSensorOnStemma = onStemma;
  environmentSensorAddress = address;
  environmentSensorFailures = 0;
  Serial.printf("BME280 found at 0x%02X on %s bus\n", address,
                onStemma ? "STEMMA QT" : "BH1750");
  return true;
}


float pressureReducedToSeaLevel(float pressureHpa, float elevationM) {
  const float altitudeFactor = 1.0f - elevationM / 44330.0f;
  if (!isfinite(pressureHpa) || altitudeFactor <= 0.0f) return NAN;
  return pressureHpa / powf(altitudeFactor, 5.255f);
}


bool findEnvironmentSensor() {
  lastEnvironmentSensorProbe = millis();
  const uint8_t addresses[] = {
      BME280_PREFERRED_ADDRESS, BME280_ALTERNATE_ADDRESS};
  for (const uint8_t address : addresses) {
    if (beginEnvironmentSensorAt(Wire, address, false)) return true;
  }
  for (const uint8_t address : addresses) {
    if (beginEnvironmentSensorAt(environmentWire, address, true)) return true;
  }
  environmentSensorAvailable = false;
  environmentSensorAddress = 0;
  ambientTemperatureC = NAN;
  relativeHumidityPercent = NAN;
  stationPressureHpa = NAN;
  airPressureHpa = NAN;
  Serial.println("BME280 not found");
  return false;
}


void serviceEnvironmentSensor() {
  const uint32_t now = millis();
  if (!environmentSensorAvailable) {
    if (now - lastEnvironmentSensorProbe >= ENVIRONMENT_SENSOR_RETRY_MS) {
      findEnvironmentSensor();
    }
    return;
  }
  if (now - lastEnvironmentSensorRead < ENVIRONMENT_SENSOR_INTERVAL_MS) return;
  lastEnvironmentSensorRead = now;

  const float temperature = environmentSensor.readTemperature();
  const float humidity = environmentSensor.readHumidity();
  const float pressure = environmentSensor.readPressure() / 100.0f;
  if (!isfinite(temperature) || !isfinite(humidity) || !isfinite(pressure)) {
    if (++environmentSensorFailures >= ENVIRONMENT_SENSOR_FAILURE_LIMIT) {
      environmentSensorAvailable = false;
      ambientTemperatureC = NAN;
      relativeHumidityPercent = NAN;
      stationPressureHpa = NAN;
      airPressureHpa = NAN;
      Serial.println("BME280 stopped responding");
    }
    return;
  }
  environmentSensorFailures = 0;
  ambientTemperatureC = temperature;
  relativeHumidityPercent = humidity;
  stationPressureHpa = pressure;
  airPressureHpa = pressureReducedToSeaLevel(pressure, observerElevationM);
}


uint8_t automaticBrightnessIndex(float lux) {
  if (lux <= lowLuxThreshold) return 0;
  if (lux >= highLuxThreshold) return BRIGHTNESS_LEVEL_COUNT - 1;

  const float normalized = logf(lux / lowLuxThreshold) /
                           logf(highLuxThreshold / lowLuxThreshold);
  const float scaled = normalized * (BRIGHTNESS_LEVEL_COUNT - 1);
  const uint8_t target = constrain(
      static_cast<int>(roundf(scaled)), 0,
      static_cast<int>(BRIGHTNESS_LEVEL_COUNT - 1));

  // Require a little extra movement across a boundary to avoid flickering.
  if (target > brightnessIndex && scaled < brightnessIndex + 0.65f) {
    return brightnessIndex;
  }
  if (target < brightnessIndex && scaled > brightnessIndex - 0.65f) {
    return brightnessIndex;
  }
  return target;
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
    buttonSettingsWritePending = true;
    if (lightSensorAvailable && !isnan(filteredLux)) {
      applyBrightnessIndex(automaticBrightnessIndex(filteredLux), false);
      brightnessMessageTitle = uiText("AUTO LICHT", "AUTO LIGHT");
      brightnessMessageValue = "AKTIV";
    } else {
      applyBrightnessIndex(manualBrightnessIndex, false);
      brightnessMessageTitle = uiText("MANUELL", "MANUAL");
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
    buttonSettingsWritePending = true;
  }
  if ((events & BUTTON_EVENT_UP) &&
      manualBrightnessIndex + 1 < BRIGHTNESS_LEVEL_COUNT) {
    ++manualBrightnessIndex;
  } else if ((events & BUTTON_EVENT_DOWN) && manualBrightnessIndex > 0) {
    --manualBrightnessIndex;
  }
  buttonSettingsWritePending = true;
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
    const String incomingEmergencyKey = networkFlight.emergency
        ? networkFlight.hex + ":" + networkFlight.squawk : "";
    if (incomingEmergencyKey != displayedEmergencyKey) {
      displayedEmergencyKey = incomingEmergencyKey;
      emergencyPagePending = networkFlight.emergency;
      currentPageSlot = 0;
      currentPageStarted = 0;
      activeRenderedPage = -1;
      transitionPhase = TransitionPhase::Normal;
      setTransitionPercent(100);
    }
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
  configTzTime(
      LOCAL_TIME_ZONE, NTP_SERVER_PRIMARY, NTP_SERVER_SECONDARY);
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


const String &activeAdsbUrl() {
  return activeAdsbSource == 1 ? adsbUrl2 : adsbUrl1;
}


String airlineCode(const String &callsign) {
  if (callsign.startsWith("GAF")) return "GAF";
  if (callsign.startsWith("BPO")) return "BPO";
  if (callsign.length() < 4) return "";
  if (!isAlpha(callsign[0]) || !isAlpha(callsign[1]) ||
      !isAlpha(callsign[2])) return "";
  for (size_t index = 3; index < callsign.length(); ++index) {
    if (isDigit(callsign[index])) return callsign.substring(0, 3);
  }
  return "";
}


String specialTypeDisplayName(String specialType) {
  String category = specialType;
  category.toUpperCase();
  if (category.indexOf("BUNDESPOLIZEI") >= 0) return "Bundespolizei";
  if (category.indexOf("AIRFORCE") >= 0) return "German Airforce";
  if (category.indexOf("MILIT") >= 0) return "Military";
  if (category.indexOf("POLICE") >= 0 ||
      category.indexOf("POLIZEI") >= 0) return "Police";
  if (category.indexOf("GOVERNMENT") >= 0 ||
      category.indexOf("REGIERUNG") >= 0) return "Government";
  if (category.indexOf("HELICOPTER") >= 0 ||
      category.indexOf("HUBSCHRAUBER") >= 0) return "Helicopter";
  if (category.indexOf("AIRSHIP") >= 0 ||
      category.indexOf("LUFTSCHIFF") >= 0 ||
      category.indexOf("SHIP") >= 0) return "Airship";
  if (category.indexOf("A380") >= 0) return "A380";
  return specialType;
}


String verticalMotion(const Aircraft &aircraft) {
  if (!aircraft.hasVerticalRate) return "unbekannt";
  if (aircraft.verticalRateFpm > 64) return "steigt";
  if (aircraft.verticalRateFpm < -64) return "sinkt";
  return "haelt";
}


void appendEmergencyTestAircraft(AdsbResult &result) {
  if (!emergencyTestCount ||
      static_cast<int32_t>(emergencyTestUntil - millis()) <= 0) {
    emergencyTestCount = 0;
    emergencyTestCode = "";
    return;
  }
  for (uint8_t index = 0; index < emergencyTestCount; ++index) {
    Aircraft test;
    test.hex = "TEST" + String(index + 1);
    test.flight = "TEST" + String(index + 1);
    test.type = "A320";
    test.airline = "Notfalltest";
    test.squawk = emergencyTestCode;
    test.emergency = true;
    test.hasPosition = true;
    test.latitude = siteLatitude + 0.03 * (index + 1);
    test.longitude = siteLongitude + 0.02 * (index + 1);
    test.distanceKm = distanceKm(siteLatitude, siteLongitude,
                                 test.latitude, test.longitude);
    test.bearing = bearingDegrees(siteLatitude, siteLongitude,
                                  test.latitude, test.longitude);
    test.direction = compassDirection(test.bearing);
    test.hasAltitude = true;
    test.altitudeFt = 12000 + index * 1000;
    test.hasSpeed = true;
    test.speedKn = 320;
    result.candidates.push_back(test);
  }
}


bool fetchLocalFlight() {
  lastAdsbAttempt = millis();
  if (WiFi.status() != WL_CONNECTED) {
    lastAdsbError = "WLAN GETRENNT";
    setNetworkError("WLAN GETRENNT");
    Serial.println("ADS-B: Wi-Fi is disconnected");
    return false;
  }

  ProcessingConfig config;
  config.latitude = siteLatitude;
  config.longitude = siteLongitude;
  config.maxPositionAgeSeconds = 15.0f;
  config.radiusKm = displayRadiusKm;
  config.hideSportAircraft = hideSportAircraft;
  config.maximumAltitudeFt = altitudeFilterEnabled ? maximumAltitudeFt : 0;
  adsbClient.setUrl(activeAdsbUrl());
  routeClient.configure(routesEnabled, routeUrl);

  AdsbResult result = adsbClient.fetch(config);
  lastReceivedAircraft = result.receivedAircraft;
  lastCandidateAircraft = result.candidates.size();
  lastFeedTimestamp = result.feedTimestamp;
  appendEmergencyTestAircraft(result);
  if (!result.ok && emergencyTestCount) {
    result.ok = true;
    result.error = "";
  }
  if (!result.ok) {
    lastAdsbError = result.error.length() ? result.error : "ADS-B FEHLER";
    setNetworkError(result.error.length() ? result.error : "ADS-B FEHLER");
    Serial.printf("ADS-B error: %s\n", result.error.c_str());
    return false;
  }
  FlightData next;
  Aircraft selected;
  size_t emergencyCount = 0;
  size_t emergencyIndex = 0;
  const bool advanceEmergency = emergencyAdvanceRequested;
  emergencyAdvanceRequested = false;
  if (selectAircraft(result.candidates, config, selected,
                     selectedEmergencyHex, advanceEmergency,
                     emergencyCount, emergencyIndex)) {
    const bool retryRoute = currentRoute.status == "unavailable" &&
                            millis() - lastRouteAttempt >= 60000;
    if (selected.flight != routedFlight || retryRoute) {
      routedFlight = selected.flight;
      lastRouteAttempt = millis();
      if (selected.flight.startsWith("TEST")) currentRoute = {};
      else currentRoute = routeClient.lookup(
          selected.flight,
          selected.hasPosition ? selected.latitude : siteLatitude,
          selected.hasPosition ? selected.longitude : siteLongitude,
          result.feedTimestamp);
      lastRouteError = routeClient.lastError();
    }

    next.available = true;
    next.emergency = selected.emergency;
    next.hasPosition = selected.hasPosition;
    next.hex = selected.hex;
    next.flight = selected.flight.length() ? selected.flight : selected.hex;
    next.squawk = selected.squawk;
    next.emergencyCount = static_cast<uint8_t>(emergencyCount > 255 ? 255 : emergencyCount);
    next.emergencyIndex = selected.emergency
        ? static_cast<uint8_t>(emergencyIndex > 254 ? 255 : emergencyIndex + 1)
        : 0;
    next.specialType = selected.specialTypes.empty()
                           ? "" : selected.specialTypes.front();
    const String callsignAirlineCode = airlineCode(selected.flight);
    String routeAirlineCode = currentRoute.airlineCode;
    routeAirlineCode.trim();
    routeAirlineCode.toUpperCase();
    if (routeAirlineCode == "UNKNOWN") routeAirlineCode = "";
    const String code = callsignAirlineCode.length()
                            ? callsignAirlineCode
                            : routeAirlineCode;
    const String translatedAirline = airlineDisplayName(code);
    if (!code.isEmpty() && translatedAirline.isEmpty())
      rememberUnknownAirline(code);
    String providedAirline = selected.airline;
    providedAirline.trim();
    String providedAirlineComparison = providedAirline;
    providedAirlineComparison.toUpperCase();
    if (selected.flight.startsWith("BPO")) {
      next.airline = "Bundespolizei";
    } else if (selected.flight.startsWith("GAF")) {
      next.airline = "German Airforce";
    } else {
      next.airline = providedAirline.length() &&
                             providedAirlineComparison != "UNKNOWN"
                         ? providedAirline
                         : translatedAirline;
    }
    if (next.airline.isEmpty()) next.airline = code;
    if (next.airline.isEmpty() && next.specialType.length())
      next.airline = specialTypeDisplayName(next.specialType);
    next.type = selected.type;
    next.direction = selected.direction;
    next.verticalMotion = verticalMotion(selected);
    next.motion = selected.motion;
    next.origin = currentRoute.origin;
    next.destination = currentRoute.destination;
    next.routeCodes = currentRoute.routeCodes;
    next.routeNames = currentRoute.routeNames;
    next.originName = currentRoute.originName;
    next.destinationName = currentRoute.destinationName;
    next.distanceKm = selected.hasPosition ? selected.distanceKm : NAN;
    next.altitudeFt = selected.hasAltitude ? selected.altitudeFt : NAN;
    next.speedKmh = selected.hasSpeed ? selected.speedKn * 1.852f : NAN;
    next.cpaKm = selected.hasCpa ? selected.cpaKm : NAN;
    next.cpaMinutes = selected.hasCpaMinutes && selected.cpaMinutes >= 0.0f
                          ? selected.cpaMinutes : NAN;
    next.verticalRateFpm = selected.hasVerticalRate
                               ? selected.verticalRateFpm : NAN;
    next.bearing = selected.hasPosition ? selected.bearing : NAN;
    next.relativeBearing = selected.hasPosition
                               ? relativeBearing(selected.bearing, viewHeading)
                               : NAN;
  } else {
    routedFlight = "";
    currentRoute = {};
  }

  if (xSemaphoreTake(flightMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    networkFlight = next;
    networkLastSuccessfulPoll = millis();
    networkLastError = "";
    xSemaphoreGive(flightMutex);
  }
  lastAdsbSuccess = millis();
  lastAdsbError = "";
  Serial.printf("ADS-B: %u aircraft, %u candidates, selection %s\n",
                static_cast<unsigned>(lastReceivedAircraft),
                static_cast<unsigned>(lastCandidateAircraft),
                next.available ? next.flight.c_str() : "none");
  return true;
}


String checked(bool value) {
  return value ? " checked" : "";
}


String selected(bool value) {
  return value ? " selected" : "";
}


String minutesAsTime(uint16_t minutes) {
  minutes %= 24 * 60;
  char value[6];
  snprintf(value, sizeof(value), "%02u:%02u", minutes / 60, minutes % 60);
  return String(value);
}


bool parseTimeMinutes(const String &value, uint16_t &minutes) {
  if (value.length() != 5 || value[2] != ':' ||
      !isDigit(value[0]) || !isDigit(value[1]) ||
      !isDigit(value[3]) || !isDigit(value[4])) return false;
  const uint8_t hours = value.substring(0, 2).toInt();
  const uint8_t mins = value.substring(3, 5).toInt();
  if (hours > 23 || mins > 59) return false;
  minutes = hours * 60 + mins;
  return true;
}


String htmlEscape(String value) {
  value.replace("&", "&amp;");
  value.replace("\"", "&quot;");
  value.replace("<", "&lt;");
  value.replace(">", "&gt;");
  return value;
}


String pageName(uint8_t page) {
  if (page == 0) return uiText("Flugdaten", "Flight data");
  if (page == 1) return uiText("Airline und Route", "Airline and route");
  if (page == 2) return uiText("CPA und Sondertyp", "CPA and special type");
  if (page == 3) return uiText("Flughafennamen", "Airport names");
  if (page == 4) return uiText("Freitext", "Custom text");
  if (page == 5) return uiText("Kompaktansicht", "Compact view");
  return uiText("Kompakt + Flughäfen", "Compact + airports");
}


String resetReasonText() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON: return uiText("Einschalten", "Power on");
    case ESP_RST_SW: return uiText("Software-Neustart", "Software restart");
    case ESP_RST_PANIC: return uiText("Absturz/Panic", "Crash/panic");
    case ESP_RST_INT_WDT: return "Interrupt-Watchdog";
    case ESP_RST_TASK_WDT: return "Task-Watchdog";
    case ESP_RST_WDT: return "Watchdog";
    case ESP_RST_BROWNOUT: return uiText("Unterspannung", "Brownout");
    case ESP_RST_EXT: return uiText("Externer Reset", "External reset");
    case ESP_RST_DEEPSLEEP: return "Deep-Sleep";
    default: return String(uiText("Unbekannt", "Unknown")) + " (" +
                    String(static_cast<int>(esp_reset_reason())) + ")";
  }
}


String uptimeText() {
  uint32_t seconds = millis() / 1000;
  const uint32_t days = seconds / 86400;
  seconds %= 86400;
  const uint8_t hours = seconds / 3600;
  seconds %= 3600;
  const uint8_t minutes = seconds / 60;
  return String(days) + "d " + String(hours) + "h " + String(minutes) + "m";
}


float chipTemperatureC() {
  return temperatureRead();
}


String jsonEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  value.replace("\r", " ");
  value.replace("\n", " ");
  return value;
}


String unknownCodesText(bool airports) {
  const size_t count = airports ? unknownAirportCount() : unknownAirlineCount();
  String text;
  for (size_t index = 0; index < count; ++index) {
    if (index) text += ", ";
    text += airports ? unknownAirportAt(index) : unknownAirlineAt(index);
  }
  return text.length() ? text : uiText("keine", "none");
}


String unknownCodesJson(bool airports) {
  const size_t count = airports ? unknownAirportCount() : unknownAirlineCount();
  String json = "[";
  for (size_t index = 0; index < count; ++index) {
    if (index) json += ',';
    json += "\"" + jsonEscape(airports ? unknownAirportAt(index)
                                         : unknownAirlineAt(index)) + "\"";
  }
  return json + "]";
}


String ageText(uint32_t timestamp) {
  return timestamp ? String((millis() - timestamp) / 1000) +
                         uiText(" s zuvor", " s ago")
                   : uiText("nie", "never");
}


String numberOrDash(float value, uint8_t decimals = 1) {
  if (isnan(value)) return "-";
  String text(value, static_cast<unsigned int>(decimals));
  text.trim();
  return text;
}


String jsonNumberOrNull(float value, uint8_t decimals = 1) {
  if (isnan(value)) return "null";
  String text(value, static_cast<unsigned int>(decimals));
  text.trim();
  return text;
}


void translateWebPageToEnglish(String &html) {
  if (!uiEnglish) return;
  struct Translation { const char *de; const char *en; };
  static const Translation translations[] = {
    {"lang='de'", "lang='en'"},
    {"lang=\"de\"", "lang=\"en\""},
    {"ADS-B-Adresse Quelle", "ADS-B address source"},
    {"Nur die ausgewählte Quelle wird abgefragt. Quelle 2 darf unkonfiguriert bleiben.", "Only the selected source is queried. Source 2 may remain unconfigured."},
    {"Regen wird für die Anzeige von cm in mm umgerechnet. Ein leeres Passwortfeld behält das gespeicherte Passwort bei.", "Rain is converted from cm to mm for display. An empty password field keeps the saved password."},
    {"Sprache / Language", "Language / Sprache"},
    {"Deutsch", "German"},
    {"Display eingeschaltet", "Display enabled"},
    {"Wetterdaten anzeigen, wenn kein Flugzeug in der Nähe ist", "Show weather data when no aircraft is nearby"},
    {"Anzeigeradius", "Display radius"},
    {"Standorthöhe über NHN", "Site elevation above sea level"},
    {"Ab dieser Entfernung \"weit\"", "Use \"far\" beyond this distance"},
    {"Nachtruhe aktiv", "Night mode enabled"},
    {"Display dunkel ab", "Display off from"},
    {"Display wieder aktiv ab", "Display on again at"},
    {"Die Nachtruhe darf über Mitternacht reichen. Gleiche Zeiten deaktivieren die Abdunklung.", "Night mode may span midnight. Equal times disable it."},
    {"Displaytest beenden", "Stop display test"},
    {"Displaytest starten", "Start display test"},
    {"Etwa 23 Sekunden: Matrix-Prüfbilder und alle Typensymbole mit Bedeutung.", "About 23 seconds: matrix test patterns and all type symbols with their meaning."},
    {"Flugzeugdaten", "Aircraft data"}, {"Aktive ADS-B-Quelle", "Active ADS-B source"},
    {"Quelle 1", "Source 1"}, {"Quelle 2", "Source 2"},
    {"Breitengrad", "Latitude"}, {"Längengrad", "Longitude"}, {"Blickrichtung (Grad)", "Viewing direction (degrees)"},
    {"Sport- und Kleinflugzeuge ausblenden", "Hide sport and light aircraft"},
    {"Flugzeuge oberhalb der Maximalhöhe ausblenden", "Hide aircraft above maximum altitude"},
    {"Maximalhöhe", "Maximum altitude"}, {"Routen über adsb.im ergänzen", "Add routes via adsb.im"},
    {"Routen-Dienst", "Route service"}, {"Routendienst testen", "Test route service"},
    {"ADS-B-Verbindung testen", "Test ADS-B connection"}, {"Notfalltest-Code", "Emergency test code"},
    {"Anzahl Testflugzeuge", "Number of test aircraft"}, {"Notfallanzeige testen", "Test emergency display"},
    {"Test beenden", "Stop test"},
    {"Der Notfalltest läuft höchstens zwei Minuten und verändert keine ADS-B-Daten oder gespeicherten Einstellungen.", "The emergency test runs for no more than two minutes and does not modify ADS-B data or saved settings."},
    {"MQTT-Wetter", "MQTT weather"}, {"Wetterseite in der Ruheanzeige aktivieren", "Enable weather page while idle"},
    {"Benutzer", "User"}, {"Passwort", "Password"}, {"unverändert lassen", "leave unchanged"},
    {"Windrichtung", "Wind direction"}, {"Windgeschwindigkeit", "Wind speed"}, {"Windböen", "Wind gusts"},
    {"Regen 24 h", "Rain 24 h"},
    {"Anzeigeseiten", "Display pages"}, {"Sanfte Übergänge", "Smooth transitions"},
    {"Freitext (maximal 128 Zeichen)", "Custom text (maximum 128 characters)"},
    {"Freitext-Scrollgeschwindigkeit", "Custom text scroll speed"}, {">schnell<", ">fast<"},
    {">mittel<", ">medium<"}, {">langsam<", ">slow<"},
    {"Ein leerer Freitext überspringt die Seite automatisch.", "An empty custom text automatically skips the page."},
    {"Dauer (Sekunden)", "Duration (seconds)"}, {"Reihenfolge:", "Order:"}, {"Position ", "Position "},
    {"Helligkeit", "Brightness"}, {"BH1750-Automatik", "BH1750 automatic mode"},
    {"Manuelle Stufe", "Manual level"}, {"Schwelle dunkel", "Dark threshold"},
    {"Schwelle hell", "Bright threshold"}, {"Einstellungen speichern", "Save settings"},
    {"wird automatisch aktualisiert", "updates automatically"}, {"System eingeschränkt", "System degraded"},
    {"MQTT aus", "MQTT off"}, {"MQTT getrennt", "MQTT disconnected"}, {"Sensoren OK", "Sensors OK"},
    {"Sensor prüfen", "Check sensor"}, {"Netzwerk", "Network"}, {"Adresse", "Address"},
    {"WLAN-Neuverbindungen", "Wi-Fi reconnects"}, {"Webserver-Starts", "Web server starts"},
    {"Letzte WLAN-Unterbrechung", "Last Wi-Fi interruption"}, {"Aktuelles Flugzeug", "Current aircraft"},
    {"<span>Flug</span>", "<span>Flight</span>"}, {"Notfallauswahl", "Emergency selection"},
    {"Airline / Typ", "Airline / type"}, {"Entfernung / Höhe", "Distance / altitude"},
    {"Radius normal / aktuell", "Normal / current radius"},
    {"ADS-B und Route", "ADS-B and route"}, {"ADS-B-Fehler", "ADS-B error"},
    {"Routenfehler", "Route error"}, {"Routencache wartet auf Flash", "Route cache pending flash write"},
    {"Empfangen / Kandidaten", "Received / candidates"}, {"Letzter Erfolg", "Last success"},
    {"Vertikalbewegung", "Vertical motion"}, {"Routenstatus", "Route status"}, {"Letzter ADS-B-Erfolg", "Last ADS-B success"},
    {"Konfiguration", "Configuration"}, {"Standort / Blickrichtung", "Location / viewing direction"},
    {"Höhenfilter", "Altitude filter"}, {"Kleinflugzeugfilter", "Light aircraft filter"},
    {"Wetteranzeige", "Weather display"}, {"Übersetzungstabellen", "Translation tables"},
    {"Unbekannte Flughäfen", "Unknown airports"}, {"Unbekannte Airlines", "Unknown airlines"},
    {"Technische Diagnose", "Technical diagnostics"}, {"Diagnose", "Diagnostics"},
    {"ADS-B-Quelle", "ADS-B source"}, {"Letzter ADS-B-Versuch", "Last ADS-B attempt"},
    {"Letzter Versuch", "Last attempt"}, {"Anzeige aktualisiert", "Display updated"},
    {"ADS-B-Zeitstempel", "ADS-B timestamp"}, {"Erstellt", "Built"},
    {"Laufzeit", "Uptime"}, {"Freier RAM", "Free RAM"},
    {"ESP32-S3-Chiptemperatur", "ESP32-S3 chip temperature"},
    {"Letzte Display-Aktualisierung", "Last display update"}, {"Empfangene Flugzeuge", "Received aircraft"},
    {"Geeignete Flugzeuge", "Eligible aircraft"}, {"Wetter und Sensoren", "Weather and sensors"},
    {"Umgebungstemperatur", "Ambient temperature"}, {"Relative Feuchte", "Relative humidity"},
    {"Luftdruck auf Meereshöhe", "Sea-level pressure"}, {"Stationsdruck (BME280 roh)", "Station pressure (raw BME280)"},
    {"Wetterdaten-Ruheanzeige", "Idle weather display"}, {"Letzte MQTT-Nachricht", "Last MQTT message"},
    {"MQTT-Server", "MQTT server"}, {"MQTT-Fehler", "MQTT error"}, {"Wind / Richtung", "Wind / direction"},
    {"Matrix-Farbtiefe", "Matrix color depth"}, {"Matrix-Refresh", "Matrix refresh"},
    {"Bildübergabe", "Frame handoff"}, {"Blickgeometrie", "Viewing geometry"},
    {"Lokale Zeit", "Local time"}, {"Nachtruhe", "Night mode"},
    {"Task- und Resetdiagnose", "Task and reset diagnostics"}, {"Netz Core", "Network core"},
    {"Stackreserve", "Stack reserve"}, {"Letzter Reset", "Last reset"},
    {"ESP wirklich neu starten?", "Really restart the ESP?"}, {"ESP neu starten", "Restart ESP"},
    {"Web-Display öffnen", "Open web display"}, {"Firmware drahtlos aktualisieren", "Update firmware wirelessly"},
    {"UP/DOWN: manuell; beide Tasten: Automatik.", "UP/DOWN: manual; both buttons: automatic."},
    {"Installierte Version", "Installed version"},
    {"PlatformIO-Datei", "PlatformIO file"}, {"auswählen", "select"},
    {"Während des Updates die Stromversorgung nicht trennen.", "Do not disconnect power during the update."},
    {"Prüfen und installieren", "Verify and install"},
    {"Bitte eine .bin-Firmware auswählen.", "Please select a .bin firmware file."},
    {"Ungültige ESP32-Firmware: Dateikopf fehlt.", "Invalid ESP32 firmware: missing image header."},
    {"wirklich installieren?", "really install?"},
    {"Netzwerkfehler während des Uploads.", "Network error during upload."},
    {"Ungültige Antwort des ESP.", "Invalid response from the ESP."},
    {"Upload läuft…", "Upload in progress…"}, {"Warte auf Neustart…", "Waiting for restart…"},
    {"ESP nicht wieder erreichbar.", "ESP did not become reachable again."},
    {"Update abgeschlossen. Version vorher:", "Update complete. Previous version:"}, {", danach:", ", new version:"},
    {"Zurück", "Back"},
    {"aktualisiert ", "updated "}, {"Aktualisierung fehlgeschlagen", "Update failed"},
    {">verbunden /", ">connected /"}, {">verbunden<", ">connected<"},
    {">nicht gefunden<", ">not found<"}, {">getrennt<", ">disconnected<"},
    {">keiner<", ">none<"}, {">keine<", ">none<"}, {">aktiv<", ">enabled<"}, {">inaktiv<", ">inactive<"},
    {">ja<", ">yes<"}, {">nein<", ">no<"}, {">bis ", ">up to "},
    {"% / Automatik", "% / automatic"}, {">Automatik<", ">automatic<"},
    {">manuell<", ">manual<"}, {">aus<", ">off<"}, {">warte auf NTP<", ">waiting for NTP<"},
    {" m NHN / weit ab ", " m ASL / far beyond "}, {"Netz Core 0 / Display Core 1", "Network core 0 / display core 1"},
    {" / aktiv", " / enabled"}, {" / inaktiv", " / inactive"},
    {"toLocaleTimeString('de-DE')", "toLocaleTimeString('en-GB')"}
  };
  for (const auto &translation : translations) html.replace(translation.de, translation.en);
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
  String statusWindDirection;
  float statusWindSpeed = NAN;
  float statusWindGust = NAN;
  float statusRain = NAN;
  if (xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    statusWindDirection = windDirection;
    statusWindSpeed = windSpeedKph;
    statusWindGust = windGustKph;
    statusRain = rain24Mm;
    xSemaphoreGive(weatherMutex);
  }

  String html;
  html.reserve(9000);
  html += F("<!doctype html><html lang='de'><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>FlightWall MatrixPortal</title><style>");
  html += F("body{font:16px system-ui;background:#10131a;color:#eef2ff;margin:auto;max-width:920px;padding:20px}");
  html += F("h1{font-size:1.5rem}fieldset{border:1px solid #445;border-radius:10px;margin:14px 0;padding:14px}");
  html += F("label{display:block;margin:12px 0}input[type=number],input[type=text],input[type=url],select{width:100%;box-sizing:border-box;padding:9px}");
  html += F("button{background:#00bcd4;color:#001018;border:0;border-radius:8px;padding:12px 18px;font-weight:bold}");
  html += F(".status{display:grid;grid-template-columns:minmax(110px,auto) 1fr;gap:7px 14px}.status span{color:#aab}.status b{overflow-wrap:anywhere}.statusCards{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}.statusCard{background:#171c26;border:1px solid #343d50;border-radius:12px;padding:14px}.statusCard h3{margin:0 0 12px;font-size:1rem;color:#7ee7f0}.healthbar{display:flex;flex-wrap:wrap;gap:8px;margin:2px 0 14px}.pill{border-radius:999px;padding:7px 11px;font-size:.86rem;font-weight:700}.ok{background:#153b2b;color:#74e3aa}.warn{background:#493c16;color:#ffd166}.bad{background:#4a1f29;color:#ff8597}.off{background:#303541;color:#abb3c3}.wide{grid-column:1/-1}details summary{cursor:pointer;font-weight:700;color:#7ee7f0}details .status{margin-top:12px}@media(max-width:650px){body{padding:12px}.statusCards{grid-template-columns:1fr}.wide{grid-column:auto}.status{grid-template-columns:1fr}.status b{margin-bottom:7px}}.muted{color:#aab}</style></head><body>");
  html += F("<h1>FlightWall MatrixPortal ");
  html += APP_VERSION;
  html += F("</h1><form method='post' action='/save'>");
  html += F("<fieldset><legend>Sprache / Language</legend><label><select name='uiLanguage'>"
            "<option value='de'");
  html += selected(!uiEnglish);
  html += F(">Deutsch</option><option value='en'");
  html += selected(uiEnglish);
  html += F(">English</option></select></label></fieldset>");
  html += F("<fieldset><legend>Display</legend><label><input type='checkbox' name='display' value='1'");
  html += checked(displayEnabled);
  html += F("> Display eingeschaltet</label><label><input type='checkbox' name='environmentIdle' value='1'");
  html += checked(environmentIdleDisplayEnabled);
  html += F("> Wetterdaten anzeigen, wenn kein Flugzeug in der Nähe ist</label><label>Anzeigeradius (km)<input type='number' name='radius' min='1' max='100' step='0.5' value='");
  html += String(displayRadiusKm, 1);
  html += F("'></label><label>Standorthöhe über NHN (m)<input type='number' name='observerElevation' min='-500' max='9000' step='1' value='");
  html += String(observerElevationM, 0);
  html += F("'></label><label>Ab dieser Entfernung \"weit\" (km)<input type='number' name='farDistance' min='0.5' max='100' step='0.1' value='");
  html += String(farDirectionDistanceKm, 1);
  html += F("'></label><label><input type='checkbox' name='quietHours' value='1'");
  html += checked(quietHoursEnabled);
  html += F("> Nachtruhe aktiv</label><label>Display dunkel ab<input type='time' name='quietStart' value='");
  html += minutesAsTime(quietHoursStartMinutes);
  html += F("'></label><label>Display wieder aktiv ab<input type='time' name='quietEnd' value='");
  html += minutesAsTime(quietHoursEndMinutes);
  html += F("'></label><p class='muted'>Die Nachtruhe darf über Mitternacht reichen. Gleiche Zeiten deaktivieren die Abdunklung.</p><button type='submit' formaction='/display-test'>");
  html += (displayTestActive || displayTestCommand == 1) ?
      "Displaytest beenden" : "Displaytest starten";
  html += F("</button><p class='muted'>Etwa 23 Sekunden: Matrix-Prüfbilder und alle Typensymbole mit Bedeutung.</p></fieldset><fieldset><legend>Flugzeugdaten</legend><label>Aktive ADS-B-Quelle<select name='activeAdsbSource'><option value='1'");
  html += selected(activeAdsbSource == 0);
  html += F(">Quelle 1</option><option value='2'");
  html += selected(activeAdsbSource == 1);
  html += F(">Quelle 2</option></select></label><label>ADS-B-Adresse Quelle 1<input type='url' name='adsbUrl1' maxlength='180' value='");
  html += htmlEscape(adsbUrl1);
  html += F("'></label><label>ADS-B-Adresse Quelle 2<input type='url' name='adsbUrl2' maxlength='180' value='");
  html += htmlEscape(adsbUrl2);
  html += F("' placeholder='http://zweiter-empfaenger/data/aircraft.json'></label><p class='muted'>Nur die ausgewählte Quelle wird abgefragt. Quelle 2 darf unkonfiguriert bleiben.</p><label>Breitengrad<input type='number' name='siteLat' min='-90' max='90' step='0.000001' value='");
  html += String(siteLatitude, 6);
  html += F("'></label><label>Längengrad<input type='number' name='siteLon' min='-180' max='180' step='0.000001' value='");
  html += String(siteLongitude, 6);
  html += F("'></label><label>Blickrichtung (Grad)<input type='number' name='viewHeading' min='0' max='359' step='1' value='");
  html += String(viewHeading, 0);
  html += F("'></label><label><input type='checkbox' name='hideSport' value='1'");
  html += checked(hideSportAircraft);
  html += F("> Sport- und Kleinflugzeuge ausblenden</label><label><input type='checkbox' name='altitudeFilter' value='1'");
  html += checked(altitudeFilterEnabled);
  html += F("> Flugzeuge oberhalb der Maximalhöhe ausblenden</label><label>Maximalhöhe (ft)<input type='number' name='maximumAltitude' min='1000' max='100000' step='500' value='");
  html += String(maximumAltitudeFt);
  html += F("'></label><label><input type='checkbox' name='routesEnabled' value='1'");
  html += checked(routesEnabled);
  html += F("> Routen über adsb.im ergänzen</label><label>Routen-Dienst<input type='url' name='routeUrl' maxlength='180' value='");
  html += htmlEscape(routeUrl);
  html += F("'></label><label>Test-Callsign<input type='text' name='routeTestCallsign' maxlength='10' value='DLH123'></label>"
            "<button type='submit' formaction='/test-route'>Routendienst testen</button> "
            "<button type='submit' formaction='/test-adsb'>ADS-B-Verbindung testen</button>"
            "<hr><label>Notfalltest-Code<select name='emergencyTestCode'>"
            "<option value='7700'>7700 Emergency</option>"
            "<option value='7600'>7600 Radio</option>"
            "<option value='7500'>7500 Hijack</option></select></label>"
            "<label>Anzahl Testflugzeuge<select name='emergencyTestCount'>"
            "<option value='1'>1</option><option value='2'>2</option>"
            "<option value='3'>3</option></select></label>"
            "<button type='submit' formaction='/test-emergency'>Notfallanzeige testen</button> "
            "<button type='submit' formaction='/stop-emergency-test'>Test beenden</button>"
            "<p class='muted'>Der Notfalltest läuft höchstens zwei Minuten und verändert keine ADS-B-Daten oder gespeicherten Einstellungen.</p></fieldset>");
  html += F("<fieldset><legend>MQTT-Wetter</legend><label><input type='checkbox' name='mqttWeather' value='1'");
  html += checked(mqttWeatherEnabled);
  html += F("> Wetterseite in der Ruheanzeige aktivieren</label><label>Mosquitto-Server<input type='text' name='mqttHost' maxlength='96' value='");
  html += htmlEscape(mqttHost);
  html += F("'></label><label>Port<input type='number' name='mqttPort' min='1' max='65535' value='");
  html += String(mqttPort);
  html += F("'></label><label>Benutzer<input type='text' name='mqttUser' maxlength='64' value='");
  html += htmlEscape(mqttUser);
  html += F("'></label><label>Passwort<input type='password' name='mqttPassword' maxlength='96' value='' placeholder='unverändert lassen'></label>");
  html += F("<label>Windrichtung<input type='text' name='windDirectionTopic' maxlength='128' value='");
  html += htmlEscape(windDirectionTopic);
  html += F("'></label><label>Windgeschwindigkeit (km/h)<input type='text' name='windSpeedTopic' maxlength='128' value='");
  html += htmlEscape(windSpeedTopic);
  html += F("'></label><label>Windböen (km/h)<input type='text' name='windGustTopic' maxlength='128' value='");
  html += htmlEscape(windGustTopic);
  html += F("'></label><label>Regen 24 h (cm)<input type='text' name='rainTopic' maxlength='128' value='");
  html += htmlEscape(rainTopic);
  html += F("'></label><p class='muted'>Regen wird für die Anzeige von cm in mm umgerechnet. Ein leeres Passwortfeld behält das gespeicherte Passwort bei.</p></fieldset>");
  html += F("<fieldset><legend>Anzeigeseiten</legend><label><input type='checkbox' name='smooth' value='1'");
  html += checked(smoothTransitions);
  html += F("> Sanfte Übergänge</label><label>Freitext (maximal 128 Zeichen)<textarea name='customText' maxlength='128' rows='3' style='width:100%;box-sizing:border-box;padding:9px'>");
  html += htmlEscape(customPageText);
  html += F("</textarea></label><label>Freitext-Scrollgeschwindigkeit<select name='customScrollSpeed'><option value='2'");
  html += selected(customScrollSpeed == 2);
  html += F(">schnell</option><option value='1'");
  html += selected(customScrollSpeed == 1);
  html += F(">mittel</option><option value='0'");
  html += selected(customScrollSpeed == 0);
  html += F(">langsam</option></select></label><p class='muted'>Ein leerer Freitext überspringt die Seite automatisch.</p>");
  for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
    html += "<label><input type='checkbox' name='page" + String(page) + "On' value='1'";
    html += checked(pageEnabled[page]);
    html += "> " + pageName(page) + "</label><label>Dauer (Sekunden)<input type='number' name='page";
    html += String(page) + "Sec' min='2' max='60' value='" + String(pageDurationMs[page] / 1000) + "'></label>";
  }
  html += F("<p class='muted'>Reihenfolge:</p>");
  for (uint8_t slot = 0; slot < PAGE_COUNT; ++slot) {
    html += "<label>Position " + String(slot + 1) + "<select name='order" + String(slot) + "'>";
    for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
      html += "<option value='" + String(page) + "'" +
              selected(pageOrder[slot] == page) + ">" + pageName(page) + "</option>";
    }
    html += "</select></label>";
  }
  html += F("</fieldset><fieldset><legend>Helligkeit</legend>");
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
  html += F("<fieldset><legend>Status <small class='muted' id='statusRefresh'>wird automatisch aktualisiert</small></legend><div id='statusData'><div class='healthbar'>");
  html += statusError.length() ? F("<span class='pill warn'>● System eingeschränkt</span>") : F("<span class='pill ok'>● System OK</span>");
  html += lastAdsbError.length() ? F("<span class='pill bad'>● ADS-B Fehler</span>") : F("<span class='pill ok'>● ADS-B OK</span>");
  if (statusFlight.emergency)
    html += F("<span class='pill bad'>● TRANSPONDER-NOTFALL</span>");
  html += !mqttWeatherEnabled ? F("<span class='pill off'>● MQTT aus</span>") :
          (mqttConnected ? F("<span class='pill ok'>● MQTT OK</span>") : F("<span class='pill warn'>● MQTT getrennt</span>"));
  html += environmentSensorAvailable && lightSensorAvailable
              ? F("<span class='pill ok'>● Sensoren OK</span>")
              : F("<span class='pill warn'>● Sensor prüfen</span>");
  html += F("</div><div class='statusCards'><section class='statusCard'><h3>Netzwerk</h3><div class='status'><span>Adresse</span><b>http://matrixportal.local</b><span>IP</span><b>");
  html += WiFi.localIP().toString();
  html += F("</b><span>WLAN</span><b>");
  html += String(WiFi.RSSI()) + " dBm";
  html += F("</b><span>WLAN-Neuverbindungen</span><b>");
  html += String(wifiReconnectCount);
  html += F("</b><span>Webserver-Starts</span><b>");
  html += String(webServerStartCount);
  html += F("</b><span>Letzte WLAN-Unterbrechung</span><b>");
  html += lastWifiDisconnect ? ageText(lastWifiDisconnect) : "keine";
  html += F("</b><span>BH1750</span><b>");
  html += lightSensorAvailable ? "verbunden" : "nicht gefunden";
  html += F("</b><span>Lux</span><b>");
  html += isnan(filteredLux) ? "-" : String(filteredLux, 1);
  html += F("</b></div></section><section class='statusCard'><h3>Aktuelles Flugzeug</h3><div class='status'><span>Flug</span><b>");
  html += statusFlight.available ? displayAscii(statusFlight.flight) : "keiner";
  html += F("</b><span>Transponder</span><b>");
  html += statusFlight.squawk.length()
              ? htmlEscape(statusFlight.squawk + " " +
                  (statusFlight.emergency ? emergencyMeaning(statusFlight.squawk) : ""))
              : "-";
  html += F("</b><span>Notfallauswahl</span><b>");
  html += statusFlight.emergency
              ? String(statusFlight.emergencyIndex) + " / " +
                    String(statusFlight.emergencyCount)
              : "keine";
  html += F("</b><span>Airline / Typ</span><b>");
  html += statusFlight.available
              ? htmlEscape(displayAscii(statusFlight.airline) + " / " +
                           displayAscii(statusFlight.type))
              : "-";
  html += F("</b><span>Route</span><b>");
  html += statusFlight.available &&
                  (statusFlight.origin.length() || statusFlight.destination.length())
              ? htmlEscape(statusFlight.routeCodes.length()
                               ? statusFlight.routeCodes
                               : statusFlight.origin + " → " + statusFlight.destination)
              : "-";
  html += F("</b><span>Entfernung / Höhe</span><b>");
  html += statusFlight.available
              ? numberOrDash(statusFlight.distanceKm) + " km / " +
                    numberOrDash(statusFlight.altitudeFt, 0) + " ft"
              : "-";
  html += F("</b><span>CPA</span><b>");
  html += statusFlight.available
              ? numberOrDash(statusFlight.cpaKm) + " km / " +
                    numberOrDash(statusFlight.cpaMinutes, 0) + " min"
              : "-";
  html += F("</b><span>Special Type</span><b>");
  html += statusFlight.specialType.length()
              ? htmlEscape(statusFlight.specialType) : "nein";
  html += F("</b><span>Radius normal / aktuell</span><b>");
  html += statusFlight.emergency
              ? String(displayRadiusKm, 1) + " km / unbegrenzt"
              : String(displayRadiusKm, 1) + " / " +
                    String(statusFlight.specialType.length()
                               ? displayRadiusKm * 2.0f : displayRadiusKm, 1) + " km";
  html += F("</b></div></section><section class='statusCard'><h3>ADS-B und Route</h3><div class='status'><span>ADS-B</span><b>");
  html += statusError.length() ? displayAscii(statusError) : "OK";
  html += F("</b><span>ADS-B-Fehler</span><b>");
  html += lastAdsbError.length() ? htmlEscape(lastAdsbError) : "keiner";
  html += F("</b><span>Routenfehler</span><b>");
  html += lastRouteError.length() ? htmlEscape(lastRouteError) : "keiner";
  html += F("</b><span>Routenstatus</span><b>");
  html += htmlEscape(currentRoute.status);
  html += F("</b><span>Routencache wartet auf Flash</span><b>");
  html += routeClient.hasPendingWrites() ? "ja" : "nein";
  html += F("</b><span>Aktive ADS-B-Quelle</span><b>");
  html += "Quelle " + String(activeAdsbSource + 1);
  html += F("</b><span>Empfangen / Kandidaten</span><b>");
  html += String(lastReceivedAircraft) + " / " + String(lastCandidateAircraft);
  html += F("</b><span>Letzter Erfolg</span><b>");
  html += ageText(lastAdsbSuccess);
  html += F("</b></div></section><section class='statusCard'><h3>Konfiguration</h3><div class='status'><span>Standort / Blickrichtung</span><b>");
  html += String(siteLatitude, 6) + " / " + String(siteLongitude, 6) +
          " / " + String(viewHeading, 0) + "°";
  html += F("</b><span>Anzeigeradius</span><b>");
  html += String(displayRadiusKm, 1) + " km";
  html += F("</b><span>Höhenfilter</span><b>");
  html += altitudeFilterEnabled ? "bis " + String(maximumAltitudeFt) + " ft" : "aus";
  html += F("</b><span>Kleinflugzeugfilter</span><b>");
  html += hideSportAircraft ? "aktiv" : "aus";
  html += F("</b><span>Wetteranzeige</span><b>");
  html += environmentIdleDisplayEnabled ? "aktiv" : "aus";
  html += F("</b></div></section><section class='statusCard wide'><h3>Übersetzungstabellen</h3><div class='status'><span>Unbekannte Flughäfen</span><b>");
  html += unknownCodesText(true);
  html += F("</b><span>Unbekannte Airlines</span><b>");
  html += unknownCodesText(false);
  html += F("</b></div></section><details id='technicalDiagnosis' class='statusCard wide'><summary>Technische Diagnose</summary><div class='status'><span>ADS-B-Quelle 1</span><b>");
  html += htmlEscape(adsbUrl1);
  html += F("</b><span>ADS-B-Quelle 2</span><b>");
  html += adsbUrl2.length() ? htmlEscape(adsbUrl2) : "nicht konfiguriert";
  html += F("</b><span>Letzter Versuch</span><b>");
  html += ageText(lastAdsbAttempt);
  html += F("</b><span>Anzeige aktualisiert</span><b>");
  html += ageText(statusUpdated);
  html += F("</b><span>ADS-B-Zeitstempel</span><b>");
  html += lastFeedTimestamp ? String(lastFeedTimestamp) : "-";
  html += F("</b><span>Firmware</span><b>");
  html += APP_VERSION;
  html += F("</b><span>Erstellt</span><b>");
  html += BUILD_TIMESTAMP;
  html += F("</b><span>Laufzeit</span><b>");
  html += uptimeText();
  html += F("</b><span>Freier RAM</span><b>");
  html += String(ESP.getFreeHeap() / 1024) + " KiB (Minimum " +
          String(ESP.getMinFreeHeap() / 1024) + " KiB)";
  html += F("</b><span>ESP32-S3-Chiptemperatur</span><b>");
  html += numberOrDash(chipTemperatureC()) + " °C";
  html += F("</b></div></details><section class='statusCard'><h3>Wetter und Sensoren</h3><div class='status'><span>BME280</span><b>");
  if (environmentSensorAvailable) {
    html += "verbunden / 0x" + String(environmentSensorAddress, HEX) +
            (environmentSensorOnStemma ? " / STEMMA QT" : " / GPIO 3/9");
  } else {
    html += "nicht gefunden";
  }
  html += F("</b><span>Umgebungstemperatur</span><b>");
  html += numberOrDash(ambientTemperatureC) + " °C";
  html += F("</b><span>Relative Feuchte</span><b>");
  html += numberOrDash(relativeHumidityPercent) + " %";
  html += F("</b><span>Luftdruck auf Meereshöhe</span><b>");
  html += numberOrDash(airPressureHpa) + " hPa";
  html += F("</b><span>Stationsdruck (BME280 roh)</span><b>");
  html += numberOrDash(stationPressureHpa) + " hPa";
  html += F("</b><span>Wetterdaten-Ruheanzeige</span><b>");
  html += environmentIdleDisplayEnabled ? "aktiv" : "aus";
  html += F("</b><span>MQTT</span><b>");
  html += !mqttWeatherEnabled ? "aus" : (mqttConnected ? "verbunden" : "getrennt");
  html += F("</b><span>MQTT-Server</span><b>");
  html += htmlEscape(mqttHost) + ":" + String(mqttPort);
  html += F("</b><span>Letzte MQTT-Nachricht</span><b>");
  html += ageText(lastMqttMessage);
  html += F("</b><span>MQTT-Fehler</span><b>");
  html += mqttLastError.length() ? htmlEscape(mqttLastError) : "keiner";
  html += F("</b><span>Wind / Richtung</span><b>");
  html += numberOrDash(statusWindSpeed) + " km/h / " +
          (statusWindDirection.length() ? htmlEscape(statusWindDirection) : "-");
  html += F("</b><span>Windböen</span><b>");
  html += numberOrDash(statusWindGust) + " km/h";
  html += F("</b><span>Regen 24 h</span><b>");
  html += numberOrDash(statusRain) + " mm";
  html += F("</b></div></section><section class='statusCard'><h3>Display</h3><div class='status'><span>Helligkeit</span><b>");
  html += String(BRIGHTNESS_LEVELS[brightnessIndex]) + "% / " +
          (automaticBrightnessEnabled ? "Automatik" : "manuell");
  html += F("</b><span>Matrix-Farbtiefe</span><b>");
  html += String(BIT_DEPTH) + " Bit";
  html += F("</b><span>Matrix-Refresh</span><b>");
  html += String(matrixRefreshHz) + " Hz";
  html += F("</b><span>Bildübergabe</span><b>");
  html += String(matrixShowLastUs) + " / max. " +
          String(matrixShowMaxUs) + " µs";
  html += F("</b><span>Blickgeometrie</span><b>");
  html += String(observerElevationM, 0) + " m NHN / weit ab " +
          String(farDirectionDistanceKm, 1) + " km";
  struct tm localClock;
  html += F("</b><span>Lokale Zeit</span><b>");
  if (readLocalClock(localClock)) {
    char clockText[20];
    strftime(clockText, sizeof(clockText), "%d.%m.%Y %H:%M", &localClock);
    html += clockText;
  } else {
    html += "warte auf NTP";
  }
  html += F("</b><span>Nachtruhe</span><b>");
  html += quietHoursEnabled ?
      (minutesAsTime(quietHoursStartMinutes) + "–" +
       minutesAsTime(quietHoursEndMinutes) +
       (quietHoursAreActive() ? " / aktiv" : " / inaktiv")) : "aus";
  html += F("</b></div></section><details id='taskDiagnosis' class='statusCard wide'><summary>Task- und Resetdiagnose</summary><div class='status'><span>Tasks</span><b>Netz Core 0 / Display Core 1</b><span>Stackreserve</span><b>");
  html += networkTaskHandle ? String(uxTaskGetStackHighWaterMark(networkTaskHandle) * sizeof(StackType_t)) : "-";
  html += " / ";
  html += displayTaskHandle ? String(uxTaskGetStackHighWaterMark(displayTaskHandle) * sizeof(StackType_t)) : "-";
  html += F(" Bytes</b><span>Letzter Reset</span><b>");
  html += resetReasonText();
  html += F("</b></div></details></div></div><form method='post' action='/restart' onsubmit=\"return confirm('ESP wirklich neu starten?')\"><p><button type='submit'>ESP neu starten</button></p></form></fieldset>");
  html += F("<p><a href='/display'>Web-Display öffnen</a></p>");
  if (strlen(OTA_PASSWORD) >= 8) {
    html += F("<p><a href='/update'>Firmware drahtlos aktualisieren</a></p>");
  }
  html += F("<p class='muted'>UP/DOWN: manuell; beide Tasten: Automatik.</p>"
            "<script>let statusBusy=false;async function refreshStatus(){"
            "if(statusBusy||document.hidden)return;statusBusy=true;"
            "const note=document.getElementById('statusRefresh');"
            "const current=document.getElementById('statusData');"
            "const openDetails=current?[...current.querySelectorAll('details[id][open]')].map(item=>item.id):[];"
            "try{const response=await fetch('/',{cache:'no-store'});if(!response.ok)throw 0;"
            "const page=new DOMParser().parseFromString(await response.text(),'text/html');"
            "const fresh=page.getElementById('statusData');"
            "if(fresh&&current){current.innerHTML=fresh.innerHTML;openDetails.forEach(id=>{const item=document.getElementById(id);if(item)item.open=true;});}"
            "if(note)note.textContent='aktualisiert '+new Date().toLocaleTimeString('de-DE');"
            "}catch(error){if(note)note.textContent='Aktualisierung fehlgeschlagen';}"
            "finally{statusBusy=false;}}setInterval(refreshStatus,5000);</script></body></html>");
  translateWebPageToEnglish(html);
  return html;
}


bool validFlightWallHost(const String &host) {
  if (!host.length()) return false;
  for (size_t i = 0; i < host.length(); ++i) {
    const char c = host[i];
    if (!isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '-' && c != '_') {
      return false;
    }
  }
  return true;
}


void applyWebSettings() {
  settingsWritePreparing = true;
  vTaskDelay(pdMS_TO_TICKS(120));
  settingsWritePreparing = false;
  settingsWriteInProgress = true;
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
    uiEnglish = webServer.arg("uiLanguage") == "en";
    displayEnabled = webServer.hasArg("display");
    environmentIdleDisplayEnabled = webServer.hasArg("environmentIdle");
    mqttWeatherEnabled = webServer.hasArg("mqttWeather");
    quietHoursEnabled = webServer.hasArg("quietHours");
    uint16_t requestedQuietTime;
    if (parseTimeMinutes(webServer.arg("quietStart"), requestedQuietTime)) {
      quietHoursStartMinutes = requestedQuietTime;
    }
    if (parseTimeMinutes(webServer.arg("quietEnd"), requestedQuietTime)) {
      quietHoursEndMinutes = requestedQuietTime;
    }
    automaticBrightnessEnabled = webServer.hasArg("autoLight");
    displayRadiusKm = constrain(webServer.arg("radius").toFloat(), 1.0f, 100.0f);
    observerElevationM = constrain(
        webServer.arg("observerElevation").toFloat(), -500.0f, 9000.0f);
    farDirectionDistanceKm = constrain(
        webServer.arg("farDistance").toFloat(), 0.5f, 100.0f);
    lowLuxThreshold = constrain(webServer.arg("lowLux").toFloat(), 1.0f, 500.0f);
    highLuxThreshold = constrain(webServer.arg("highLux").toFloat(), 50.0f, 50000.0f);
    if (highLuxThreshold <= lowLuxThreshold) highLuxThreshold = lowLuxThreshold + 50.0f;
    customPageText = displayAscii(webServer.arg("customText"));
    customPageText.trim();
    if (customPageText.length() > 128) customPageText.remove(128);
    const int requestedCustomScrollSpeed =
        webServer.arg("customScrollSpeed").toInt();
    customScrollSpeed = requestedCustomScrollSpeed >= 0 &&
                                requestedCustomScrollSpeed <= 2
                            ? requestedCustomScrollSpeed
                            : 1;
    String requestedAdsbUrl1 = webServer.arg("adsbUrl1");
    requestedAdsbUrl1.trim();
    if (requestedAdsbUrl1.startsWith("http://") ||
        requestedAdsbUrl1.startsWith("https://")) adsbUrl1 = requestedAdsbUrl1;
    String requestedAdsbUrl2 = webServer.arg("adsbUrl2");
    requestedAdsbUrl2.trim();
    if (requestedAdsbUrl2.isEmpty() || requestedAdsbUrl2.startsWith("http://") ||
        requestedAdsbUrl2.startsWith("https://")) adsbUrl2 = requestedAdsbUrl2;
    const uint8_t requestedAdsbSource =
        webServer.arg("activeAdsbSource").toInt() == 2 ? 1 : 0;
    activeAdsbSource = requestedAdsbSource == 1 && adsbUrl2.length()
                           ? 1 : 0;
    String requestedRouteUrl = webServer.arg("routeUrl");
    requestedRouteUrl.trim();
    if (requestedRouteUrl.startsWith("http://") ||
        requestedRouteUrl.startsWith("https://")) routeUrl = requestedRouteUrl;
    siteLatitude = constrain(webServer.arg("siteLat").toDouble(), -90.0, 90.0);
    siteLongitude = constrain(webServer.arg("siteLon").toDouble(), -180.0, 180.0);
    viewHeading = constrain(webServer.arg("viewHeading").toFloat(), 0.0f, 359.0f);
    routesEnabled = webServer.hasArg("routesEnabled");
    hideSportAircraft = webServer.hasArg("hideSport");
    altitudeFilterEnabled = webServer.hasArg("altitudeFilter");
    maximumAltitudeFt = constrain(
        webServer.arg("maximumAltitude").toInt(), 1000, 100000);
    String requestedMqttHost = webServer.arg("mqttHost");
    requestedMqttHost.trim();
    if (validFlightWallHost(requestedMqttHost)) mqttHost = requestedMqttHost;
    mqttPort = constrain(webServer.arg("mqttPort").toInt(), 1, 65535);
    mqttUser = webServer.arg("mqttUser");
    mqttUser.trim();
    String requestedMqttPassword = webServer.arg("mqttPassword");
    if (requestedMqttPassword.length()) mqttPassword = requestedMqttPassword;
    windDirectionTopic = webServer.arg("windDirectionTopic");
    windSpeedTopic = webServer.arg("windSpeedTopic");
    windGustTopic = webServer.arg("windGustTopic");
    rainTopic = webServer.arg("rainTopic");
    windDirectionTopic.trim();
    windSpeedTopic.trim();
    windGustTopic.trim();
    rainTopic.trim();
    smoothTransitions = webServer.hasArg("smooth");
    bool anyPageEnabled = false;
    for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
      pageEnabled[page] = webServer.hasArg("page" + String(page) + "On");
      anyPageEnabled |= pageEnabled[page];
      pageDurationMs[page] = constrain(
          webServer.arg("page" + String(page) + "Sec").toInt(), 2L, 60L) * 1000UL;
    }
    if (!anyPageEnabled) pageEnabled[0] = true;
    bool usedPages[PAGE_COUNT] = {};
    bool validOrder = true;
    for (uint8_t slot = 0; slot < PAGE_COUNT; ++slot) {
      const int page = webServer.arg("order" + String(slot)).toInt();
      if (page < 0 || page >= PAGE_COUNT || usedPages[page]) validOrder = false;
      else {
        pageOrder[slot] = page;
        usedPages[page] = true;
      }
    }
    if (!validOrder) {
      for (uint8_t page = 0; page < PAGE_COUNT; ++page) pageOrder[page] = page;
    }
    const int requestedBrightness = webServer.arg("brightness").toInt();
    if (requestedBrightness >= 0 && requestedBrightness < BRIGHTNESS_LEVEL_COUNT) {
      manualBrightnessIndex = requestedBrightness;
    }
    preferences.putBool("displayOn", displayEnabled);
    preferences.putBool("uiEnglish", uiEnglish);
    preferences.putBool("envIdle", environmentIdleDisplayEnabled);
    preferences.putBool("mqttOn", mqttWeatherEnabled);
    preferences.putString("mqttHost", mqttHost);
    preferences.putUShort("mqttPort", mqttPort);
    preferences.putString("mqttUser", mqttUser);
    preferences.putString("mqttPass", mqttPassword);
    preferences.putString("windDirTop", windDirectionTopic);
    preferences.putString("windSpdTop", windSpeedTopic);
    preferences.putString("windGstTop", windGustTopic);
    preferences.putString("rainTop", rainTopic);
    preferences.putBool("quietOn", quietHoursEnabled);
    preferences.putUShort("quietStart", quietHoursStartMinutes);
    preferences.putUShort("quietEnd", quietHoursEndMinutes);
    preferences.putBool("autoLight", automaticBrightnessEnabled);
    preferences.putUChar("brightness", manualBrightnessIndex);
    preferences.putFloat("radiusKm", displayRadiusKm);
    preferences.putFloat("observerElevM", observerElevationM);
    preferences.putFloat("farKm", farDirectionDistanceKm);
    preferences.putFloat("lowLux", lowLuxThreshold);
    preferences.putFloat("highLux", highLuxThreshold);
    preferences.putString("adsbUrl", adsbUrl1);
    preferences.putString("adsbUrl2", adsbUrl2);
    preferences.putUChar("adsbActive", activeAdsbSource);
    preferences.putString("routeUrl", routeUrl);
    preferences.putDouble("siteLat", siteLatitude);
    preferences.putDouble("siteLon", siteLongitude);
    preferences.putFloat("heading", viewHeading);
    preferences.putBool("routesOn", routesEnabled);
    preferences.putBool("hideSport", hideSportAircraft);
    preferences.putBool("altFilter", altitudeFilterEnabled);
    preferences.putInt("maxAltFt", maximumAltitudeFt);
    preferences.putString("customText", customPageText);
    preferences.putUChar("customSpd", customScrollSpeed);
    preferences.putBool("smooth", smoothTransitions);
    for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
      preferences.putBool(("p" + String(page) + "On").c_str(), pageEnabled[page]);
      preferences.putUInt(("p" + String(page) + "Ms").c_str(), pageDurationMs[page]);
      preferences.putUChar(("order" + String(page)).c_str(), pageOrder[page]);
    }
    xSemaphoreGive(preferencesMutex);
  }
  settingsWriteInProgress = false;
  if (mqttClient.connected()) mqttClient.disconnect();
  mqttConnected = false;
  lastMqttAttempt = 0;
  resetPageTransition();
}


void handleWebSave() {
  applyWebSettings();
  webServer.sendHeader("Location", "/");
  webServer.send(303, "text/plain", uiText("Gespeichert", "Saved"));
}


void handleAdsbTest() {
  applyWebSettings();
  const bool success = fetchLocalFlight();
  const String message = success ? uiText("Verbindung erfolgreich.", "Connection successful.") :
      String(uiText("Verbindung fehlgeschlagen: ", "Connection failed: ")) + networkLastError;
  webServer.send(success ? 200 : 502, "text/html; charset=utf-8",
      String("<!doctype html><html lang='") + (uiEnglish ? "en" : "de") +
      "'><meta name='viewport' content='width=device-width'>"
      "<body style='font:18px system-ui;max-width:600px;margin:40px auto'><h1>ADS-B-Test</h1><p>" +
      htmlEscape(message) + "</p><p><a href='/'>" + uiText("Zurück", "Back") +
      "</a></p></body></html>");
}


void handleEmergencyTest() {
  String code = webServer.arg("emergencyTestCode");
  if (!isEmergencySquawk(code)) code = "7700";
  emergencyTestCode = code;
  emergencyTestCount = constrain(webServer.arg("emergencyTestCount").toInt(), 1, 3);
  emergencyTestUntil = millis() + EMERGENCY_TEST_DURATION_MS;
  selectedEmergencyHex = "";
  emergencyAdvanceRequested = false;
  fetchLocalFlight();
  webServer.sendHeader("Location", "/");
  webServer.send(303, "text/plain; charset=utf-8", uiText("Notfalltest gestartet", "Emergency test started"));
}


void handleStopEmergencyTest() {
  emergencyTestCount = 0;
  emergencyTestCode = "";
  emergencyTestUntil = 0;
  selectedEmergencyHex = "";
  emergencyAdvanceRequested = false;
  fetchLocalFlight();
  webServer.sendHeader("Location", "/");
  webServer.send(303, "text/plain; charset=utf-8", uiText("Notfalltest beendet", "Emergency test stopped"));
}


void handleRouteTest() {
  String callsign = webServer.arg("routeTestCallsign");
  callsign.trim();
  callsign.toUpperCase();
  bool valid = callsign.length() >= 3 && callsign.length() <= 10;
  for (size_t index = 0; valid && index < callsign.length(); ++index) {
    valid = isAlphaNumeric(callsign[index]);
  }
  if (!valid) callsign = "DLH123";

  String testUrl = webServer.arg("routeUrl");
  testUrl.trim();
  if (!testUrl.startsWith("http://") && !testUrl.startsWith("https://"))
    testUrl = routeUrl;
  routeClient.configure(true, testUrl);
  const RouteResult result = routeClient.test(
      callsign, siteLatitude, siteLongitude);
  routeClient.configure(routesEnabled, routeUrl);
  lastRouteError = result.error;
  const bool reachable = result.error.isEmpty();
  String detail;
  if (!reachable) {
    detail = String(uiText("Fehler: ", "Error: ")) + htmlEscape(result.error);
  } else if (result.found()) {
    detail = "Route: <b>" + htmlEscape(result.routeCodes) + "</b>";
    if (result.routeNames.length()) {
      detail += " (" + htmlEscape(result.routeNames) + ")";
    }
    if (result.airlineCode.length())
      detail += "<br>Airline-Code: <b>" +
                htmlEscape(result.airlineCode) + "</b>";
  } else {
    detail = uiText("Dienst erreichbar, aber keine Route für dieses Callsign gefunden.",
                    "Service reachable, but no route found for this callsign.");
  }
  webServer.send(reachable ? 200 : 502, "text/html; charset=utf-8",
      String("<!doctype html><html lang='") + (uiEnglish ? "en" : "de") +
      "'><meta name='viewport' content='width=device-width'>"
      "<body style='font:18px system-ui;max-width:600px;margin:40px auto;padding:16px'>"
      "<h1>" + uiText("Routendienst-Test", "Route service test") + "</h1><p>" +
      uiText("Direkte Abfrage ohne Cache für ", "Direct query without cache for ") + "<b>" +
      htmlEscape(callsign) + "</b>.</p><p>" + detail +
      "</p><p><a href='/'>" + uiText("Zurück", "Back") + "</a></p></body></html>");
}


void handleWebDisplay() {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "text/html; charset=utf-8", webDisplayPage(uiEnglish));
}


void handleDisplayApi() {
  FlightData shown;
  String error;
  uint32_t updated = 0;
  if (xSemaphoreTake(flightMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    shown = networkFlight;
    error = networkLastError;
    updated = networkLastSuccessfulPoll;
    xSemaphoreGive(flightMutex);
  }
  String json;
  json.reserve(1800);
  json = "{\"language\":\"" + String(uiEnglish ? "en" : "de") +
         "\",\"age_seconds\":" +
         (updated ? String((millis() - updated) / 1000) : "null") +
         ",\"error\":\"" + jsonEscape(error) + "\"" +
         ",\"custom_text\":\"" + jsonEscape(customPageText) + "\"" +
         ",\"custom_scroll_speed\":\"" + customScrollSpeedName() + "\"" +
         ",\"page_enabled\":[";
  for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
    if (page) json += ',';
    json += pageEnabled[page] ? "true" : "false";
  }
  json += "],\"page_order\":[";
  for (uint8_t slot = 0; slot < PAGE_COUNT; ++slot) {
    if (slot) json += ',';
    json += String(pageOrder[slot]);
  }
  json += "],\"page_duration_ms\":[";
  for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
    if (page) json += ',';
    json += String(pageDurationMs[page]);
  }
  json += "],\"flight\":{";
  json += "\"available\":" + String(shown.available ? "true" : "false") +
          ",\"emergency\":" + String(shown.emergency ? "true" : "false") +
          ",\"has_position\":" + String(shown.hasPosition ? "true" : "false") +
          ",\"hex\":\"" + jsonEscape(shown.hex) + "\"" +
          ",\"flight\":\"" + jsonEscape(shown.flight) + "\"" +
          ",\"squawk\":\"" + jsonEscape(shown.squawk) + "\"" +
          ",\"emergency_meaning\":\"" +
              jsonEscape(shown.emergency ? emergencyMeaning(shown.squawk) : "") + "\"" +
          ",\"emergency_count\":" + String(shown.emergencyCount) +
          ",\"emergency_index\":" + String(shown.emergencyIndex) +
          ",\"airline\":\"" + jsonEscape(shown.airline) + "\"" +
          ",\"aircraft_type\":\"" + jsonEscape(shown.type) + "\"" +
          ",\"special_type\":\"" + jsonEscape(shown.specialType) + "\"" +
          ",\"direction\":\"" + jsonEscape(shown.direction) + "\"" +
          ",\"vertical_motion\":\"" + jsonEscape(shown.verticalMotion) + "\"" +
          ",\"motion\":\"" + jsonEscape(shown.motion) + "\"" +
          ",\"origin\":\"" + jsonEscape(shown.origin) + "\"" +
          ",\"destination\":\"" + jsonEscape(shown.destination) + "\"" +
          ",\"route_codes\":\"" + jsonEscape(shown.routeCodes) + "\"" +
          ",\"route_names\":\"" + jsonEscape(shown.routeNames) + "\"" +
          ",\"origin_name\":\"" + jsonEscape(shown.originName) + "\"" +
          ",\"destination_name\":\"" + jsonEscape(shown.destinationName) + "\"" +
          ",\"distance_km\":" + jsonNumberOrNull(shown.distanceKm) +
          ",\"altitude_ft\":" + jsonNumberOrNull(shown.altitudeFt, 0) +
          ",\"speed_kmh\":" + jsonNumberOrNull(shown.speedKmh, 0) +
          ",\"cpa_km\":" + jsonNumberOrNull(shown.cpaKm) +
          ",\"cpa_minutes\":" + jsonNumberOrNull(shown.cpaMinutes, 0) +
          ",\"vertical_rate_fpm\":" +
              jsonNumberOrNull(shown.verticalRateFpm, 0) +
          ",\"bearing\":" + jsonNumberOrNull(shown.bearing, 0) +
          ",\"relative_bearing\":" +
              jsonNumberOrNull(shown.relativeBearing) + "}}";
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json; charset=utf-8", json);
}


void handleStatusApi() {
  FlightData statusFlight;
  String statusError;
  uint32_t statusUpdated = 0;
  if (xSemaphoreTake(flightMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    statusFlight = networkFlight;
    statusError = networkLastError;
    statusUpdated = networkLastSuccessfulPoll;
    xSemaphoreGive(flightMutex);
  }
  String statusWindDirection;
  float statusWindSpeed = NAN;
  float statusWindGust = NAN;
  float statusRain = NAN;
  if (xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    statusWindDirection = windDirection;
    statusWindSpeed = windSpeedKph;
    statusWindGust = windGustKph;
    statusRain = rain24Mm;
    xSemaphoreGive(weatherMutex);
  }
  const float effectiveRadiusKm = statusFlight.emergency ? NAN :
                                  statusFlight.specialType.length()
                                      ? displayRadiusKm * 2.0f
                                      : displayRadiusKm;
  String json = "{\"version\":\"" + jsonEscape(APP_VERSION) +
      "\",\"language\":\"" + String(uiEnglish ? "en" : "de") +
      "\",\"build\":\"" + jsonEscape(BUILD_TIMESTAMP) +
      "\",\"uptime_seconds\":" + String(millis() / 1000) +
      ",\"free_heap\":" + String(ESP.getFreeHeap()) +
      ",\"chip_temperature_c\":" + jsonNumberOrNull(chipTemperatureC()) +
      ",\"bme280_available\":" + String(environmentSensorAvailable ? "true" : "false") +
      ",\"environment_idle_display_enabled\":" +
          String(environmentIdleDisplayEnabled ? "true" : "false") +
      ",\"ambient_temperature_c\":" + jsonNumberOrNull(ambientTemperatureC) +
      ",\"relative_humidity_percent\":" + jsonNumberOrNull(relativeHumidityPercent) +
      ",\"station_pressure_hpa\":" + jsonNumberOrNull(stationPressureHpa) +
      ",\"air_pressure_hpa\":" + jsonNumberOrNull(airPressureHpa) +
      ",\"mqtt_weather_enabled\":" + String(mqttWeatherEnabled ? "true" : "false") +
      ",\"mqtt_connected\":" + String(mqttConnected ? "true" : "false") +
      ",\"mqtt_host\":\"" + jsonEscape(mqttHost) + "\"" +
      ",\"mqtt_port\":" + String(mqttPort) +
      ",\"mqtt_last_message_age_seconds\":" +
          (lastMqttMessage ? String((millis() - lastMqttMessage) / 1000) : "null") +
      ",\"mqtt_error\":\"" + jsonEscape(mqttLastError) + "\"" +
      ",\"wind_direction\":\"" + jsonEscape(statusWindDirection) + "\"" +
      ",\"wind_speed_kph\":" + jsonNumberOrNull(statusWindSpeed) +
      ",\"wind_gust_kph\":" + jsonNumberOrNull(statusWindGust) +
      ",\"rain24_mm\":" + jsonNumberOrNull(statusRain) +
      ",\"rssi\":" + String(WiFi.RSSI()) +
      ",\"wifi_reconnects\":" + String(wifiReconnectCount) +
      ",\"webserver_starts\":" + String(webServerStartCount) +
      ",\"last_wifi_disconnect_age_seconds\":" +
          (lastWifiDisconnect
               ? String((millis() - lastWifiDisconnect) / 1000)
               : "null") +
      ",\"lux\":" + (isnan(filteredLux) ? String("null") : String(filteredLux, 1)) +
      ",\"brightness_percent\":" + String(BRIGHTNESS_LEVELS[brightnessIndex]) +
      ",\"matrix_bit_depth\":" + String(BIT_DEPTH) +
      ",\"matrix_refresh_hz\":" + String(matrixRefreshHz) +
      ",\"matrix_show_count\":" + String(matrixShowCount) +
      ",\"matrix_show_last_us\":" + String(matrixShowLastUs) +
      ",\"matrix_show_max_us\":" + String(matrixShowMaxUs) +
      ",\"observer_elevation_m\":" + String(observerElevationM, 0) +
      ",\"far_direction_km\":" + String(farDirectionDistanceKm, 1) +
      ",\"adsb_url\":\"" + jsonEscape(activeAdsbUrl()) + "\"" +
      ",\"adsb_url_1\":\"" + jsonEscape(adsbUrl1) + "\"" +
      ",\"adsb_url_2\":\"" + jsonEscape(adsbUrl2) + "\"" +
      ",\"active_adsb_source\":" + String(activeAdsbSource + 1) +
      ",\"site_latitude\":" + String(siteLatitude, 6) +
      ",\"site_longitude\":" + String(siteLongitude, 6) +
      ",\"view_heading\":" + String(viewHeading, 0) +
      ",\"display_radius_km\":" + String(displayRadiusKm, 1) +
      ",\"effective_radius_km\":" + jsonNumberOrNull(effectiveRadiusKm) +
      ",\"routes_enabled\":" + String(routesEnabled ? "true" : "false") +
      ",\"hide_sport_aircraft\":" + String(hideSportAircraft ? "true" : "false") +
      ",\"altitude_filter_enabled\":" + String(altitudeFilterEnabled ? "true" : "false") +
      ",\"maximum_altitude_ft\":" + String(maximumAltitudeFt) +
      ",\"received_aircraft\":" + String(lastReceivedAircraft) +
      ",\"candidate_aircraft\":" + String(lastCandidateAircraft) +
      ",\"last_adsb_attempt_age_seconds\":" +
          (lastAdsbAttempt ? String((millis() - lastAdsbAttempt) / 1000) : "null") +
      ",\"last_adsb_success_age_seconds\":" +
          (lastAdsbSuccess ? String((millis() - lastAdsbSuccess) / 1000) : "null") +
      ",\"display_update_age_seconds\":" +
          (statusUpdated ? String((millis() - statusUpdated) / 1000) : "null") +
      ",\"feed_timestamp\":" +
          (lastFeedTimestamp ? String(lastFeedTimestamp) : "null") +
      ",\"adsb_error\":\"" + jsonEscape(lastAdsbError) + "\"" +
      ",\"display_error\":\"" + jsonEscape(statusError) + "\"" +
      ",\"route_error\":\"" + jsonEscape(lastRouteError) + "\"" +
      ",\"route_status\":\"" + jsonEscape(currentRoute.status) + "\"" +
      ",\"route_cache_write_pending\":" +
          String(routeClient.hasPendingWrites() ? "true" : "false") +
      ",\"flight_available\":" + String(statusFlight.available ? "true" : "false") +
      ",\"emergency\":" + String(statusFlight.emergency ? "true" : "false") +
      ",\"has_position\":" + String(statusFlight.hasPosition ? "true" : "false") +
      ",\"hex\":\"" + jsonEscape(statusFlight.hex) + "\"" +
      ",\"flight\":\"" + jsonEscape(statusFlight.flight) + "\"" +
      ",\"squawk\":\"" + jsonEscape(statusFlight.squawk) + "\"" +
      ",\"emergency_meaning\":\"" +
          jsonEscape(statusFlight.emergency ? emergencyMeaning(statusFlight.squawk) : "") + "\"" +
      ",\"emergency_count\":" + String(statusFlight.emergencyCount) +
      ",\"emergency_index\":" + String(statusFlight.emergencyIndex) +
      ",\"emergency_test_active\":" +
          String(emergencyTestCount &&
                 static_cast<int32_t>(emergencyTestUntil - millis()) > 0
                     ? "true" : "false") +
      ",\"airline\":\"" + jsonEscape(statusFlight.airline) + "\"" +
      ",\"aircraft_type\":\"" + jsonEscape(statusFlight.type) + "\"" +
      ",\"special_type\":\"" + jsonEscape(statusFlight.specialType) + "\"" +
      ",\"origin\":\"" + jsonEscape(statusFlight.origin) + "\"" +
      ",\"destination\":\"" + jsonEscape(statusFlight.destination) + "\"" +
      ",\"route_codes\":\"" + jsonEscape(statusFlight.routeCodes) + "\"" +
      ",\"route_names\":\"" + jsonEscape(statusFlight.routeNames) + "\"" +
      ",\"distance_km\":" + jsonNumberOrNull(statusFlight.distanceKm) +
      ",\"altitude_ft\":" + jsonNumberOrNull(statusFlight.altitudeFt, 0) +
      ",\"speed_kmh\":" + jsonNumberOrNull(statusFlight.speedKmh, 0) +
      ",\"cpa_km\":" + jsonNumberOrNull(statusFlight.cpaKm) +
      ",\"cpa_minutes\":" + jsonNumberOrNull(statusFlight.cpaMinutes, 0) +
      ",\"vertical_rate_fpm\":" +
          jsonNumberOrNull(statusFlight.verticalRateFpm, 0) +
      ",\"vertical_motion\":\"" +
          jsonEscape(statusFlight.verticalMotion) + "\"" +
      ",\"motion\":\"" + jsonEscape(statusFlight.motion) + "\"" +
      ",\"bearing\":" + jsonNumberOrNull(statusFlight.bearing, 0) +
      ",\"relative_bearing\":" +
          jsonNumberOrNull(statusFlight.relativeBearing) +
      ",\"unknown_airports\":" + unknownCodesJson(true) +
      ",\"unknown_airlines\":" + unknownCodesJson(false) +
      ",\"quiet_hours_enabled\":" + String(quietHoursEnabled ? "true" : "false") +
      ",\"quiet_hours_active\":" + String(quietHoursAreActive() ? "true" : "false") +
      ",\"display_test\":" + String(displayTestActive ? "true" : "false") +
      ",\"reset_reason\":\"" + jsonEscape(resetReasonText()) + "\"}";
  webServer.send(200, "application/json; charset=utf-8", json);
}


void handleRestart() {
  webServer.send(200, "text/html; charset=utf-8",
      String("<!doctype html><html lang='") + (uiEnglish ? "en" : "de") +
      "'><meta name='viewport' content='width=device-width'>"
      "<body style='font:18px system-ui;max-width:600px;margin:40px auto'>"
      "<h1>" + uiText("Neustart", "Restart") + "</h1><p>" +
      uiText("Der ESP wird neu gestartet.", "The ESP is restarting.") + "</p></body></html>");
  manualRebootAt = millis() + 750;
}


void handleDisplayTest() {
  if (displayTestActive || displayTestCommand == 1) displayTestCommand = 2;
  else displayTestCommand = 1;
  webServer.sendHeader("Location", "/");
  webServer.send(303, "text/plain; charset=utf-8", uiText("Displaytest umgeschaltet", "Display test toggled"));
}


bool otaAuthenticated() {
  if (strlen(OTA_PASSWORD) < 8) {
    webServer.send(503, "text/plain; charset=utf-8", uiText("OTA ist nicht konfiguriert.", "OTA is not configured."));
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
  String html = R"HTML(<!doctype html><html lang="de"><meta name="viewport" content="width=device-width">
<title>Firmware-Update</title><style>body{font:16px system-ui;max-width:600px;margin:40px auto;padding:16px}progress{width:100%;height:24px}button{padding:10px}</style>
<body><h1>Firmware-Update</h1><p>Installierte Version: <b id="before">%VERSION%</b></p>
<p>PlatformIO-Datei <b>firmware.bin</b> auswählen. Während des Updates die Stromversorgung nicht trennen.</p>
<form id="upload"><input id="file" type="file" name="firmware" accept=".bin" required> <button>Prüfen und installieren</button></form>
<p><progress id="progress" max="100" value="0"></progress> <span id="percent">0%</span></p><p id="result"></p>
<script>
const form=document.getElementById('upload'),file=document.getElementById('file'),result=document.getElementById('result');
form.addEventListener('submit',async e=>{e.preventDefault();const f=file.files[0];if(!f||!f.name.toLowerCase().endsWith('.bin')){result.textContent='Bitte eine .bin-Firmware auswählen.';return}
const head=new Uint8Array(await f.slice(0,1).arrayBuffer());if(head[0]!==0xE9){result.textContent='Ungültige ESP32-Firmware: Dateikopf fehlt.';return}
if(!confirm('Firmware '+f.name+' wirklich installieren?'))return;const x=new XMLHttpRequest();x.open('POST','/update');
x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(e.loaded/e.total*100);progress.value=p;percent.textContent=p+'%'}};
x.onerror=()=>result.textContent='Netzwerkfehler während des Uploads.';
x.onload=()=>{let r;try{r=JSON.parse(x.responseText)}catch(e){result.textContent='Ungültige Antwort des ESP.';return}result.textContent=r.message;if(r.ok)pollVersion()};
const data=new FormData();data.append('firmware',f);result.textContent='Upload läuft…';x.send(data)});
function pollVersion(){let tries=0;result.textContent+=' Warte auf Neustart…';const timer=setInterval(async()=>{if(++tries>30){clearInterval(timer);result.textContent+=' ESP nicht wieder erreichbar.';return}try{const r=await fetch('/api/status',{cache:'no-store'});if(r.ok){const s=await r.json();clearInterval(timer);result.textContent='Update abgeschlossen. Version vorher: %VERSION%, danach: '+s.version+' ('+s.build+').' }}catch(e){}},2000)}
</script><p><a href="/">Zurück</a></p></body></html>)HTML";
  html.replace("%VERSION%", APP_VERSION);
  html.replace("%VERSION%", APP_VERSION);
  translateWebPageToEnglish(html);
  webServer.send(200, "text/html; charset=utf-8", html);
}


void handleOtaUpload() {
  if (strlen(OTA_PASSWORD) < 8 ||
      !webServer.authenticate("admin", OTA_PASSWORD)) return;
  HTTPUpload &upload = webServer.upload();
  if (upload.status == UPLOAD_FILE_START) {
    otaUploadValid = false;
    otaBytesWritten = 0;
    otaErrorMessage = "";
    String filename = upload.filename;
    filename.toLowerCase();
    if (!filename.endsWith(".bin")) {
      otaErrorMessage = "Nur .bin-Firmwaredateien sind erlaubt.";
      return;
    }
    otaPreparing = true;
    vTaskDelay(pdMS_TO_TICKS(900));
    otaPreparing = false;
    otaInProgress = true;
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.printf("Web OTA start: %s\n", upload.filename.c_str());
    otaUploadValid = Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH);
    if (!otaUploadValid) otaErrorMessage = "OTA-Partition konnte nicht geöffnet werden.";
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (!otaUploadValid || Update.hasError()) return;
    if (otaBytesWritten == 0 &&
        (upload.currentSize == 0 || upload.buf[0] != 0xE9)) {
      otaUploadValid = false;
      otaErrorMessage = "Ungültige ESP32-Firmware: falscher Dateikopf.";
      Update.abort();
      return;
    }
    const size_t written = Update.write(upload.buf, upload.currentSize);
    otaBytesWritten += written;
    if (written != upload.currentSize) {
      otaUploadValid = false;
      otaErrorMessage = "Flash-Schreibfehler während des Updates.";
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (otaUploadValid && otaBytesWritten > 0 &&
        !Update.hasError() && Update.end(true)) {
      Serial.printf("Web OTA complete: %u bytes\n", upload.totalSize);
    } else if (!otaErrorMessage.length()) {
      otaUploadValid = false;
      otaErrorMessage = "Firmwareprüfung oder Abschluss des Updates fehlgeschlagen.";
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    otaPreparing = false;
    otaInProgress = false;
    otaUploadValid = false;
    otaErrorMessage = "Upload wurde abgebrochen.";
    Serial.println("Web OTA aborted");
  }
}


void finishOtaUpload() {
  if (!otaAuthenticated()) return;
  const bool success = otaUploadValid && !Update.hasError() && Update.isFinished();
  if (!success && !otaErrorMessage.length()) otaErrorMessage = "Update fehlgeschlagen.";
  const String message = success ?
      "Firmware geprüft und installiert. Der ESP startet neu." : otaErrorMessage;
  const String json = String("{\"ok\":") + (success ? "true" : "false") +
      ",\"message\":\"" + jsonEscape(message) +
      "\",\"version_before\":\"" + APP_VERSION + "\"}";
  webServer.send(success ? 200 : 500, "application/json; charset=utf-8", json);
  if (success) otaRebootAt = millis() + 1500;
  else {
    otaPreparing = false;
    otaInProgress = false;
  }
}


void startArduinoOta() {
  if (otaStarted || strlen(OTA_PASSWORD) < 8) return;
  ArduinoOTA.setHostname("matrixportal");
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    otaPreparing = true;
    vTaskDelay(pdMS_TO_TICKS(900));
    otaPreparing = false;
    otaInProgress = true;
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("PlatformIO OTA start");
    setNetworkError("OTA UPDATE");
  });
  ArduinoOTA.onEnd([]() { Serial.println("PlatformIO OTA complete"); });
  ArduinoOTA.onError([](ota_error_t error) {
    otaPreparing = false;
    otaInProgress = false;
    Serial.printf("PlatformIO OTA error: %u\n", error);
    setNetworkError("OTA FEHLER");
  });
  ArduinoOTA.begin();
  otaStarted = true;
  Serial.println("PlatformIO OTA enabled for matrixportal.local");
}


bool parseMqttNumber(const String &payload, float &value) {
  char *end = nullptr;
  const float parsed = strtof(payload.c_str(), &end);
  while (end && *end && isspace(static_cast<unsigned char>(*end))) ++end;
  if (end == payload.c_str() || (end && *end) || !isfinite(parsed)) return false;
  value = parsed;
  return true;
}


void handleMqttMessage(char *topic, byte *payload, unsigned int length) {
  if (length > 48 || weatherMutex == nullptr) return;
  String value;
  value.reserve(length);
  for (unsigned int index = 0; index < length; ++index)
    value += static_cast<char>(payload[index]);
  value.trim();
  const uint32_t now = millis();
  if (xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(50)) != pdTRUE) return;
  if (windDirectionTopic == topic) {
    float degrees;
    if (parseMqttNumber(value, degrees)) {
      windDirection = weatherCompassDirection(degrees);
      windDirectionUpdated = now;
    } else if (value.length() >= 1 && value.length() <= 3) {
      value.toUpperCase();
      value.replace("E", "O");
      windDirection = displayAscii(value);
      windDirectionUpdated = now;
    }
  } else {
    float number;
    if (parseMqttNumber(value, number)) {
      if (windSpeedTopic == topic && number >= 0.0f) {
        windSpeedKph = number;
        windSpeedUpdated = now;
      } else if (windGustTopic == topic && number >= 0.0f) {
        windGustKph = number;
        windGustUpdated = now;
      } else if (rainTopic == topic && number >= 0.0f) {
        // The source publishes centimetres; the display uses millimetres.
        rain24Mm = number * 10.0f;
        rainUpdated = now;
      }
    }
  }
  lastMqttMessage = now;
  xSemaphoreGive(weatherMutex);
}


void subscribeMqttTopics() {
  if (windDirectionTopic.length()) mqttClient.subscribe(windDirectionTopic.c_str());
  if (windSpeedTopic.length()) mqttClient.subscribe(windSpeedTopic.c_str());
  if (windGustTopic.length()) mqttClient.subscribe(windGustTopic.c_str());
  if (rainTopic.length()) mqttClient.subscribe(rainTopic.c_str());
}


void serviceMqtt() {
  if (!mqttWeatherEnabled || !mqttHost.length() || WiFi.status() != WL_CONNECTED) {
    if (mqttClient.connected()) mqttClient.disconnect();
    mqttConnected = false;
    return;
  }
  if (mqttClient.connected()) {
    mqttConnected = true;
    mqttClient.loop();
    return;
  }
  mqttConnected = false;
  const uint32_t now = millis();
  if (lastMqttAttempt && now - lastMqttAttempt < MQTT_RETRY_MS) return;
  lastMqttAttempt = now;
  mqttClient.setServer(mqttHost.c_str(), mqttPort);
  mqttClient.setCallback(handleMqttMessage);
  mqttClient.setSocketTimeout(2);
  const String clientId = "flightwall-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFF), HEX);
  const bool connected = mqttUser.length()
      ? mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPassword.c_str())
      : mqttClient.connect(clientId.c_str());
  if (connected) {
    mqttConnected = true;
    mqttLastError = "";
    ++mqttReconnectCount;
    subscribeMqttTopics();
    Serial.printf("MQTT connected to %s:%u\n", mqttHost.c_str(), mqttPort);
  } else {
    mqttLastError = "MQTT FEHLER " + String(mqttClient.state());
    Serial.printf("MQTT connection failed: %d\n", mqttClient.state());
  }
}


void startWebServer() {
  if (webServerStarted) return;
  if (!webServerConfigured) {
    webServer.on("/", HTTP_GET, []() { webServer.send(200, "text/html; charset=utf-8", webPage()); });
    webServer.on("/display", HTTP_GET, handleWebDisplay);
    webServer.on("/api/display", HTTP_GET, handleDisplayApi);
    webServer.on("/api/status", HTTP_GET, handleStatusApi);
    webServer.on("/save", HTTP_POST, handleWebSave);
    webServer.on("/test-adsb", HTTP_POST, handleAdsbTest);
    webServer.on("/test-route", HTTP_POST, handleRouteTest);
    webServer.on("/test-emergency", HTTP_POST, handleEmergencyTest);
    webServer.on("/stop-emergency-test", HTTP_POST, handleStopEmergencyTest);
    webServer.on("/display-test", HTTP_POST, handleDisplayTest);
    webServer.on("/restart", HTTP_POST, handleRestart);
    webServer.on("/update", HTTP_GET, handleOtaPage);
    webServer.on("/update", HTTP_POST, finishOtaUpload, handleOtaUpload);
    webServer.onNotFound([]() { webServer.send(404, "text/plain; charset=utf-8", "Nicht gefunden"); });
    webServerConfigured = true;
  }
  webServer.begin();
  if (MDNS.begin("matrixportal")) MDNS.addService("http", "tcp", 80);
  webServerStarted = true;
  ++webServerStartCount;
  startArduinoOta();
  Serial.println("Web interface: http://matrixportal.local");
}


void stopNetworkServices() {
  if (mqttClient.connected()) mqttClient.disconnect();
  mqttConnected = false;
  if (otaStarted) {
    ArduinoOTA.end();
    otaStarted = false;
  }
  MDNS.end();
  if (webServerStarted) {
    webServer.stop();
    webServerStarted = false;
  }
  Serial.println("Network services stopped after Wi-Fi disconnect");
}


void flushDeferredRouteCacheIfIdle() {
  if (!routeClient.hasPendingWrites() || routeCacheWritePreparing ||
      routeCacheWriteInProgress || settingsWritePreparing ||
      settingsWriteInProgress) return;
  bool aircraftActive = true;
  if (xSemaphoreTake(flightMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    aircraftActive = networkFlight.available;
    xSemaphoreGive(flightMutex);
  }
  if (aircraftActive && !quietHoursAreActive()) return;

  routeCacheWritePreparing = true;
  vTaskDelay(pdMS_TO_TICKS(120));
  routeCacheWriteInProgress = true;
  routeClient.flushPendingWrites();
  routeCacheWriteInProgress = false;
  routeCacheWritePreparing = false;
  Serial.println("Deferred route cache written while display was blanked");
}


void flushDeferredButtonSettings() {
  if (!buttonSettingsWritePending || settingsWritePreparing ||
      settingsWriteInProgress || routeCacheWritePreparing ||
      routeCacheWriteInProgress) return;
  settingsWritePreparing = true;
  vTaskDelay(pdMS_TO_TICKS(120));
  settingsWritePreparing = false;
  settingsWriteInProgress = true;
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
    preferences.putBool("autoLight", automaticBrightnessEnabled);
    preferences.putUChar("brightness", manualBrightnessIndex);
    xSemaphoreGive(preferencesMutex);
    buttonSettingsWritePending = false;
  }
  settingsWriteInProgress = false;
}


void networkTask(void *) {
  lastWiFiAttempt = millis();
  if (connectWiFi(20000)) {
    wifiWasConnected = true;
    startWebServer();
    fetchLocalFlight();
    lastPoll = millis();
  }
  for (;;) {
    const uint32_t now = millis();
    const bool wifiConnected = WiFi.status() == WL_CONNECTED;
    if (!wifiConnected && wifiWasConnected) {
      wifiWasConnected = false;
      lastWifiDisconnect = now;
      stopNetworkServices();
      setNetworkError("WLAN GETRENNT");
    } else if (wifiConnected && !wifiWasConnected) {
      wifiWasConnected = true;
      ++wifiReconnectCount;
      startWebServer();
      Serial.println("Network services restarted after Wi-Fi reconnect");
    }
    if (wifiConnected) {
      webServer.handleClient();
      if (otaStarted) ArduinoOTA.handle();
      serviceMqtt();
    }
    if (otaRebootAt && static_cast<int32_t>(now - otaRebootAt) >= 0) ESP.restart();
    if (manualRebootAt && static_cast<int32_t>(now - manualRebootAt) >= 0) ESP.restart();
    if (!wifiConnected && now - lastWiFiAttempt >= WIFI_RETRY_MS) {
      lastWiFiAttempt = now;
      connectWiFi(10000);
    }
    if (wifiConnected && now - lastPoll >= POLL_INTERVAL_MS) {
      lastPoll = now;
      fetchLocalFlight();
    }
    flushDeferredButtonSettings();
    flushDeferredRouteCacheIfIdle();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}


void setup() {
  Serial.begin(115200);
  delay(250);
  displayTaskHandle = xTaskGetCurrentTaskHandle();
  flightMutex = xSemaphoreCreateMutex();
  preferencesMutex = xSemaphoreCreateMutex();
  weatherMutex = xSemaphoreCreateMutex();
  if (flightMutex == nullptr || preferencesMutex == nullptr ||
      weatherMutex == nullptr) {
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
  const uint8_t settingsVersion = preferences.getUChar("settingsVer", 1);
  manualBrightnessIndex = preferences.getUChar("brightness", 1);
  if (settingsVersion < 2) {
    const uint8_t migratedLevels[] = {1, 3, 6};
    manualBrightnessIndex = manualBrightnessIndex < 3
        ? migratedLevels[manualBrightnessIndex] : 3;
    preferences.putUChar("brightness", manualBrightnessIndex);
    preferences.putUChar("settingsVer", 2);
  }
  if (settingsVersion < 11) {
    // Preserve the approximate brightness selected with the former seven
    // 3-bit levels when migrating to the three native 2-bit levels.
    if (manualBrightnessIndex <= 1) {
      manualBrightnessIndex = 0;
    } else if (manualBrightnessIndex <= 4) {
      manualBrightnessIndex = 1;
    } else {
      manualBrightnessIndex = 2;
    }
    preferences.putUChar("brightness", manualBrightnessIndex);
  }
  if (settingsVersion < 12) {
    // Restore the closest native 3-bit levels from the stable 2-bit firmware.
    const uint8_t migratedLevels[] = {1, 3, 6};
    manualBrightnessIndex = manualBrightnessIndex < 3
        ? migratedLevels[manualBrightnessIndex] : 3;
    preferences.putUChar("brightness", manualBrightnessIndex);
  }
  if (manualBrightnessIndex >= BRIGHTNESS_LEVEL_COUNT) manualBrightnessIndex = 1;
  brightnessIndex = manualBrightnessIndex;
  automaticBrightnessEnabled = preferences.getBool("autoLight", true);
  displayEnabled = preferences.getBool("displayOn", true);
  uiEnglish = preferences.getBool("uiEnglish", false);
  environmentIdleDisplayEnabled = preferences.getBool("envIdle", true);
  mqttWeatherEnabled = preferences.getBool("mqttOn", true);
  mqttHost = preferences.getString("mqttHost", DEFAULT_MQTT_HOST);
  mqttPort = preferences.getUShort("mqttPort", DEFAULT_MQTT_PORT);
  mqttUser = preferences.getString("mqttUser", DEFAULT_MQTT_USER);
  mqttPassword = preferences.getString("mqttPass", MQTT_PASSWORD);
  windDirectionTopic = preferences.getString(
      "windDirTop", DEFAULT_WIND_DIRECTION_TOPIC);
  windSpeedTopic = preferences.getString(
      "windSpdTop", DEFAULT_WIND_SPEED_TOPIC);
  windGustTopic = preferences.getString(
      "windGstTop", DEFAULT_WIND_GUST_TOPIC);
  rainTopic = preferences.getString("rainTop", DEFAULT_RAIN_TOPIC);
  quietHoursEnabled = preferences.getBool("quietOn", false);
  quietHoursStartMinutes = constrain(
      preferences.getUShort("quietStart", 23 * 60 + 55),
      static_cast<uint16_t>(0), static_cast<uint16_t>(1439));
  quietHoursEndMinutes = constrain(
      preferences.getUShort("quietEnd", 5 * 60 + 55),
      static_cast<uint16_t>(0), static_cast<uint16_t>(1439));
  displayRadiusKm = preferences.getFloat("radiusKm", DEFAULT_DISPLAY_RADIUS_KM);
  observerElevationM = constrain(
      preferences.getFloat("observerElevM", 0.0f), -500.0f, 9000.0f);
  farDirectionDistanceKm = constrain(
      preferences.getFloat("farKm", 5.0f), 0.5f, 100.0f);
  pageIntervalMs = preferences.getUInt("pageMs", DEFAULT_PAGE_INTERVAL_MS);
  lowLuxThreshold = preferences.getFloat("lowLux", 20.0f);
  highLuxThreshold = preferences.getFloat("highLux", 800.0f);
  adsbUrl1 = preferences.getString("adsbUrl", DEFAULT_ADSB_URL);
  adsbUrl2 = preferences.getString("adsbUrl2", "");
  activeAdsbSource = preferences.getUChar("adsbActive", 0);
  if (activeAdsbSource > 1 || (activeAdsbSource == 1 && !adsbUrl2.length()))
    activeAdsbSource = 0;
  routeUrl = preferences.getString("routeUrl", DEFAULT_ROUTE_URL);
  siteLatitude = preferences.getDouble("siteLat", DEFAULT_SITE_LATITUDE);
  siteLongitude = preferences.getDouble("siteLon", DEFAULT_SITE_LONGITUDE);
  viewHeading = preferences.getFloat("heading", DEFAULT_VIEW_HEADING);
  routesEnabled = preferences.getBool("routesOn", true);
  hideSportAircraft = preferences.getBool("hideSport", true);
  altitudeFilterEnabled = preferences.getBool("altFilter", false);
  maximumAltitudeFt = preferences.getInt(
      "maxAltFt", DEFAULT_MAXIMUM_ALTITUDE_FT);
  customPageText = displayAscii(preferences.getString("customText", ""));
  customPageText.trim();
  if (customPageText.length() > 128) customPageText.remove(128);
  customScrollSpeed = preferences.getUChar("customSpd", 1);
  if (customScrollSpeed > 2) customScrollSpeed = 1;
  smoothTransitions = preferences.getBool("smooth", true);
  for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
    pageEnabled[page] = preferences.getBool(
        ("p" + String(page) + "On").c_str(), true);
    pageDurationMs[page] = preferences.getUInt(
        ("p" + String(page) + "Ms").c_str(), pageIntervalMs);
    pageOrder[page] = preferences.getUChar(
        ("order" + String(page)).c_str(), page);
  }
  if (settingsVersion < 12) preferences.putUChar("settingsVer", 12);
  displayRadiusKm = constrain(displayRadiusKm, 1.0f, 100.0f);
  siteLatitude = constrain(siteLatitude, -90.0, 90.0);
  siteLongitude = constrain(siteLongitude, -180.0, 180.0);
  viewHeading = constrain(viewHeading, 0.0f, 359.0f);
  maximumAltitudeFt = constrain(maximumAltitudeFt, 1000, 100000);
  pageIntervalMs = constrain(pageIntervalMs, 2000UL, 60000UL);
  bool validStoredOrder = true;
  bool storedOrderUsed[PAGE_COUNT] = {};
  for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
    pageDurationMs[page] = constrain(pageDurationMs[page], 2000UL, 60000UL);
    if (pageOrder[page] >= PAGE_COUNT || storedOrderUsed[pageOrder[page]]) {
      validStoredOrder = false;
    } else {
      storedOrderUsed[pageOrder[page]] = true;
    }
  }
  if (!validStoredOrder) {
    for (uint8_t page = 0; page < PAGE_COUNT; ++page) pageOrder[page] = page;
  }
  bool anyStoredPageEnabled = false;
  for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
    anyStoredPageEnabled |= pageEnabled[page];
  }
  if (!anyStoredPageEnabled) pageEnabled[0] = true;
  lowLuxThreshold = constrain(lowLuxThreshold, 1.0f, 500.0f);
  highLuxThreshold = constrain(highLuxThreshold, 50.0f, 50000.0f);
  if (highLuxThreshold <= lowLuxThreshold) highLuxThreshold = lowLuxThreshold + 50.0f;
  updateColors();

  Wire.begin(LIGHT_SENSOR_SDA_PIN, LIGHT_SENSOR_SCL_PIN);
  Wire.setTimeOut(50);
  environmentWire.begin(STEMMA_SDA_PIN, STEMMA_SCL_PIN);
  environmentWire.setTimeOut(50);
  findLightSensor();
  findEnvironmentSensor();

  drawStatus("FLIGHTWALL", uiText("VERBINDET...", "CONNECTING..."));
  if (xTaskCreatePinnedToCore(
          networkTask, "network", 12288, nullptr, 1,
          &networkTaskHandle, 0) != pdPASS) {
    drawStatus(uiText("NETZWERK", "NETWORK"), uiText("TASK FEHLER", "TASK ERROR"));
    while (true) delay(1000);
  }
}


void serviceMatrixDiagnostics(uint32_t now) {
  static uint32_t lastSample = 0;
  if (lastSample == 0) {
    lastSample = now;
    matrix.getFrameCount();
    return;
  }
  const uint32_t elapsed = now - lastSample;
  if (elapsed < 1000) return;
  const uint32_t frameCount = matrix.getFrameCount();
  matrixRefreshHz = static_cast<uint32_t>(
      (static_cast<uint64_t>(frameCount) * 1000ULL) /
      elapsed);
  lastSample = now;
}


uint32_t displayDrawIntervalMs() {
  if (otaPreparing || otaInProgress || routeCacheWritePreparing ||
      routeCacheWriteInProgress || settingsWritePreparing ||
      settingsWriteInProgress || displayTestActive ||
      transitionPhase != TransitionPhase::Normal ||
      static_cast<int32_t>(brightnessMessageUntil - millis()) > 0) {
    return 50;
  }
  if (!aircraftIsInsideDisplayRadius()) return 500;
  if (currentPageSlot < PAGE_COUNT) {
    const uint8_t page = pageOrder[currentPageSlot];
    // Route-name and custom-text pages scroll pixel by pixel. The compact
    // airport page can also contain scrolling multi-hop routes.
    if (page == 4 && customScrollSpeed == 2) return 25;
    if (page == 3 || page == 4 || page == 6) return 50;
  }
  // Static pages only need a periodic deadline check; ADS-B changes and page
  // changes are detected comfortably within a quarter second.
  return 250;
}


void loop() {
  const uint32_t now = millis();
  syncNetworkState();
  handleBrightnessButtons();
  serviceLightSensor();
  serviceEnvironmentSensor();
  serviceMatrixDiagnostics(now);
  if (!automaticBrightnessEnabled && brightnessIndex != manualBrightnessIndex) {
    applyBrightnessIndex(manualBrightnessIndex, false);
  }

  static uint32_t lastDraw = 0;
  if (now - lastDraw >= displayDrawIntervalMs()) {
    lastDraw = now;
    drawDisplay();
  }
  delay(10);
}
