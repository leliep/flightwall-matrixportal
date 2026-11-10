// Version generated: 2026-10-03 12:17:49 CEST (Europe/Berlin)
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <time.h>
#include <vector>
#include <utility>

#ifndef FLIGHTWALL_HAS_MATRIX
#define FLIGHTWALL_HAS_MATRIX 1
#endif

#include <Adafruit_GFX.h>
#include <Adafruit_BME280.h>
#if FLIGHTWALL_HAS_MATRIX
#include <Adafruit_Protomatter.h>
#endif
#include <PubSubClient.h>
#include "AdsbClient.hpp"
#include "AircraftProcessor.hpp"
#include "AirlineNames.hpp"
#include "CallsignClassifier.hpp"
#include "Geo.hpp"
#include "RadarTrailStorage.hpp"
#include "RouteClient.hpp"
#include "StatusLed.hpp"
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
constexpr char APP_VERSION[] = "3.6.1-dual-target.37";
constexpr char BUILD_TIMESTAMP[] = "2026-11-10 CPA return scroll and Robinson type names";
#if FLIGHTWALL_HAS_MATRIX
constexpr char DEVICE_NAME[] = "FlightWall MatrixPortal";
constexpr char DEVICE_VARIANT[] = "matrixportal_s3";
constexpr char DEVICE_HOSTNAME[] = "matrixportal";
#else
constexpr char DEVICE_NAME[] = "FlightWall ESP32-S3 Server";
constexpr char DEVICE_VARIANT[] = "esp32s3_n16r8_headless";
constexpr char DEVICE_HOSTNAME[] = "flightwall-server";
#endif
constexpr uint8_t PAGE_COUNT = 8;
constexpr uint8_t BUTTON_UP_PIN = 6;
constexpr uint8_t BUTTON_DOWN_PIN = 7;
constexpr uint32_t WIFI_RETRY_MS = 60000;
constexpr char WIFI_SETUP_AP_PREFIX[] = "FlightWall-Setup";
constexpr char WIFI_SETUP_AP_PASSWORD[] = "flightwall";
constexpr uint32_t POLL_INTERVAL_MS = 5000;
constexpr uint32_t ADSB_PREFERRED_RETRY_MS = 60000;
constexpr uint32_t DEFAULT_PAGE_INTERVAL_MS = 5000;
constexpr uint8_t DEFAULT_RADAR_SWEEP_INTERVAL_SECONDS = 5;
constexpr uint32_t PAGE_TRANSITION_MS = 350;
constexpr uint32_t STALE_AFTER_MS = 20000;
constexpr uint32_t EMERGENCY_PAGE_DURATION_MS = 5000;
constexpr uint32_t EMERGENCY_ROTATION_MS = 10000;
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
constexpr float MOLD_HUMIDITY_ELEVATED_PERCENT = 60.0f;
constexpr float MOLD_HUMIDITY_HIGH_PERCENT = 65.0f;
constexpr float MOLD_HUMIDITY_ACUTE_PERCENT = 75.0f;
constexpr uint32_t MOLD_HIGH_DELAY_MS = 30UL * 60UL * 1000UL;
constexpr uint32_t MOLD_CLEAR_DELAY_MS = 15UL * 60UL * 1000UL;
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
constexpr float DEFAULT_FAR_ELEVATION_DEG = 30.0f;
// Viewing-direction geometry. The zenith angle is measured from straight up.
constexpr float DIRECTLY_ABOVE_ZENITH_DEG = 10.0f;
constexpr float ABOVE_ZENITH_DEG = 30.0f;
constexpr float ZENITH_HYSTERESIS_DEG = 2.0f;

constexpr char DEFAULT_ADSB_URL[] =
    "http://t-eddl146.fritz.box:8080/data/aircraft.json";
constexpr char ADSB_FI_BASE_URL[] = "https://opendata.adsb.fi/api/v3";
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

#if FLIGHTWALL_HAS_MATRIX
uint8_t rgbPins[] = {42, 41, 40, 38, 39, 37};
uint8_t addrPins[] = {45, 36, 48, 35};
constexpr uint8_t clockPin = 2;
constexpr uint8_t latchPin = 47;
constexpr uint8_t oePin = 14;
Adafruit_Protomatter matrix(
    MATRIX_WIDTH, BIT_DEPTH, 1, rgbPins, 4, addrPins,
    clockPin, latchPin, oePin, true);
#else
// Retain the common drawing code without allocating a HUB75 framebuffer or
// using the ESP32-S3 LCD/DMA peripheral. Headless builds never call it.
class HeadlessDisplay final : public Adafruit_GFX {
 public:
  HeadlessDisplay() : Adafruit_GFX(MATRIX_WIDTH, 32) {}
  void drawPixel(int16_t, int16_t, uint16_t) override {}
  void show() {}
  uint32_t getFrameCount() const { return 0; }
  uint16_t color565(uint8_t red, uint8_t green, uint8_t blue) const {
    return ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3);
  }
};
HeadlessDisplay matrix;
#endif
constexpr uint16_t AIRPORT_TEXT_CANVAS_WIDTH = 128 * 6;
GFXcanvas1 airportTextCanvas(AIRPORT_TEXT_CANVAS_WIDTH, 8);
TwoWire environmentWire(1);
Adafruit_BME280 environmentSensor;

struct FlightData {
  bool available = false;
  bool emergency = false;
  bool hasPosition = false;
  bool onGround = false;
  String hex;
  String flight;
  String squawk;
  String airline;
  String type;
  String typeDescription;
  String specialType;
  String direction;
  String verticalMotion;
  String motion;
  String selectionReason;
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
  uint32_t cpaReferenceMs = 0;
  float verticalRateFpm = NAN;
  float bearing = NAN;
  float relativeBearing = NAN;
  uint8_t emergencyCount = 0;
  uint8_t emergencyIndex = 0;
};

FlightData flight;
FlightData networkFlight;

struct RadarPoint {
  float distanceKm = 0.0F;
  float bearing = 0.0F;
  uint8_t flags = 0;
};

constexpr uint8_t RADAR_POINT_SELECTED = 0x01;
constexpr uint8_t RADAR_POINT_SPECIAL = 0x02;
constexpr uint8_t RADAR_POINT_EMERGENCY = 0x04;
constexpr uint8_t RADAR_POINT_GROUND = 0x08;
constexpr uint8_t RADAR_POINT_OUTSIDE_RADIUS = 0x10;
constexpr size_t RADAR_POINT_LIMIT = 64;
RadarPoint radarPoints[RADAR_POINT_LIMIT];
RadarPoint networkRadarPoints[RADAR_POINT_LIMIT];
size_t radarPointCount = 0;
size_t networkRadarPointCount = 0;
RadarTrailStorage *radarTrailStorage = nullptr;
bool radarTrailsEnabled = true;
// A separate display-core copy avoids reading configuration during rendering.
bool displayedRadarTrailsEnabled = true;

constexpr int32_t CPA_COUNTDOWN_HORIZON_SECONDS = 3 * 60;

int32_t cpaCountdownSeconds(const FlightData &value) {
  if (!value.available || value.motion == "departing" ||
      isnan(value.cpaMinutes) || !value.cpaReferenceMs) return -1;
  const int32_t initial = static_cast<int32_t>(roundf(value.cpaMinutes * 60.0f));
  if (initial < 0 || initial > CPA_COUNTDOWN_HORIZON_SECONDS) return -1;
  const int32_t elapsed = static_cast<int32_t>(
      (millis() - value.cpaReferenceMs) / 1000UL);
  const int32_t remaining = initial - elapsed;
  return remaining >= 0 ? remaining : -1;
}

String cpaCountdownText(const FlightData &value, bool compact = false) {
  const int32_t seconds = cpaCountdownSeconds(value);
  if (seconds < 0) return "";
  if (seconds < 60) return String(seconds) + "s";
  const int32_t minutes = seconds / 60;
  const int32_t rest = seconds % 60;
  if (compact) return String(minutes) + "m" + String(rest) + "s";
  return String(minutes) + ":" + (rest < 10 ? "0" : "") + String(rest);
}
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
DNSServer setupDnsServer;
WiFiClient mqttTransport;
PubSubClient mqttClient(mqttTransport);
SemaphoreHandle_t flightMutex;
SemaphoreHandle_t preferencesMutex;
SemaphoreHandle_t weatherMutex;
TaskHandle_t networkTaskHandle = nullptr;
TaskHandle_t displayTaskHandle = nullptr;

enum class NetworkPhase : uint8_t {
  Unknown,
  Idle,
  Web,
  Ota,
  Mqtt,
  Wifi,
  Adsb,
  Route,
  Flash,
  Count
};

constexpr uint32_t NETWORK_PHASE_RTC_MAGIC = 0x46574E50;  // "FWNP"
RTC_DATA_ATTR uint32_t rtcNetworkPhaseMagic = 0;
RTC_DATA_ATTR uint8_t rtcNetworkPhase =
    static_cast<uint8_t>(NetworkPhase::Unknown);
NetworkPhase activeNetworkPhase = NetworkPhase::Idle;
NetworkPhase previousResetNetworkPhase = NetworkPhase::Unknown;
uint32_t networkPhaseMaxMs[static_cast<size_t>(NetworkPhase::Count)] = {};

const char *networkPhaseName(NetworkPhase phase) {
  switch (phase) {
    case NetworkPhase::Idle: return "Idle";
    case NetworkPhase::Web: return "Webserver";
    case NetworkPhase::Ota: return "OTA";
    case NetworkPhase::Mqtt: return "MQTT";
    case NetworkPhase::Wifi: return "WLAN";
    case NetworkPhase::Adsb: return "ADS-B";
    case NetworkPhase::Route: return "Route";
    case NetworkPhase::Flash: return "Flash";
    default: return "unbekannt";
  }
}

class NetworkPhaseScope {
 public:
  explicit NetworkPhaseScope(NetworkPhase phase)
      : phase_(phase), previous_(activeNetworkPhase), started_(millis()) {
    activeNetworkPhase = phase_;
    rtcNetworkPhaseMagic = NETWORK_PHASE_RTC_MAGIC;
    rtcNetworkPhase = static_cast<uint8_t>(phase_);
  }

  ~NetworkPhaseScope() {
    const uint32_t elapsed = millis() - started_;
    const size_t index = static_cast<size_t>(phase_);
    if (index < static_cast<size_t>(NetworkPhase::Count) &&
        elapsed > networkPhaseMaxMs[index]) {
      networkPhaseMaxMs[index] = elapsed;
    }
    activeNetworkPhase = previous_;
    rtcNetworkPhase = static_cast<uint8_t>(previous_);
  }

 private:
  NetworkPhase phase_;
  NetworkPhase previous_;
  uint32_t started_;
};

void initializeNetworkDiagnostics() {
  if (esp_reset_reason() == ESP_RST_TASK_WDT &&
      rtcNetworkPhaseMagic == NETWORK_PHASE_RTC_MAGIC &&
      rtcNetworkPhase < static_cast<uint8_t>(NetworkPhase::Count)) {
    previousResetNetworkPhase = static_cast<NetworkPhase>(rtcNetworkPhase);
  }
  rtcNetworkPhaseMagic = NETWORK_PHASE_RTC_MAGIC;
  rtcNetworkPhase = static_cast<uint8_t>(NetworkPhase::Idle);
  activeNetworkPhase = NetworkPhase::Idle;
}

String networkPhaseTimingText() {
  String text;
  for (uint8_t raw = static_cast<uint8_t>(NetworkPhase::Web);
       raw < static_cast<uint8_t>(NetworkPhase::Count); ++raw) {
    if (text.length()) text += " / ";
    const NetworkPhase phase = static_cast<NetworkPhase>(raw);
    text += String(networkPhaseName(phase)) + " " +
            String(networkPhaseMaxMs[raw]) + " ms";
  }
  return text;
}
bool webServerStarted = false;
bool webServerConfigured = false;
bool otaStarted = false;
bool wifiWasConnected = false;
volatile bool wifiSetupAccessPointActive = false;
bool wifiSettingsChanged = false;
String wifiSettingsError;
String wifiSsid = WIFI_SSID;
String wifiPassword = WIFI_PASSWORD;
bool otaSettingsChanged = false;
String otaSettingsError;
String otaPassword = OTA_PASSWORD;
String wifiSetupAccessPointSsid;
bool wifiScanRequested = false;
uint32_t wifiScanStartedAt = 0;
String wifiScanResultJson;
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
uint8_t statusLedBrightnessMode = 1;
volatile uint32_t statusLedErrorUntil = 0;
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
float dewPointC = NAN;
float moldCriticalSurfaceTemperatureC = NAN;
enum class MoldRiskLevel : uint8_t { Unavailable, Normal, Elevated, High };
MoldRiskLevel moldRiskLevel = MoldRiskLevel::Unavailable;
uint32_t elevatedHumiditySince = 0;
uint32_t highHumiditySince = 0;
uint32_t lowHumiditySince = 0;
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
float windDirectionDegrees = NAN;
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
float farDirectionElevationDeg = DEFAULT_FAR_ELEVATION_DEG;
uint32_t pageIntervalMs = DEFAULT_PAGE_INTERVAL_MS;
float lowLuxThreshold = 20.0f;
float highLuxThreshold = 800.0f;
String adsbUrl1 = DEFAULT_ADSB_URL;
String adsbUrl2;
uint8_t activeAdsbSource = 0;
uint8_t currentAdsbSource = 0;
bool automaticAdsbFailover = true;
uint32_t adsbFailoverSince = 0;
uint32_t lastPreferredAdsbProbe = 0;
String routeUrl = DEFAULT_ROUTE_URL;
double siteLatitude = DEFAULT_SITE_LATITUDE;
double siteLongitude = DEFAULT_SITE_LONGITUDE;
float viewHeading = DEFAULT_VIEW_HEADING;
bool routesEnabled = true;
bool adsbDbFallbackEnabled = true;
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
uint32_t emergencySelectionStarted = 0;
String emergencyTestCode;
uint8_t emergencyTestCount = 0;
uint32_t emergencyTestUntil = 0;
uint32_t lastRouteAttempt = 0;
size_t lastReceivedAircraft = 0;
size_t lastCandidateAircraft = 0;

struct RouteObservation {
  String callsign;
  String hex;
  uint32_t lastSeenMs = 0;
};
constexpr size_t ROUTE_OBSERVATION_SLOTS = 16;
constexpr uint32_t ROUTE_SESSION_GAP_MS = 30U * 60U * 1000U;
RouteObservation routeObservations[ROUTE_OBSERVATION_SLOTS];
size_t nextRouteObservationSlot = 0;

bool routeObservationRequiresRefresh(const Aircraft& aircraft,
                                     uint32_t nowMs) {
  if (aircraft.flight.isEmpty()) return false;
  RouteObservation* observation = nullptr;
  for (RouteObservation& candidate : routeObservations) {
    if (candidate.callsign == aircraft.flight) {
      observation = &candidate;
      break;
    }
  }
  if (observation == nullptr) {
    observation = &routeObservations[
        nextRouteObservationSlot++ % ROUTE_OBSERVATION_SLOTS];
    *observation = {};
    observation->callsign = aircraft.flight;
  }
  const bool returnedAfterGap = observation->lastSeenMs != 0 &&
      nowMs - observation->lastSeenMs >= ROUTE_SESSION_GAP_MS;
  const bool aircraftChanged = !observation->hex.isEmpty() &&
      !aircraft.hex.isEmpty() && observation->hex != aircraft.hex;
  observation->hex = aircraft.hex;
  observation->lastSeenMs = nowMs;
  return returnedAfterGap || aircraftChanged;
}

enum class SpecialStatisticCategory : uint8_t {
  Emergency7500,
  Emergency7600,
  Emergency7700,
  GermanAirforce,
  Military,
  FederalPolice,
  Police,
  Government,
  Helicopter,
  Airship,
  A380,
  Other,
  Count
};

struct SeenSpecialAircraft {
  uint64_t identity = 0;
  SpecialStatisticCategory category = SpecialStatisticCategory::Other;
};

constexpr size_t SPECIAL_AIRCRAFT_TRACKING_LIMIT = 256;
SeenSpecialAircraft seenSpecialAircraft[SPECIAL_AIRCRAFT_TRACKING_LIMIT];
size_t seenSpecialAircraftCount = 0;
uint32_t specialAircraftCounts[
    static_cast<size_t>(SpecialStatisticCategory::Count)] = {};
bool specialAircraftTrackingLimitReached = false;
constexpr size_t OBSERVED_AIRCRAFT_TRACKING_LIMIT = 4096;
uint64_t seenAircraft[OBSERVED_AIRCRAFT_TRACKING_LIMIT];
size_t seenAircraftCount = 0;
bool aircraftTrackingLimitReached = false;
uint32_t dailyStatisticsDate = 0;
time_t dailyStatisticsStarted = 0;

struct DailyJournalEntry {
  uint64_t identity = 0;
  time_t firstSeen = 0;
  time_t lastSeen = 0;
  PsramText hex;
  PsramText flight;
  PsramText airline;
  PsramText type;
  PsramText typeDescription;
  PsramText specialType;
  PsramText squawk;
  PsramText routeCodes;
  PsramText routeNames;
  PsramText selectionReason;
  float minimumDistanceKm = NAN;
  float minimumCpaKm = NAN;
  float altitudeFt = NAN;
};

constexpr size_t DAILY_JOURNAL_LIMIT = 256;
constexpr size_t DAILY_JOURNAL_STATUS_ROWS = 64;
DailyJournalEntry dailyJournal[DAILY_JOURNAL_LIMIT];
size_t dailyJournalCount = 0;
bool dailyJournalLimitReached = false;

struct SpecialChronicleEntry {
  uint64_t identity = 0;
  time_t firstSeen = 0;
  time_t lastSeen = 0;
  PsramText hex;
  PsramText flight;
  PsramText type;
  PsramText typeDescription;
  PsramText specialType;
  PsramText squawk;
  PsramText routeCodes;
  PsramText selectionReason;
  float minimumDistanceKm = NAN;
  float minimumCpaKm = NAN;
  bool displayed = false;
};

constexpr size_t SPECIAL_CHRONICLE_LIMIT = 64;
SpecialChronicleEntry specialChronicle[SPECIAL_CHRONICLE_LIMIT];
size_t specialChronicleCount = 0;
bool specialChronicleLimitReached = false;

String customPageText;
uint8_t customScrollSpeed = 1;
uint8_t radarSweepIntervalSeconds = DEFAULT_RADAR_SWEEP_INTERVAL_SECONDS;
bool pageEnabled[] = {true, true, true, true, true, true, true, true};
uint8_t pageOrder[] = {0, 1, 2, 3, 4, 5, 6, 7};
uint32_t pageDurationMs[] = {
    DEFAULT_PAGE_INTERVAL_MS, DEFAULT_PAGE_INTERVAL_MS,
    DEFAULT_PAGE_INTERVAL_MS, DEFAULT_PAGE_INTERVAL_MS,
    DEFAULT_PAGE_INTERVAL_MS, DEFAULT_PAGE_INTERVAL_MS,
    DEFAULT_PAGE_INTERVAL_MS, DEFAULT_PAGE_INTERVAL_MS};
bool smoothTransitions = true;
uint8_t currentPageSlot = 0;
uint32_t currentPageStarted = 0;
uint32_t radarSweepStarted = 0;
uint32_t radarSweepLastDraw = 0;
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


float elevationAngleDegrees() {
  const float zenith = zenithAngleDegrees();
  return isnan(zenith) ? NAN : 90.0f - zenith;
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

  const float elevation = elevationAngleDegrees();
  if (!isnan(elevation)) {
    const float farLimit = farDirectionElevationDeg +
        (lastLookRange == LookRange::Far ? ZENITH_HYSTERESIS_DEG : 0.0f);
    if (elevation <= farLimit) {
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


String localizedCompassToken(String direction) {
  if (!uiEnglish) direction.replace("E", "O");
  return direction;
}


void drawMainPage() {
  drawHeader();

  String metrics;
  if (!isnan(flight.distanceKm)) metrics += String(flight.distanceKm, 1) + "km ";
  metrics += clipped(localizedCompassToken(flight.direction), 2);
  if (flight.onGround) {
    metrics += " ";
    metrics += uiText(flight.verticalMotion == "rollt" ? "rollt" :
                          flight.verticalMotion == "steht" ? "steht" :
                          "am Boden",
                      flight.verticalMotion == "rollt" ? "taxiing" :
                          flight.verticalMotion == "steht" ? "standing" :
                          "on ground");
  } else if (!isnan(flight.altitudeFt)) {
    metrics += " " + String(static_cast<int>(round(flight.altitudeFt))) + "ft";
  }
  const String shownMetrics = clipped(metrics, 19);
  drawLine(0, 16, shownMetrics, white);
  drawCompactAltitudeIndicatorAt(121, 16);

  drawLookArrowAt(flight.relativeBearing, 0, 24, yellow);
  drawLine(10, 24, compactLookDirection(flight.relativeBearing), yellow);
}


bool flightHasClosedLoopRoute() {
  String codes = flight.routeCodes;
  codes.trim();
  const int firstSeparator = codes.indexOf('-');
  const int lastSeparator = codes.lastIndexOf('-');
  if (firstSeparator <= 0 || lastSeparator <= firstSeparator ||
      lastSeparator >= static_cast<int>(codes.length()) - 1) {
    return false;
  }

  String first = codes.substring(0, firstSeparator);
  String last = codes.substring(lastSeparator + 1);
  first.trim();
  last.trim();
  first.toUpperCase();
  last.toUpperCase();
  return first.length() && first == last;
}


String compactLoopRouteCodes() {
  String route = flight.routeCodes;
  if (flightHasClosedLoopRoute()) {
    route.remove(route.lastIndexOf('-'));
    route.replace("-", "<>");
  } else {
    route.replace("-", ">");
  }
  return route;
}


String compactLoopRouteNames() {
  String names = displayAscii(flight.routeNames);
  if (names.isEmpty()) return compactLoopRouteCodes();
  if (!flightHasClosedLoopRoute()) return names;

  const int finalSeparator = names.lastIndexOf(" > ");
  if (finalSeparator > 0) names.remove(finalSeparator);
  names.replace(" > ", " <> ");
  return names;
}


String compactRoute() {
  if (flight.routeCodes.length()) {
    return clipped(compactLoopRouteCodes(), 21);
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


uint32_t airportScrollDuration(const String &text, int16_t regionWidth);
int16_t airportTextX(const String &text, int16_t regionWidth, uint32_t elapsedMs);

uint32_t scrollingTextDuration(const String &text);
int16_t scrollingTextX(const String &text, uint32_t elapsedMs);


String cpaPageDetails() {
  String details = flight.specialType;
  String description = flight.typeDescription;
  description.trim();
  if (!description.length()) description = flight.type;
  if (description.length()) {
    if (details.length()) details += " | ";
    details += description;
  }
  return displayAscii(details);
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
    const String countdown = cpaCountdownText(flight);
    cpa += " in " + (countdown.length()
                         ? countdown
                         : String(static_cast<int>(round(flight.cpaMinutes))) + "min");
  }
  drawLine(0, 16, clipped(cpa, 21), departing ? orange : green);

  const String details = cpaPageDetails();
  const uint32_t elapsed = millis() - currentPageStarted;
  drawLine(airportTextX(details, MATRIX_WIDTH, elapsed), 24, details, aircraftColor());
}


uint32_t cpaPageDuration() {
  return airportScrollDuration(cpaPageDetails(), MATRIX_WIDTH);
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
  const String altitude = flight.onGround
      ? uiText(flight.verticalMotion == "rollt" ? "rollt" :
                   flight.verticalMotion == "steht" ? "steht" : "Boden",
               flight.verticalMotion == "rollt" ? "taxi" :
                   flight.verticalMotion == "steht" ? "stand" : "ground")
      : (isnan(flight.altitudeFt)
             ? "?ft"
             : String(static_cast<int>(round(flight.altitudeFt))) + "ft");
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
      const String countdown = cpaCountdownText(flight, true);
      const String withMinutes = cpa + "/" +
          (countdown.length() ? countdown :
           String(static_cast<int>(round(flight.cpaMinutes))) + "m");
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
  if (textWidth <= static_cast<uint32_t>(regionWidth)) return 0;
  const uint32_t travelPixels = textWidth - regionWidth;
  return AIRPORT_HALF_PAUSE_MS +
         travelPixels * AIRPORT_HALF_PIXEL_MS +
         AIRPORT_HALF_PAUSE_MS +
         travelPixels * AIRPORT_RETURN_PIXEL_MS +
         AIRPORT_HALF_PAUSE_MS;
}


uint32_t compactAirportsNamePhaseDuration() {
  if (flightHasClosedLoopRoute()) {
    return max(airportScrollDuration(
                   limitedAirportText(compactLoopRouteNames()), MATRIX_WIDTH),
               AIRPORT_NAMES_MINIMUM_MS);
  }

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

  if (flightHasClosedLoopRoute()) {
    const String route = limitedAirportText(compactLoopRouteNames());
    drawAirportRegion(route, 0, 24, MATRIX_WIDTH,
                      airportTextX(route, MATRIX_WIDTH, elapsed), white);
    return;
  }

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


void drawWifiSetupPage() {
  matrix.fillScreen(0);
  drawLine(0, 0, "WLAN SETUP", yellow);
  const String ssidText = "SSID " + wifiSetupAccessPointSsid;
  const uint32_t scrollDuration = scrollingTextDuration(ssidText);
  const uint32_t scrollElapsed = scrollDuration
      ? millis() % scrollDuration : 0;
  drawLine(scrollingTextX(ssidText, scrollElapsed), 8, ssidText, cyan);
  drawLine(0, 16, String("PW ") + WIFI_SETUP_AP_PASSWORD, orange);
  drawLine(0, 24, "CONFIG IP 192.168.4.1", green);
  showMatrix();
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
  const String position = flight.emergencyCount > 1
                              ? String(flight.emergencyIndex) + "/" +
                                    String(flight.emergencyCount)
                              : "";
  const uint8_t identityCharacters = position.length()
                                         ? 20 - position.length()
                                         : 21;
  drawLine(0, 0, clipped(identity, identityCharacters), white);
  if (position.length())
    drawLine(MATRIX_WIDTH - position.length() * 6, 0, position, white);

  const String squawk = isEmergencySquawk(flight.squawk)
                            ? flight.squawk
                            : "7???";
  const uint16_t alertColor = squawk == "7600" ? yellow : red;
  const int16_t squawkX = (MATRIX_WIDTH - squawk.length() * 12) / 2;
  drawLine(squawkX, 8, squawk, alertColor, 2);

  const String meaning = squawk == "7???"
                             ? "ALARM"
                             : emergencyMeaning(squawk);
  int16_t meaningX = (MATRIX_WIDTH - meaning.length() * 6) / 2;
  if (meaningX < 0) meaningX = 0;
  drawLine(meaningX, 24, meaning, alertColor);
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
    if (weatherValueFresh(windDirectionUpdated) &&
        isfinite(windDirectionDegrees))
      direction = weatherCompassDirection(windDirectionDegrees);
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
  const uint16_t humidityColor = moldRiskLevel == MoldRiskLevel::High
                                     ? red
                                     : moldRiskLevel == MoldRiskLevel::Elevated
                                           ? yellow
                                           : cyan;
  drawLine(humidityX, 5, humidityText, humidityColor);
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


uint16_t radarPointColor(const RadarPoint &point) {
  if (point.flags & RADAR_POINT_EMERGENCY) return red;
  if (point.flags & RADAR_POINT_SELECTED) return yellow;
  if (point.flags & RADAR_POINT_SPECIAL) return violet;
  if (point.flags & RADAR_POINT_GROUND) return orange;
  return cyan;
}


constexpr int16_t RADAR_CENTER_X = MATRIX_WIDTH / 2;
constexpr int16_t RADAR_CENTER_Y = 15;
constexpr int16_t RADAR_RADIUS_PIXELS = 15;
constexpr uint8_t RADAR_RADIUS_DOT_COUNT = 32;
constexpr uint32_t RADAR_SWEEP_PERIOD_MS = 800;
constexpr uint32_t RADAR_SWEEP_DRAW_INTERVAL_MS = 25;
constexpr uint32_t RADAR_SWEEP_SESSION_GAP_MS = 750;
// The furthest matrix corner is about 4.4 configured radii away when the
// same distance scale is used on both axes. A small margin avoids losing
// edge pixels through rounding or receiver-side distance filtering.
constexpr float RADAR_COVERAGE_RADIUS_FACTOR = 4.5F;


float radarCoverageRadiusKm() {
  return displayRadiusKm * RADAR_COVERAGE_RADIUS_FACTOR;
}


bool radarDataCollectionEnabled() {
#if FLIGHTWALL_HAS_MATRIX
  return pageEnabled[7];
#else
  // The headless controller supplies the web radar independently of the
  // matrix-page settings retained in the shared configuration format.
  return true;
#endif
}


bool radarPointCoordinates(float distanceKm, float bearing,
                           int16_t &x, int16_t &y) {
  if (displayRadiusKm <= 0.0F || !isfinite(distanceKm) ||
      !isfinite(bearing)) return false;
  const float pixelsPerKm = RADAR_RADIUS_PIXELS / displayRadiusKm;
  const float relativeRadians = (bearing - viewHeading) * PI / 180.0F;
  x = RADAR_CENTER_X + static_cast<int16_t>(
      roundf(sinf(relativeRadians) * distanceKm * pixelsPerKm));
  y = RADAR_CENTER_Y - static_cast<int16_t>(
      roundf(cosf(relativeRadians) * distanceKm * pixelsPerKm));
  return x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < matrix.height();
}


void drawRadarPage() {
  // Trails are drawn first so the ring, own position and live targets win.
  if (displayedRadarTrailsEnabled && radarTrailStorage && transitionPercent > 0) {
    const uint32_t now = millis();
    for (size_t i = 0; i < radarPointCount; ++i) {
      const RadarPoint &point = radarPoints[i];
      const uint16_t base = radarPointColor(point);
      const auto &trail = radarTrailStorage->consumer[i];
      for (uint8_t j = 0; j < trail.trailCount; ++j) {
        const RadarTrailDot &dot = trail.trail[j];
        const uint32_t age = now - dot.time;
        if (age >= RadarTrails::LifetimeMs) continue;
        // At minimum brightness a distinct darker native level is impossible.
        // Keep nonzero channels at the first visible level, never above base.
        const uint8_t level = age < 30000 ? 3 : 1;
        auto dimChannel = [level](uint8_t channel) -> uint8_t {
          if (!channel) return 0;
          return min(channel, static_cast<uint8_t>(max(36, channel * level / 7)));
        };
        const uint16_t color = matrix.color565(
            dimChannel(((base >> 11) & 31) * 255 / 31),
            dimChannel(((base >> 5) & 63) * 255 / 63),
            dimChannel((base & 31) * 255 / 31));
        matrix.drawPixel(dot.x, dot.y, color);
      }
    }
  }
  for (uint8_t dot = 0; dot < RADAR_RADIUS_DOT_COUNT; ++dot) {
    const float angle = 2.0F * PI * dot / RADAR_RADIUS_DOT_COUNT;
    matrix.drawPixel(
        RADAR_CENTER_X + static_cast<int16_t>(
            roundf(cosf(angle) * RADAR_RADIUS_PIXELS)),
        RADAR_CENTER_Y + static_cast<int16_t>(
            roundf(sinf(angle) * RADAR_RADIUS_PIXELS)),
        blue);
  }
  const uint32_t now = millis();
  if (radarSweepIntervalSeconds > 0) {
    if (!radarSweepStarted ||
        now - radarSweepLastDraw > RADAR_SWEEP_SESSION_GAP_MS)
      radarSweepStarted = now;
    radarSweepLastDraw = now;
    const uint32_t sweepCycleMs =
        static_cast<uint32_t>(radarSweepIntervalSeconds) * 1000U;
    const uint32_t sweepElapsed =
        (now - radarSweepStarted) % sweepCycleMs;
    if (sweepElapsed < RADAR_SWEEP_PERIOD_MS) {
      // Include both ends of the circle. The final visible position is the
      // same 12-o'clock pixel at which the sweep started.
      constexpr uint32_t sweepEndpointMs =
          RADAR_SWEEP_PERIOD_MS - RADAR_SWEEP_DRAW_INTERVAL_MS;
      uint32_t sweepDot =
          (sweepElapsed * RADAR_RADIUS_DOT_COUNT + sweepEndpointMs / 2U) /
          sweepEndpointMs;
      if (sweepDot > RADAR_RADIUS_DOT_COUNT)
        sweepDot = RADAR_RADIUS_DOT_COUNT;
      // Start at 12 o'clock and advance clockwise exactly once per interval.
      const float sweepAngle = -PI / 2.0F +
          2.0F * PI * sweepDot / RADAR_RADIUS_DOT_COUNT;
      matrix.drawPixel(
          RADAR_CENTER_X + static_cast<int16_t>(
              roundf(cosf(sweepAngle) * RADAR_RADIUS_PIXELS)),
          RADAR_CENTER_Y + static_cast<int16_t>(
              roundf(sinf(sweepAngle) * RADAR_RADIUS_PIXELS)),
          white);
    }
  } else {
    radarSweepStarted = 0;
    radarSweepLastDraw = now;
  }
  // Own position remains visible even when no aircraft is present.
  matrix.drawPixel(RADAR_CENTER_X, RADAR_CENTER_Y, green);
  matrix.drawPixel(RADAR_CENTER_X - 1, RADAR_CENTER_Y, green);
  matrix.drawPixel(RADAR_CENTER_X + 1, RADAR_CENTER_Y, green);
  matrix.drawPixel(RADAR_CENTER_X, RADAR_CENTER_Y - 1, green);
  matrix.drawPixel(RADAR_CENTER_X, RADAR_CENTER_Y + 1, green);

  const bool blinkOn = (millis() / 500U) % 2U == 0;
  for (uint8_t pass = 0; pass < 2; ++pass) {
    for (size_t index = 0; index < radarPointCount; ++index) {
      const RadarPoint &point = radarPoints[index];
      const bool selectedPoint = point.flags & RADAR_POINT_SELECTED;
      if (selectedPoint != (pass == 1)) continue;
      if ((point.flags & RADAR_POINT_EMERGENCY) && !blinkOn) continue;
      int16_t x = 0;
      int16_t y = 0;
      if (!radarPointCoordinates(point.distanceKm, point.bearing, x, y))
        continue;
      const uint16_t color = radarPointColor(point);
      matrix.drawPixel(x, y, color);
      if (selectedPoint) {
        matrix.drawPixel(x - 1, y, color);
        matrix.drawPixel(x + 1, y, color);
        matrix.drawPixel(x, y - 1, color);
        matrix.drawPixel(x, y + 1, color);
      }
    }
  }
  if (lastError.length() &&
      (millis() / IDLE_BLINK_HALF_PERIOD_MS) % 2 == 0)
    matrix.drawPixel(0, 31, idleErrorPixel);
}


void drawIdleDisplay() {
  matrix.fillScreen(0);
  const bool bmeAvailable = environmentIdleDisplayEnabled &&
      environmentSensorAvailable && isfinite(ambientTemperatureC) &&
      isfinite(relativeHumidityPercent) && isfinite(airPressureHpa);
  const bool mqttAvailable = environmentIdleDisplayEnabled &&
      mqttWeatherAvailable();
  bool radarIdleVisible = false;
  if (bmeAvailable || mqttAvailable) {
    drawCompactClimateDisplay(bmeAvailable, mqttAvailable);
  } else if (!environmentIdleDisplayEnabled && pageEnabled[7]) {
    drawRadarPage();
    radarIdleVisible = true;
  }
  if (!radarIdleVisible &&
      (millis() / IDLE_BLINK_HALF_PERIOD_MS) % 2 == 0) {
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
  currentPageSlot = (currentPageSlot + 1) % PAGE_COUNT;
  selectEnabledPage();
}


void drawConfiguredPage(uint8_t page) {
  switch (page) {
    case 0: drawMainPage(); break;
    case 1: drawRoutePage(); break;
    case 2: drawCpaPage(); break;
    case 3: drawAirportNamesPage(); break;
    case 4: drawCustomTextPage(); break;
    case 5: drawCompactPage(); break;
    case 6: drawCompactAirportsPage(); break;
    default: drawRadarPage(); break;
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
  if (wifiSetupAccessPointActive) {
    resetPageTransition();
    drawWifiSetupPage();
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
    }
    showMatrix();
    return;
  }

  const uint32_t now = millis();
  if (currentPageStarted == 0) currentPageStarted = now;
  const uint8_t page = pageOrder[currentPageSlot];
  uint32_t duration = pageDurationMs[page];
  if (page == 2) duration = max(duration, cpaPageDuration());
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
#if FLIGHTWALL_HAS_MATRIX
                onStemma ? "STEMMA QT" : "BH1750");
#else
                "GPIO 16/17");
#endif
  return true;
}


float pressureReducedToSeaLevel(float pressureHpa, float elevationM) {
  const float altitudeFactor = 1.0f - elevationM / 44330.0f;
  if (!isfinite(pressureHpa) || altitudeFactor <= 0.0f) return NAN;
  return pressureHpa / powf(altitudeFactor, 5.255f);
}


void resetMoldRisk() {
  dewPointC = NAN;
  moldCriticalSurfaceTemperatureC = NAN;
  moldRiskLevel = MoldRiskLevel::Unavailable;
  elevatedHumiditySince = 0;
  highHumiditySince = 0;
  lowHumiditySince = 0;
}


float magnusTemperature(float gamma) {
  constexpr float MAGNUS_A = 17.62f;
  constexpr float MAGNUS_B_C = 243.12f;
  if (!isfinite(gamma) || gamma >= MAGNUS_A) return NAN;
  return MAGNUS_B_C * gamma / (MAGNUS_A - gamma);
}


void updateMoldRisk(float temperatureC, float humidityPercent,
                    uint32_t now) {
  if (!isfinite(temperatureC) || !isfinite(humidityPercent) ||
      humidityPercent <= 0.0f || humidityPercent > 100.0f) {
    resetMoldRisk();
    return;
  }

  constexpr float MAGNUS_A = 17.62f;
  constexpr float MAGNUS_B_C = 243.12f;
  const float saturationTerm =
      MAGNUS_A * temperatureC / (MAGNUS_B_C + temperatureC);
  dewPointC = magnusTemperature(
      logf(humidityPercent / 100.0f) + saturationTerm);
  // Surface temperature at which the room-air water vapour reaches 80 % RH.
  moldCriticalSurfaceTemperatureC = magnusTemperature(
      logf((humidityPercent / 100.0f) / 0.80f) + saturationTerm);

  if (humidityPercent >= MOLD_HUMIDITY_ELEVATED_PERCENT) {
    if (!elevatedHumiditySince) elevatedHumiditySince = now;
    lowHumiditySince = 0;
  } else {
    elevatedHumiditySince = 0;
  }

  if (humidityPercent >= MOLD_HUMIDITY_HIGH_PERCENT) {
    if (!highHumiditySince) highHumiditySince = now;
  } else {
    highHumiditySince = 0;
  }

  const bool acute = humidityPercent >= MOLD_HUMIDITY_ACUTE_PERCENT;
  const bool sustainedHigh = highHumiditySince &&
      now - highHumiditySince >= MOLD_HIGH_DELAY_MS;
  if (acute || sustainedHigh) {
    moldRiskLevel = MoldRiskLevel::High;
    return;
  }

  if (moldRiskLevel == MoldRiskLevel::High) {
    if (humidityPercent < MOLD_HUMIDITY_ELEVATED_PERCENT) {
      if (!lowHumiditySince) lowHumiditySince = now;
      if (now - lowHumiditySince >= MOLD_CLEAR_DELAY_MS) {
        moldRiskLevel = MoldRiskLevel::Normal;
      }
    }
    return;
  }

  moldRiskLevel = humidityPercent >= MOLD_HUMIDITY_ELEVATED_PERCENT
                      ? MoldRiskLevel::Elevated
                      : MoldRiskLevel::Normal;
}


bool findEnvironmentSensor() {
  lastEnvironmentSensorProbe = millis();
  const uint8_t addresses[] = {
      BME280_PREFERRED_ADDRESS, BME280_ALTERNATE_ADDRESS};
  for (const uint8_t address : addresses) {
    if (beginEnvironmentSensorAt(Wire, address, false)) return true;
  }
#if FLIGHTWALL_HAS_MATRIX
  for (const uint8_t address : addresses) {
    if (beginEnvironmentSensorAt(environmentWire, address, true)) return true;
  }
#endif
  environmentSensorAvailable = false;
  environmentSensorAddress = 0;
  ambientTemperatureC = NAN;
  relativeHumidityPercent = NAN;
  stationPressureHpa = NAN;
  airPressureHpa = NAN;
  resetMoldRisk();
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
      resetMoldRisk();
      Serial.println("BME280 stopped responding");
    }
    return;
  }
  environmentSensorFailures = 0;
  ambientTemperatureC = temperature;
  relativeHumidityPercent = humidity;
  stationPressureHpa = pressure;
  airPressureHpa = pressureReducedToSeaLevel(pressure, observerElevationM);
  updateMoldRisk(temperature, humidity, now);
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
    displayedRadarTrailsEnabled = radarTrailsEnabled;
    radarPointCount = min(networkRadarPointCount, RADAR_POINT_LIMIT);
    for (size_t index = 0; index < radarPointCount; ++index)
      radarPoints[index] = networkRadarPoints[index];
#if FLIGHTWALL_HAS_MATRIX
    if (radarTrailStorage) radarTrailStorage->publish(radarPointCount);
#endif
    lastSuccessfulPoll = networkLastSuccessfulPoll;
    lastError = networkLastError;
    xSemaphoreGive(flightMutex);
  }
}


bool connectWiFi(uint32_t timeoutMs) {
  if (WiFi.status() == WL_CONNECTED) return true;
  WiFi.mode(wifiSetupAccessPointActive ? WIFI_AP_STA : WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());

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


String adsbFiUrl() {
  // adsb.fi expects nautical miles. Include the complete rectangular radar
  // viewport when enabled; selection still uses the configured circular
  // radius and the doubled radius for specials.
  const float requiredKm = max(
      radarDataCollectionEnabled()
          ? radarCoverageRadiusKm() : displayRadiusKm * 2.0F,
      18.52F);
  const uint16_t radiusNm = constrain(
      static_cast<uint16_t>(ceilf(requiredKm / 1.852F)),
      static_cast<uint16_t>(10), static_cast<uint16_t>(250));
  return String(ADSB_FI_BASE_URL) + "/lat/" + String(siteLatitude, 6) +
         "/lon/" + String(siteLongitude, 6) + "/dist/" + String(radiusNm);
}


String adsbUrlForSource(uint8_t source) {
  if (source == 2) return adsbFiUrl();
  return source == 1 ? adsbUrl2 : adsbUrl1;
}


bool adsbSourceConfigured(uint8_t source) {
  return source == 2 || (source == 1 ? adsbUrl2.length() : adsbUrl1.length());
}


String adsbSourceName(uint8_t source) {
  if (source == 2) return "adsb.fi";
  return String(uiText("Quelle ", "Source ")) + String(source + 1);
}


String activeAdsbUrl() {
  return adsbUrlForSource(currentAdsbSource);
}


String activeAdsbSourceName() {
  return adsbSourceName(currentAdsbSource);
}


String airlineCode(const String &callsign) {
  if (isGermanAirforceCallsign(callsign)) return "GAF";
  if (isMilitaryCallsign(callsign)) return "";
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
  if (aircraft.onGround) {
    if (!aircraft.hasSpeed) return "am Boden";
    return aircraftIsStanding(aircraft) ? "steht" : "rollt";
  }
  if (!aircraft.hasVerticalRate) return "unbekannt";
  if (aircraft.verticalRateFpm > 64) return "steigt";
  if (aircraft.verticalRateFpm < -64) return "sinkt";
  return "haelt";
}


bool isEmergencyStatisticCategory(SpecialStatisticCategory category) {
  return category == SpecialStatisticCategory::Emergency7500 ||
         category == SpecialStatisticCategory::Emergency7600 ||
         category == SpecialStatisticCategory::Emergency7700;
}


uint8_t specialStatisticPriority(SpecialStatisticCategory category) {
  if (isEmergencyStatisticCategory(category)) return 0;
  switch (category) {
    case SpecialStatisticCategory::GermanAirforce: return 1;
    case SpecialStatisticCategory::FederalPolice: return 2;
    case SpecialStatisticCategory::Police: return 3;
    case SpecialStatisticCategory::Military: return 4;
    case SpecialStatisticCategory::Government: return 5;
    case SpecialStatisticCategory::Helicopter: return 6;
    case SpecialStatisticCategory::Airship: return 7;
    case SpecialStatisticCategory::A380: return 8;
    default: return 9;
  }
}


bool specialStatisticCategory(const Aircraft &aircraft,
                              SpecialStatisticCategory &category) {
  if (aircraft.emergency) {
    if (aircraft.squawk == "7500")
      category = SpecialStatisticCategory::Emergency7500;
    else if (aircraft.squawk == "7600")
      category = SpecialStatisticCategory::Emergency7600;
    else if (aircraft.squawk == "7700")
      category = SpecialStatisticCategory::Emergency7700;
    else
      return false;
    return true;
  }
  if (aircraft.specialTypes.empty()) return false;

  String specialType = aircraft.specialTypes.front();
  specialType.toUpperCase();
  if (specialType.indexOf("AIRFORCE") >= 0)
    category = SpecialStatisticCategory::GermanAirforce;
  else if (specialType.indexOf("BUNDESPOLIZEI") >= 0)
    category = SpecialStatisticCategory::FederalPolice;
  else if (specialType.indexOf("POLICE") >= 0 ||
           specialType.indexOf("POLIZEI") >= 0)
    category = SpecialStatisticCategory::Police;
  else if (specialType.indexOf("MILIT") >= 0)
    category = SpecialStatisticCategory::Military;
  else if (specialType.indexOf("GOVERNMENT") >= 0 ||
           specialType.indexOf("REGIERUNG") >= 0)
    category = SpecialStatisticCategory::Government;
  else if (specialType.indexOf("HELICOPTER") >= 0 ||
           specialType.indexOf("HUBSCHRAUBER") >= 0)
    category = SpecialStatisticCategory::Helicopter;
  else if (specialType.indexOf("AIRSHIP") >= 0 ||
           specialType.indexOf("LUFTSCHIFF") >= 0 ||
           specialType.indexOf("SHIP") >= 0)
    category = SpecialStatisticCategory::Airship;
  else if (specialType.indexOf("A380") >= 0)
    category = SpecialStatisticCategory::A380;
  else
    category = SpecialStatisticCategory::Other;
  return true;
}


String specialCategoryName(const Aircraft &aircraft) {
  if (aircraft.emergency) return emergencyMeaning(aircraft.squawk);
  if (!aircraft.specialTypes.empty())
    return specialTypeDisplayName(aircraft.specialTypes.front());
  return uiText("Sonstige", "Other");
}


void updateSpecialChronicleObservation(const Aircraft &aircraft) {
  const uint64_t identity = aircraftIdentity(aircraft);
  if (!identity || aircraft.flight.startsWith("TEST")) return;
  size_t index = 0;
  for (; index < specialChronicleCount; ++index) {
    if (specialChronicle[index].identity == identity) break;
  }
  if (index == specialChronicleCount) {
    if (specialChronicleCount >= SPECIAL_CHRONICLE_LIMIT) {
      specialChronicleLimitReached = true;
      return;
    }
  }
  const bool newEntry = index == specialChronicleCount;
  const PsramTextTransaction transaction;
  SpecialChronicleEntry entry = specialChronicle[index];
  if (newEntry) { entry.identity = identity; entry.firstSeen = time(nullptr); }
  entry.lastSeen = time(nullptr);
  if (aircraft.hex.length()) entry.hex = aircraft.hex;
  if (aircraft.flight.length()) entry.flight = aircraft.flight;
  if (aircraft.type.length()) entry.type = aircraft.type;
  if (aircraft.description.length()) entry.typeDescription = aircraft.description;
  entry.specialType = specialCategoryName(aircraft);
  if (aircraft.squawk.length()) entry.squawk = aircraft.squawk;
  if (aircraft.hasPosition &&
      (isnan(entry.minimumDistanceKm) ||
       aircraft.distanceKm < entry.minimumDistanceKm))
    entry.minimumDistanceKm = aircraft.distanceKm;
  if (aircraft.hasCpa &&
      (isnan(entry.minimumCpaKm) || aircraft.cpaKm < entry.minimumCpaKm))
    entry.minimumCpaKm = aircraft.cpaKm;
  if (!transaction.succeeded()) return;
  specialChronicle[index] = std::move(entry);
  if (newEntry) ++specialChronicleCount;
}


String aircraftSelectionReason(const Aircraft &selected,
                               const std::vector<Aircraft> &aircraft,
                               const ProcessingConfig &config,
                               size_t emergencyCount,
                               size_t emergencyIndex) {
  if (selected.emergency) {
    String reason = uiText("Transpondernotfall ", "Transponder emergency ") +
                    selected.squawk +
                    uiText(" – höchste Priorität", " – highest priority");
    if (emergencyCount > 1)
      reason += " (" + String(emergencyIndex + 1) + "/" +
                String(emergencyCount) + ")";
    return reason;
  }

  size_t eligibleAirborne = 0;
  size_t eligibleTaxiing = 0;
  for (const Aircraft &candidate : aircraft) {
    if (!candidate.hasPosition || candidate.emergency) continue;
    if (config.hideSportAircraft && candidate.sportAircraft &&
        candidate.specialTypes.empty()) continue;
    const float radius = candidate.specialTypes.empty()
                             ? config.radiusKm : config.radiusKm * 2.0f;
    if (candidate.distanceKm > radius) continue;
    if (candidate.onGround) {
      if (aircraftIsTaxiing(candidate)) ++eligibleTaxiing;
    } else {
      ++eligibleAirborne;
    }
  }
  if (selected.onGround) {
    String reason = uiText(
        "Rollendes Flugzeug – kein geeignetes fliegendes Flugzeug im Radius",
        "Taxiing aircraft – no eligible airborne aircraft within radius");
    if (eligibleTaxiing > 1)
      reason += uiText(" · nächstes von ", " · nearest of ") +
                String(eligibleTaxiing);
    return reason;
  }
  const bool special = !selected.specialTypes.empty();
  if (special && selected.distanceKm > config.radiusKm) {
    return uiText("Nächstes Special im doppelten Radius: ",
                  "Nearest special within double radius: ") +
           specialCategoryName(selected);
  }
  String reason = uiText("Geringste Entfernung", "Shortest distance");
  if (eligibleAirborne > 1)
    reason += uiText(" unter ", " among ") + String(eligibleAirborne) +
              uiText(" geeigneten Flugzeugen", " eligible aircraft");
  if (special)
    reason += " · " + specialCategoryName(selected);
  return reason;
}


void markSpecialChronicleDisplayed(const Aircraft &selected,
                                   const FlightData &shown) {
  const uint64_t identity = aircraftIdentity(selected);
  for (size_t index = 0; index < specialChronicleCount; ++index) {
    if (specialChronicle[index].identity != identity) continue;
    const PsramTextTransaction transaction;
    SpecialChronicleEntry entry = specialChronicle[index];
    entry.displayed = true;
    entry.selectionReason = shown.selectionReason;
    if (shown.routeCodes.length()) entry.routeCodes = shown.routeCodes;
    if (!isnan(shown.cpaKm) &&
        (isnan(entry.minimumCpaKm) || shown.cpaKm < entry.minimumCpaKm))
      entry.minimumCpaKm = shown.cpaKm;
    if (transaction.succeeded()) specialChronicle[index] = std::move(entry);
    return;
  }
}


uint64_t specialAircraftIdentity(const Aircraft &aircraft) {
  return aircraftIdentity(aircraft);
}


void updateSpecialAircraftStatistics(const std::vector<Aircraft> &aircraft) {
  for (const Aircraft &candidate : aircraft) {
    SpecialStatisticCategory category;
    if (!specialStatisticCategory(candidate, category)) continue;
    updateSpecialChronicleObservation(candidate);
    const uint64_t identity = specialAircraftIdentity(candidate);
    if (!identity) continue;

    size_t index = 0;
    for (; index < seenSpecialAircraftCount; ++index) {
      if (seenSpecialAircraft[index].identity == identity) break;
    }
    if (index < seenSpecialAircraftCount) {
      const SpecialStatisticCategory previous =
          seenSpecialAircraft[index].category;
      const bool emergencyChanged = isEmergencyStatisticCategory(category) &&
                                    category != previous;
      const bool moreSpecific = !isEmergencyStatisticCategory(previous) &&
          specialStatisticPriority(category) < specialStatisticPriority(previous);
      if (emergencyChanged || moreSpecific) {
        --specialAircraftCounts[static_cast<size_t>(previous)];
        ++specialAircraftCounts[static_cast<size_t>(category)];
        seenSpecialAircraft[index].category = category;
      }
      continue;
    }

    if (seenSpecialAircraftCount >= SPECIAL_AIRCRAFT_TRACKING_LIMIT) {
      specialAircraftTrackingLimitReached = true;
      continue;
    }
    seenSpecialAircraft[seenSpecialAircraftCount].identity = identity;
    seenSpecialAircraft[seenSpecialAircraftCount].category = category;
    ++seenSpecialAircraftCount;
    ++specialAircraftCounts[static_cast<size_t>(category)];
  }
}


uint32_t specialAircraftCount(SpecialStatisticCategory category) {
  return specialAircraftCounts[static_cast<size_t>(category)];
}


uint32_t statisticsDateKey(const struct tm &localClock) {
  return static_cast<uint32_t>(localClock.tm_year + 1900) * 10000UL +
         static_cast<uint32_t>(localClock.tm_mon + 1) * 100UL +
         static_cast<uint32_t>(localClock.tm_mday);
}


void resetDailyJournal() {
  for (size_t index = 0; index < dailyJournalCount; ++index)
    dailyJournal[index] = {};
  dailyJournalCount = 0;
  dailyJournalLimitReached = false;
}


void resetSpecialChronicle() {
  for (size_t index = 0; index < specialChronicleCount; ++index)
    specialChronicle[index] = {};
  specialChronicleCount = 0;
  specialChronicleLimitReached = false;
}


void updateDailyJournal(const Aircraft &selected, const FlightData &shown) {
  const uint64_t identity = aircraftIdentity(selected);
  if (!identity || selected.flight.startsWith("TEST")) return;

  size_t index = 0;
  for (; index < dailyJournalCount; ++index) {
    if (dailyJournal[index].identity == identity) break;
  }
  if (index == dailyJournalCount) {
    if (dailyJournalCount >= DAILY_JOURNAL_LIMIT) {
      dailyJournalLimitReached = true;
      return;
    }
  }

  const bool newEntry = index == dailyJournalCount;
  const PsramTextTransaction transaction;
  DailyJournalEntry entry = dailyJournal[index];
  if (newEntry) { entry.identity = identity; entry.firstSeen = time(nullptr); }
  entry.lastSeen = time(nullptr);
  if (shown.hex.length()) entry.hex = shown.hex;
  if (shown.flight.length()) entry.flight = shown.flight;
  if (shown.airline.length()) entry.airline = shown.airline;
  if (shown.type.length()) entry.type = shown.type;
  if (shown.typeDescription.length()) entry.typeDescription = shown.typeDescription;
  if (shown.specialType.length()) entry.specialType = shown.specialType;
  if (shown.squawk.length()) entry.squawk = shown.squawk;
  if (shown.routeCodes.length()) entry.routeCodes = shown.routeCodes;
  if (shown.routeNames.length()) entry.routeNames = shown.routeNames;
  if (shown.selectionReason.length())
    entry.selectionReason = shown.selectionReason;
  if (!isnan(shown.distanceKm) &&
      (isnan(entry.minimumDistanceKm) || shown.distanceKm < entry.minimumDistanceKm))
    entry.minimumDistanceKm = shown.distanceKm;
  if (!isnan(shown.cpaKm) &&
      (isnan(entry.minimumCpaKm) || shown.cpaKm < entry.minimumCpaKm))
    entry.minimumCpaKm = shown.cpaKm;
  if (!isnan(shown.altitudeFt)) entry.altitudeFt = shown.altitudeFt;
  if (!transaction.succeeded()) return;
  dailyJournal[index] = std::move(entry);
  if (newEntry) ++dailyJournalCount;
}


String journalClassification(const DailyJournalEntry &entry) {
  if (isEmergencySquawk(entry.squawk)) return emergencyMeaning(entry.squawk);
  if (entry.specialType.length()) return specialTypeDisplayName(entry.specialType);
  return uiText("Normal", "Normal");
}


String journalTimeText(time_t timestamp, bool includeDate = false) {
  if (timestamp <= 0) return "-";
  struct tm localClock;
  localtime_r(&timestamp, &localClock);
  char text[24];
  strftime(text, sizeof(text), includeDate ? "%Y-%m-%d %H:%M:%S" : "%H:%M:%S",
           &localClock);
  return String(text);
}


void resetDailyAircraftStatistics(const struct tm &localClock) {
  seenAircraftCount = 0;
  aircraftTrackingLimitReached = false;
  seenSpecialAircraftCount = 0;
  specialAircraftTrackingLimitReached = false;
  for (size_t index = 0;
       index < static_cast<size_t>(SpecialStatisticCategory::Count); ++index)
    specialAircraftCounts[index] = 0;
  resetDailyJournal();
  resetSpecialChronicle();
  dailyStatisticsDate = statisticsDateKey(localClock);
  dailyStatisticsStarted = time(nullptr);
  Serial.println("Daily aircraft statistics reset");
}


void serviceDailyAircraftStatistics() {
  static uint32_t lastCheck = 0;
  const uint32_t nowMs = millis();
  if (lastCheck && nowMs - lastCheck < 1000) return;
  lastCheck = nowMs;

  struct tm localClock;
  if (!readLocalClock(localClock)) return;
  const uint32_t date = statisticsDateKey(localClock);
  if (!dailyStatisticsDate) {
    dailyStatisticsDate = date;
    dailyStatisticsStarted = time(nullptr) - nowMs / 1000;
  } else if (date != dailyStatisticsDate) {
    resetDailyAircraftStatistics(localClock);
  }
}


void updateObservedAircraftStatistics(
    const std::vector<uint64_t> &identities) {
  for (const uint64_t identity : identities) {
    if (!identity) continue;
    size_t index = 0;
    for (; index < seenAircraftCount; ++index) {
      if (seenAircraft[index] == identity) break;
    }
    if (index < seenAircraftCount) continue;
    if (seenAircraftCount >= OBSERVED_AIRCRAFT_TRACKING_LIMIT) {
      aircraftTrackingLimitReached = true;
      continue;
    }
    seenAircraft[seenAircraftCount++] = identity;
  }
}


String dailyStatisticsStartedText() {
  if (!dailyStatisticsStarted)
    return uiText("Neustart / warte auf NTP", "restart / waiting for NTP");
  struct tm localClock;
  localtime_r(&dailyStatisticsStarted, &localClock);
  char text[24];
  strftime(text, sizeof(text), "%d.%m.%Y %H:%M", &localClock);
  return String(text);
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
  config.candidateRadiusKm = radarDataCollectionEnabled()
                                 ? radarCoverageRadiusKm()
                                 : displayRadiusKm * 2.0F;
  config.hideSportAircraft = hideSportAircraft;
  config.maximumAltitudeFt = altitudeFilterEnabled ? maximumAltitudeFt : 0;
  routeClient.configure(routesEnabled, routeUrl, adsbDbFallbackEnabled);

  AdsbResult result;
  uint8_t attemptedSource = currentAdsbSource;
  const uint32_t fetchStarted = millis();
  if (automaticAdsbFailover && currentAdsbSource != activeAdsbSource &&
      fetchStarted - lastPreferredAdsbProbe >= ADSB_PREFERRED_RETRY_MS) {
    attemptedSource = activeAdsbSource;
    lastPreferredAdsbProbe = fetchStarted;
  }
  auto fetchSource = [&](uint8_t source) {
    adsbClient.setUrl(adsbUrlForSource(source));
    NetworkPhaseScope phase(NetworkPhase::Adsb);
    return adsbClient.fetch(config);
  };
  result = fetchSource(attemptedSource);
  uint8_t successfulSource = attemptedSource;
  if (!result.ok && automaticAdsbFailover) {
    const uint8_t alternatives[] = {activeAdsbSource, 0, 1, 2};
    bool attempted[3] = {};
    attempted[attemptedSource] = true;
    String combinedError = adsbSourceName(attemptedSource) + ": " + result.error;
    for (uint8_t alternative : alternatives) {
      if (alternative > 2 || attempted[alternative] ||
          !adsbSourceConfigured(alternative)) continue;
      attempted[alternative] = true;
      AdsbResult fallback = fetchSource(alternative);
      if (fallback.ok) {
        result = std::move(fallback);
        successfulSource = alternative;
        break;
      }
      combinedError += " / " + adsbSourceName(alternative) + ": " +
                       fallback.error;
      result.error = combinedError;
    }
  }
  if (result.ok) {
    if (successfulSource != activeAdsbSource) {
      if (!adsbFailoverSince) adsbFailoverSince = millis();
    } else {
      adsbFailoverSince = 0;
    }
    currentAdsbSource = successfulSource;
  }
  lastReceivedAircraft = result.receivedAircraft;
  lastCandidateAircraft = result.candidates.size();
  lastFeedTimestamp = result.feedTimestamp;
  if (result.ok) {
    updateObservedAircraftStatistics(result.observedAircraftIdentities);
    updateSpecialAircraftStatistics(result.candidates);
  }
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
  const uint32_t now = millis();
  const bool advanceEmergency = selectedEmergencyHex.length() &&
      networkFlight.emergencyCount > 1 && emergencySelectionStarted &&
      now - emergencySelectionStarted >= EMERGENCY_ROTATION_MS;
  const String previousEmergencyHex = selectedEmergencyHex;
  if (selectAircraft(result.candidates, config, selected,
                     selectedEmergencyHex, advanceEmergency,
                     emergencyCount, emergencyIndex)) {
    if (selected.emergency) {
      if (emergencyCount <= 1 || !emergencySelectionStarted ||
          selectedEmergencyHex != previousEmergencyHex)
        emergencySelectionStarted = now;
    } else {
      emergencySelectionStarted = 0;
    }
    const bool newRouteSession =
        routeObservationRequiresRefresh(selected, millis());
    if (newRouteSession) routeClient.removeCacheEntry(selected.flight);
    const bool retryRoute = currentRoute.status == "unavailable" &&
                            millis() - lastRouteAttempt >= 60000;
    if (selected.flight != routedFlight || retryRoute || newRouteSession) {
      routedFlight = selected.flight;
      lastRouteAttempt = millis();
      if (selected.flight.startsWith("TEST")) currentRoute = {};
      else {
        NetworkPhaseScope phase(NetworkPhase::Route);
        RouteContext routeContext;
        routeContext.hasPosition = true;
        routeContext.latitude = selected.hasPosition
                                    ? selected.latitude : siteLatitude;
        routeContext.longitude = selected.hasPosition
                                     ? selected.longitude : siteLongitude;
        routeContext.hasAltitude = selected.hasAltitude;
        routeContext.altitudeFt = selected.altitudeFt;
        routeContext.hasTrack = selected.hasTrack;
        routeContext.trackDegrees = selected.track;
        routeContext.hasVerticalRate = selected.hasVerticalRate;
        routeContext.verticalRateFpm = selected.verticalRateFpm;
        currentRoute = routeClient.lookup(selected.flight, routeContext,
                                          result.feedTimestamp);
      }
      lastRouteError = routeClient.lastError();
    }

    next.available = true;
    next.emergency = selected.emergency;
    next.hasPosition = selected.hasPosition;
    next.onGround = selected.onGround;
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
    } else if (isGermanAirforceCallsign(selected.flight)) {
      next.airline = "German Airforce";
    } else if (selected.flight.startsWith("IAM")) {
      next.airline = "Italian Air Force";
    } else if (isMilitaryCallsign(selected.flight)) {
      next.airline = "Military";
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
    next.typeDescription = selected.description;
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
    next.cpaReferenceMs = !isnan(next.cpaMinutes) && next.motion != "departing"
                              ? millis() : 0;
    next.verticalRateFpm = selected.hasVerticalRate
                               ? selected.verticalRateFpm : NAN;
    next.bearing = selected.hasPosition ? selected.bearing : NAN;
    next.relativeBearing = selected.hasPosition
                               ? relativeBearing(selected.bearing, viewHeading)
                               : NAN;
    next.selectionReason = aircraftSelectionReason(
        selected, result.candidates, config, emergencyCount, emergencyIndex);
    if (selected.emergency || !selected.specialTypes.empty())
      markSpecialChronicleDisplayed(selected, next);
    updateDailyJournal(selected, next);
  } else {
    emergencySelectionStarted = 0;
    routedFlight = "";
    currentRoute = {};
  }

  static double trailLatitude = NAN, trailLongitude = NAN;
  static float trailRadius = NAN, trailHeading = NAN;
  static bool trailSport = false, trailAltitude = false;
  static int trailMaxAltitude = 0;
  static uint8_t trailSource = 255;
  if (!radarTrailsEnabled || trailLatitude != siteLatitude ||
      trailLongitude != siteLongitude || trailRadius != displayRadiusKm ||
      trailHeading != viewHeading || trailSport != hideSportAircraft ||
      trailAltitude != altitudeFilterEnabled || trailMaxAltitude != maximumAltitudeFt ||
      trailSource != successfulSource) {
    if (radarTrailStorage) radarTrailStorage->tracker.clear();
    trailLatitude = siteLatitude; trailLongitude = siteLongitude;
    trailRadius = displayRadiusKm; trailHeading = viewHeading;
    trailSport = hideSportAircraft; trailAltitude = altitudeFilterEnabled;
    trailMaxAltitude = maximumAltitudeFt; trailSource = successfulSource;
  }
  const uint32_t trailNow = millis();
  if (xSemaphoreTake(flightMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    networkFlight = next;
    networkRadarPointCount = 0;
    const uint64_t selectedIdentity = next.available
        ? aircraftIdentity(selected) : 0;
    // Preserve the selected target and all selection-relevant traffic before
    // filling the remaining fixed capacity with the wider radar surroundings.
    for (uint8_t pass = 0; pass < 3; ++pass) {
      for (const Aircraft &candidate : result.candidates) {
        if (!candidate.hasPosition ||
            networkRadarPointCount >= RADAR_POINT_LIMIT) continue;
        const bool isSelected = selectedIdentity &&
            aircraftIdentity(candidate) == selectedIdentity;
        const bool special = !candidate.specialTypes.empty();
        const bool selectionRelevant = candidate.emergency ||
            candidate.distanceKm <= displayRadiusKm ||
            (special && candidate.distanceKm <= displayRadiusKm * 2.0F);
        if ((pass == 0 && !isSelected) ||
            (pass == 1 && (isSelected || !selectionRelevant)) ||
            (pass == 2 && (isSelected || selectionRelevant))) continue;
        if (hideSportAircraft && candidate.sportAircraft && !special) continue;
        int16_t radarX = 0;
        int16_t radarY = 0;
        if (!radarPointCoordinates(
                candidate.distanceKm, candidate.bearing, radarX, radarY))
          continue;
        RadarPoint &point = networkRadarPoints[networkRadarPointCount++];
        RadarTrailSnapshot *trail = radarTrailStorage
            ? &radarTrailStorage->network[networkRadarPointCount - 1] : nullptr;
        if (trail) trail->trailCount = 0;
        if (radarTrailsEnabled && trail &&
            (candidate.hex.length() || candidate.registration.length() || candidate.flight.length())) {
          const float angle = candidate.bearing * PI / 180.0F;
          const auto *track = radarTrailStorage->tracker.update(aircraftIdentity(candidate),
              sinf(angle) * candidate.distanceKm,
              cosf(angle) * candidate.distanceKm, trailNow);
          if (track) {
            for (size_t j = 0; j + 1 < track->count; ++j) {
              const auto &fix = track->fixes[j];
              int16_t x = 0, y = 0;
              if (!radarPointCoordinates(hypotf(fix.east, fix.north),
                      atan2f(fix.east, fix.north) * 180.0F / PI, x, y) ||
                  (x == radarX && y == radarY)) continue;
              bool duplicate = false;
              for (uint8_t k = 0; k < trail->trailCount; ++k) {
                auto &dot = trail->trail[k];
                if (dot.x == x && dot.y == y) {
                  dot.time = fix.time; duplicate = true; break;
                }
              }
              if (!duplicate) {
                auto &dot = trail->trail[trail->trailCount++];
                dot.x = x; dot.y = y; dot.time = fix.time;
              }
            }
          }
        }
        point.distanceKm = candidate.distanceKm;
        point.bearing = candidate.bearing;
        point.flags = isSelected ? RADAR_POINT_SELECTED : 0;
        if (special) point.flags |= RADAR_POINT_SPECIAL;
        if (candidate.emergency) point.flags |= RADAR_POINT_EMERGENCY;
        if (candidate.onGround) point.flags |= RADAR_POINT_GROUND;
        if (candidate.distanceKm > displayRadiusKm)
          point.flags |= RADAR_POINT_OUTSIDE_RADIUS;
      }
    }
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
  if (page == 6) return uiText("Kompakt + Flughäfen", "Compact + airports");
  return uiText("Radaransicht", "Radar view");
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


String configurationJson() {
  JsonDocument document;
  document["format"] = "flightwall-config-v1";
  document["firmware"] = APP_VERSION;
  document["device_variant"] = DEVICE_VARIANT;
  JsonObject settings = document["settings"].to<JsonObject>();
  settings["wifi_ssid"] = wifiSsid;
  settings["wifi_password"] = wifiPassword;
  settings["ota_password"] = otaPassword;
  settings["ui_english"] = uiEnglish;
  settings["status_led"] = statusLedBrightnessMode;
  settings["display_enabled"] = displayEnabled;
  settings["environment_idle"] = environmentIdleDisplayEnabled;
  settings["mqtt_enabled"] = mqttWeatherEnabled;
  settings["mqtt_host"] = mqttHost;
  settings["mqtt_port"] = mqttPort;
  settings["mqtt_user"] = mqttUser;
  settings["mqtt_password"] = mqttPassword;
  settings["wind_direction_topic"] = windDirectionTopic;
  settings["wind_speed_topic"] = windSpeedTopic;
  settings["wind_gust_topic"] = windGustTopic;
  settings["rain_topic"] = rainTopic;
  settings["quiet_hours"] = quietHoursEnabled;
  settings["quiet_start"] = quietHoursStartMinutes;
  settings["quiet_end"] = quietHoursEndMinutes;
  settings["automatic_brightness"] = automaticBrightnessEnabled;
  settings["manual_brightness"] = manualBrightnessIndex;
  settings["low_lux"] = lowLuxThreshold;
  settings["high_lux"] = highLuxThreshold;
  settings["display_radius_km"] = displayRadiusKm;
  settings["observer_elevation_m"] = observerElevationM;
  settings["far_elevation_deg"] = farDirectionElevationDeg;
  settings["adsb_url_1"] = adsbUrl1;
  settings["adsb_url_2"] = adsbUrl2;
  settings["preferred_adsb_source"] = activeAdsbSource;
  settings["automatic_adsb_failover"] = automaticAdsbFailover;
  settings["route_url"] = routeUrl;
  settings["site_latitude"] = siteLatitude;
  settings["site_longitude"] = siteLongitude;
  settings["view_heading"] = viewHeading;
  settings["radar_trails_enabled"] = radarTrailsEnabled;
  settings["routes_enabled"] = routesEnabled;
  settings["adsbdb_fallback"] = adsbDbFallbackEnabled;
  settings["hide_sport_aircraft"] = hideSportAircraft;
  settings["altitude_filter"] = altitudeFilterEnabled;
  settings["maximum_altitude_ft"] = maximumAltitudeFt;
#if FLIGHTWALL_HAS_MATRIX
  settings["custom_text"] = customPageText;
  settings["custom_scroll_speed"] = customScrollSpeed;
  settings["radar_sweep_interval_seconds"] = radarSweepIntervalSeconds;
  settings["smooth_transitions"] = smoothTransitions;
  JsonArray pages = settings["pages"].to<JsonArray>();
  for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
    JsonObject item = pages.add<JsonObject>();
    item["enabled"] = pageEnabled[page];
    item["duration_ms"] = pageDurationMs[page];
    item["order"] = pageOrder[page];
  }
#endif
  String json;
  serializeJsonPretty(document, json);
  return json;
}


void handleConfigurationDownload() {
  if (otaPassword.length() >= 8 &&
      !webServer.authenticate("admin", otaPassword.c_str())) {
    webServer.requestAuthentication();
    return;
  }
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.sendHeader(
      "Content-Disposition",
      String("attachment; filename=\"flightwall-config-") +
          DEVICE_VARIANT + ".json\"");
  webServer.send(200, "application/json; charset=utf-8", configurationJson());
}


void handleConfigurationRestore() {
  JsonDocument document;
  const DeserializationError error =
      deserializeJson(document, webServer.arg("plain"));
  if (error || String(document["format"] | "") != "flightwall-config-v1" ||
      !document["settings"].is<JsonObjectConst>()) {
    webServer.send(400, "text/plain; charset=utf-8",
                   uiText("Ungültige FlightWall-Sicherungsdatei.",
                          "Invalid FlightWall backup file."));
    return;
  }
  JsonObjectConst s = document["settings"].as<JsonObjectConst>();
  settingsWritePreparing = true;
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
    settingsWritePreparing = false;
    webServer.send(503, "text/plain; charset=utf-8", "Preferences busy");
    return;
  }
  settingsWritePreparing = false;
  settingsWriteInProgress = true;
  auto putString = [&](const char* key, const char* field) {
    if (!s[field].isNull()) preferences.putString(key, s[field].as<String>());
  };
  auto putBool = [&](const char* key, const char* field) {
    if (!s[field].isNull()) preferences.putBool(key, s[field].as<bool>());
  };
  putString("wifiSsid", "wifi_ssid"); putString("wifiPass", "wifi_password");
  putString("otaPass", "ota_password"); putBool("uiEnglish", "ui_english");
  if (!s["status_led"].isNull()) preferences.putUChar("statusLed", constrain(s["status_led"].as<int>(), 0, 2));
  putBool("displayOn", "display_enabled"); putBool("envIdle", "environment_idle");
  putBool("mqttOn", "mqtt_enabled"); putString("mqttHost", "mqtt_host");
  if (!s["mqtt_port"].isNull()) preferences.putUShort("mqttPort", constrain(s["mqtt_port"].as<int>(), 1, 65535));
  putString("mqttUser", "mqtt_user"); putString("mqttPass", "mqtt_password");
  putString("windDirTop", "wind_direction_topic"); putString("windSpdTop", "wind_speed_topic");
  putString("windGstTop", "wind_gust_topic"); putString("rainTop", "rain_topic");
  putBool("quietOn", "quiet_hours");
  if (!s["quiet_start"].isNull()) preferences.putUShort("quietStart", constrain(s["quiet_start"].as<int>(), 0, 1439));
  if (!s["quiet_end"].isNull()) preferences.putUShort("quietEnd", constrain(s["quiet_end"].as<int>(), 0, 1439));
  putBool("autoLight", "automatic_brightness");
  if (!s["manual_brightness"].isNull()) preferences.putUChar("brightness", constrain(s["manual_brightness"].as<int>(), 0, BRIGHTNESS_LEVEL_COUNT - 1));
  if (!s["low_lux"].isNull()) preferences.putFloat("lowLux", constrain(s["low_lux"].as<float>(), 1.0f, 500.0f));
  if (!s["high_lux"].isNull()) preferences.putFloat("highLux", constrain(s["high_lux"].as<float>(), 50.0f, 50000.0f));
  if (!s["display_radius_km"].isNull()) preferences.putFloat("radiusKm", constrain(s["display_radius_km"].as<float>(), 1.0f, 100.0f));
  if (!s["observer_elevation_m"].isNull()) preferences.putFloat("observerElevM", constrain(s["observer_elevation_m"].as<float>(), -500.0f, 9000.0f));
  if (!s["far_elevation_deg"].isNull()) preferences.putFloat("farAngle", constrain(s["far_elevation_deg"].as<float>(), 10.0f, 45.0f));
  putString("adsbUrl", "adsb_url_1"); putString("adsbUrl2", "adsb_url_2");
  if (!s["preferred_adsb_source"].isNull()) preferences.putUChar("adsbActive", constrain(s["preferred_adsb_source"].as<int>(), 0, 2));
  putBool("adsbFailover", "automatic_adsb_failover"); putString("routeUrl", "route_url");
  if (!s["site_latitude"].isNull()) preferences.putDouble("siteLat", constrain(s["site_latitude"].as<double>(), -90.0, 90.0));
  if (!s["site_longitude"].isNull()) preferences.putDouble("siteLon", constrain(s["site_longitude"].as<double>(), -180.0, 180.0));
  if (!s["view_heading"].isNull()) preferences.putFloat("heading", constrain(s["view_heading"].as<float>(), 0.0f, 359.0f));
  putBool("radarTrails", "radar_trails_enabled");
  putBool("routesOn", "routes_enabled"); putBool("routeFallback", "adsbdb_fallback");
  putBool("hideSport", "hide_sport_aircraft"); putBool("altFilter", "altitude_filter");
  if (!s["maximum_altitude_ft"].isNull()) preferences.putInt("maxAltFt", constrain(s["maximum_altitude_ft"].as<int>(), 1000, 100000));
#if FLIGHTWALL_HAS_MATRIX
  putString("customText", "custom_text");
  if (!s["custom_scroll_speed"].isNull()) preferences.putUChar("customSpd", constrain(s["custom_scroll_speed"].as<int>(), 0, 2));
  if (!s["radar_sweep_interval_seconds"].isNull()) preferences.putUChar("radarSweepSec", constrain(s["radar_sweep_interval_seconds"].as<int>(), 0, 10));
  putBool("smooth", "smooth_transitions");
  if (s["pages"].is<JsonArrayConst>()) {
    uint8_t page = 0;
    for (JsonObjectConst item : s["pages"].as<JsonArrayConst>()) {
      if (page >= PAGE_COUNT) break;
      preferences.putBool(("p" + String(page) + "On").c_str(), item["enabled"] | true);
      preferences.putUInt(("p" + String(page) + "Ms").c_str(), constrain(item["duration_ms"] | 5000U, 2000U, 60000U));
      preferences.putUChar(("order" + String(page)).c_str(), constrain(item["order"] | page, 0, PAGE_COUNT - 1));
      ++page;
    }
  }
#endif
  xSemaphoreGive(preferencesMutex);
  settingsWriteInProgress = false;
  webServer.send(200, "text/plain; charset=utf-8",
                 uiText("Konfiguration wiederhergestellt. Neustart…",
                        "Configuration restored. Restarting…"));
  manualRebootAt = millis() + 1500;
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


const char *moldRiskText() {
  switch (moldRiskLevel) {
    case MoldRiskLevel::Normal: return uiText("unauffällig", "low");
    case MoldRiskLevel::Elevated: return uiText("erhöht", "elevated");
    case MoldRiskLevel::High: return uiText("hoch", "high");
    default: return uiText("nicht verfügbar", "unavailable");
  }
}


const char *moldRiskCssClass() {
  switch (moldRiskLevel) {
    case MoldRiskLevel::Normal: return "ok";
    case MoldRiskLevel::Elevated: return "warn";
    case MoldRiskLevel::High: return "bad";
    default: return "off";
  }
}


String elevatedHumidityDurationText() {
  if (!elevatedHumiditySince) return uiText("keine", "none");
  const uint32_t elapsedMinutes =
      (millis() - elevatedHumiditySince) / 60000UL;
  return String(elapsedMinutes) + " min";
}


String currentNetworkIpText() {
  if (WiFi.status() == WL_CONNECTED) return WiFi.localIP().toString();
  if (wifiSetupAccessPointActive) return WiFi.softAPIP().toString();
  return "-";
}


String wifiSignalQualityText() {
  if (WiFi.status() != WL_CONNECTED)
    return uiText("getrennt", "disconnected");
  const int32_t rssi = WiFi.RSSI();
  const char *quality = rssi >= -55 ? uiText("sehr gut", "excellent")
                        : rssi >= -67 ? uiText("gut", "good")
                        : rssi >= -75 ? uiText("ausreichend", "fair")
                                      : uiText("schwach", "weak");
  return String(rssi) + " dBm · " + quality;
}


void translateWebPageToEnglish(String &html) {
  if (!uiEnglish) return;
  struct Translation { const char *de; const char *en; };
  static const Translation translations[] = {
    {"lang='de'", "lang='en'"},
    {"lang=\"de\"", "lang=\"en\""},
    {"ADS-B-Adresse Quelle", "ADS-B address source"},
    {"Nur die ausgewählte Quelle wird abgefragt. Quelle 2 darf unkonfiguriert bleiben. Der Radius von adsb.fi wird automatisch aus dem Anzeigeradius berechnet.", "Only the selected source is queried. Source 2 may remain unconfigured. The adsb.fi radius is calculated automatically from the display radius."},
    {"Regen wird für die Anzeige von cm in mm umgerechnet. Ein leeres Passwortfeld behält das gespeicherte Passwort bei.", "Rain is converted from cm to mm for display. An empty password field keeps the saved password."},
    {"WLAN-Zugang", "Wi-Fi access"},
    {"Aktuelle Netzwerkadresse", "Current network address"},
    {"Verbindungsstatus", "Connection status"},
    {"Verbundenes WLAN", "Connected Wi-Fi"},
    {"Gespeichertes WLAN", "Stored Wi-Fi"},
    {"IP-Adresse", "IP address"},
    {"Gateway / Router", "Gateway / router"},
    {"Subnetzmaske", "Subnet mask"},
    {"DNS-Server", "DNS server"},
    {"WLAN-Kanal", "Wi-Fi channel"},
    {"MAC-Adresse", "MAC address"},
    {"Hostname", "Hostname"},
    {"WLAN-Name (SSID)", "Wi-Fi name (SSID)"},
    {"WLANs suchen", "Scan for Wi-Fi networks"},
    {"Erkannte WLANs", "Detected Wi-Fi networks"},
    {"WLAN auswählen …", "Select Wi-Fi network…"},
    {"Suche läuft …", "Scanning…"},
    {"Keine WLANs gefunden.", "No Wi-Fi networks found."},
    {"WLAN-Suche fehlgeschlagen.", "Wi-Fi scan failed."},
    {" WLANs gefunden", " Wi-Fi networks found"},
    {" · aktuell", " · current"},
    {"Neues WLAN-Passwort", "New Wi-Fi password"},
    {"Offenes WLAN ohne Passwort", "Open Wi-Fi network without a password"},
    {"Ein leeres Passwortfeld behält das gespeicherte WLAN-Passwort bei. Änderungen werden nach einem automatischen Neustart aktiv.", "An empty password field keeps the stored Wi-Fi password. Changes take effect after an automatic restart."},
    {"Falls keine Verbindung möglich ist, startet das Einrichtungsnetz", "If no connection is possible, the setup network starts"},
    {"Einrichtungsnetz", "Setup network"},
    {"WLAN-Name", "Wi-Fi name"},
    {"Firmware-Updates", "Firmware updates"},
    {"Neues OTA-Passwort", "New OTA password"},
    {"OTA deaktivieren", "Disable OTA"},
    {"Mindestens acht Zeichen. Ein leeres Feld behält das gespeicherte OTA-Passwort bei. Änderungen werden nach einem automatischen Neustart aktiv.", "At least eight characters. An empty field keeps the stored OTA password. Changes take effect after an automatic restart."},
    {"OTA ist konfiguriert.", "OTA is configured."},
    {"OTA ist noch nicht konfiguriert.", "OTA is not configured yet."},
    {"Sprache / Language", "Language / Sprache"},
    {"Deutsch", "German"},
    {"Display eingeschaltet", "Display enabled"},
    {"Wetterdaten anzeigen, wenn kein Flugzeug in der Nähe ist", "Show weather data when no aircraft is nearby"},
    {"Anzeigeradius", "Display radius"},
    {"Standorthöhe über NHN", "Site elevation above sea level"},
    {"Unter diesem Blickwinkel \"weit\"", "Use \"far\" below this elevation angle"},
    {"Nachtruhe aktiv", "Night mode enabled"},
    {"Display dunkel ab", "Display off from"},
    {"Display wieder aktiv ab", "Display on again at"},
    {"Die Nachtruhe darf über Mitternacht reichen. Gleiche Zeiten deaktivieren die Abdunklung.", "Night mode may span midnight. Equal times disable it."},
    {"Status-LED", "Status LED"}, {">aus<", ">off<"},
    {">schwach<", ">dim<"}, {">normal<", ">normal<"},
    {"Nachtabsenkung aktiv", "Night dimming enabled"},
    {"Absenkung ab", "Dim from"}, {"Normalbetrieb ab", "Normal operation from"},
    {"Grün: normal, Blau: WLAN-Verbindung, Türkis: Setup-WLAN, Orange: ADS-B-Warnung, Violett: Update, Rot: Fehler oder Transpondernotfall. Gelb signalisiert einen Bedienzugriff; regelmäßige Statusabfragen bleiben ruhig.", "Green: normal, blue: Wi-Fi connection, cyan: setup network, orange: ADS-B warning, violet: update, red: error or transponder emergency. Yellow indicates user activity; regular status polling remains quiet."},
    {"Displaytest beenden", "Stop display test"},
    {"Displaytest starten", "Start display test"},
    {"Etwa 23 Sekunden: Matrix-Prüfbilder und alle Typensymbole mit Bedeutung.", "About 23 seconds: matrix test patterns and all type symbols with their meaning."},
    {"Flugzeugdaten", "Aircraft data"}, {"Aktive ADS-B-Quelle", "Active ADS-B source"},
    {"Quelle 1", "Source 1"}, {"Quelle 2", "Source 2"},
    {"Externe Quelle", "External source"},
    {"adsb.fi (externe Quelle)", "adsb.fi (external source)"},
    {"Breitengrad", "Latitude"}, {"Längengrad", "Longitude"}, {"Blickrichtung (Grad)", "Viewing direction (degrees)"},
    {"Sport- und Kleinflugzeuge ausblenden", "Hide sport and light aircraft"},
    {"Flugzeuge oberhalb der Maximalhöhe ausblenden", "Hide aircraft above maximum altitude"},
    {"Maximalhöhe", "Maximum altitude"}, {"Routen über adsb.im ergänzen", "Add routes via adsb.im"},
    {"ADSBDB bei fehlender Route verwenden", "Use ADSBDB when no route is found"},
    {"Routen-Dienst", "Route service"}, {"Routendienst testen", "Test route service"},
    {"ADS-B-Verbindung testen", "Test ADS-B connection"}, {"Notfalltest-Code", "Emergency test code"},
    {"Anzahl Testflugzeuge", "Number of test aircraft"}, {"Notfallanzeige testen", "Test emergency display"},
    {"Test beenden", "Stop test"},
    {"Der Notfalltest läuft höchstens zwei Minuten und verändert keine ADS-B-Daten oder gespeicherten Einstellungen.", "The emergency test runs for no more than two minutes and does not modify ADS-B data or saved settings."},
    {"MQTT-Wetter", "MQTT weather"}, {"Wetterseite in der Ruheanzeige aktivieren", "Enable weather page while idle"},
    {"Benutzer", "User"}, {"Passwort", "Password"}, {"unverändert lassen", "leave unchanged"},
    {"Windrichtung", "Wind direction"}, {"Windgeschwindigkeit", "Wind speed"}, {"Windböen", "Wind gusts"},
    {"Regen heute", "Rain today"},
    {">Zeit<", ">Time<"}, {">Flug<", ">Flight<"},
    {">Typ<", ">Type<"}, {">Klasse<", ">Class<"},
    {"Auswahlbegründung", "Selection reason"},
    {"min. Entfernung", "min. distance"},
    {">Angezeigt<", ">Displayed<"},
    {"JSON herunterladen", "Download JSON"},
    {"CSV herunterladen", "Download CSV"},
    {"Anzeigeseiten", "Display pages"}, {"Sanfte Übergänge", "Smooth transitions"},
    {"Freitext (maximal 128 Zeichen)", "Custom text (maximum 128 characters)"},
    {"Freitext-Scrollgeschwindigkeit", "Custom text scroll speed"}, {">schnell<", ">fast<"},
    {">mittel<", ">medium<"}, {">langsam<", ">slow<"},
    {"Radarspuren (60 Sekunden)", "Radar trails (60 seconds)"},
    {"Radar-Umlaufintervall", "Radar sweep interval"}, {">Aus<", ">Off<"},
    {"Ein Umlauf dauert etwa 0,8 Sekunden. 0 schaltet ihn aus.", "One sweep takes about 0.8 seconds. 0 disables it."},
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
    {"Boden-/Flugstatus", "Ground/flight status"},
    {"Radius normal / aktuell", "Normal / current radius"},
    {"ADS-B und Route", "ADS-B and route"}, {"ADS-B-Fehler", "ADS-B error"},
    {"Routenfehler", "Route error"}, {"Routencache wartet auf Flash", "Route cache pending flash write"},
    {"Empfangen / Kandidaten", "Received / candidates"}, {"Letzter Erfolg", "Last success"},
    {"Vertikalbewegung", "Vertical motion"}, {"Routenstatus", "Route status"},
    {"Routenquelle", "Route source"}, {"ADSBDB-Fallback", "ADSBDB fallback"},
    {"Letzter ADS-B-Erfolg", "Last ADS-B success"},
    {"Gefundene Routen gelten sechs Stunden und werden zusätzlich anhand von Position, Höhe und Flugbewegung geprüft. ADSBDB-Routen bleiben nur im RAM. Negative Ergebnisse gelten eine Stunde und erkannte Einsatzrufzeichen 30 Minuten. Unplausible Routen werden verworfen und nicht gespeichert.", "Found routes are valid for six hours and are additionally checked against position, altitude and aircraft motion. ADSBDB routes remain in RAM only. Negative results are valid for one hour and recognised operational callsigns for 30 minutes. Implausible routes are rejected and not stored."},
    {"Gesamten Routencache wirklich löschen?", "Really clear the entire route cache?"},
    {"Diesen Cacheeintrag löschen?", "Delete this cache entry?"},
    {"Aktuelle Route neu laden", "Refresh current route"},
    {"Gesamten Cache löschen", "Clear entire cache"},
    {"Der Routencache ist leer.", "The route cache is empty."},
    {"Routencache", "Route cache"}, {"Callsign", "Callsign"},
    {"Quelle", "Source"}, {"Alter", "Age"}, {"Speicher", "Storage"}, {"Aktionen", "Actions"},
    {"Neu laden", "Refresh"}, {"Löschen", "Delete"},
    {"nicht gefunden", "not found"}, {"gefunden", "found"},
    {"wartet", "pending"}, {"abgelaufen", "expired"},
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
    {"Schimmelrisiko", "Mold risk"}, {"Taupunkt", "Dew point"},
    {"Kritische Oberflächentemperatur", "Critical surface temperature"},
    {"Dauer erhöhter Feuchte", "Elevated humidity duration"},
    {"Oberflächen unterhalb dieses Werts können 80 % relative Feuchte erreichen.", "Surfaces below this value may reach 80% relative humidity."},
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
    {"Firmware-Update", "Firmware update"},
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
    {">konfiguriert<", ">configured<"}, {">nicht konfiguriert<", ">not configured<"},
    {">keiner<", ">none<"}, {">keine<", ">none<"}, {">aktiv<", ">enabled<"}, {">inaktiv<", ">inactive<"},
    {">ja<", ">yes<"}, {">nein<", ">no<"}, {">bis ", ">up to "},
    {"% / Automatik", "% / automatic"}, {">Automatik<", ">automatic<"},
    {">manuell<", ">manual<"}, {">aus<", ">off<"}, {">warte auf NTP<", ">waiting for NTP<"},
    {" m NHN / weit unter ", " m ASL / far below "}, {"Netz Core 0 / Display Core 1", "Network core 0 / display core 1"},
    {"Netzphase beim Watchdog-Reset", "Network phase at watchdog reset"},
    {"Maximale Netzlaufzeiten", "Maximum network phase durations"},
    {" / aktiv", " / enabled"}, {" / inaktiv", " / inactive"},
    {"toLocaleTimeString('de-DE')", "toLocaleTimeString('en-GB')"}
  };
  for (const auto &translation : translations) html.replace(translation.de, translation.en);
}


void appendDailyJournalStatus(String &html) {
  html += F("<section class='statusCard wide'><h3>");
  html += uiText("Tagesjournal", "Daily journal");
  html += " · " + String(dailyJournalCount) + " / " +
          String(DAILY_JOURNAL_LIMIT) + "</h3>";
  if (!dailyJournalCount) {
    html += "<p class='muted'>" +
            String(uiText("Heute wurde noch kein angezeigtes Flugzeug erfasst.",
                          "No displayed aircraft has been recorded today.")) +
            "</p>";
  } else {
    html += F("<div class='tableScroll'><table class='cacheTable'><thead><tr><th>Zeit</th><th>Flug</th><th>Typ</th><th>Route</th><th>Klasse</th><th>min. CPA</th><th>Auswahlbegründung</th></tr></thead><tbody>");
    const size_t visibleJournalRows =
        min(dailyJournalCount, DAILY_JOURNAL_STATUS_ROWS);
    for (size_t offset = 0; offset < visibleJournalRows; ++offset) {
      const DailyJournalEntry &entry =
          dailyJournal[dailyJournalCount - 1 - offset];
      html += "<tr><td>" + journalTimeText(entry.lastSeen) + "</td><td><b>" +
              htmlEscape(entry.flight.length() ? entry.flight : entry.hex) +
              "</b></td><td>" + htmlEscape(entry.type) + "</td><td>" +
              htmlEscape(entry.routeCodes.length() ? String(entry.routeCodes) : String("-")) +
              "</td><td>" + htmlEscape(journalClassification(entry)) +
              "</td><td>" + numberOrDash(entry.minimumCpaKm) +
              " km</td><td>" + htmlEscape(entry.selectionReason) + "</td></tr>";
    }
    html += F("</tbody></table></div>");
    if (dailyJournalCount > visibleJournalRows) {
      html += "<p class='muted'>" +
              String(uiText("Neueste ", "Latest ")) +
              String(visibleJournalRows) + uiText(" von ", " of ") +
              String(dailyJournalCount) +
              uiText(" Einträgen angezeigt. Die Downloads enthalten alle Einträge.",
                     " entries shown. Downloads contain all entries.") +
              "</p>";
    }
  }
  if (dailyJournalLimitReached)
    html += "<p class='pill warn'>" +
            String(uiText("Journal-Limit erreicht", "Journal limit reached")) +
            "</p>";
  html += F("<div class='buttonRow'><form class='inlineForm' method='get' action='/journal.json'><button type='submit'>JSON herunterladen</button></form><form class='inlineForm' method='get' action='/journal.csv'><button type='submit'>CSV herunterladen</button></form></div></section>");
}


void appendRouteMemoryJson(String &json, const RouteMemorySnapshot &memory) {
  json += "{\"free_internal_bytes\":" + String(memory.freeInternal) +
          ",\"largest_internal_block_bytes\":" + String(memory.largestInternal) +
          ",\"minimum_internal_bytes\":" + String(memory.minimumInternal) + '}';
}

void appendPersistentTextMemoryJson(String &json) {
  const auto &memory = psramTextMemoryStats();
  json += "{\"storage\":\"psram\",\"allocated_bytes\":" + String(memory.allocatedBytes) +
          ",\"blocks\":" + String(memory.blocks) +
          ",\"allocation_failures\":" + String(memory.allocationFailures) + '}';
}

void appendRouteRequestJson(String &json, const RouteRequestDiagnostics &d) {
  if (!d.available) { json += "null"; return; }
  char hex[16];
  snprintf(hex, sizeof(hex), "-0x%04lX", static_cast<unsigned long>(0U - static_cast<uint32_t>(d.tlsCode)));
  json += "{\"source\":\"" + jsonEscape(d.source) +
          "\",\"callsign\":\"" + jsonEscape(d.callsign) +
          "\",\"attempts\":" + String(d.attempts) +
          ",\"started_ms\":" + String(d.startedMs) +
          ",\"age_seconds\":" + String((millis() - d.startedMs) / 1000U) +
          ",\"duration_ms\":" + String(d.durationMs) +
          ",\"http_attempted\":" + String(d.httpAttempted ? "true" : "false") +
          ",\"http_code\":" + String(d.httpCode) +
          ",\"tls_error_code\":" + String(d.tlsCode) +
          ",\"tls_error_hex\":\"" + String(d.tlsCode < 0 ? hex : "") +
          "\",\"tls_error_detail_available\":" + String(d.tlsCode < -1 ? "true" : "false") +
          ",\"tls_error_text\":\"" + jsonEscape(d.tlsMessage) +
          "\",\"entry\":";
  appendRouteMemoryJson(json, d.entry);
  json += ",\"before_request\":";
  appendRouteMemoryJson(json, d.before);
  json += ",\"after_request\":";
  appendRouteMemoryJson(json, d.after);
  json += '}';
}

void appendRouteRequestHtml(String &html, const char *label,
                            const RouteRequestDiagnostics &d) {
  html += "<h4>" + String(label) + "</h4>";
  if (!d.available) {
    html += "<p>" + String(uiText("Noch keine Anfrage", "No request yet")) + "</p>";
    return;
  }
  html += "<p><b>" + htmlEscape(d.source) + " · " + htmlEscape(d.callsign) +
          "</b> · " + String((millis() - d.startedMs) / 1000U) + " s · " +
          String(d.durationMs) + " ms · " + String(d.attempts) +
          " " + uiText("Anfragen", "requests") + "<br>";
  if (!d.httpAttempted)
    html += uiText("HTTP-Anfrage nicht gestartet", "HTTP request not started");
  else
    html += "HTTP " + String(d.httpCode) + " · TLS " + String(d.tlsCode);
  if (d.tlsCode < -1) {
    char hex[16];
    snprintf(hex, sizeof(hex), "-0x%04lX", static_cast<unsigned long>(0U - static_cast<uint32_t>(d.tlsCode)));
    html += " (" + String(hex) + "): " + htmlEscape(d.tlsMessage);
  } else if (d.httpCode < 0) {
    html += " · " + String(uiText("Keine TLS-Details; DNS-/Transport-/HTTP-Fehler möglich",
                                  "No TLS detail; DNS/transport/HTTP failure possible"));
  }
  html += "<br>" + String(uiText("Interner RAM: frei / größter Block (Bytes)",
                                 "Internal RAM: free / largest block (bytes)")) +
          "<br>" + uiText("Vor Vorbereitung: ", "Before preparation: ") +
          String(d.entry.freeInternal) + " / " + String(d.entry.largestInternal) +
          "<br>" + uiText("Vor HTTP: ", "Before HTTP: ") +
          String(d.before.freeInternal) + " / " + String(d.before.largestInternal) +
          "<br>" + uiText("Nach HTTP: ", "After HTTP: ") +
          String(d.after.freeInternal) + " / " + String(d.after.largestInternal) +
          "<br>" + uiText("Minimum seit Start: ", "Minimum since boot: ") +
          String(d.after.minimumInternal) + "</p>";
}

void appendRouteDiagnosticsHtml(String &html) {
  const auto &diagnostics = routeClient.diagnostics();
  html += "<details class='statusCard wide'><summary>" +
          String(uiText("Routen-Verbindungsdiagnose", "Route connection diagnostics")) + "</summary>";
  appendRouteRequestHtml(html, uiText("Hauptdienst", "Primary service"), diagnostics.primary);
  appendRouteRequestHtml(html, "ADSBDB", diagnostics.fallback);
  appendRouteRequestHtml(html, uiText("Letzter gespeicherter Fehler", "Last recorded failure"), diagnostics.lastFailure);
  html += "<p class='muted'>" + String(uiText(
      "Messungen vor und nach HTTP, nicht während des TLS-Handshakes. Cachetreffer starten keine Messung. Fehler bleiben bis zum Neustart oder nächsten Fehler erhalten.",
      "Measurements before and after HTTP, not during the TLS handshake. Cache hits do not start measurements. Failures are retained until reboot or the next failure.")) + "</p></details>";
}

String webPage(bool configurationPage = false, bool includeRouteCache = true) {
  const RouteMemorySnapshot statusMemory = routeMemorySnapshot();
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
    if (isfinite(windDirectionDegrees))
      statusWindDirection = weatherCompassDirection(windDirectionDegrees);
    statusWindSpeed = windSpeedKph;
    statusWindGust = windGustKph;
    statusRain = rain24Mm;
    xSemaphoreGive(weatherMutex);
  }

  String html;
  // Status and configuration are deliberately separate. Besides making the
  // UI clearer, this keeps a normal status request from assembling the large
  // settings form and keeps automatic refreshes away from user input.
  html.reserve(configurationPage ? 30000 : (includeRouteCache ? 42000 : 34000));
  html += F("<!doctype html><html lang='de'><head><meta charset='utf-8'>");
  html += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>");
  html += DEVICE_NAME;
  html += F("</title><style>");
  html += F("body{font:16px system-ui;background:#10131a;color:#eef2ff;margin:auto;max-width:920px;padding:20px}");
  html += F("h1{font-size:1.5rem}fieldset{border:1px solid #445;border-radius:10px;margin:14px 0;padding:14px;min-width:0}");
  html += F("label{display:block;margin:12px 0}input[type=number],input[type=text],input[type=password],input[type=url],input[type=time],select,textarea{width:100%;box-sizing:border-box;padding:9px;min-height:42px}input[type=checkbox]{width:auto;min-height:auto}.fieldGrid{display:grid;grid-template-columns:repeat(12,minmax(0,1fr));gap:10px 14px;align-items:end;margin:10px 0}.fieldGrid>label{grid-column:span 6;margin:0;min-width:0}.fieldGrid>.third{grid-column:span 4}.fieldGrid>.quarter{grid-column:span 3}.fieldGrid>.twoThird{grid-column:span 8}.fieldGrid>.threeQuarter{grid-column:span 9}.fieldGrid>.fieldWide{grid-column:1/-1}.fieldGrid .check{align-self:center;padding:9px 0}.buttonRow{display:flex;flex-wrap:wrap;gap:8px;margin:10px 0}.pageSettings{display:grid;grid-template-columns:minmax(0,1fr) minmax(120px,180px);gap:8px 14px;align-items:end;margin:10px 0}.pageSettings label{margin:0}.orderGrid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:10px 14px}.orderGrid label{margin:0}");
  html += F("button{background:#00bcd4;color:#001018;border:0;border-radius:8px;padding:12px 18px;font-weight:bold}.danger{background:#ff8597;color:#24050b}.smallButton{padding:7px 10px;font-size:.82rem}.inlineForm{display:inline;margin:0 4px 0 0}.networkSummary{background:#172533;border:1px solid #31546b;border-radius:10px;padding:11px 13px;margin:0 0 13px;overflow-wrap:anywhere}.networkSummary a{color:#7ee7f0;font-weight:700}.tableScroll{overflow-x:auto}.cacheTable{width:100%;border-collapse:collapse}.cacheTable th,.cacheTable td{text-align:left;padding:8px 6px;border-bottom:1px solid #343d50;vertical-align:top}.cacheTable th{color:#aab;font-size:.8rem}.cacheActions{white-space:nowrap}");
  html += F(".status{display:grid;grid-template-columns:minmax(145px,42%) minmax(0,1fr);gap:7px 14px}.status span{color:#aab}.status b{overflow-wrap:anywhere}.statusNote{grid-column:1/-1;margin:2px 0 5px;line-height:1.35}.statusCards{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}.statusCard{background:#171c26;border:1px solid #343d50;border-radius:12px;padding:14px}.statusCard h3{margin:0 0 12px;font-size:1rem;color:#7ee7f0}.healthbar{display:flex;flex-wrap:wrap;gap:8px;margin:2px 0 14px}.pill{border-radius:999px;padding:7px 11px;font-size:.86rem;font-weight:700}.ok{background:#153b2b;color:#74e3aa}.warn{background:#493c16;color:#ffd166}.bad{background:#4a1f29;color:#ff8597}.off{background:#303541;color:#abb3c3}.wide{grid-column:1/-1}details summary{cursor:pointer;font-weight:700;color:#7ee7f0}details .status{margin-top:12px}.nav{display:flex;flex-wrap:wrap;gap:7px;margin:0 0 18px}.nav a{color:#cbd5e1;text-decoration:none;background:#171c26;border:1px solid #343d50;border-radius:9px;padding:9px 12px;font-weight:700}.nav a.active{color:#06141b;background:#7ee7f0;border-color:#7ee7f0}@media(max-width:650px){body{padding:12px}.fieldGrid>label,.fieldGrid>.third,.fieldGrid>.quarter,.fieldGrid>.twoThird,.fieldGrid>.threeQuarter{grid-column:span 6}.fieldGrid>.fieldWide{grid-column:1/-1}.statusCards{grid-template-columns:1fr}.wide{grid-column:auto}.status{grid-template-columns:1fr}.status b{margin-bottom:7px}}@media(max-width:430px){.fieldGrid>label,.fieldGrid>.third,.fieldGrid>.quarter,.fieldGrid>.twoThird,.fieldGrid>.threeQuarter{grid-column:1/-1}.pageSettings{grid-template-columns:minmax(0,1fr) 112px;gap:8px}.pageSettings label{font-size:.92rem}fieldset{padding:11px}.nav{display:grid;grid-template-columns:1fr 1fr}.nav a{text-align:center;padding:10px 6px}}.muted{color:#aab}</style></head><body>");
  html += F("<h1>");
  html += DEVICE_NAME;
  html += ' ';
  html += APP_VERSION;
  html += F("</h1><nav class='nav'><a href='/status'");
  if (!configurationPage) html += F(" class='active'");
  html += F(">Status</a><a href='/config'");
  if (configurationPage) html += F(" class='active'");
  html += F(">Konfiguration</a><a href='/display'>FlightWall</a><a href='/update'>Firmware-Update</a></nav>");
  if (configurationPage) {
    html += F("<form method='post' action='/save'>");
  html += F("<fieldset><legend>Sprache / Language</legend><div class='fieldGrid'><label class='third'><select name='uiLanguage'>"
            "<option value='de'");
  html += selected(!uiEnglish);
  html += F(">Deutsch</option><option value='en'");
  html += selected(uiEnglish);
  html += F(">English</option></select></label></div></fieldset>");
  html += F("<fieldset><legend>WLAN-Zugang</legend><div class='networkSummary'>Aktuelle Netzwerkadresse: <a href='http://");
  html += currentNetworkIpText();
  html += F("/'>http://");
  html += currentNetworkIpText();
  html += F("/</a> · ");
  html += WiFi.status() == WL_CONNECTED
              ? uiText("verbunden", "connected")
              : (wifiSetupAccessPointActive
                     ? uiText("Einrichtungsnetz aktiv", "setup network active")
                     : uiText("getrennt", "disconnected"));
  html += F("</div><div class='fieldGrid'><label>WLAN-Name (SSID)<input id='wifiSsid' type='text' name='wifiSsid' maxlength='32' value='");
  html += htmlEscape(wifiSsid);
  html += F("' required></label><label>Neues WLAN-Passwort<input type='password' name='wifiPassword' minlength='8' maxlength='63' value='' placeholder='unverändert lassen'></label><label class='fieldWide check'><input type='checkbox' name='wifiOpen' value='1'");
  html += checked(!wifiPassword.length());
  html += F("> Offenes WLAN ohne Passwort</label><label id='wifiScanLabel' class='fieldWide' hidden>Erkannte WLANs<select id='wifiScanResults'><option value='' selected>WLAN auswählen …</option></select></label></div><div class='buttonRow'><button id='wifiScanButton' type='button'>WLANs suchen</button><span id='wifiScanStatus' class='muted' aria-live='polite'></span></div><p class='muted'>Ein leeres Passwortfeld behält das gespeicherte WLAN-Passwort bei. Änderungen werden nach einem automatischen Neustart aktiv.</p><p class='muted'>Falls keine Verbindung möglich ist, startet das Einrichtungsnetz <b>");
  html += htmlEscape(wifiSetupAccessPointSsid.length()
                         ? wifiSetupAccessPointSsid
                         : String(WIFI_SETUP_AP_PREFIX));
  html += F("</b> mit dem Passwort <b>flightwall</b>. Konfiguration: <b>http://192.168.4.1</b></p></fieldset>");
  html += F("<fieldset><legend>Firmware-Updates</legend><div class='fieldGrid'><label>Neues OTA-Passwort<input type='password' name='otaPassword' minlength='8' maxlength='63' value='' placeholder='unverändert lassen'></label><label class='check'><input type='checkbox' name='otaDisable' value='1'> OTA deaktivieren</label></div><p class='muted'>Mindestens acht Zeichen. Ein leeres Feld behält das gespeicherte OTA-Passwort bei. Änderungen werden nach einem automatischen Neustart aktiv.</p><p>");
  if (otaPassword.length() >= 8) {
    html += F("OTA ist konfiguriert. <a href='/update'>Firmware drahtlos aktualisieren</a>");
  } else {
    html += F("OTA ist noch nicht konfiguriert.");
  }
  html += F("</p></fieldset>");
#if FLIGHTWALL_HAS_MATRIX
  html += F("<fieldset><legend>Display</legend><div class='fieldGrid'><label class='check'><input type='checkbox' name='display' value='1'");
  html += checked(displayEnabled);
  html += F("> Display eingeschaltet</label><label class='check'><input type='checkbox' name='environmentIdle' value='1'");
  html += checked(environmentIdleDisplayEnabled);
  html += F("> Wetterdaten anzeigen, wenn kein Flugzeug in der Nähe ist</label><label class='third'>Status-LED<select name='statusLedBrightness'><option value='0'");
  html += selected(statusLedBrightnessMode == 0);
  html += F(">aus</option><option value='1'");
  html += selected(statusLedBrightnessMode == 1);
  html += F(">schwach</option><option value='2'");
  html += selected(statusLedBrightnessMode == 2);
  html += F(">normal</option></select></label><label class='third'>Anzeigeradius (km)<input type='number' name='radius' min='1' max='100' step='0.5' value='");
  html += String(displayRadiusKm, 1);
  html += F("'></label><label class='third'>Standorthöhe über NHN (m)<input type='number' name='observerElevation' min='-500' max='9000' step='1' value='");
  html += String(observerElevationM, 0);
  html += F("'></label><label class='third'>Unter diesem Blickwinkel \"weit\" (°)<input type='number' name='farElevation' min='10' max='45' step='1' value='");
  html += String(farDirectionElevationDeg, 0);
  html += F("'></label><label class='third check'><input type='checkbox' name='quietHours' value='1'");
  html += checked(quietHoursEnabled);
  html += F("> Nachtruhe aktiv</label><label class='third'>Display dunkel ab<input type='time' name='quietStart' value='");
  html += minutesAsTime(quietHoursStartMinutes);
  html += F("'></label><label class='third'>Display wieder aktiv ab<input type='time' name='quietEnd' value='");
  html += minutesAsTime(quietHoursEndMinutes);
  html += F("'></label></div><p class='muted'>Die Nachtruhe darf über Mitternacht reichen. Gleiche Zeiten deaktivieren die Abdunklung.</p><button type='submit' formaction='/display-test'>");
  html += (displayTestActive || displayTestCommand == 1) ?
      "Displaytest beenden" : "Displaytest starten";
  html += F("</button><p class='muted'>Etwa 23 Sekunden: Matrix-Prüfbilder und alle Typensymbole mit Bedeutung.</p></fieldset>");
#else
  html += F("<fieldset><legend>Standort und Auswahl</legend><div class='fieldGrid'><label class='third'>Anzeigeradius (km)<input type='number' name='radius' min='1' max='100' step='0.5' value='");
  html += String(displayRadiusKm, 1);
  html += F("'></label><label class='third'>Standorthöhe über NHN (m)<input type='number' name='observerElevation' min='-500' max='9000' step='1' value='");
  html += String(observerElevationM, 0);
  html += F("'></label><label class='third'>Unter diesem Blickwinkel \"weit\" (°)<input type='number' name='farElevation' min='10' max='45' step='1' value='");
  html += String(farDirectionElevationDeg, 0);
  html += F("'></label></div><p class='muted'>Diese Servervariante stellt die Flugdaten über die Weboberfläche und die JSON-APIs bereit; eine HUB75-Matrix wird nicht angesteuert.</p></fieldset>");
  html += F("<fieldset><legend>Status-LED</legend><div class='fieldGrid'><label class='third'>Helligkeit<select name='statusLedBrightness'><option value='0'");
  html += selected(statusLedBrightnessMode == 0);
  html += F(">aus</option><option value='1'");
  html += selected(statusLedBrightnessMode == 1);
  html += F(">schwach</option><option value='2'");
  html += selected(statusLedBrightnessMode == 2);
  html += F(">normal</option></select></label><label class='third check'><input type='checkbox' name='quietHours' value='1'");
  html += checked(quietHoursEnabled);
  html += F("> Nachtabsenkung aktiv</label><label class='third'>Absenkung ab<input type='time' name='quietStart' value='");
  html += minutesAsTime(quietHoursStartMinutes);
  html += F("'></label><label class='third'>Normalbetrieb ab<input type='time' name='quietEnd' value='");
  html += minutesAsTime(quietHoursEndMinutes);
  html += F("'></label></div><p class='muted'>Grün: normal, Blau: WLAN-Verbindung, Türkis: Setup-WLAN, Orange: ADS-B-Warnung, Violett: Update, Rot: Fehler oder Transpondernotfall. Gelb signalisiert einen Bedienzugriff; regelmäßige Statusabfragen bleiben ruhig.</p></fieldset>");
#endif
  html += F("<fieldset><legend>Flugzeugdaten</legend><div class='fieldGrid'><label class='third'>Aktive ADS-B-Quelle<select name='activeAdsbSource'><option value='1'");
  html += selected(activeAdsbSource == 0);
  html += F(">Quelle 1</option><option value='2'");
  html += selected(activeAdsbSource == 1);
  html += F(">Quelle 2</option><option value='3'");
  html += selected(activeAdsbSource == 2);
  html += F(">adsb.fi (externe Quelle)</option></select></label><label class='twoThird check'><input type='checkbox' name='adsbFailover' value='1'");
  html += checked(automaticAdsbFailover);
  html += F("> Automatisch auf eine erreichbare ADS-B-Quelle wechseln</label><label class='fieldWide'>ADS-B-Adresse Quelle 1<input type='url' name='adsbUrl1' maxlength='180' value='");
  html += htmlEscape(adsbUrl1);
  html += F("'></label><label class='fieldWide'>ADS-B-Adresse Quelle 2<input type='url' name='adsbUrl2' maxlength='180' value='");
  html += htmlEscape(adsbUrl2);
  html += F("' placeholder='http://zweiter-empfaenger/data/aircraft.json'></label></div><p class='muted'>Bei aktivem Failover wird nach einem Fehler sofort eine andere konfigurierte Quelle verwendet; die bevorzugte Quelle wird jede Minute erneut geprüft. Quelle 2 darf unkonfiguriert bleiben. Der Radius von adsb.fi wird automatisch aus dem Anzeigeradius berechnet. Daten: <a href='https://adsb.fi/' target='_blank' rel='noopener'>adsb.fi</a>.</p><div class='fieldGrid'><label class='third'>Breitengrad<input type='number' name='siteLat' min='-90' max='90' step='0.000001' value='");
  html += String(siteLatitude, 6);
  html += F("'></label><label class='third'>Längengrad<input type='number' name='siteLon' min='-180' max='180' step='0.000001' value='");
  html += String(siteLongitude, 6);
  html += F("'></label><label class='third'>Blickrichtung (Grad)<input type='number' name='viewHeading' min='0' max='359' step='1' value='");
  html += String(viewHeading, 0);
  html += F("'></label><label class='check'><input type='checkbox' name='hideSport' value='1'");
  html += checked(hideSportAircraft);
  html += F("> Sport- und Kleinflugzeuge ausblenden</label><label class='check'><input type='checkbox' name='altitudeFilter' value='1'");
  html += checked(altitudeFilterEnabled);
  html += F("> Flugzeuge oberhalb der Maximalhöhe ausblenden</label><label class='third'>Maximalhöhe (ft)<input type='number' name='maximumAltitude' min='1000' max='100000' step='500' value='");
  html += String(maximumAltitudeFt);
  html += F("'></label><label class='twoThird check'><input type='checkbox' name='routesEnabled' value='1'");
  html += checked(routesEnabled);
  html += F("> Routen über adsb.im ergänzen</label><label class='twoThird check'><input type='checkbox' name='adsbDbFallback' value='1'");
  html += checked(adsbDbFallbackEnabled);
  html += F("> ADSBDB bei fehlender Route verwenden</label><label class='fieldWide'>Routen-Dienst<input type='url' name='routeUrl' maxlength='180' value='");
  html += htmlEscape(routeUrl);
  html += F("'></label><label class='third'>Test-Callsign<input type='text' name='routeTestCallsign' maxlength='10' value='DLH123'></label></div>"
            "<div class='buttonRow'><button type='submit' formaction='/test-route'>Routendienst testen</button>"
            "<button type='submit' formaction='/test-adsb'>ADS-B-Verbindung testen</button></div>"
            "<hr><div class='fieldGrid'><label>Notfalltest-Code<select name='emergencyTestCode'>"
            "<option value='7700'>7700 Emergency</option>"
            "<option value='7600'>7600 Radio</option>"
            "<option value='7500'>7500 Hijack</option></select></label>"
            "<label>Anzahl Testflugzeuge<select name='emergencyTestCount'>"
            "<option value='1'>1</option><option value='2'>2</option>"
            "<option value='3'>3</option></select></label></div>"
            "<div class='buttonRow'><button type='submit' formaction='/test-emergency'>Notfallanzeige testen</button>"
            "<button type='submit' formaction='/stop-emergency-test'>Test beenden</button></div>"
            "<p class='muted'>Der Notfalltest läuft höchstens zwei Minuten und verändert keine ADS-B-Daten oder gespeicherten Einstellungen.</p></fieldset>");
  html += F("<fieldset><legend>MQTT-Wetter</legend><div class='fieldGrid'><label class='fieldWide check'><input type='checkbox' name='mqttWeather' value='1'");
  html += checked(mqttWeatherEnabled);
  html += F("> Wetterseite in der Ruheanzeige aktivieren</label><label class='threeQuarter'>Mosquitto-Server<input type='text' name='mqttHost' maxlength='96' value='");
  html += htmlEscape(mqttHost);
  html += F("'></label><label class='quarter'>Port<input type='number' name='mqttPort' min='1' max='65535' value='");
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
  html += F("'></label><label>Regen heute (cm)<input type='text' name='rainTopic' maxlength='128' value='");
  html += htmlEscape(rainTopic);
  html += F("'></label></div><p class='muted'>Regen wird für die Anzeige von cm in mm umgerechnet. Ein leeres Passwortfeld behält das gespeicherte Passwort bei.</p></fieldset>");
  html += F("<fieldset><legend>Radar</legend><label class='check'><input type='checkbox' name='radarTrails' value='1'");
  html += checked(radarTrailsEnabled);
  html += F("> Radarspuren (60 Sekunden)</label>");
  if (!radarTrailStorage)
    html += "<p class='pill warn'>" + String(uiText(
        "Radarspuren nicht verfügbar: externer Speicher konnte nicht reserviert werden.",
        "Radar trails unavailable: external memory could not be reserved.")) + "</p>";
  html += F("</fieldset>");
#if FLIGHTWALL_HAS_MATRIX
  html += F("<fieldset><legend>Anzeigeseiten</legend><div class='fieldGrid'><label class='check'><input type='checkbox' name='smooth' value='1'");
  html += checked(smoothTransitions);
  html += F("> Sanfte Übergänge</label><label class='fieldWide'>Freitext (maximal 128 Zeichen)<textarea name='customText' maxlength='128' rows='3'>");
  html += htmlEscape(customPageText);
  html += F("</textarea></label><label>Freitext-Scrollgeschwindigkeit<select name='customScrollSpeed'><option value='2'");
  html += selected(customScrollSpeed == 2);
  html += F(">schnell</option><option value='1'");
  html += selected(customScrollSpeed == 1);
  html += F(">mittel</option><option value='0'");
  html += selected(customScrollSpeed == 0);
  html += F(">langsam</option></select></label><label>Radar-Umlaufintervall<select name='radarSweepInterval'><option value='0'");
  html += selected(radarSweepIntervalSeconds == 0);
  html += F(">Aus</option>");
  for (uint8_t seconds = 1; seconds <= 10; ++seconds) {
    html += "<option value='" + String(seconds) + "'" +
            selected(radarSweepIntervalSeconds == seconds) + ">" +
            String(seconds) + " s</option>";
  }
  html += F("</select></label></div><p class='muted'>Ein Umlauf dauert etwa 0,8 Sekunden. 0 schaltet ihn aus. Ein leerer Freitext überspringt die Seite automatisch.</p><div class='pageSettings'>");
  for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
    html += "<label><input type='checkbox' name='page" + String(page) + "On' value='1'";
    html += checked(pageEnabled[page]);
    html += "> " + pageName(page) + "</label><label>Dauer (Sekunden)<input type='number' name='page";
    html += String(page) + "Sec' min='2' max='60' value='" + String(pageDurationMs[page] / 1000) + "'></label>";
  }
  html += F("</div><p class='muted'>Reihenfolge:</p><div class='orderGrid'>");
  for (uint8_t slot = 0; slot < PAGE_COUNT; ++slot) {
    html += "<label>Position " + String(slot + 1) + "<select name='order" + String(slot) + "'>";
    for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
      html += "<option value='" + String(page) + "'" +
              selected(pageOrder[slot] == page) + ">" + pageName(page) + "</option>";
    }
    html += "</select></label>";
  }
  html += F("</div></fieldset><fieldset><legend>Helligkeit</legend><div class='fieldGrid'>");
  html += F("<label class='quarter check'><input type='checkbox' name='autoLight' value='1'");
  html += checked(automaticBrightnessEnabled);
  html += F("> BH1750-Automatik</label><label class='quarter'>Manuelle Stufe<select name='brightness'>");
  for (uint8_t i = 0; i < BRIGHTNESS_LEVEL_COUNT; ++i) {
    html += "<option value='" + String(i) + "'";
    if (i == manualBrightnessIndex) html += " selected";
    html += ">" + String(BRIGHTNESS_LEVELS[i]) + "%</option>";
  }
  html += F("</select></label><label class='quarter'>Schwelle dunkel (Lux)<input type='number' name='lowLux' min='1' max='500' step='1' value='");
  html += String(lowLuxThreshold, 0);
  html += F("'></label><label class='quarter'>Schwelle hell (Lux)<input type='number' name='highLux' min='50' max='50000' step='10' value='");
  html += String(highLuxThreshold, 0);
  html += F("'></label></div></fieldset>");
#endif
    html += F("<fieldset><legend>Konfiguration sichern</legend><p class='muted'>Die Sicherungsdatei enthält auch WLAN-, MQTT- und OTA-Passwörter. Bewahre sie geschützt auf.</p><div class='buttonRow'><a class='button' href='/configuration.json' download>Konfiguration herunterladen</a><input id='configRestoreFile' type='file' accept='application/json,.json'><button type='button' onclick='restoreConfiguration()'>Sicherung wiederherstellen</button></div><p id='configRestoreResult' class='muted'></p></fieldset><button type='submit'>Einstellungen speichern</button></form><script>async function restoreConfiguration(){const f=document.getElementById('configRestoreFile').files[0],o=document.getElementById('configRestoreResult');if(!f){o.textContent='Bitte zuerst eine JSON-Datei auswählen.';return}if(!confirm('Gesamte Konfiguration ersetzen und Controller neu starten?'))return;o.textContent='Wiederherstellung läuft…';try{const r=await fetch('/configuration/restore',{method:'POST',headers:{'Content-Type':'application/json'},body:await f.text()});o.textContent=await r.text();if(r.ok)setTimeout(()=>location.href='/status',5000)}catch(e){o.textContent='Wiederherstellung fehlgeschlagen.'}}</script>");
  } else {
    html += F("<fieldset><legend>Status <small class='muted' id='statusRefresh'>wird automatisch aktualisiert</small></legend><div id='statusData'><div class='healthbar'>");
  html += statusError.length() ? F("<span class='pill warn'>● System eingeschränkt</span>") : F("<span class='pill ok'>● System OK</span>");
  html += lastAdsbError.length() ? F("<span class='pill bad'>● ADS-B Fehler</span>") : F("<span class='pill ok'>● ADS-B OK</span>");
  if (statusFlight.emergency)
    html += F("<span class='pill bad'>● TRANSPONDER-NOTFALL</span>");
  html += !mqttWeatherEnabled ? F("<span class='pill off'>● MQTT aus</span>") :
          (mqttConnected ? F("<span class='pill ok'>● MQTT OK</span>") : F("<span class='pill warn'>● MQTT getrennt</span>"));
#if FLIGHTWALL_HAS_MATRIX
  html += environmentSensorAvailable && lightSensorAvailable
              ? F("<span class='pill ok'>● Sensoren OK</span>")
              : F("<span class='pill warn'>● Sensor prüfen</span>");
#else
  html += environmentSensorAvailable
              ? F("<span class='pill ok'>● BME280 OK</span>")
              : F("<span class='pill off'>● BME280 nicht verbunden</span>");
#endif
  html += "<span class='pill " + String(moldRiskCssClass()) +
          "'>● Schimmelrisiko: " + moldRiskText() + "</span>";
  html += F("</div><div class='statusCards'><section class='statusCard'><h3>Netzwerk</h3><div class='status'><span>Verbindungsstatus</span><b>");
  html += WiFi.status() == WL_CONNECTED
              ? uiText("verbunden", "connected")
              : uiText("getrennt", "disconnected");
  html += F("</b><span>Hostname</span><b>http://");
  html += DEVICE_HOSTNAME;
  html += F(".local</b><span>IP-Adresse</span><b>");
  html += currentNetworkIpText();
  html += F("</b><span>Verbundenes WLAN</span><b>");
  html += WiFi.status() == WL_CONNECTED ? htmlEscape(WiFi.SSID()) : "-";
  html += F("</b><span>Gespeichertes WLAN</span><b>");
  html += htmlEscape(wifiSsid);
  html += F("</b><span>Signal</span><b>");
  html += wifiSignalQualityText();
  html += F("</b><span>Gateway / Router</span><b>");
  html += WiFi.status() == WL_CONNECTED ? WiFi.gatewayIP().toString() : "-";
  html += F("</b><span>Subnetzmaske</span><b>");
  html += WiFi.status() == WL_CONNECTED ? WiFi.subnetMask().toString() : "-";
  html += F("</b><span>DNS-Server</span><b>");
  html += WiFi.status() == WL_CONNECTED ? WiFi.dnsIP(0).toString() : "-";
  html += F("</b><span>WLAN-Kanal</span><b>");
  html += WiFi.status() == WL_CONNECTED ? String(WiFi.channel()) : "-";
  html += F("</b><span>MAC-Adresse</span><b>");
  html += WiFi.macAddress();
  html += F("</b><span>Einrichtungsnetz</span><b>");
  html += wifiSetupAccessPointActive
              ? htmlEscape(wifiSetupAccessPointSsid + " / 192.168.4.1")
              : uiText("inaktiv", "inactive");
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
  html += F("</b><span>Boden-/Flugstatus</span><b>");
  if (!statusFlight.available) {
    html += "-";
  } else if (statusFlight.verticalMotion == "rollt") {
    html += uiText("rollt", "taxiing");
  } else if (statusFlight.verticalMotion == "steht") {
    html += uiText("steht", "standing");
  } else if (statusFlight.verticalMotion == "am Boden") {
    html += uiText("am Boden", "on ground");
  } else if (statusFlight.verticalMotion == "steigt") {
    html += uiText("steigt", "climbing");
  } else if (statusFlight.verticalMotion == "sinkt") {
    html += uiText("sinkt", "descending");
  } else if (statusFlight.verticalMotion == "haelt") {
    html += uiText("hält Höhe", "level");
  } else {
    html += uiText("unbekannt", "unknown");
  }
  html += F("</b><span>CPA</span><b>");
  if (statusFlight.available) {
    html += numberOrDash(statusFlight.cpaKm) + " km / ";
    const String countdown = cpaCountdownText(statusFlight);
    html += countdown.length()
                ? uiText("noch ", "in ") + countdown
                : numberOrDash(statusFlight.cpaMinutes, 0) + " min";
  } else {
    html += "-";
  }
  html += F("</b><span>Special Type</span><b>");
  html += statusFlight.specialType.length()
              ? htmlEscape(statusFlight.specialType) : "nein";
  html += F("</b><span>Auswahlbegründung</span><b>");
  html += statusFlight.selectionReason.length()
              ? htmlEscape(statusFlight.selectionReason) : "-";
  html += F("</b><span>Radius normal / aktuell</span><b>");
  html += statusFlight.emergency
              ? String(displayRadiusKm, 1) + " km / unbegrenzt"
              : String(displayRadiusKm, 1) + " / " +
                    String(statusFlight.specialType.length()
                               ? displayRadiusKm * 2.0f : displayRadiusKm, 1) + " km";
  html += "</b></div></section><section class='statusCard wide'><h3>";
  html += uiText("Tagesstatistik Flugzeuge", "Daily aircraft statistics");
  html += "</h3><div class='status'><span>";
  html += uiText("Flugzeuge gesamt", "Unique aircraft");
  html += "</span><b>" + String(seenAircraftCount);
  html += "</b><span>";
  html += uiText("Specials gesamt", "Total specials");
  html += "</span><b>" + String(seenSpecialAircraftCount);
  html += "</b><span>";
  html += uiText("Special-Anteil", "Special share");
  html += "</span><b>";
  html += seenAircraftCount
              ? String(100.0f * seenSpecialAircraftCount /
                           static_cast<float>(seenAircraftCount), 1) + " %"
              : "-";
  html += "</b><span>";
  html += uiText("Erfassung seit", "Tracking since");
  html += "</span><b>" + dailyStatisticsStartedText();
  html += F("</b><span>7500 · Hijack</span><b>");
  html += String(specialAircraftCount(SpecialStatisticCategory::Emergency7500));
  html += F("</b><span>7600 · Radio</span><b>");
  html += String(specialAircraftCount(SpecialStatisticCategory::Emergency7600));
  html += F("</b><span>7700 · Emergency</span><b>");
  html += String(specialAircraftCount(SpecialStatisticCategory::Emergency7700));
  html += F("</b><span>German Airforce</span><b>");
  html += String(specialAircraftCount(SpecialStatisticCategory::GermanAirforce));
  html += F("</b><span>Military</span><b>");
  html += String(specialAircraftCount(SpecialStatisticCategory::Military));
  html += "</b><span>";
  html += uiText("Bundespolizei", "Federal Police");
  html += "</span><b>" +
          String(specialAircraftCount(SpecialStatisticCategory::FederalPolice));
  html += "</b><span>";
  html += uiText("Polizei", "Police");
  html += "</span><b>" +
          String(specialAircraftCount(SpecialStatisticCategory::Police));
  html += "</b><span>";
  html += uiText("Regierung", "Government");
  html += "</span><b>" +
          String(specialAircraftCount(SpecialStatisticCategory::Government));
  html += "</b><span>";
  html += uiText("Hubschrauber", "Helicopter");
  html += "</span><b>" +
          String(specialAircraftCount(SpecialStatisticCategory::Helicopter));
  html += "</b><span>";
  html += uiText("Luftschiff", "Airship");
  html += "</span><b>" +
          String(specialAircraftCount(SpecialStatisticCategory::Airship));
  html += F("</b><span>A380</span><b>");
  html += String(specialAircraftCount(SpecialStatisticCategory::A380));
  html += "</b><span>";
  html += uiText("Sonstige", "Other");
  html += "</span><b>" +
          String(specialAircraftCount(SpecialStatisticCategory::Other));
  html += "</b><span>";
  html += uiText("Special-Erfassung", "Special tracking");
  html += "</span><b>";
  html += specialAircraftTrackingLimitReached
              ? uiText("Limit erreicht", "limit reached")
              : String(seenSpecialAircraftCount) + " / " +
                    String(SPECIAL_AIRCRAFT_TRACKING_LIMIT);
  html += "</b><span>";
  html += uiText("Flugzeug-Erfassung", "Aircraft tracking");
  html += "</span><b>";
  html += aircraftTrackingLimitReached
              ? uiText("Limit erreicht", "limit reached")
              : String(seenAircraftCount) + " / " +
                    String(OBSERVED_AIRCRAFT_TRACKING_LIMIT);
  html += F("</b></div></section>");
  html += F("<section class='statusCard wide'><h3>");
  html += uiText("Special-Chronik", "Special chronicle");
  html += " · " + String(specialChronicleCount) + " / " +
          String(SPECIAL_CHRONICLE_LIMIT) + "</h3>";
  if (!specialChronicleCount) {
    html += "<p class='muted'>" +
            String(uiText("Heute wurde noch kein Special erkannt.",
                          "No special aircraft has been detected today.")) +
            "</p>";
  } else {
    html += "<div class='tableScroll'><table class='cacheTable'><thead><tr><th>" +
            String(uiText("Zuletzt gesehen", "Last seen")) +
            "</th><th>Flug</th><th>Klasse</th><th>Typ</th><th>min. Entfernung</th><th>min. CPA</th><th>Angezeigt</th><th>Auswahlbegründung</th></tr></thead><tbody>";
    size_t chronologicalOrder[SPECIAL_CHRONICLE_LIMIT];
    for (size_t index = 0; index < specialChronicleCount; ++index)
      chronologicalOrder[index] = index;
    for (size_t index = 1; index < specialChronicleCount; ++index) {
      const size_t candidate = chronologicalOrder[index];
      size_t position = index;
      while (position > 0 &&
             specialChronicle[chronologicalOrder[position - 1]].lastSeen <
                 specialChronicle[candidate].lastSeen) {
        chronologicalOrder[position] = chronologicalOrder[position - 1];
        --position;
      }
      chronologicalOrder[position] = candidate;
    }
    for (size_t offset = 0; offset < specialChronicleCount; ++offset) {
      const SpecialChronicleEntry &entry =
          specialChronicle[chronologicalOrder[offset]];
      html += "<tr><td>" + journalTimeText(entry.lastSeen) + "</td><td><b>" +
              htmlEscape(entry.flight.length() ? entry.flight : entry.hex) +
              "</b></td><td>" + htmlEscape(entry.specialType) +
              "</td><td>" + htmlEscape(entry.type) + "</td><td>" +
              numberOrDash(entry.minimumDistanceKm) + " km</td><td>" +
              numberOrDash(entry.minimumCpaKm) + " km</td><td>" +
              uiText(entry.displayed ? "ja" : "nein",
                     entry.displayed ? "yes" : "no") +
              "</td><td>" +
              htmlEscape(entry.selectionReason.length()
                             ? entry.selectionReason
                             : uiText("nicht ausgewählt", "not selected")) +
              "</td></tr>";
    }
    html += F("</tbody></table></div>");
  }
  if (specialChronicleLimitReached)
    html += "<p class='pill warn'>" +
            String(uiText("Chronik-Limit erreicht", "Chronicle limit reached")) +
            "</p>";
  html += F("<div class='buttonRow'><form class='inlineForm' method='get' action='/specials.json'><button type='submit'>JSON herunterladen</button></form><form class='inlineForm' method='get' action='/specials.csv'><button type='submit'>CSV herunterladen</button></form></div></section>");
  appendDailyJournalStatus(html);
  html += F("<section class='statusCard'><h3>ADS-B und Route</h3><div class='status'><span>ADS-B</span><b>");
  html += statusError.length() ? displayAscii(statusError) : "OK";
  html += F("</b><span>ADS-B-Fehler</span><b>");
  html += lastAdsbError.length() ? htmlEscape(lastAdsbError) : "keiner";
  html += F("</b><span>Routenfehler</span><b>");
  html += lastRouteError.length() ? htmlEscape(lastRouteError) : "keiner";
  html += F("</b><span>Routenstatus</span><b>");
  html += htmlEscape(currentRoute.status);
  html += F("</b><span>Routenquelle</span><b>");
  html += currentRoute.source.length() ? htmlEscape(currentRoute.source) : "-";
  html += F("</b><span>Routenqualität</span><b>");
  html += currentRoute.found()
              ? String(currentRoute.confidence) + "% · " +
                    htmlEscape(currentRoute.confidenceReason)
              : "-";
  html += F("</b><span>ADSBDB-Fallback</span><b>");
  html += adsbDbFallbackEnabled ? "aktiv" : "aus";
  html += F("</b><span>Routencache wartet auf Flash</span><b>");
  html += routeClient.hasPendingWrites() ? "ja" : "nein";
  html += F("</b><span>Aktive ADS-B-Quelle</span><b>");
  html += activeAdsbSourceName();
  html += F("</b><span>Bevorzugte ADS-B-Quelle</span><b>");
  html += adsbSourceName(activeAdsbSource);
  html += F("</b><span>Automatisches Failover</span><b>");
  html += automaticAdsbFailover
              ? (currentAdsbSource == activeAdsbSource
                     ? uiText("bereit", "ready")
                     : String(uiText("aktiv seit ", "active for ")) +
                           ageText(adsbFailoverSince))
              : uiText("aus", "off");
  html += F("</b><span>Empfangen / Kandidaten</span><b>");
  html += String(lastReceivedAircraft) + " / " + String(lastCandidateAircraft);
  html += F("</b><span>Letzter Erfolg</span><b>");
  html += ageText(lastAdsbSuccess);
  html += F("</b></div></section>");
  appendRouteDiagnosticsHtml(html);
  html += F("<section class='statusCard'><h3>Konfiguration</h3><div class='status'><span>Standort / Blickrichtung</span><b>");
  html += String(siteLatitude, 6) + " / " + String(siteLongitude, 6) +
          " / " + String(viewHeading, 0) + "°";
  html += F("</b><span>Anzeigeradius</span><b>");
  html += String(displayRadiusKm, 1) + " km";
  html += F("</b><span>Höhenfilter</span><b>");
  html += altitudeFilterEnabled ? "bis " + String(maximumAltitudeFt) + " ft" : "aus";
  html += F("</b><span>Kleinflugzeugfilter</span><b>");
  html += hideSportAircraft ? "aktiv" : "aus";
#if FLIGHTWALL_HAS_MATRIX
  html += F("</b><span>Wetteranzeige</span><b>");
  html += environmentIdleDisplayEnabled ? "aktiv" : "aus";
#endif
  html += F("</b></div></section><section class='statusCard wide'><h3>Übersetzungstabellen</h3><div class='status'><span>Unbekannte Flughäfen</span><b>");
  html += unknownCodesText(true);
  html += F("</b><span>Unbekannte Airlines</span><b>");
  html += unknownCodesText(false);
  html += "</b><span>";
  html += uiText("Überlauf Flughäfen / Airlines",
                 "Overflow airports / airlines");
  html += "</span><b>" + String(droppedUnknownAirports()) + " / " +
          String(droppedUnknownAirlines());
  html += F("</b></div><form class='inlineForm' method='get' action='/unknown-codes.json'><button type='submit'>");
  html += uiText("Codes herunterladen (.json)", "Download codes (.json)");
  html += F("</button></form></section><details id='technicalDiagnosis' class='statusCard wide'><summary>Technische Diagnose</summary><div class='status'><span>ADS-B-Quelle 1</span><b>");
  html += htmlEscape(adsbUrl1);
  html += F("</b><span>ADS-B-Quelle 2</span><b>");
  html += adsbUrl2.length() ? htmlEscape(adsbUrl2) : "nicht konfiguriert";
  html += F("</b><span>ADS-B-Quelle 3</span><b>");
  html += htmlEscape(adsbFiUrl());
  html += F("</b><span>Letzter Versuch</span><b>");
  html += ageText(lastAdsbAttempt);
  html += F("</b><span>Anzeige aktualisiert</span><b>");
  html += ageText(statusUpdated);
  html += F("</b><span>ADS-B-Zeitstempel</span><b>");
  html += lastFeedTimestamp ? String(lastFeedTimestamp) : "-";
  html += F("</b><span>Firmware</span><b>");
  html += APP_VERSION;
  html += F("</b><span>OTA</span><b>");
  html += otaPassword.length() >= 8 ? "konfiguriert" : "nicht konfiguriert";
  html += F("</b><span>Erstellt</span><b>");
  html += BUILD_TIMESTAMP;
  html += F("</b><span>Laufzeit</span><b>");
  html += uptimeText();
  html += F("</b><span>Freier RAM</span><b>");
  html += String(statusMemory.freeInternal / 1024) + " KiB (Minimum " +
          String(statusMemory.minimumInternal / 1024) + " KiB)";
  html += "</b><span>" + String(uiText("Größter freier interner Block", "Largest free internal block")) +
          "</span><b>" + String(statusMemory.largestInternal) + " Bytes";
  html += "</b><span>" + String(uiText("Radarspuren-Speicher", "Radar trail storage")) + "</span><b>";
  html += radarTrailStorage
      ? "PSRAM · " + String(sizeof(RadarTrailStorage)) + " Bytes"
      : String(uiText("Nicht verfügbar; Live-Radar bleibt aktiv", "Unavailable; live radar remains active"));
  const auto &textMemory = psramTextMemoryStats();
  html += "</b><span>" + String(uiText("Journal-/Cachetexte", "Journal/cache text")) + "</span><b>PSRAM · " +
          String(textMemory.allocatedBytes) + " Bytes / " + String(textMemory.blocks) +
          " " + uiText("Textpuffer", "text buffers");
  html += "</b><span>" + String(uiText("Textspeicher-Fehler", "Text allocation failures")) + "</span><b>" +
          String(textMemory.allocationFailures);
  html += F("</b><span>ESP32-S3-Chiptemperatur</span><b>");
  html += numberOrDash(chipTemperatureC()) + " °C";
  html += F("</b></div></details><section class='statusCard'><h3>Wetter und Sensoren</h3><div class='status'><span>BME280</span><b>");
  if (environmentSensorAvailable) {
    html += "verbunden / 0x" + String(environmentSensorAddress, HEX) +
#if FLIGHTWALL_HAS_MATRIX
            (environmentSensorOnStemma ? " / STEMMA QT" : " / GPIO 3/9");
#else
            " / GPIO 16/17";
#endif
  } else {
    html += "nicht gefunden";
  }
  html += F("</b><span>Umgebungstemperatur</span><b>");
  html += numberOrDash(ambientTemperatureC) + " °C";
  html += F("</b><span>Relative Feuchte</span><b>");
  html += numberOrDash(relativeHumidityPercent) + " %";
  html += F("</b><span>Schimmelrisiko</span><b class='");
  html += moldRiskCssClass();
  html += "'>" + String(moldRiskText());
  html += F("</b><span>Taupunkt</span><b>");
  html += numberOrDash(dewPointC) + " °C";
  html += F("</b><span>Kritische Oberflächentemperatur</span><b>");
  html += numberOrDash(moldCriticalSurfaceTemperatureC) + " °C";
  html += F("</b><span>Dauer erhöhter Feuchte</span><b>");
  html += elevatedHumidityDurationText();
  html += F("</b><p class='statusNote muted'>Oberflächen unterhalb dieses Werts können 80 % relative Feuchte erreichen.</p><span>Luftdruck auf Meereshöhe</span><b>");
  html += numberOrDash(airPressureHpa) + " hPa";
  html += F("</b><span>Stationsdruck (BME280 roh)</span><b>");
  html += numberOrDash(stationPressureHpa) + " hPa";
#if FLIGHTWALL_HAS_MATRIX
  html += F("</b><span>Wetterdaten-Ruheanzeige</span><b>");
  html += environmentIdleDisplayEnabled ? "aktiv" : "aus";
#endif
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
  html += F("</b><span>Regen heute</span><b>");
  html += numberOrDash(statusRain) + " mm";
  html += F("</b></div></section><section class='statusCard'><h3>");
#if FLIGHTWALL_HAS_MATRIX
  html += F("Display</h3><div class='status'><span>Helligkeit</span><b>");
  html += String(BRIGHTNESS_LEVELS[brightnessIndex]) + "% / " +
          (automaticBrightnessEnabled ? "Automatik" : "manuell");
  html += F("</b><span>Matrix-Farbtiefe</span><b>");
  html += String(BIT_DEPTH) + " Bit";
  html += F("</b><span>Matrix-Refresh</span><b>");
  html += String(matrixRefreshHz) + " Hz";
  html += F("</b><span>Bildübergabe</span><b>");
  html += String(matrixShowLastUs) + " / max. " +
          String(matrixShowMaxUs) + " µs";
#else
  html += F("Server</h3><div class='status'><span>Betriebsart</span><b>ESP32-S3 ohne Matrix");
#endif
  html += F("</b><span>Status-LED</span><b>");
  html += statusLedBrightnessMode == 0
      ? "aus"
      : String(statusLedStateName(statusLedState())) +
            (statusLedBrightnessMode == 1 ? " / schwach" : " / normal");
  html += F("</b><span>Blickgeometrie</span><b>");
  html += String(observerElevationM, 0) + " m NHN / weit unter " +
          String(farDirectionElevationDeg, 0) + "°";
  struct tm localClock;
  html += F("</b><span>Lokale Zeit</span><b>");
  if (readLocalClock(localClock)) {
    char clockText[20];
    strftime(clockText, sizeof(clockText), "%d.%m.%Y %H:%M", &localClock);
    html += clockText;
  } else {
    html += "warte auf NTP";
  }
  html += F("</b><span>Nachtmodus</span><b>");
  html += quietHoursEnabled ?
      (minutesAsTime(quietHoursStartMinutes) + "–" +
       minutesAsTime(quietHoursEndMinutes) +
       (quietHoursAreActive() ? " / aktiv" : " / inaktiv")) : "aus";
  html += F("</b></div></section><details id='taskDiagnosis' class='statusCard wide'><summary>Task- und Resetdiagnose</summary><div class='status'><span>Tasks</span><b>");
#if FLIGHTWALL_HAS_MATRIX
  html += F("Netz Core 0 / Display Core 1");
#else
  html += F("Netz Core 0 / Server Core 1");
#endif
  html += F("</b><span>Stackreserve</span><b>");
  html += networkTaskHandle ? String(uxTaskGetStackHighWaterMark(networkTaskHandle) * sizeof(StackType_t)) : "-";
  html += " / ";
  html += displayTaskHandle ? String(uxTaskGetStackHighWaterMark(displayTaskHandle) * sizeof(StackType_t)) : "-";
  html += F(" Bytes</b><span>Letzter Reset</span><b>");
  html += resetReasonText();
  html += F("</b><span>Netzphase beim Watchdog-Reset</span><b>");
  html += networkPhaseName(previousResetNetworkPhase);
  html += F("</b><span>Maximale Netzlaufzeiten</span><b>");
  html += networkPhaseTimingText();
  html += F("</b></div></details></div></div><form method='post' action='/restart' onsubmit=\"return confirm('ESP wirklich neu starten?')\"><p><button type='submit'>ESP neu starten</button></p></form></fieldset>");
  if (includeRouteCache) {
    const std::vector<RouteCacheEntryInfo> cacheEntries =
        routeClient.cacheEntries(lastFeedTimestamp);
    html += F("<fieldset id='routeCache'><legend>Routencache</legend><p class='muted'>Gefundene Routen gelten sechs Stunden und werden zusätzlich anhand von Position, Höhe und Flugbewegung geprüft. ADSBDB-Routen bleiben nur im RAM. Negative Ergebnisse gelten eine Stunde und erkannte Einsatzrufzeichen 30 Minuten. Unplausible Routen werden verworfen und nicht gespeichert.</p>");
    if (routedFlight.length()) {
      html += F("<form class='inlineForm' method='post' action='/route-cache/refresh'><input type='hidden' name='callsign' value='");
      html += htmlEscape(routedFlight);
      html += F("'><button type='submit'>Aktuelle Route neu laden</button></form>");
    }
    html += F("<form class='inlineForm' method='post' action='/route-cache/clear' onsubmit=\"return confirm('Gesamten Routencache wirklich löschen?')\"><button class='danger' type='submit'>Gesamten Cache löschen</button></form>");
    if (cacheEntries.empty()) {
      html += F("<p>Der Routencache ist leer.</p>");
    } else {
      html += F("<div style='overflow-x:auto'><table class='cacheTable'><thead><tr><th>Callsign</th><th>Route</th><th>Quelle</th><th>Alter</th><th>Speicher</th><th>Aktionen</th></tr></thead><tbody>");
      for (const RouteCacheEntryInfo& entry : cacheEntries) {
        String age = "-";
        if (lastFeedTimestamp && entry.updated && lastFeedTimestamp >= entry.updated) {
          const uint32_t seconds = lastFeedTimestamp - entry.updated;
          if (seconds < 3600U) age = String(seconds / 60U) + " min";
          else if (seconds < 86400U) age = String(seconds / 3600U) + " h";
          else age = String(seconds / 86400U) + " d";
        }
        String storage;
        if (entry.inMemory) storage += "RAM";
        if (entry.persistent) {
          if (storage.length()) storage += " + ";
          storage += "Flash";
        }
        if (entry.pendingWrite) storage += " / wartet";
        if (entry.expired) storage += " / abgelaufen";
        html += F("<tr><td><b>");
        html += htmlEscape(entry.callsign);
        html += F("</b></td><td>");
        html += entry.found
                    ? htmlEscape(entry.routeCodes.length()
                                     ? entry.routeCodes : "gefunden")
                    : "nicht gefunden";
        html += F("</td><td>");
        html += entry.source.length() ? htmlEscape(entry.source) : "-";
        html += F("</td><td>");
        html += age;
        html += F("</td><td>");
        html += storage;
        html += F("</td><td class='cacheActions'>");
        if (entry.callsign == routedFlight) {
          html += F("<form class='inlineForm' method='post' action='/route-cache/refresh'><input type='hidden' name='callsign' value='");
          html += htmlEscape(entry.callsign);
          html += F("'><button class='smallButton' type='submit'>Neu laden</button></form>");
        }
        html += F("<form class='inlineForm' method='post' action='/route-cache/delete' onsubmit=\"return confirm('Diesen Cacheeintrag löschen?')\"><input type='hidden' name='callsign' value='");
        html += htmlEscape(entry.callsign);
        html += F("'><button class='smallButton danger' type='submit'>Löschen</button></form></td></tr>");
      }
      html += F("</tbody></table></div>");
    }
    html += F("</fieldset>");
  }
  }
  if (configurationPage) {
    html += F(R"HTML(<script>
const wifiScanButton=document.getElementById('wifiScanButton');
const wifiScanLabel=document.getElementById('wifiScanLabel');
const wifiScanResults=document.getElementById('wifiScanResults');
const wifiScanStatus=document.getElementById('wifiScanStatus');
const wifiSsid=document.getElementById('wifiSsid');
async function pollWifiScan(){
  try{
    const response=await fetch('/api/wifi-scan',{cache:'no-store'});
    const data=await response.json();
    if(data.state==='running'){
      wifiScanStatus.textContent='Suche läuft …';
      setTimeout(pollWifiScan,700);
      return;
    }
    if(data.state!=='ready')throw new Error();
    wifiScanResults.replaceChildren();
    const placeholder=document.createElement('option');
    placeholder.value='';placeholder.textContent='WLAN auswählen …';placeholder.selected=true;
    wifiScanResults.appendChild(placeholder);
    data.networks.forEach(network=>{
      const option=document.createElement('option');
      option.value=network.ssid;
      option.dataset.open=network.open?'1':'0';
      option.textContent=network.ssid+' · '+network.rssi+' dBm · '+network.security+
          (network.current?' · aktuell':'');
      wifiScanResults.appendChild(option);
    });
    wifiScanLabel.hidden=false;
    wifiScanStatus.textContent=data.networks.length?
        data.networks.length+' WLANs gefunden':'Keine WLANs gefunden.';
    wifiScanButton.disabled=false;
  }catch(error){
    wifiScanStatus.textContent='WLAN-Suche fehlgeschlagen.';
    wifiScanButton.disabled=false;
  }
}
wifiScanButton.addEventListener('click',async()=>{
  wifiScanButton.disabled=true;
  wifiScanStatus.textContent='Suche läuft …';
  try{
    const response=await fetch('/api/wifi-scan',{method:'POST'});
    if(!response.ok)throw new Error();
    pollWifiScan();
  }catch(error){
    wifiScanStatus.textContent='WLAN-Suche fehlgeschlagen.';
    wifiScanButton.disabled=false;
  }
});
wifiScanResults.addEventListener('change',()=>{
  const option=wifiScanResults.selectedOptions[0];
  if(!option||!option.value)return;
  wifiSsid.value=option.value;
  const openNetwork=document.querySelector("input[name='wifiOpen']");
  if(openNetwork)openNetwork.checked=option.dataset.open==='1';
});
</script>)HTML");
  } else {
    html += F("<script>let statusBusy=false;async function refreshStatus(){"
            "if(statusBusy||document.hidden)return;statusBusy=true;"
            "const note=document.getElementById('statusRefresh');"
            "const current=document.getElementById('statusData');"
            "const openDetails=current?[...current.querySelectorAll('details[id][open]')].map(item=>item.id):[];"
            "try{const response=await fetch('/api/status-fragment',{cache:'no-store'});if(!response.ok)throw 0;"
            "const fresh=await response.text();"
            "if(fresh&&current){current.innerHTML=fresh;openDetails.forEach(id=>{const item=document.getElementById(id);if(item)item.open=true;});}"
            "if(note)note.textContent='aktualisiert '+new Date().toLocaleTimeString('de-DE');"
            "}catch(error){if(note)note.textContent='Aktualisierung fehlgeschlagen';}"
            "finally{statusBusy=false;}}setInterval(refreshStatus,10000);</script>");
  }
#if FLIGHTWALL_HAS_MATRIX
  html += F("<p class='muted'>UP/DOWN: manuell; beide Tasten: Automatik.</p>");
#endif
  html += F("</body></html>");
  translateWebPageToEnglish(html);
  return html;
}


void handleStatusFragment() {
  String page = webPage(false, false);
  const String opening = "<div id='statusData'>";
  const String closing = "</div><form method='post' action='/restart'";
  const int start = page.indexOf(opening);
  const int end = start >= 0 ? page.indexOf(closing, start) : -1;
  if (start < 0 || end < 0) {
    webServer.send(500, "text/plain; charset=utf-8", "Status nicht verfügbar");
    return;
  }
  const String fragment = page.substring(start + opening.length(), end);
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.sendHeader("Connection", "close");
  webServer.send(200, "text/html; charset=utf-8", fragment);
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
  wifiSettingsChanged = false;
  wifiSettingsError = "";
  otaSettingsChanged = false;
  otaSettingsError = "";
  settingsWritePreparing = true;
  vTaskDelay(pdMS_TO_TICKS(120));
  settingsWritePreparing = false;
  settingsWriteInProgress = true;
  if (xSemaphoreTake(preferencesMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
    String requestedWifiSsid = webServer.arg("wifiSsid");
    const String requestedWifiPassword = webServer.arg("wifiPassword");
    const bool requestedOpenWifi = webServer.hasArg("wifiOpen");
    if (!requestedWifiSsid.length() || requestedWifiSsid.length() > 32) {
      wifiSettingsError = uiText(
          "Der WLAN-Name muss 1 bis 32 Zeichen lang sein.",
          "The Wi-Fi name must contain 1 to 32 characters.");
    } else if (!requestedOpenWifi && requestedWifiPassword.length() &&
               (requestedWifiPassword.length() < 8 ||
                requestedWifiPassword.length() > 63)) {
      wifiSettingsError = uiText(
          "Das WLAN-Passwort muss 8 bis 63 Zeichen lang sein.",
          "The Wi-Fi password must contain 8 to 63 characters.");
    } else {
      String nextWifiPassword = wifiPassword;
      if (requestedOpenWifi) nextWifiPassword = "";
      else if (requestedWifiPassword.length())
        nextWifiPassword = requestedWifiPassword;
      wifiSettingsChanged = requestedWifiSsid != wifiSsid ||
                            nextWifiPassword != wifiPassword;
      wifiSsid = requestedWifiSsid;
      wifiPassword = nextWifiPassword;
    }
    const String requestedOtaPassword = webServer.arg("otaPassword");
    const bool requestedOtaDisabled = webServer.hasArg("otaDisable");
    String nextOtaPassword = otaPassword;
    if (requestedOtaDisabled) {
      nextOtaPassword = "";
    } else if (requestedOtaPassword.length()) {
      if (requestedOtaPassword.length() < 8 ||
          requestedOtaPassword.length() > 63) {
        otaSettingsError = uiText(
            "Das OTA-Passwort muss 8 bis 63 Zeichen lang sein.",
            "The OTA password must contain 8 to 63 characters.");
      } else {
        nextOtaPassword = requestedOtaPassword;
      }
    }
    if (!otaSettingsError.length() && nextOtaPassword != otaPassword) {
      otaPassword = nextOtaPassword;
      otaSettingsChanged = true;
    }
    uiEnglish = webServer.arg("uiLanguage") == "en";
#if FLIGHTWALL_HAS_MATRIX
    displayEnabled = webServer.hasArg("display");
    environmentIdleDisplayEnabled = webServer.hasArg("environmentIdle");
#endif
    statusLedBrightnessMode = constrain(
        webServer.arg("statusLedBrightness").toInt(), 0, 2);
    setStatusLedBrightness(statusLedBrightnessMode);
    mqttWeatherEnabled = webServer.hasArg("mqttWeather");
    quietHoursEnabled = webServer.hasArg("quietHours");
    uint16_t requestedQuietTime;
    if (parseTimeMinutes(webServer.arg("quietStart"), requestedQuietTime)) {
      quietHoursStartMinutes = requestedQuietTime;
    }
    if (parseTimeMinutes(webServer.arg("quietEnd"), requestedQuietTime)) {
      quietHoursEndMinutes = requestedQuietTime;
    }
#if FLIGHTWALL_HAS_MATRIX
    automaticBrightnessEnabled = webServer.hasArg("autoLight");
#endif
    displayRadiusKm = constrain(webServer.arg("radius").toFloat(), 1.0f, 100.0f);
    observerElevationM = constrain(
        webServer.arg("observerElevation").toFloat(), -500.0f, 9000.0f);
    farDirectionElevationDeg = constrain(
        webServer.arg("farElevation").toFloat(), 10.0f, 45.0f);
#if FLIGHTWALL_HAS_MATRIX
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
    radarSweepIntervalSeconds = constrain(
        webServer.arg("radarSweepInterval").toInt(), 0, 10);
    radarSweepStarted = 0;
    radarSweepLastDraw = 0;
#endif
    radarTrailsEnabled = webServer.hasArg("radarTrails");
    // Saving configuration starts a fresh trail, including source URL edits.
    if (radarTrailStorage) radarTrailStorage->tracker.clear();
    if (xSemaphoreTake(flightMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      if (radarTrailStorage) {
        for (size_t i = 0; i < networkRadarPointCount; ++i)
          radarTrailStorage->network[i].trailCount = 0;
      }
      xSemaphoreGive(flightMutex);
    }
    String requestedAdsbUrl1 = webServer.arg("adsbUrl1");
    requestedAdsbUrl1.trim();
    if (requestedAdsbUrl1.startsWith("http://") ||
        requestedAdsbUrl1.startsWith("https://")) adsbUrl1 = requestedAdsbUrl1;
    String requestedAdsbUrl2 = webServer.arg("adsbUrl2");
    requestedAdsbUrl2.trim();
    if (requestedAdsbUrl2.isEmpty() || requestedAdsbUrl2.startsWith("http://") ||
        requestedAdsbUrl2.startsWith("https://")) adsbUrl2 = requestedAdsbUrl2;
    const int requestedAdsbSource =
        constrain(webServer.arg("activeAdsbSource").toInt(), 1, 3) - 1;
    activeAdsbSource = requestedAdsbSource == 1 && !adsbUrl2.length()
                           ? 0 : requestedAdsbSource;
    automaticAdsbFailover = webServer.hasArg("adsbFailover");
    if (!automaticAdsbFailover || !adsbSourceConfigured(currentAdsbSource)) {
      currentAdsbSource = activeAdsbSource;
      adsbFailoverSince = 0;
    }
    String requestedRouteUrl = webServer.arg("routeUrl");
    requestedRouteUrl.trim();
    if (requestedRouteUrl.startsWith("http://") ||
        requestedRouteUrl.startsWith("https://")) routeUrl = requestedRouteUrl;
    siteLatitude = constrain(webServer.arg("siteLat").toDouble(), -90.0, 90.0);
    siteLongitude = constrain(webServer.arg("siteLon").toDouble(), -180.0, 180.0);
    viewHeading = constrain(webServer.arg("viewHeading").toFloat(), 0.0f, 359.0f);
    routesEnabled = webServer.hasArg("routesEnabled");
    adsbDbFallbackEnabled = webServer.hasArg("adsbDbFallback");
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
#if FLIGHTWALL_HAS_MATRIX
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
#endif
#if FLIGHTWALL_HAS_MATRIX
    preferences.putBool("displayOn", displayEnabled);
#endif
    preferences.putUChar("statusLed", statusLedBrightnessMode);
    if (wifiSettingsChanged) {
      preferences.putString("wifiSsid", wifiSsid);
      preferences.putString("wifiPass", wifiPassword);
    }
    if (otaSettingsChanged) preferences.putString("otaPass", otaPassword);
    preferences.putBool("uiEnglish", uiEnglish);
#if FLIGHTWALL_HAS_MATRIX
    preferences.putBool("envIdle", environmentIdleDisplayEnabled);
#endif
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
#if FLIGHTWALL_HAS_MATRIX
    preferences.putBool("autoLight", automaticBrightnessEnabled);
    preferences.putUChar("brightness", manualBrightnessIndex);
    preferences.putFloat("lowLux", lowLuxThreshold);
    preferences.putFloat("highLux", highLuxThreshold);
#endif
    preferences.putFloat("radiusKm", displayRadiusKm);
    preferences.putFloat("observerElevM", observerElevationM);
    preferences.putFloat("farAngle", farDirectionElevationDeg);
    preferences.putString("adsbUrl", adsbUrl1);
    preferences.putString("adsbUrl2", adsbUrl2);
    preferences.putUChar("adsbActive", activeAdsbSource);
    preferences.putBool("adsbFailover", automaticAdsbFailover);
    preferences.putString("routeUrl", routeUrl);
    preferences.putDouble("siteLat", siteLatitude);
    preferences.putDouble("siteLon", siteLongitude);
    preferences.putFloat("heading", viewHeading);
    preferences.putBool("radarTrails", radarTrailsEnabled);
    preferences.putBool("routesOn", routesEnabled);
    preferences.putBool("routeFallback", adsbDbFallbackEnabled);
    preferences.putBool("hideSport", hideSportAircraft);
    preferences.putBool("altFilter", altitudeFilterEnabled);
    preferences.putInt("maxAltFt", maximumAltitudeFt);
#if FLIGHTWALL_HAS_MATRIX
    preferences.putString("customText", customPageText);
    preferences.putUChar("customSpd", customScrollSpeed);
    preferences.putUChar("radarSweepSec", radarSweepIntervalSeconds);
    preferences.putBool("smooth", smoothTransitions);
    for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
      preferences.putBool(("p" + String(page) + "On").c_str(), pageEnabled[page]);
      preferences.putUInt(("p" + String(page) + "Ms").c_str(), pageDurationMs[page]);
      preferences.putUChar(("order" + String(page)).c_str(), pageOrder[page]);
    }
#endif
    xSemaphoreGive(preferencesMutex);
  }
  settingsWriteInProgress = false;
  if (mqttClient.connected()) mqttClient.disconnect();
  mqttConnected = false;
  lastMqttAttempt = 0;
  resetPageTransition();
  if (wifiSettingsChanged || otaSettingsChanged)
    manualRebootAt = millis() + 2500;
}


void handleWebSave() {
  applyWebSettings();
  if (wifiSettingsError.length()) {
    webServer.send(400, "text/html; charset=utf-8",
        String("<!doctype html><html lang='") + (uiEnglish ? "en" : "de") +
        "'><meta name='viewport' content='width=device-width'>"
        "<body style='font:18px system-ui;max-width:600px;margin:40px auto'>"
        "<h1>" + uiText("WLAN-Einstellungen ungültig",
                         "Invalid Wi-Fi settings") +
        "</h1><p>" + htmlEscape(wifiSettingsError) +
        "</p><p><a href='/config'>" + uiText("Zurück", "Back") +
        "</a></p></body></html>");
    return;
  }
  if (otaSettingsError.length()) {
    webServer.send(400, "text/html; charset=utf-8",
        String("<!doctype html><html lang='") + (uiEnglish ? "en" : "de") +
        "'><meta name='viewport' content='width=device-width'>"
        "<body style='font:18px system-ui;max-width:600px;margin:40px auto'>"
        "<h1>" + uiText("OTA-Einstellungen ungültig",
                         "Invalid OTA settings") +
        "</h1><p>" + htmlEscape(otaSettingsError) +
        "</p><p><a href='/config'>" + uiText("Zurück", "Back") +
        "</a></p></body></html>");
    return;
  }
  if (wifiSettingsChanged || otaSettingsChanged) {
    webServer.send(200, "text/html; charset=utf-8",
        String("<!doctype html><html lang='") + (uiEnglish ? "en" : "de") +
        "'><meta name='viewport' content='width=device-width'>"
        "<body style='font:18px system-ui;max-width:600px;margin:40px auto'>" +
        "<h1>" + uiText("Einstellungen gespeichert", "Settings saved") +
        "</h1><p>" + uiText(
            "Der ESP startet neu, damit die Netzwerkdienste die Änderung übernehmen.",
            "The ESP restarts so the network services can apply the change.") +
        "</p></body></html>");
    return;
  }
  webServer.sendHeader("Location", "/config");
  webServer.send(303, "text/plain", uiText("Gespeichert", "Saved"));
}


String requestedRouteCacheCallsign() {
  String callsign = webServer.arg("callsign");
  callsign.trim();
  callsign.toUpperCase();
  if (callsign.isEmpty() || callsign.length() > 12) return "";
  for (size_t index = 0; index < callsign.length(); ++index) {
    if (!isAlphaNumeric(callsign[index])) return "";
  }
  return callsign;
}


void beginRouteCacheMutation() {
  routeCacheWritePreparing = true;
  vTaskDelay(pdMS_TO_TICKS(120));
  routeCacheWriteInProgress = true;
}


void endRouteCacheMutation() {
  routeCacheWriteInProgress = false;
  routeCacheWritePreparing = false;
}


void redirectToRouteCache() {
  webServer.sendHeader("Location", "/status#routeCache");
  webServer.send(303, "text/plain; charset=utf-8",
                 uiText("Routencache aktualisiert", "Route cache updated"));
}


void handleRouteCacheDelete() {
  const String callsign = requestedRouteCacheCallsign();
  if (callsign.isEmpty()) {
    webServer.send(400, "text/plain; charset=utf-8",
                   uiText("Ungültiges Callsign", "Invalid callsign"));
    return;
  }
  beginRouteCacheMutation();
  routeClient.removeCacheEntry(callsign);
  endRouteCacheMutation();
  if (routedFlight == callsign) {
    currentRoute = {};
    currentRoute.status = "not_found";
  }
  redirectToRouteCache();
}


void handleRouteCacheRefresh() {
  const String callsign = requestedRouteCacheCallsign();
  if (callsign.isEmpty()) {
    webServer.send(400, "text/plain; charset=utf-8",
                   uiText("Ungültiges Callsign", "Invalid callsign"));
    return;
  }
  if (routedFlight != callsign) {
    webServer.send(409, "text/plain; charset=utf-8",
                   uiText("Nur die aktuell beobachtete Route kann sofort neu geladen werden.",
                          "Only the currently observed route can be refreshed immediately."));
    return;
  }
  beginRouteCacheMutation();
  routeClient.removeCacheEntry(callsign);
  endRouteCacheMutation();
  routedFlight = "";
  currentRoute = {};
  lastRouteAttempt = 0;
  fetchLocalFlight();
  redirectToRouteCache();
}


void handleRouteCacheClear() {
  beginRouteCacheMutation();
  routeClient.clearCache();
  endRouteCacheMutation();
  redirectToRouteCache();
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
      htmlEscape(message) + "</p><p><a href='/config'>" + uiText("Zurück", "Back") +
      "</a></p></body></html>");
}


void handleEmergencyTest() {
  String code = webServer.arg("emergencyTestCode");
  if (!isEmergencySquawk(code)) code = "7700";
  emergencyTestCode = code;
  emergencyTestCount = constrain(webServer.arg("emergencyTestCount").toInt(), 1, 3);
  emergencyTestUntil = millis() + EMERGENCY_TEST_DURATION_MS;
  selectedEmergencyHex = "";
  emergencySelectionStarted = 0;
  fetchLocalFlight();
  webServer.sendHeader("Location", "/status");
  webServer.send(303, "text/plain; charset=utf-8", uiText("Notfalltest gestartet", "Emergency test started"));
}


void handleStopEmergencyTest() {
  emergencyTestCount = 0;
  emergencyTestCode = "";
  emergencyTestUntil = 0;
  selectedEmergencyHex = "";
  emergencySelectionStarted = 0;
  fetchLocalFlight();
  webServer.sendHeader("Location", "/status");
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
  routeClient.configure(true, testUrl, adsbDbFallbackEnabled);
  RouteResult result;
  {
    NetworkPhaseScope phase(NetworkPhase::Route);
    RouteContext routeContext;
    routeContext.hasPosition = true;
    routeContext.latitude = siteLatitude;
    routeContext.longitude = siteLongitude;
    result = routeClient.test(callsign, routeContext);
  }
  routeClient.configure(routesEnabled, routeUrl, adsbDbFallbackEnabled);
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
    if (result.source.length())
      detail += "<br>" + String(uiText("Quelle: ", "Source: ")) +
                "<b>" + htmlEscape(result.source) + "</b>";
  } else {
    detail = uiText("Dienst erreichbar, aber keine Route für dieses Callsign gefunden.",
                    "Service reachable, but no route found for this callsign.");
    if (result.source.length())
      detail += "<br>" + String(uiText("Letzte Quelle: ", "Last source: ")) +
                "<b>" + htmlEscape(result.source) + "</b>";
  }
  webServer.send(reachable ? 200 : 502, "text/html; charset=utf-8",
      String("<!doctype html><html lang='") + (uiEnglish ? "en" : "de") +
      "'><meta name='viewport' content='width=device-width'>"
      "<body style='font:18px system-ui;max-width:600px;margin:40px auto;padding:16px'>"
      "<h1>" + uiText("Routendienst-Test", "Route service test") + "</h1><p>" +
      uiText("Direkte Abfrage ohne Cache für ", "Direct query without cache for ") + "<b>" +
      htmlEscape(callsign) + "</b>.</p><p>" + detail +
      "</p><p><a href='/config'>" + uiText("Zurück", "Back") + "</a></p></body></html>");
}


void handleWebDisplay() {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "text/html; charset=utf-8", webDisplayPage(uiEnglish));
}


void handleDisplayApi() {
  FlightData shown;
#if !FLIGHTWALL_HAS_MATRIX
  static RadarPoint webRadarPoints[RADAR_POINT_LIMIT];
  size_t webRadarPointCount = 0;
#endif
  String error;
  uint32_t updated = 0;
  if (xSemaphoreTake(flightMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    shown = networkFlight;
#if !FLIGHTWALL_HAS_MATRIX
    webRadarPointCount = min(networkRadarPointCount, RADAR_POINT_LIMIT);
    for (size_t index = 0; index < webRadarPointCount; ++index)
      webRadarPoints[index] = networkRadarPoints[index];
    if (radarTrailStorage) radarTrailStorage->publish(webRadarPointCount);
#endif
    error = networkLastError;
    updated = networkLastSuccessfulPoll;
    xSemaphoreGive(flightMutex);
  }
  String json;
  json.reserve(FLIGHTWALL_HAS_MATRIX ? 4200 : 24000);
  json = "{\"language\":\"" + String(uiEnglish ? "en" : "de") +
         "\",\"device_variant\":\"" + jsonEscape(DEVICE_VARIANT) +
         "\",\"web_radar\":" +
             String(FLIGHTWALL_HAS_MATRIX ? "false" : "true") +
         ",\"age_seconds\":" +
         (updated ? String((millis() - updated) / 1000) : "null") +
         ",\"error\":\"" + jsonEscape(error) + "\"" +
         ",\"custom_text\":\"" + jsonEscape(customPageText) + "\"" +
         ",\"custom_scroll_speed\":\"" + customScrollSpeedName() + "\"" +
         ",\"radar_sweep_interval_seconds\":" +
             String(radarSweepIntervalSeconds) +
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
  json += "],\"radar_trails_enabled\":" + String(radarTrailsEnabled ? "true" : "false") +
          ",\"radar_trails_available\":" + String(radarTrailStorage ? "true" : "false") +
          ",\"radar\":{\"radius_km\":" + String(displayRadiusKm, 1) +
          ",\"view_heading\":" + String(viewHeading, 1) +
          ",\"points\":[";
#if !FLIGHTWALL_HAS_MATRIX
  for (size_t index = 0; index < webRadarPointCount; ++index) {
    if (index) json += ',';
    json += '[';
    json += String(webRadarPoints[index].distanceKm, 2) + ',' +
            String(webRadarPoints[index].bearing, 1) + ',' +
            String(webRadarPoints[index].flags);
    json += ",[";
    bool firstDot = true;
    const auto *trail = radarTrailStorage ? &radarTrailStorage->consumer[index] : nullptr;
    if (radarTrailsEnabled && trail) for (uint8_t j = 0; j < trail->trailCount; ++j) {
      const auto &dot = trail->trail[j];
      const uint32_t age = millis() - dot.time;
      if (age >= RadarTrails::LifetimeMs) continue;
      if (!firstDot) json += ',';
      firstDot = false;
      json += '[' + String(dot.x) + ',' + String(dot.y) + ',' +
              String(RadarTrails::LifetimeMs - age) + ']';
    }
    json += "]]";
  }
#endif
  json += "]},\"flight\":{";
  json += "\"available\":" + String(shown.available ? "true" : "false") +
          ",\"emergency\":" + String(shown.emergency ? "true" : "false") +
          ",\"has_position\":" + String(shown.hasPosition ? "true" : "false") +
          ",\"on_ground\":" + String(shown.onGround ? "true" : "false") +
          ",\"hex\":\"" + jsonEscape(shown.hex) + "\"" +
          ",\"flight\":\"" + jsonEscape(shown.flight) + "\"" +
          ",\"squawk\":\"" + jsonEscape(shown.squawk) + "\"" +
          ",\"emergency_meaning\":\"" +
              jsonEscape(shown.emergency ? emergencyMeaning(shown.squawk) : "") + "\"" +
          ",\"emergency_count\":" + String(shown.emergencyCount) +
          ",\"emergency_index\":" + String(shown.emergencyIndex) +
          ",\"airline\":\"" + jsonEscape(shown.airline) + "\"" +
          ",\"aircraft_type\":\"" + jsonEscape(shown.type) + "\"" +
          ",\"aircraft_type_description\":\"" +
              jsonEscape(shown.typeDescription) + "\"" +
          ",\"special_type\":\"" + jsonEscape(shown.specialType) + "\"" +
          ",\"direction\":\"" +
              jsonEscape(localizedCompassToken(shown.direction)) + "\"" +
          ",\"vertical_motion\":\"" + jsonEscape(shown.verticalMotion) + "\"" +
          ",\"motion\":\"" + jsonEscape(shown.motion) + "\"" +
          ",\"selection_reason\":\"" +
              jsonEscape(shown.selectionReason) + "\"" +
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
          ",\"cpa_seconds\":" +
              (cpaCountdownSeconds(shown) >= 0
                   ? String(cpaCountdownSeconds(shown)) : "null") +
          ",\"vertical_rate_fpm\":" +
              jsonNumberOrNull(shown.verticalRateFpm, 0) +
          ",\"bearing\":" + jsonNumberOrNull(shown.bearing, 0) +
          ",\"relative_bearing\":" +
              jsonNumberOrNull(shown.relativeBearing) + "}}";
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.send(200, "application/json; charset=utf-8", json);
}


void handleStatusApi() {
  const RouteMemorySnapshot statusMemory = routeMemorySnapshot();
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
    if (isfinite(windDirectionDegrees))
      statusWindDirection = weatherCompassDirection(windDirectionDegrees);
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
      "\",\"device_name\":\"" + jsonEscape(DEVICE_NAME) +
      "\",\"device_variant\":\"" + jsonEscape(DEVICE_VARIANT) +
      "\",\"matrix_available\":" +
          String(FLIGHTWALL_HAS_MATRIX ? "true" : "false") +
      ",\"status_led_available\":" +
          String("true") +
      ",\"status_led_brightness\":" + String(statusLedBrightnessMode) +
      ",\"status_led_state\":\"" +
          jsonEscape(statusLedStateName(statusLedState())) + "\"" +
      ",\"ota_configured\":" +
          String(otaPassword.length() >= 8 ? "true" : "false") +
      ",\"language\":\"" + String(uiEnglish ? "en" : "de") +
      "\",\"build\":\"" + jsonEscape(BUILD_TIMESTAMP) +
      "\",\"uptime_seconds\":" + String(millis() / 1000) +
      ",\"free_heap\":" + String(ESP.getFreeHeap()) +
      ",\"chip_temperature_c\":" + jsonNumberOrNull(chipTemperatureC()) +
      ",\"bme280_available\":" + String(environmentSensorAvailable ? "true" : "false") +
      ",\"environment_idle_display_enabled\":" +
          String(environmentIdleDisplayEnabled ? "true" : "false") +
      ",\"ambient_temperature_c\":" + jsonNumberOrNull(ambientTemperatureC) +
      ",\"relative_humidity_percent\":" + jsonNumberOrNull(relativeHumidityPercent) +
      ",\"dew_point_c\":" + jsonNumberOrNull(dewPointC) +
      ",\"mold_critical_surface_temperature_c\":" +
          jsonNumberOrNull(moldCriticalSurfaceTemperatureC) +
      ",\"mold_risk\":\"" + String(moldRiskLevel == MoldRiskLevel::High
          ? "high" : moldRiskLevel == MoldRiskLevel::Elevated
          ? "elevated" : moldRiskLevel == MoldRiskLevel::Normal
          ? "low" : "unavailable") + "\"" +
      ",\"elevated_humidity_duration_seconds\":" +
          (elevatedHumiditySince
               ? String((millis() - elevatedHumiditySince) / 1000UL)
               : "0") +
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
      ",\"wifi_ssid\":\"" + jsonEscape(wifiSsid) + "\"" +
      ",\"wifi_connected_ssid\":\"" +
          jsonEscape(WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String("")) +
          "\"" +
      ",\"wifi_connected\":" +
          String(WiFi.status() == WL_CONNECTED ? "true" : "false") +
      ",\"hostname\":\"" + String(DEVICE_HOSTNAME) + ".local\"" +
      ",\"local_ip\":\"" + currentNetworkIpText() + "\"" +
      ",\"gateway_ip\":\"" +
          (WiFi.status() == WL_CONNECTED ? WiFi.gatewayIP().toString() : "") +
          "\"" +
      ",\"subnet_mask\":\"" +
          (WiFi.status() == WL_CONNECTED ? WiFi.subnetMask().toString() : "") +
          "\"" +
      ",\"dns_server\":\"" +
          (WiFi.status() == WL_CONNECTED ? WiFi.dnsIP(0).toString() : "") +
          "\"" +
      ",\"wifi_channel\":" +
          (WiFi.status() == WL_CONNECTED ? String(WiFi.channel()) : "null") +
      ",\"mac_address\":\"" + WiFi.macAddress() + "\"" +
      ",\"wifi_signal_quality\":\"" +
          jsonEscape(wifiSignalQualityText()) + "\"" +
      ",\"wifi_setup_ap_active\":" +
          String(wifiSetupAccessPointActive ? "true" : "false") +
      ",\"wifi_setup_ap_ssid\":\"" +
          jsonEscape(wifiSetupAccessPointSsid) + "\"" +
      ",\"wifi_setup_ap_ip\":\"" +
          (wifiSetupAccessPointActive ? WiFi.softAPIP().toString() : "") +
          "\"" +
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
      ",\"far_elevation_angle_deg\":" + String(farDirectionElevationDeg, 0) +
      ",\"adsb_url\":\"" + jsonEscape(activeAdsbUrl()) + "\"" +
      ",\"adsb_url_1\":\"" + jsonEscape(adsbUrl1) + "\"" +
      ",\"adsb_url_2\":\"" + jsonEscape(adsbUrl2) + "\"" +
      ",\"adsb_url_3\":\"" + jsonEscape(adsbFiUrl()) + "\"" +
      ",\"adsb_source_name\":\"" + jsonEscape(activeAdsbSourceName()) + "\"" +
      ",\"active_adsb_source\":" + String(activeAdsbSource + 1) +
      ",\"current_adsb_source\":" + String(currentAdsbSource + 1) +
      ",\"adsb_failover_enabled\":" +
          String(automaticAdsbFailover ? "true" : "false") +
      ",\"adsb_failover_active\":" +
          String(currentAdsbSource != activeAdsbSource ? "true" : "false") +
      ",\"site_latitude\":" + String(siteLatitude, 6) +
      ",\"site_longitude\":" + String(siteLongitude, 6) +
      ",\"view_heading\":" + String(viewHeading, 0) +
      ",\"display_radius_km\":" + String(displayRadiusKm, 1) +
      ",\"radar_sweep_interval_seconds\":" +
          String(radarSweepIntervalSeconds) +
      ",\"effective_radius_km\":" + jsonNumberOrNull(effectiveRadiusKm) +
      ",\"routes_enabled\":" + String(routesEnabled ? "true" : "false") +
      ",\"adsbdb_route_fallback_enabled\":" +
          String(adsbDbFallbackEnabled ? "true" : "false") +
      ",\"hide_sport_aircraft\":" + String(hideSportAircraft ? "true" : "false") +
      ",\"altitude_filter_enabled\":" + String(altitudeFilterEnabled ? "true" : "false") +
      ",\"maximum_altitude_ft\":" + String(maximumAltitudeFt) +
      ",\"received_aircraft\":" + String(lastReceivedAircraft) +
      ",\"candidate_aircraft\":" + String(lastCandidateAircraft) +
      ",\"radar_trails_enabled\":" + String(radarTrailsEnabled ? "true" : "false") +
          ",\"radar_trails_available\":" + String(radarTrailStorage ? "true" : "false") +
      ",\"radar_aircraft\":" + String(networkRadarPointCount) +
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
      ",\"route_source\":\"" + jsonEscape(currentRoute.source) + "\"" +
      ",\"route_confidence\":" + String(currentRoute.confidence) +
      ",\"route_confidence_reason\":\"" +
          jsonEscape(currentRoute.confidenceReason) + "\"" +
      ",\"route_cache_write_pending\":" +
          String(routeClient.hasPendingWrites() ? "true" : "false") +
      ",\"flight_available\":" + String(statusFlight.available ? "true" : "false") +
      ",\"emergency\":" + String(statusFlight.emergency ? "true" : "false") +
      ",\"has_position\":" + String(statusFlight.hasPosition ? "true" : "false") +
      ",\"hex\":\"" + jsonEscape(statusFlight.hex) + "\"" +
      ",\"flight\":\"" + jsonEscape(statusFlight.flight) + "\"" +
      ",\"on_ground\":" + String(statusFlight.onGround ? "true" : "false") +
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
      ",\"aircraft_type_description\":\"" +
          jsonEscape(statusFlight.typeDescription) + "\"" +
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
      ",\"cpa_seconds\":" +
          (cpaCountdownSeconds(statusFlight) >= 0
               ? String(cpaCountdownSeconds(statusFlight)) : "null") +
      ",\"vertical_rate_fpm\":" +
          jsonNumberOrNull(statusFlight.verticalRateFpm, 0) +
      ",\"vertical_motion\":\"" +
          jsonEscape(statusFlight.verticalMotion) + "\"" +
      ",\"motion\":\"" + jsonEscape(statusFlight.motion) + "\"" +
      ",\"selection_reason\":\"" +
          jsonEscape(statusFlight.selectionReason) + "\"" +
      ",\"bearing\":" + jsonNumberOrNull(statusFlight.bearing, 0) +
      ",\"relative_bearing\":" +
          jsonNumberOrNull(statusFlight.relativeBearing) +
      ",\"daily_aircraft_statistics\":{\"unique_aircraft\":" +
          String(seenAircraftCount) +
          ",\"tracking_limit\":" +
          String(OBSERVED_AIRCRAFT_TRACKING_LIMIT) +
          ",\"tracking_limit_reached\":" +
          String(aircraftTrackingLimitReached ? "true" : "false") +
          ",\"period_started_epoch\":" +
          (dailyStatisticsStarted
               ? String(static_cast<uint32_t>(dailyStatisticsStarted))
               : "null") + "}" +
      ",\"special_statistics\":{\"total\":" +
          String(seenSpecialAircraftCount) +
          ",\"tracking_limit\":" + String(SPECIAL_AIRCRAFT_TRACKING_LIMIT) +
          ",\"tracking_limit_reached\":" +
          String(specialAircraftTrackingLimitReached ? "true" : "false") +
          ",\"categories\":{\"transponder_7500\":" +
          String(specialAircraftCount(SpecialStatisticCategory::Emergency7500)) +
          ",\"transponder_7600\":" +
          String(specialAircraftCount(SpecialStatisticCategory::Emergency7600)) +
          ",\"transponder_7700\":" +
          String(specialAircraftCount(SpecialStatisticCategory::Emergency7700)) +
          ",\"german_airforce\":" +
          String(specialAircraftCount(SpecialStatisticCategory::GermanAirforce)) +
          ",\"military\":" +
          String(specialAircraftCount(SpecialStatisticCategory::Military)) +
          ",\"federal_police\":" +
          String(specialAircraftCount(SpecialStatisticCategory::FederalPolice)) +
          ",\"police\":" +
          String(specialAircraftCount(SpecialStatisticCategory::Police)) +
          ",\"government\":" +
          String(specialAircraftCount(SpecialStatisticCategory::Government)) +
          ",\"helicopter\":" +
          String(specialAircraftCount(SpecialStatisticCategory::Helicopter)) +
          ",\"airship\":" +
          String(specialAircraftCount(SpecialStatisticCategory::Airship)) +
          ",\"a380\":" +
          String(specialAircraftCount(SpecialStatisticCategory::A380)) +
          ",\"other\":" +
          String(specialAircraftCount(SpecialStatisticCategory::Other)) + "}}" +
      ",\"daily_journal\":{\"entries\":" + String(dailyJournalCount) +
          ",\"limit\":" + String(DAILY_JOURNAL_LIMIT) +
          ",\"limit_reached\":" +
          String(dailyJournalLimitReached ? "true" : "false") + "}" +
      ",\"special_chronicle\":{\"entries\":" +
          String(specialChronicleCount) +
          ",\"limit\":" + String(SPECIAL_CHRONICLE_LIMIT) +
          ",\"limit_reached\":" +
          String(specialChronicleLimitReached ? "true" : "false") + "}" +
      ",\"unknown_airports\":" + unknownCodesJson(true) +
      ",\"unknown_airlines\":" + unknownCodesJson(false) +
      ",\"unknown_code_limit\":" + String(unknownCodeLimit) +
      ",\"dropped_unknown_airports\":" +
          String(droppedUnknownAirports()) +
      ",\"dropped_unknown_airlines\":" +
          String(droppedUnknownAirlines()) +
      ",\"quiet_hours_enabled\":" + String(quietHoursEnabled ? "true" : "false") +
      ",\"quiet_hours_active\":" + String(quietHoursAreActive() ? "true" : "false") +
      ",\"display_test\":" + String(displayTestActive ? "true" : "false") +
      ",\"reset_reason\":\"" + jsonEscape(resetReasonText()) + "\"" +
      ",\"previous_watchdog_network_phase\":\"" +
          jsonEscape(networkPhaseName(previousResetNetworkPhase)) + "\"" +
      ",\"network_phase_max_ms\":{" +
          "\"web\":" + String(networkPhaseMaxMs[static_cast<size_t>(NetworkPhase::Web)]) +
          ",\"ota\":" + String(networkPhaseMaxMs[static_cast<size_t>(NetworkPhase::Ota)]) +
          ",\"mqtt\":" + String(networkPhaseMaxMs[static_cast<size_t>(NetworkPhase::Mqtt)]) +
          ",\"wifi\":" + String(networkPhaseMaxMs[static_cast<size_t>(NetworkPhase::Wifi)]) +
          ",\"adsb\":" + String(networkPhaseMaxMs[static_cast<size_t>(NetworkPhase::Adsb)]) +
          ",\"route\":" + String(networkPhaseMaxMs[static_cast<size_t>(NetworkPhase::Route)]) +
          ",\"flash\":" + String(networkPhaseMaxMs[static_cast<size_t>(NetworkPhase::Flash)]) + "}}";
  // Append without building a second full JSON document or large stack copy.
  json.remove(json.length() - 1);
  json += ",\"radar_trails_storage_bytes\":" + String(radarTrailStorage ? sizeof(RadarTrailStorage) : 0) +
          ",\"free_psram_bytes\":" + String(ESP.getFreePsram());
  json += ",\"persistent_text_memory\":";
  appendPersistentTextMemoryJson(json);
  json += ",\"route_memory\":";
  appendRouteMemoryJson(json, statusMemory);
  json += ",\"route_diagnostics\":{\"primary\":";
  appendRouteRequestJson(json, routeClient.diagnostics().primary);
  json += ",\"fallback\":";
  appendRouteRequestJson(json, routeClient.diagnostics().fallback);
  json += ",\"last_failure\":";
  appendRouteRequestJson(json, routeClient.diagnostics().lastFailure);
  json += "}}";
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.sendHeader("Connection", "close");
  webServer.send(200, "application/json; charset=utf-8", json);
}


void handleUnknownCodesDownload() {
  String json;
  json.reserve(512);
  json = "{\n  \"firmware_version\":\"" + jsonEscape(APP_VERSION) +
         "\",\n  \"uptime_seconds\":" + String(millis() / 1000UL) +
         ",\n  \"capacity_per_type\":" + String(unknownCodeLimit) +
         ",\n  \"unknown_airports\":" + unknownCodesJson(true) +
         ",\n  \"unknown_airlines\":" + unknownCodesJson(false) +
         ",\n  \"dropped_unknown_airports\":" +
         String(droppedUnknownAirports()) +
         ",\n  \"dropped_unknown_airlines\":" +
         String(droppedUnknownAirlines()) + "\n}\n";
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.sendHeader(
      "Content-Disposition",
      "attachment; filename=\"flightwall-unknown-codes.json\"");
  webServer.sendHeader("Connection", "close");
  webServer.send(200, "application/json; charset=utf-8", json);
}


String csvEscape(String value) {
  value.replace("\"", "\"\"");
  value.replace("\r", " ");
  value.replace("\n", " ");
  return "\"" + value + "\"";
}


void flushDownloadChunk(String &chunk) {
  if (!chunk.length()) return;
  webServer.sendContent(chunk);
  chunk = "";
}


void handleJournalJsonDownload() {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.sendHeader(
      "Content-Disposition",
      "attachment; filename=\"flightwall-daily-journal.json\"");
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.send(200, "application/json; charset=utf-8", "");

  String chunk;
  chunk.reserve(4600);
  chunk = "{\n  \"firmware_version\":\"" + jsonEscape(APP_VERSION) +
          "\",\n  \"period_started\":\"" +
          jsonEscape(journalTimeText(dailyStatisticsStarted, true)) +
          "\",\n  \"entries_count\":" + String(dailyJournalCount) +
          ",\n  \"limit\":" + String(DAILY_JOURNAL_LIMIT) +
          ",\n  \"limit_reached\":" +
          String(dailyJournalLimitReached ? "true" : "false") +
          ",\n  \"entries\":[";
  for (size_t index = 0; index < dailyJournalCount; ++index) {
    const DailyJournalEntry &entry = dailyJournal[index];
    if (index) chunk += ',';
    chunk += "\n    {\"first_seen\":\"" +
             jsonEscape(journalTimeText(entry.firstSeen, true)) +
             "\",\"last_seen\":\"" +
             jsonEscape(journalTimeText(entry.lastSeen, true)) +
             "\",\"hex\":\"" + jsonEscape(entry.hex) +
             "\",\"flight\":\"" + jsonEscape(entry.flight) +
             "\",\"airline\":\"" + jsonEscape(entry.airline) +
             "\",\"type\":\"" + jsonEscape(entry.type) +
             "\",\"type_description\":\"" +
             jsonEscape(entry.typeDescription) +
             "\",\"route_codes\":\"" + jsonEscape(entry.routeCodes) +
             "\",\"route_names\":\"" + jsonEscape(entry.routeNames) +
             "\",\"classification\":\"" +
             jsonEscape(journalClassification(entry)) +
             "\",\"selection_reason\":\"" +
             jsonEscape(entry.selectionReason) +
             "\",\"squawk\":\"" + jsonEscape(entry.squawk) +
             "\",\"minimum_distance_km\":" +
             jsonNumberOrNull(entry.minimumDistanceKm) +
             ",\"minimum_cpa_km\":" + jsonNumberOrNull(entry.minimumCpaKm) +
             ",\"last_altitude_ft\":" +
             jsonNumberOrNull(entry.altitudeFt, 0) + "}";
    if (chunk.length() >= 4096) flushDownloadChunk(chunk);
  }
  chunk += "\n  ]\n}\n";
  flushDownloadChunk(chunk);
  webServer.sendContent("");
}


void handleJournalCsvDownload() {
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.sendHeader(
      "Content-Disposition",
      "attachment; filename=\"flightwall-daily-journal.csv\"");
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.send(200, "text/csv; charset=utf-8", "");

  String chunk;
  chunk.reserve(4600);
  chunk = "First seen,Last seen,Hex,Flight,Airline,Type,Type description,Route codes,Route names,Classification,Selection reason,Squawk,Minimum distance km,Minimum CPA km,Last altitude ft\r\n";
  for (size_t index = 0; index < dailyJournalCount; ++index) {
    const DailyJournalEntry &entry = dailyJournal[index];
    chunk += csvEscape(journalTimeText(entry.firstSeen, true)) + ',' +
             csvEscape(journalTimeText(entry.lastSeen, true)) + ',' +
             csvEscape(entry.hex) + ',' + csvEscape(entry.flight) + ',' +
             csvEscape(entry.airline) + ',' + csvEscape(entry.type) + ',' +
             csvEscape(entry.typeDescription) + ',' +
             csvEscape(entry.routeCodes) + ',' +
             csvEscape(entry.routeNames) + ',' +
             csvEscape(journalClassification(entry)) + ',' +
             csvEscape(entry.selectionReason) + ',' +
             csvEscape(entry.squawk) + ',' +
             numberOrDash(entry.minimumDistanceKm) + ',' +
             numberOrDash(entry.minimumCpaKm) + ',' +
             numberOrDash(entry.altitudeFt, 0) + "\r\n";
    if (chunk.length() >= 4096) flushDownloadChunk(chunk);
  }
  flushDownloadChunk(chunk);
  webServer.sendContent("");
}


void handleSpecialsJsonDownload() {
  String json;
  json.reserve(18000);
  json = "{\n  \"firmware_version\":\"" + jsonEscape(APP_VERSION) +
         "\",\n  \"period_started\":\"" +
         jsonEscape(journalTimeText(dailyStatisticsStarted, true)) +
         "\",\n  \"entries_count\":" + String(specialChronicleCount) +
         ",\n  \"limit\":" + String(SPECIAL_CHRONICLE_LIMIT) +
         ",\n  \"limit_reached\":" +
         String(specialChronicleLimitReached ? "true" : "false") +
         ",\n  \"entries\":[";
  for (size_t index = 0; index < specialChronicleCount; ++index) {
    const SpecialChronicleEntry &entry = specialChronicle[index];
    if (index) json += ',';
    json += "\n    {\"first_seen\":\"" +
            jsonEscape(journalTimeText(entry.firstSeen, true)) +
            "\",\"last_seen\":\"" +
            jsonEscape(journalTimeText(entry.lastSeen, true)) +
            "\",\"hex\":\"" + jsonEscape(entry.hex) +
            "\",\"flight\":\"" + jsonEscape(entry.flight) +
            "\",\"type\":\"" + jsonEscape(entry.type) +
            "\",\"type_description\":\"" +
            jsonEscape(entry.typeDescription) +
            "\",\"classification\":\"" + jsonEscape(entry.specialType) +
            "\",\"squawk\":\"" + jsonEscape(entry.squawk) +
            "\",\"route_codes\":\"" + jsonEscape(entry.routeCodes) +
            "\",\"displayed\":" + String(entry.displayed ? "true" : "false") +
            ",\"selection_reason\":\"" +
            jsonEscape(entry.selectionReason) +
            "\",\"minimum_distance_km\":" +
            jsonNumberOrNull(entry.minimumDistanceKm) +
            ",\"minimum_cpa_km\":" + jsonNumberOrNull(entry.minimumCpaKm) +
            "}";
  }
  json += "\n  ]\n}\n";
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.sendHeader(
      "Content-Disposition",
      "attachment; filename=\"flightwall-special-chronicle.json\"");
  webServer.sendHeader("Connection", "close");
  webServer.send(200, "application/json; charset=utf-8", json);
}


void handleSpecialsCsvDownload() {
  String csv;
  csv.reserve(14000);
  csv = "First seen,Last seen,Hex,Flight,Type,Type description,Classification,Squawk,Route codes,Displayed,Selection reason,Minimum distance km,Minimum CPA km\r\n";
  for (size_t index = 0; index < specialChronicleCount; ++index) {
    const SpecialChronicleEntry &entry = specialChronicle[index];
    csv += csvEscape(journalTimeText(entry.firstSeen, true)) + ',' +
           csvEscape(journalTimeText(entry.lastSeen, true)) + ',' +
           csvEscape(entry.hex) + ',' + csvEscape(entry.flight) + ',' +
           csvEscape(entry.type) + ',' + csvEscape(entry.typeDescription) + ',' +
           csvEscape(entry.specialType) + ',' + csvEscape(entry.squawk) + ',' +
           csvEscape(entry.routeCodes) + ',' +
           csvEscape(entry.displayed ? "yes" : "no") + ',' +
           csvEscape(entry.selectionReason) + ',' +
           numberOrDash(entry.minimumDistanceKm) + ',' +
           numberOrDash(entry.minimumCpaKm) + "\r\n";
  }
  webServer.sendHeader("Cache-Control", "no-store");
  webServer.sendHeader(
      "Content-Disposition",
      "attachment; filename=\"flightwall-special-chronicle.csv\"");
  webServer.sendHeader("Connection", "close");
  webServer.send(200, "text/csv; charset=utf-8", csv);
}


const char *wifiSecurityName(wifi_auth_mode_t mode) {
  switch (mode) {
    case WIFI_AUTH_OPEN: return "OPEN";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 Enterprise";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    default: return "SECURED";
  }
}


void handleWifiScanStart() {
  if (otaPreparing || otaInProgress) {
    webServer.send(409, "application/json; charset=utf-8",
                   "{\"state\":\"blocked\",\"message\":\"OTA läuft\"}");
    return;
  }
  if (WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
    webServer.send(202, "application/json; charset=utf-8",
                   "{\"state\":\"running\"}");
    return;
  }
  WiFi.scanDelete();
  wifiScanResultJson = "";
  wifiScanRequested = true;
  wifiScanStartedAt = millis();
  const int result = WiFi.scanNetworks(true, false, false, 160);
  if (result == WIFI_SCAN_FAILED) {
    wifiScanRequested = false;
    webServer.send(500, "application/json; charset=utf-8",
                   "{\"state\":\"failed\"}");
    return;
  }
  webServer.send(202, "application/json; charset=utf-8",
                 "{\"state\":\"running\"}");
}


void handleWifiScanResult() {
  if (!wifiScanRequested) {
    if (wifiScanResultJson.length()) {
      webServer.sendHeader("Cache-Control", "no-store");
      webServer.send(200, "application/json; charset=utf-8",
                     wifiScanResultJson);
    } else {
      webServer.send(200, "application/json; charset=utf-8",
                     "{\"state\":\"idle\",\"networks\":[]}");
    }
    return;
  }

  const int count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    if (millis() - wifiScanStartedAt > 15000UL) {
      WiFi.scanDelete();
      wifiScanRequested = false;
      webServer.send(500, "application/json; charset=utf-8",
                     "{\"state\":\"failed\",\"networks\":[]}");
    } else {
      webServer.send(202, "application/json; charset=utf-8",
                     "{\"state\":\"running\"}");
    }
    return;
  }
  if (count == WIFI_SCAN_FAILED) {
    wifiScanRequested = false;
    webServer.send(500, "application/json; charset=utf-8",
                   "{\"state\":\"failed\",\"networks\":[]}");
    return;
  }

  struct WifiScanEntry {
    String ssid;
    int32_t rssi;
    wifi_auth_mode_t security;
  };
  std::vector<WifiScanEntry> networks;
  networks.reserve(min(count, 32));
  for (int i = 0; i < count; ++i) {
    const String ssid = WiFi.SSID(i);
    if (!ssid.length()) continue;
    const int32_t rssi = WiFi.RSSI(i);
    bool merged = false;
    for (WifiScanEntry &entry : networks) {
      if (entry.ssid == ssid) {
        if (rssi > entry.rssi) {
          entry.rssi = rssi;
          entry.security = WiFi.encryptionType(i);
        }
        merged = true;
        break;
      }
    }
    if (!merged && networks.size() < 32) {
      networks.push_back({ssid, rssi, WiFi.encryptionType(i)});
    }
  }
  for (size_t i = 0; i < networks.size(); ++i) {
    for (size_t j = i + 1; j < networks.size(); ++j) {
      if (networks[j].rssi > networks[i].rssi) {
        const WifiScanEntry temporary = networks[i];
        networks[i] = networks[j];
        networks[j] = temporary;
      }
    }
  }

  String json = "{\"state\":\"ready\",\"networks\":[";
  json.reserve(256 + networks.size() * 80);
  for (size_t i = 0; i < networks.size(); ++i) {
    if (i) json += ',';
    json += "{\"ssid\":\"" + jsonEscape(networks[i].ssid) +
            "\",\"rssi\":" + String(networks[i].rssi) +
            ",\"security\":\"" + wifiSecurityName(networks[i].security) +
            "\",\"open\":" +
            String(networks[i].security == WIFI_AUTH_OPEN ? "true" : "false") +
            ",\"current\":" +
            String(WiFi.status() == WL_CONNECTED &&
                           networks[i].ssid == WiFi.SSID()
                       ? "true" : "false") + "}";
  }
  json += "]}";
  WiFi.scanDelete();
  wifiScanRequested = false;
  wifiScanResultJson = json;
  webServer.sendHeader("Cache-Control", "no-store");
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
  webServer.sendHeader("Location", "/config");
  webServer.send(303, "text/plain; charset=utf-8", uiText("Displaytest umgeschaltet", "Display test toggled"));
}


bool otaAuthenticated() {
  if (otaPassword.length() < 8) {
    webServer.send(503, "text/plain; charset=utf-8", uiText("OTA ist nicht konfiguriert.", "OTA is not configured."));
    return false;
  }
  if (!webServer.authenticate("admin", otaPassword.c_str())) {
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
<p>Hardwarevariante: <b>%VARIANT%</b></p>
<p>Nur die zu dieser Hardwarevariante gehörende PlatformIO-Datei <b>firmware.bin</b> auswählen. Während des Updates die Stromversorgung nicht trennen.</p>
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
  html.replace("%VARIANT%", DEVICE_VARIANT);
  translateWebPageToEnglish(html);
  webServer.send(200, "text/html; charset=utf-8", html);
}


void handleOtaUpload() {
  if (otaPassword.length() < 8 ||
      !webServer.authenticate("admin", otaPassword.c_str())) return;
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
  if (otaStarted || otaPassword.length() < 8) return;
  ArduinoOTA.setHostname(DEVICE_HOSTNAME);
  ArduinoOTA.setPassword(otaPassword.c_str());
  ArduinoOTA.onStart([]() {
    otaPreparing = true;
    vTaskDelay(pdMS_TO_TICKS(900));
    otaPreparing = false;
    otaInProgress = true;
    setStatusLed(LedStatus::Updating);
    vTaskDelay(pdMS_TO_TICKS(100));
    Serial.println("PlatformIO OTA start");
    setNetworkError("OTA UPDATE");
  });
  ArduinoOTA.onEnd([]() { Serial.println("PlatformIO OTA complete"); });
  ArduinoOTA.onError([](ota_error_t error) {
    otaPreparing = false;
    otaInProgress = false;
    statusLedErrorUntil = millis() + 10000;
    setStatusLed(LedStatus::Error);
    Serial.printf("PlatformIO OTA error: %u\n", error);
    setNetworkError("OTA FEHLER");
  });
  ArduinoOTA.begin();
  otaStarted = true;
  Serial.printf("PlatformIO OTA enabled for %s.local\n", DEVICE_HOSTNAME);
}


bool parseMqttNumber(const String &payload, float &value) {
  char *end = nullptr;
  const float parsed = strtof(payload.c_str(), &end);
  while (end && *end && isspace(static_cast<unsigned char>(*end))) ++end;
  if (end == payload.c_str() || (end && *end) || !isfinite(parsed)) return false;
  value = parsed;
  return true;
}


bool parseMqttCompassDirection(String value, float &degrees) {
  static const char *directions[] = {
      "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
      "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  value.trim();
  value.toUpperCase();
  // MQTT may provide either German (O/ONO/OSO) or international English
  // (E/ENE/ESE) compass tokens. Store one language-neutral angle and only
  // localize it when it is rendered.
  value.replace("O", "E");
  for (uint8_t index = 0; index < 16; ++index) {
    if (value == directions[index]) {
      degrees = index * 22.5f;
      return true;
    }
  }
  return false;
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
    if (parseMqttNumber(value, degrees) ||
        parseMqttCompassDirection(value, degrees)) {
      while (degrees < 0.0f) degrees += 360.0f;
      while (degrees >= 360.0f) degrees -= 360.0f;
      windDirectionDegrees = degrees;
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
    webServer.on("/", HTTP_GET, []() {
      pulseHttpActivity();
      webServer.sendHeader("Location",
                           wifiSetupAccessPointActive ? "/config" : "/status",
                           true);
      webServer.send(302, "text/plain; charset=utf-8", "FlightWall");
    });
    webServer.on("/status", HTTP_GET, []() {
      pulseHttpActivity();
      webServer.sendHeader("Cache-Control", "no-store");
      webServer.send(200, "text/html; charset=utf-8", webPage(false));
    });
    webServer.on("/config", HTTP_GET, []() {
      pulseHttpActivity();
      webServer.sendHeader("Cache-Control", "no-store");
      webServer.send(200, "text/html; charset=utf-8", webPage(true, false));
    });
    webServer.on("/display", HTTP_GET, []() {
      pulseHttpActivity();
      handleWebDisplay();
    });
    webServer.on("/api/display", HTTP_GET, handleDisplayApi);
    webServer.on("/api/status", HTTP_GET, handleStatusApi);
    webServer.on("/api/status-fragment", HTTP_GET, handleStatusFragment);
    webServer.on("/api/wifi-scan", HTTP_POST, []() {
      pulseHttpActivity(); handleWifiScanStart();
    });
    webServer.on("/api/wifi-scan", HTTP_GET, handleWifiScanResult);
    webServer.on("/unknown-codes.json", HTTP_GET, []() {
      pulseHttpActivity(); handleUnknownCodesDownload();
    });
    webServer.on("/configuration.json", HTTP_GET, []() {
      pulseHttpActivity(); handleConfigurationDownload();
    });
    webServer.on("/configuration/restore", HTTP_POST, []() {
      pulseHttpActivity(); handleConfigurationRestore();
    });
    webServer.on("/journal.json", HTTP_GET, []() {
      pulseHttpActivity(); handleJournalJsonDownload();
    });
    webServer.on("/journal.csv", HTTP_GET, []() {
      pulseHttpActivity(); handleJournalCsvDownload();
    });
    webServer.on("/specials.json", HTTP_GET, []() {
      pulseHttpActivity(); handleSpecialsJsonDownload();
    });
    webServer.on("/specials.csv", HTTP_GET, []() {
      pulseHttpActivity(); handleSpecialsCsvDownload();
    });
    webServer.on("/save", HTTP_POST, []() {
      pulseHttpActivity(); handleWebSave();
    });
    webServer.on("/test-adsb", HTTP_POST, []() {
      pulseHttpActivity(); handleAdsbTest();
    });
    webServer.on("/test-route", HTTP_POST, []() {
      pulseHttpActivity(); handleRouteTest();
    });
    webServer.on("/route-cache/delete", HTTP_POST, []() {
      pulseHttpActivity(); handleRouteCacheDelete();
    });
    webServer.on("/route-cache/refresh", HTTP_POST, []() {
      pulseHttpActivity(); handleRouteCacheRefresh();
    });
    webServer.on("/route-cache/clear", HTTP_POST, []() {
      pulseHttpActivity(); handleRouteCacheClear();
    });
    webServer.on("/test-emergency", HTTP_POST, []() {
      pulseHttpActivity(); handleEmergencyTest();
    });
    webServer.on("/stop-emergency-test", HTTP_POST, []() {
      pulseHttpActivity(); handleStopEmergencyTest();
    });
#if FLIGHTWALL_HAS_MATRIX
    webServer.on("/display-test", HTTP_POST, handleDisplayTest);
#endif
    webServer.on("/restart", HTTP_POST, []() {
      pulseHttpActivity(); handleRestart();
    });
    webServer.on("/update", HTTP_GET, []() {
      pulseHttpActivity(); handleOtaPage();
    });
    webServer.on("/update", HTTP_POST, finishOtaUpload, handleOtaUpload);
    webServer.onNotFound([]() {
      pulseHttpActivity();
      if (wifiSetupAccessPointActive) {
        webServer.sendHeader("Location", "http://192.168.4.1/", true);
        webServer.send(302, "text/plain; charset=utf-8", "FlightWall Setup");
      } else {
        webServer.send(404, "text/plain; charset=utf-8", "Nicht gefunden");
      }
    });
    webServerConfigured = true;
  }
  webServer.begin();
  if (WiFi.status() == WL_CONNECTED && MDNS.begin(DEVICE_HOSTNAME))
    MDNS.addService("http", "tcp", 80);
  webServerStarted = true;
  ++webServerStartCount;
  if (WiFi.status() == WL_CONNECTED) {
    startArduinoOta();
    Serial.printf("Web interface: http://%s.local\n", DEVICE_HOSTNAME);
  }
}


void startSetupAccessPoint() {
  if (wifiSetupAccessPointActive) return;
  if (!wifiSetupAccessPointSsid.length()) {
    const uint32_t suffix = static_cast<uint32_t>(ESP.getEfuseMac() & 0xFFFFFF);
    char suffixText[7];
    snprintf(suffixText, sizeof(suffixText), "%06X", suffix);
    wifiSetupAccessPointSsid = String(WIFI_SETUP_AP_PREFIX) + "-" + suffixText;
  }
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.softAP(wifiSetupAccessPointSsid.c_str(),
                   WIFI_SETUP_AP_PASSWORD)) {
    setNetworkError("SETUP-WLAN FEHLER");
    return;
  }
  wifiSetupAccessPointActive = true;
  setupDnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  setupDnsServer.start(53, "*", WiFi.softAPIP());
  startWebServer();
  setNetworkError("WLAN EINRICHTUNG");
  Serial.printf("Wi-Fi setup network: %s / http://%s\n",
                wifiSetupAccessPointSsid.c_str(),
                WiFi.softAPIP().toString().c_str());
}


void stopSetupAccessPoint() {
  if (!wifiSetupAccessPointActive) return;
  setupDnsServer.stop();
  WiFi.softAPdisconnect(false);
  wifiSetupAccessPointActive = false;
  Serial.println("Wi-Fi setup network stopped");
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
  bool initiallyConnected = false;
  {
    NetworkPhaseScope phase(NetworkPhase::Wifi);
    initiallyConnected = connectWiFi(20000);
  }
  if (initiallyConnected) {
    wifiWasConnected = true;
    startWebServer();
    fetchLocalFlight();
    lastPoll = millis();
  } else {
    startSetupAccessPoint();
  }
  for (;;) {
    const uint32_t now = millis();
    serviceDailyAircraftStatistics();
    const bool wifiConnected = WiFi.status() == WL_CONNECTED;
    if (!wifiConnected && wifiWasConnected) {
      wifiWasConnected = false;
      lastWifiDisconnect = now;
      stopNetworkServices();
      setNetworkError("WLAN GETRENNT");
      startSetupAccessPoint();
    } else if (wifiConnected && !wifiWasConnected) {
      wifiWasConnected = true;
      ++wifiReconnectCount;
      if (wifiSetupAccessPointActive) {
        stopSetupAccessPoint();
        stopNetworkServices();
      }
      startWebServer();
      Serial.println("Network services restarted after Wi-Fi reconnect");
    }
    if (wifiConnected || wifiSetupAccessPointActive) {
      {
        NetworkPhaseScope phase(NetworkPhase::Web);
        webServer.handleClient();
      }
      vTaskDelay(pdMS_TO_TICKS(1));
      if (wifiSetupAccessPointActive) setupDnsServer.processNextRequest();
    }
    if (wifiConnected) {
      if (otaStarted) {
        NetworkPhaseScope phase(NetworkPhase::Ota);
        ArduinoOTA.handle();
      }
      vTaskDelay(pdMS_TO_TICKS(1));
      {
        NetworkPhaseScope phase(NetworkPhase::Mqtt);
        serviceMqtt();
      }
    }
    if (otaRebootAt && static_cast<int32_t>(now - otaRebootAt) >= 0) ESP.restart();
    if (manualRebootAt && static_cast<int32_t>(now - manualRebootAt) >= 0) ESP.restart();
    if (!wifiConnected && now - lastWiFiAttempt >= WIFI_RETRY_MS) {
      lastWiFiAttempt = now;
      NetworkPhaseScope phase(NetworkPhase::Wifi);
      connectWiFi(10000);
    }
    if (wifiConnected && now - lastPoll >= POLL_INTERVAL_MS) {
      lastPoll = now;
      fetchLocalFlight();
    }
    {
      NetworkPhaseScope phase(NetworkPhase::Flash);
      flushDeferredButtonSettings();
      flushDeferredRouteCacheIfIdle();
    }
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}


void setup() {
  Serial.begin(115200);
  delay(250);
  setStatusLedBrightness(1);
  setStatusLed(LedStatus::Starting);
  updateStatusLed(false);
  initializeNetworkDiagnostics();
  // Arduino initializes board PSRAM before setup(). Allocate only external
  // memory, once, before Protomatter reserves internal DMA buffers.
  radarTrailStorage = allocateRadarTrailStorage([](size_t bytes) -> void * {
    return heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  });
  Serial.printf("Radar history: %s (%u bytes), PSRAM free: %u bytes\n",
      radarTrailStorage ? "external PSRAM" : "unavailable",
      static_cast<unsigned>(radarTrailStorage ? sizeof(RadarTrailStorage) : 0),
      ESP.getFreePsram());
  displayTaskHandle = xTaskGetCurrentTaskHandle();
  flightMutex = xSemaphoreCreateMutex();
  preferencesMutex = xSemaphoreCreateMutex();
  weatherMutex = xSemaphoreCreateMutex();
  if (flightMutex == nullptr || preferencesMutex == nullptr ||
      weatherMutex == nullptr) {
    Serial.println("Cannot create FreeRTOS mutexes");
    while (true) delay(1000);
  }
#if FLIGHTWALL_HAS_MATRIX
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
#else
  Serial.println("Headless ESP32-S3 mode: HUB75 output disabled");
  Serial.printf("Flash: %u bytes, PSRAM: %u bytes\n",
                ESP.getFlashChipSize(), ESP.getPsramSize());
#endif

  preferences.begin("flightwall", false);
  const uint8_t settingsVersion = preferences.getUChar("settingsVer", 1);
  wifiSsid = preferences.getString("wifiSsid", WIFI_SSID);
  wifiPassword = preferences.getString("wifiPass", WIFI_PASSWORD);
  otaPassword = preferences.getString("otaPass", OTA_PASSWORD);
  if (otaPassword.length() > 0 && otaPassword.length() < 8)
    otaPassword = "";
  if (otaPassword.length() > 63) otaPassword.remove(63);
  if (!wifiSsid.length() || wifiSsid.length() > 32) wifiSsid = WIFI_SSID;
  const uint32_t setupSuffix =
      static_cast<uint32_t>(ESP.getEfuseMac() & 0xFFFFFF);
  char setupSuffixText[7];
  snprintf(setupSuffixText, sizeof(setupSuffixText), "%06X", setupSuffix);
  wifiSetupAccessPointSsid =
      String(WIFI_SETUP_AP_PREFIX) + "-" + setupSuffixText;
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
  if (settingsVersion < 13) {
    preferences.putFloat("farAngle", DEFAULT_FAR_ELEVATION_DEG);
  }
  if (manualBrightnessIndex >= BRIGHTNESS_LEVEL_COUNT) manualBrightnessIndex = 1;
  brightnessIndex = manualBrightnessIndex;
  automaticBrightnessEnabled = preferences.getBool("autoLight", true);
  displayEnabled = preferences.getBool("displayOn", true);
  uiEnglish = preferences.getBool("uiEnglish", false);
#if FLIGHTWALL_HAS_MATRIX
  environmentIdleDisplayEnabled = preferences.getBool("envIdle", true);
#else
  environmentIdleDisplayEnabled = false;
#endif
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
  statusLedBrightnessMode = constrain(
      preferences.getUChar("statusLed", 1), 0, 2);
  setStatusLedBrightness(statusLedBrightnessMode);
  displayRadiusKm = preferences.getFloat("radiusKm", DEFAULT_DISPLAY_RADIUS_KM);
  observerElevationM = constrain(
      preferences.getFloat("observerElevM", 0.0f), -500.0f, 9000.0f);
  farDirectionElevationDeg = constrain(
      preferences.getFloat("farAngle", DEFAULT_FAR_ELEVATION_DEG),
      10.0f, 45.0f);
  pageIntervalMs = preferences.getUInt("pageMs", DEFAULT_PAGE_INTERVAL_MS);
  lowLuxThreshold = preferences.getFloat("lowLux", 20.0f);
  highLuxThreshold = preferences.getFloat("highLux", 800.0f);
  adsbUrl1 = preferences.getString("adsbUrl", DEFAULT_ADSB_URL);
  adsbUrl2 = preferences.getString("adsbUrl2", "");
  activeAdsbSource = preferences.getUChar("adsbActive", 0);
  if (activeAdsbSource > 2 || (activeAdsbSource == 1 && !adsbUrl2.length()))
    activeAdsbSource = 0;
  automaticAdsbFailover = preferences.getBool("adsbFailover", true);
  currentAdsbSource = activeAdsbSource;
  routeUrl = preferences.getString("routeUrl", DEFAULT_ROUTE_URL);
  siteLatitude = preferences.getDouble("siteLat", DEFAULT_SITE_LATITUDE);
  siteLongitude = preferences.getDouble("siteLon", DEFAULT_SITE_LONGITUDE);
  viewHeading = preferences.getFloat("heading", DEFAULT_VIEW_HEADING);
  routesEnabled = preferences.getBool("routesOn", true);
  adsbDbFallbackEnabled = preferences.getBool("routeFallback", true);
  radarTrailsEnabled = preferences.getBool("radarTrails", true);
  hideSportAircraft = preferences.getBool("hideSport", true);
  altitudeFilterEnabled = preferences.getBool("altFilter", false);
  maximumAltitudeFt = preferences.getInt(
      "maxAltFt", DEFAULT_MAXIMUM_ALTITUDE_FT);
  customPageText = displayAscii(preferences.getString("customText", ""));
  customPageText.trim();
  if (customPageText.length() > 128) customPageText.remove(128);
  customScrollSpeed = preferences.getUChar("customSpd", 1);
  if (customScrollSpeed > 2) customScrollSpeed = 1;
  radarSweepIntervalSeconds = constrain(
      preferences.getUChar(
          "radarSweepSec", DEFAULT_RADAR_SWEEP_INTERVAL_SECONDS),
      static_cast<uint8_t>(0), static_cast<uint8_t>(10));
  smoothTransitions = preferences.getBool("smooth", true);
  for (uint8_t page = 0; page < PAGE_COUNT; ++page) {
    pageEnabled[page] = preferences.getBool(
        ("p" + String(page) + "On").c_str(), true);
    pageDurationMs[page] = preferences.getUInt(
        ("p" + String(page) + "Ms").c_str(), pageIntervalMs);
    pageOrder[page] = preferences.getUChar(
        ("order" + String(page)).c_str(), page);
  }
  if (settingsVersion < 15) preferences.putUChar("settingsVer", 15);
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

#if FLIGHTWALL_HAS_MATRIX
  Wire.begin(LIGHT_SENSOR_SDA_PIN, LIGHT_SENSOR_SCL_PIN);
#else
  // The generic N16R8 board has no fixed STEMMA connector. GPIO 16/17 are
  // the documented default I2C connection for optional sensors.
  Wire.begin(STEMMA_SDA_PIN, STEMMA_SCL_PIN);
#endif
  Wire.setTimeOut(50);
#if FLIGHTWALL_HAS_MATRIX
  environmentWire.begin(STEMMA_SDA_PIN, STEMMA_SCL_PIN);
  environmentWire.setTimeOut(50);
  findLightSensor();
#endif
  findEnvironmentSensor();

#if FLIGHTWALL_HAS_MATRIX
  drawStatus("FLIGHTWALL", uiText("VERBINDET...", "CONNECTING..."));
#endif
  if (xTaskCreatePinnedToCore(
          networkTask, "network", 12288, nullptr, tskIDLE_PRIORITY,
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
  if (wifiSetupAccessPointActive) return 50;
  if (otaPreparing || otaInProgress || routeCacheWritePreparing ||
      routeCacheWriteInProgress || settingsWritePreparing ||
      settingsWriteInProgress || displayTestActive ||
      transitionPhase != TransitionPhase::Normal ||
      static_cast<int32_t>(brightnessMessageUntil - millis()) > 0) {
    return 50;
  }
  if (!aircraftIsInsideDisplayRadius()) {
#if FLIGHTWALL_HAS_MATRIX
    if (!environmentIdleDisplayEnabled && pageEnabled[7]) {
      if (!radarSweepIntervalSeconds) return 500;
      if (!radarSweepStarted) return 25;
      const uint32_t cycleMs =
          static_cast<uint32_t>(radarSweepIntervalSeconds) * 1000U;
      const uint32_t elapsed = (millis() - radarSweepStarted) % cycleMs;
      if (elapsed < RADAR_SWEEP_PERIOD_MS)
        return RADAR_SWEEP_DRAW_INTERVAL_MS;
      return min(250U, cycleMs - elapsed);
    }
#endif
    return 500;
  }
  if (currentPageSlot < PAGE_COUNT) {
    const uint8_t page = pageOrder[currentPageSlot];
    // Route-name and custom-text pages scroll pixel by pixel. The compact
    // airport page can also contain scrolling multi-hop routes.
    if (page == 7) {
      if (!radarSweepIntervalSeconds) return 250;
      if (!radarSweepStarted) return 25;
      const uint32_t cycleMs =
          static_cast<uint32_t>(radarSweepIntervalSeconds) * 1000U;
      const uint32_t elapsed = (millis() - radarSweepStarted) % cycleMs;
      if (elapsed < RADAR_SWEEP_PERIOD_MS)
        return RADAR_SWEEP_DRAW_INTERVAL_MS;
      return min(250U, cycleMs - elapsed);
    }
    if (page == 4 && customScrollSpeed == 2) return 25;
    if (page == 2 || page == 3 || page == 4 || page == 6) return 50;
  }
  // Static pages only need a periodic deadline check; ADS-B changes and page
  // changes are detected comfortably within a quarter second.
  return 250;
}


void serviceStatusLed() {
  LedStatus target = LedStatus::Healthy;
  const uint32_t now = millis();
  if (flight.emergency) {
    target = LedStatus::Emergency;
  } else if (otaPreparing || otaInProgress) {
    target = LedStatus::Updating;
  } else if (static_cast<int32_t>(statusLedErrorUntil - now) > 0) {
    target = LedStatus::Error;
  } else if (wifiSetupAccessPointActive) {
    target = LedStatus::Setup;
  } else if (WiFi.status() != WL_CONNECTED) {
    target = LedStatus::Connecting;
  } else if (lastError.length()) {
    target = LedStatus::Warning;
  }
  setStatusLed(target);
  updateStatusLed(quietHoursAreActive());
}


void loop() {
  const uint32_t now = millis();
  syncNetworkState();
  serviceStatusLed();
#if FLIGHTWALL_HAS_MATRIX
  handleBrightnessButtons();
  serviceLightSensor();
#endif
  serviceEnvironmentSensor();
#if FLIGHTWALL_HAS_MATRIX
  serviceMatrixDiagnostics(now);
  if (!automaticBrightnessEnabled && brightnessIndex != manualBrightnessIndex) {
    applyBrightnessIndex(manualBrightnessIndex, false);
  }

  static uint32_t lastDraw = 0;
  if (now - lastDraw >= displayDrawIntervalMs()) {
    lastDraw = now;
    drawDisplay();
  }
#else
  (void)now;
#endif
  delay(10);
}
