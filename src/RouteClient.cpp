#include "RouteClient.hpp"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>

#include "AirportNames.hpp"
#include "UnknownCodes.hpp"

namespace {
constexpr size_t cacheSlots = 32;
constexpr uint32_t positiveTtl = 30U * 24U * 60U * 60U;
constexpr uint32_t negativeTtl = 6U * 60U * 60U;

String cacheKey(size_t slot) {
  char key[5];
  snprintf(key, sizeof(key), "r%02u", static_cast<unsigned>(slot));
  return key;
}

std::vector<String> split(const String& input, char delimiter) {
  std::vector<String> values;
  int start = 0;
  while (start <= static_cast<int>(input.length())) {
    int end = input.indexOf(delimiter, start);
    if (end < 0) end = input.length();
    values.push_back(input.substring(start, end));
    start = end + 1;
  }
  return values;
}

String upperTrimmed(String value) {
  value.trim();
  value.toUpperCase();
  return value;
}

void fillNames(RouteResult& route) {
  if (route.routeCodes.isEmpty() &&
      (!route.origin.isEmpty() || !route.destination.isEmpty())) {
    route.routeCodes = route.origin;
    if (!route.destination.isEmpty()) {
      if (!route.routeCodes.isEmpty()) route.routeCodes += '-';
      route.routeCodes += route.destination;
    }
  }
  const std::vector<String> codes = split(route.routeCodes, '-');
  route.routeNames = "";
  if (!codes.empty()) {
    route.origin = upperTrimmed(codes.front());
    route.destination = upperTrimmed(codes.back());
  }
  for (const String& rawCode : codes) {
    const String code = upperTrimmed(rawCode);
    if (code.isEmpty()) continue;
    const String name = airportDisplayName(code);
    if (name.isEmpty()) rememberUnknownAirport(code);
    if (!route.routeNames.isEmpty()) route.routeNames += " > ";
    route.routeNames += name.isEmpty() ? code : name;
  }
  route.originName = airportDisplayName(route.origin);
  route.destinationName = airportDisplayName(route.destination);
}

void extractCodes(JsonObjectConst row, RouteResult& route) {
  const char* keys[] = {"_airport_codes_iata", "airport_codes_iata",
                        "_airport_codes_icao", "airport_codes_icao",
                        "airport_codes"};
  for (const char* key : keys) {
    if (!row[key].is<const char*>()) continue;
    String codes = row[key].as<String>();
    codes.replace("->", "-");
    codes.replace(">", "-");
    route.routeCodes = upperTrimmed(codes);
    if (route.routeCodes.indexOf('-') > 0) {
      return;
    }
  }

  JsonVariantConst origin = row["origin"];
  if (origin.isNull()) origin = row["from"];
  JsonVariantConst destination = row["destination"];
  if (destination.isNull()) destination = row["to"];
  if (origin.is<const char*>()) route.origin = upperTrimmed(origin.as<String>());
  if (destination.is<const char*>())
    route.destination = upperTrimmed(destination.as<String>());
  if (origin.is<JsonObjectConst>())
    route.origin = upperTrimmed(origin["iata"] | origin["icao"] | "");
  if (destination.is<JsonObjectConst>())
    route.destination =
        upperTrimmed(destination["iata"] | destination["icao"] | "");
  route.routeCodes = route.origin;
  if (!route.destination.isEmpty()) {
    if (!route.routeCodes.isEmpty()) route.routeCodes += '-';
    route.routeCodes += route.destination;
  }
}
}  // namespace

void RouteClient::configure(bool enabled, const String& url) {
  enabled_ = enabled;
  url_ = url;
}

RouteResult RouteClient::readMemoryCache(const String& callsign,
                                         uint32_t now) {
  for (MemoryCacheEntry& entry : memoryCache_) {
    if (entry.callsign != callsign) continue;
    const uint32_t ttl = entry.route.found() ? positiveTtl : negativeTtl;
    if (now && entry.updated && now - entry.updated >= ttl) return {};
    RouteResult result = entry.route;
    result.status = result.found() ? "memory" : "not_found";
    return result;
  }
  return {};
}

void RouteClient::rememberInMemory(const String& callsign,
                                   const RouteResult& route, uint32_t now,
                                   bool pendingWrite) {
  MemoryCacheEntry* target = nullptr;
  for (MemoryCacheEntry& entry : memoryCache_) {
    if (entry.callsign == callsign) {
      target = &entry;
      break;
    }
  }
  if (target == nullptr) {
    target = &memoryCache_[nextMemorySlot_++ % memoryCacheSlots_];
  }
  target->callsign = callsign;
  target->route = route;
  target->updated = now;
  target->pendingWrite = target->pendingWrite || pendingWrite;
}

bool RouteClient::hasPendingWrites() const {
  for (const MemoryCacheEntry& entry : memoryCache_) {
    if (entry.pendingWrite) return true;
  }
  return false;
}

void RouteClient::flushPendingWrites() {
  for (MemoryCacheEntry& entry : memoryCache_) {
    if (!entry.pendingWrite || entry.callsign.isEmpty()) continue;
    writeCache(entry.callsign, entry.route, entry.updated);
    entry.pendingWrite = false;
  }
}

RouteResult RouteClient::readCache(const String& callsign, uint32_t now) {
  Preferences preferences;
  preferences.begin("routecache", true);
  for (size_t slot = 0; slot < cacheSlots; ++slot) {
    const String raw = preferences.getString(cacheKey(slot).c_str(), "");
    if (raw.isEmpty()) continue;
    const std::vector<String> fields = split(raw, '|');
    if (fields.size() < 6 || fields[0] != callsign) continue;
    const uint32_t updated = static_cast<uint32_t>(strtoul(fields[1].c_str(), nullptr, 10));
    const bool found = fields[2] == "1";
    const uint32_t ttl = found ? positiveTtl : negativeTtl;
    if (!now || !updated || now - updated >= ttl) break;
    RouteResult result;
    result.origin = fields[3];
    result.destination = fields[4];
    result.airlineCode = fields[5];
    result.routeCodes = fields.size() >= 7
                            ? fields[6]
                            : result.origin + "-" + result.destination;
    result.status = found ? "cached" : "not_found";
    fillNames(result);
    preferences.end();
    return result;
  }
  preferences.end();
  return {};
}

void RouteClient::writeCache(const String& callsign, const RouteResult& route,
                             uint32_t now) {
  Preferences preferences;
  if (!preferences.begin("routecache", false)) return;
  const uint8_t slot = preferences.getUChar("next", 0) % cacheSlots;
  const String value = callsign + "|" + String(now) + "|" +
                       (route.found() ? "1" : "0") + "|" + route.origin +
                       "|" + route.destination + "|" + route.airlineCode +
                       "|" + route.routeCodes;
  preferences.putString(cacheKey(slot).c_str(), value);
  preferences.putUChar("next", (slot + 1) % cacheSlots);
  preferences.end();
}

RouteResult RouteClient::fetch(const String& callsign, double latitude,
                               double longitude) {
  RouteResult result;
  WiFiClientSecure client;
  // The ESP32 has no trusted wall clock before ADS-B data arrives. HTTPS still
  // encrypts transport, but certificate verification is therefore disabled.
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(8000);
  if (!http.begin(client, url_)) {
    result.error = "Ungültige Routen-Adresse";
    return result;
  }

  JsonDocument request;
  JsonObject plane = request["planes"].add<JsonObject>();
  plane["callsign"] = callsign;
  plane["lat"] = latitude;
  plane["lng"] = longitude;
  String body;
  serializeJson(request, body);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  const int status = http.POST(body);
  if (status != HTTP_CODE_OK) {
    result.error = "Routen-HTTP " + String(status);
    http.end();
    return result;
  }

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, http.getStream());
  http.end();
  if (error) {
    result.error = "Routen-JSON: " + String(error.c_str());
    return result;
  }

  JsonArrayConst rows;
  if (document.is<JsonArrayConst>()) rows = document.as<JsonArrayConst>();
  else if (document["routes"].is<JsonArrayConst>())
    rows = document["routes"].as<JsonArrayConst>();
  else if (document["planes"].is<JsonArrayConst>())
    rows = document["planes"].as<JsonArrayConst>();
  for (JsonObjectConst row : rows) {
    const String returned = upperTrimmed(row["callsign"] | "");
    if (!returned.isEmpty() && returned != callsign) continue;
    extractCodes(row, result);
    result.airlineCode = upperTrimmed(row["airline_code"] | "");
    if (result.airlineCode == "UNKNOWN") result.airlineCode = "";
    break;
  }
  fillNames(result);
  result.status = result.found() ? "current" : "not_found";
  return result;
}

RouteResult RouteClient::lookup(const String& callsign, double latitude,
                                double longitude, uint32_t now) {
  if (!enabled_ || callsign.isEmpty()) {
    lastError_ = "";
    return {};
  }
  RouteResult cached = readMemoryCache(callsign, now);
  if (cached.status != "unavailable") {
    lastError_ = "";
    return cached;
  }
  cached = readCache(callsign, now);
  if (cached.status != "unavailable") {
    rememberInMemory(callsign, cached, now, false);
    lastError_ = "";
    return cached;
  }

  RouteResult result = fetch(callsign, latitude, longitude);
  lastError_ = result.error;
  if (result.error.isEmpty()) rememberInMemory(callsign, result, now, true);
  return result;
}

RouteResult RouteClient::test(const String& callsign, double latitude,
                              double longitude) {
  if (callsign.isEmpty()) {
    lastError_ = "Test-Callsign fehlt";
    RouteResult result;
    result.error = lastError_;
    return result;
  }
  RouteResult result = fetch(callsign, latitude, longitude);
  lastError_ = result.error;
  return result;
}
