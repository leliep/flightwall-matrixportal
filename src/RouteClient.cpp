#include "TemporaryPsram.hpp"
#include "RouteClient.hpp"
#include "RouteCallsign.hpp"
#include "NetworkIo.hpp"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <esp_heap_caps.h>

#include <algorithm>
#include <cmath>
#include <utility>

#include "AirportCoordinates.hpp"
#include "AirportNames.hpp"
#include "CallsignClassifier.hpp"
#include "Geo.hpp"
#include "UnknownCodes.hpp"

RouteMemorySnapshot routeMemorySnapshot() {
  constexpr uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
  RouteMemorySnapshot result;
  result.freeInternal = heap_caps_get_free_size(caps);
  result.largestInternal = heap_caps_get_largest_free_block(caps);
  result.minimumInternal = heap_caps_get_minimum_free_size(caps);
  return result;
}

namespace {
constexpr size_t cacheSlots = 32;
constexpr uint32_t positiveTtl = 6U * 60U * 60U;
constexpr uint32_t adsbDbPositiveTtl = 6U * 60U * 60U;
constexpr uint32_t negativeTtl = 60U * 60U;
constexpr uint32_t specialTtl = 30U * 60U;
constexpr char adsbDbUrl[] = "https://api.adsbdb.com/v0/callsign/";
constexpr double earthRadiusKm = 6371.0;

void captureRouteTestJson(RouteTestResponse &response, const JsonDocument &document) {
  PsramBuffer json;
  struct Writer {
    PsramBuffer &json;
    bool &truncated;
    size_t write(uint8_t value) { return write(&value, 1); }
    size_t write(const uint8_t *data, size_t length) {
      const size_t remaining = RouteTestResponse::jsonLimit - json.length();
      const size_t keep = length < remaining ? length : remaining;
      if (keep) json.write(data, keep);
      if (length > keep) truncated = true;
      // Continue traversal without retaining bytes beyond the limit.
      return length;
    }
  } writer{json, response.truncated};
  serializeJsonPretty(document, writer);
  response.captureFailed = json.failed();
  if (!response.captureFailed)
    response.captureFailed = !response.json.assign(json.c_str(), json.length());
}

// HTTPClient de-chunks the body into this bounded sink. Stop reading after the
// limit instead of allocating or draining an unbounded error response.
void captureRouteTestHttpError(RouteTestResponse &response, HTTPClient &http) {
  PsramBuffer body;
  class Sink : public Stream {
   public:
    Sink(PsramBuffer &body, bool &truncated) : body_(body), truncated_(truncated) {}
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
    size_t write(uint8_t value) override { return write(&value, 1); }
    size_t write(const uint8_t *data, size_t length) override {
      const size_t remaining = RouteTestResponse::jsonLimit - body_.length();
      const size_t keep = std::min(length, remaining);
      if (length > keep) truncated_ = true;
      if (keep && !body_.write(data, keep)) return 0;
      return keep;
    }
   private:
    PsramBuffer &body_;
    bool &truncated_;
  } sink(body, response.truncated);
  int written;
  { NetworkIoScope io; written = http.writeToStream(&sink); }
  if (networkOperationCancelled()) return;
  response.rawBody = true;
  response.captureFailed = body.failed() || (written < 0 && !response.truncated);
  if (!response.captureFailed)
    response.captureFailed = !response.json.assign(body.c_str(), body.length());
}

bool isManagedPrimaryUrl(const String &url) {
  return url == "https://adsb.im/api/0/routeset" ||
         url == "http://adsb.im/api/0/routeset" ||
         url == "https://adsb.im/api/0/routeset/" ||
         url == "http://adsb.im/api/0/routeset/";
}

constexpr char routeUserAgent[] =
    "FlightWall-MatrixPortal/3.6.1-dual-target.56 "
    "(+https://github.com/leliep/flightwall-matrixportal/issues)";
constexpr uint32_t primaryBatchWindowMs = 30000;

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

bool shortLivedCallsign(const String& callsign) {
  return isGermanAirforceCallsign(callsign) ||
         isMilitaryCallsign(callsign) || callsign.startsWith("BPO") ||
         callsign.startsWith("POL") || callsign.startsWith("HBT") ||
         callsign.startsWith("HUM") || callsign.startsWith("NRW") ||
         callsign.startsWith("CHX");
}

uint32_t cacheTtl(const String& callsign, bool found, const String& source) {
  if (!found) return negativeTtl;
  if (shortLivedCallsign(callsign)) return specialTtl;
  return source == "adsbdb" ? adsbDbPositiveTtl : positiveTtl;
}

bool cacheExpired(const String& callsign, bool found, uint32_t updated,
                  uint32_t now, const String& source) {
  return !now || !updated ||
         now - updated >= cacheTtl(callsign, found, source);
}

bool decodeCacheValue(const String& raw, String& callsign, uint32_t& updated,
                      bool& found, RouteResult& route) {
  const std::vector<String> fields = split(raw, '|');
  if (fields.size() < 6 || fields[0].isEmpty()) return false;
  callsign = fields[0];
  updated = static_cast<uint32_t>(strtoul(fields[1].c_str(), nullptr, 10));
  found = fields[2] == "1";
  route.origin = fields[3];
  route.destination = fields[4];
  route.airlineCode = fields[5];
  route.routeCodes = fields.size() >= 7
                         ? fields[6]
                         : route.origin + "-" + route.destination;
  route.source = fields.size() >= 8 && !fields[7].isEmpty()
                     ? fields[7]
                     : "primary";
  return true;
}

double greatCircleDistanceKm(double lat1, double lon1,
                             double lat2, double lon2) {
  constexpr double radiansPerDegree = 0.017453292519943295;
  const double phi1 = lat1 * radiansPerDegree;
  const double phi2 = lat2 * radiansPerDegree;
  const double deltaPhi = (lat2 - lat1) * radiansPerDegree;
  const double deltaLambda = (lon2 - lon1) * radiansPerDegree;
  const double sinPhi = sin(deltaPhi / 2.0);
  const double sinLambda = sin(deltaLambda / 2.0);
  const double a = sinPhi * sinPhi +
                   cos(phi1) * cos(phi2) * sinLambda * sinLambda;
  return earthRadiusKm * 2.0 * atan2(sqrt(a), sqrt(std::max(0.0, 1.0 - a)));
}

bool validCoordinates(double latitude, double longitude) {
  return isfinite(latitude) && isfinite(longitude) &&
         latitude >= -90.0 && latitude <= 90.0 &&
         longitude >= -180.0 && longitude <= 180.0;
}

String airportCode(JsonVariantConst airport) {
  if (!airport.is<JsonObjectConst>()) return "";
  JsonObjectConst object = airport.as<JsonObjectConst>();
  String code = upperTrimmed(object["iata_code"] | "");
  if (code.isEmpty()) code = upperTrimmed(object["icao_code"] | "");
  return code;
}

void appendRouteCode(String& routeCodes, const String& rawCode) {
  const String code = upperTrimmed(rawCode);
  if (code.isEmpty()) return;
  if (!routeCodes.isEmpty()) routeCodes += '-';
  routeCodes += code;
}

bool adsbDbRoutePlausible(JsonObjectConst route, double latitude,
                          double longitude) {
  JsonObjectConst origin = route["origin"].as<JsonObjectConst>();
  JsonObjectConst destination = route["destination"].as<JsonObjectConst>();
  if (origin.isNull() || destination.isNull() ||
      !origin["latitude"].is<double>() ||
      !origin["longitude"].is<double>() ||
      !destination["latitude"].is<double>() ||
      !destination["longitude"].is<double>())
    return false;

  const double originLat = origin["latitude"].as<double>();
  const double originLon = origin["longitude"].as<double>();
  const double destinationLat = destination["latitude"].as<double>();
  const double destinationLon = destination["longitude"].as<double>();
  if (!validCoordinates(latitude, longitude) ||
      !validCoordinates(originLat, originLon) ||
      !validCoordinates(destinationLat, destinationLon))
    return false;

  struct RoutePoint {
    double latitude;
    double longitude;
  };
  RoutePoint points[10];
  size_t pointCount = 0;
  points[pointCount++] = {originLat, originLon};

  auto appendPoint = [&](JsonVariantConst airport) {
    if (pointCount >= 9 || !airport.is<JsonObjectConst>()) return;
    JsonObjectConst object = airport.as<JsonObjectConst>();
    if (!object["latitude"].is<double>() ||
        !object["longitude"].is<double>())
      return;
    const double pointLat = object["latitude"].as<double>();
    const double pointLon = object["longitude"].as<double>();
    if (validCoordinates(pointLat, pointLon))
      points[pointCount++] = {pointLat, pointLon};
  };
  JsonVariantConst midpoint = route["midpoint"];
  if (midpoint.is<JsonArrayConst>()) {
    for (JsonVariantConst item : midpoint.as<JsonArrayConst>())
      appendPoint(item);
  } else {
    appendPoint(midpoint);
  }
  points[pointCount++] = {destinationLat, destinationLon};

  // Check every leg separately. This also accepts circular routes such as
  // DUS-BUD-DUS, whose origin and destination alone have zero distance.
  for (size_t index = 1; index < pointCount; ++index) {
    const RoutePoint& start = points[index - 1];
    const RoutePoint& end = points[index];
    const double direct = greatCircleDistanceKm(
        start.latitude, start.longitude, end.latitude, end.longitude);
    const double viaAircraft =
        greatCircleDistanceKm(start.latitude, start.longitude,
                              latitude, longitude) +
        greatCircleDistanceKm(latitude, longitude,
                              end.latitude, end.longitude);
    const double allowance = 350.0 + std::min(500.0, direct * 0.15);
    if (viaAircraft <= direct + allowance) return true;
  }
  return false;
}

String primarySourceName(const String& url) {
  return url.indexOf("adsb.im") >= 0 ? "adsb.im" : "primary";
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

void rejectRoute(RouteResult& route, const String& reason) {
  if (!route.source.endsWith("-rejected")) route.source += "-rejected";
  route.status = "rejected";
  route.error = reason;
  route.routeCodes = "";
  route.routeNames = "";
  route.origin = "";
  route.destination = "";
  route.originName = "";
  route.destinationName = "";
  route.confidence = 0;
  route.confidenceReason = reason;
}

void assessRouteConfidence(RouteResult& route, bool positionChecked,
                           bool cached) {
  if (!route.found()) {
    route.confidence = 0;
    route.confidenceReason = "Keine Route vorhanden";
    return;
  }
  uint8_t score = route.source == "adsb.im" ? 80 :
                  route.source == "adsbdb" ? 60 : 70;
  String reason = "Quelle " + route.source;
  if (positionChecked) {
    score = static_cast<uint8_t>(std::min(95, static_cast<int>(score) + 15));
    reason += ", Position plausibel";
  }
  if (cached) {
    score = score > 5 ? score - 5 : score;
    reason += ", Cache";
  }
  route.confidence = score;
  route.confidenceReason = reason;
}

bool routePlausible(const RouteResult& route, const RouteContext& context,
                    String& reason) {
  if (!route.found() || !context.hasPosition ||
      !validCoordinates(context.latitude, context.longitude))
    return true;

  const std::vector<String> codes = split(route.routeCodes, '-');
  struct RoutePoint {
    AirportCoordinates coordinates;
  };
  std::vector<RoutePoint> points;
  points.reserve(codes.size());
  for (const String& rawCode : codes) {
    AirportCoordinates coordinates;
    if (airportCoordinates(upperTrimmed(rawCode), coordinates))
      points.push_back({coordinates});
  }
  // Unknown coordinates must not turn a potentially valid route into a false
  // rejection. The lookup table normally covers every known display code.
  if (points.size() < 2) return true;

  double bestDetour = HUGE_VAL;
  double bestDirect = 0.0;
  double bestStartDistance = 0.0;
  double bestEndDistance = 0.0;
  const RoutePoint* bestStart = nullptr;
  const RoutePoint* bestEnd = nullptr;
  for (size_t index = 1; index < points.size(); ++index) {
    const RoutePoint& start = points[index - 1];
    const RoutePoint& end = points[index];
    const double direct = greatCircleDistanceKm(
        start.coordinates.latitude, start.coordinates.longitude,
        end.coordinates.latitude, end.coordinates.longitude);
    const double startDistance = greatCircleDistanceKm(
        start.coordinates.latitude, start.coordinates.longitude,
        context.latitude, context.longitude);
    const double endDistance = greatCircleDistanceKm(
        context.latitude, context.longitude,
        end.coordinates.latitude, end.coordinates.longitude);
    const double detour = startDistance + endDistance - direct;
    if (detour < bestDetour) {
      bestDetour = detour;
      bestDirect = direct;
      bestStartDistance = startDistance;
      bestEndDistance = endDistance;
      bestStart = &start;
      bestEnd = &end;
    }
  }
  if (bestStart == nullptr || bestEnd == nullptr) return true;

  const double corridorAllowance =
      180.0 + std::min(300.0, bestDirect * 0.12);
  if (bestDetour > corridorAllowance) {
    reason = "Route verworfen: Flugposition liegt nicht auf der Route";
    return false;
  }
  if (!context.hasAltitude) return true;

  const float startElevation = bestStart->coordinates.hasElevation
      ? bestStart->coordinates.elevationFt : 0.0F;
  const float endElevation = bestEnd->coordinates.hasElevation
      ? bestEnd->coordinates.elevationFt : 0.0F;
  const float heightAboveStart =
      std::max(0.0F, context.altitudeFt - startElevation);
  const float heightAboveEnd =
      std::max(0.0F, context.altitudeFt - endElevation);

  if (context.hasVerticalRate && context.verticalRateFpm < -200.0F &&
      heightAboveEnd < 10000.0F) {
    // Close to the destination, a descending aircraft may fly a holding
    // pattern, downwind leg or radar vector that temporarily points away
    // from the airport. This is normal terminal-area traffic and must not
    // invalidate an otherwise geographically plausible route.
    constexpr double terminalAreaKm = 100.0;
    if (bestEndDistance <= terminalAreaKm) return true;
    const double maximumDistance = 30.0 + heightAboveEnd * 0.012;
    if (bestEndDistance > maximumDistance) {
      reason = "Route verworfen: Ziel bei Höhe und Sinkflug unplausibel";
      return false;
    }
  }

  if (context.hasVerticalRate && context.verticalRateFpm > 200.0F &&
      heightAboveStart < 10000.0F) {
    const double maximumDistance = 30.0 + heightAboveStart * 0.012;
    if (bestStartDistance > maximumDistance) {
      reason = "Route verworfen: Start bei Höhe und Steigflug unplausibel";
      return false;
    }
  }

  if ((!context.hasVerticalRate ||
       fabsf(context.verticalRateFpm) <= 200.0F) &&
      std::min(heightAboveStart, heightAboveEnd) < 3000.0F &&
      std::min(bestStartDistance, bestEndDistance) > 55.0) {
    reason = "Route verworfen: niedriger Flug zu weit von Start und Ziel";
    return false;
  }
  return true;
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
RouteResult decodePrimaryRoute(const JsonDocument &document,
                               const String &callsign,
                               const RouteContext &context,
                               RouteTestResponse *test, bool requireCallsign = false) {
  RouteResult result;
  result.source = "adsb.im";
  JsonArrayConst rows;
  if (document.is<JsonArrayConst>()) rows = document.as<JsonArrayConst>();
  else if (document["routes"].is<JsonArrayConst>())
    rows = document["routes"].as<JsonArrayConst>();
  else if (document["planes"].is<JsonArrayConst>())
    rows = document["planes"].as<JsonArrayConst>();
  for (JsonObjectConst row : rows) {
    const String returned = upperTrimmed(row["callsign"] | "");
    if (returned.isEmpty() ? (requireCallsign || rows.size() != 1) :
        !routeCallsignMatches(callsign.c_str(), returned.c_str())) continue;
    extractCodes(row, result);
    if (test) {
      test->returnedCallsign = returned;
      test->routeCodes = result.routeCodes;
    }
    result.airlineCode = upperTrimmed(row["airline_code"] | "");
    if (result.airlineCode == "UNKNOWN") result.airlineCode = "";
    break;
  }
  fillNames(result);
  result.status = result.found() ? "current" : "not_found";
  String reason;
  if (result.found() && !routePlausible(result, context, reason))
    rejectRoute(result, reason);
  else
    assessRouteConfidence(result, context.hasPosition, false);
  return result;
}


}  // namespace

void RouteClient::configure(bool enabled, const String& url,
                            bool adsbDbFallbackEnabled) {
  const bool managed = isManagedPrimaryUrl(url);
  const String effective = managed ? String("http://adsb.im/api/0/routeset") : url;
  if (!(url_ == effective) || enabled_ != enabled) clearPrimaryQueue();
  enabled_ = enabled;
  url_ = effective;
  managedPrimary_ = managed;
  adsbDbFallbackEnabled_ = adsbDbFallbackEnabled;
}

RouteResult RouteClient::readMemoryCache(const String& callsign,
                                         const RouteContext& context,
                                         uint32_t now) {
  for (MemoryCacheEntry& entry : memoryCache_) {
    if (entry.callsign != callsign) continue;
    if (cacheExpired(callsign, entry.route.found(), entry.updated, now,
                     entry.route.source)) {
      entry = {};
      return {};
    }
    RouteResult result = entry.route;
    fillNames(result);
    String reason;
    if (result.found() && !routePlausible(result, context, reason)) {
      rejectRoute(result, reason);
      return result;
    }
    result.status = result.found() ? "memory" : "not_found";
    assessRouteConfidence(result, context.hasPosition, true);
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
  const bool newSlot = target == nullptr;
  if (newSlot) target = &memoryCache_[nextMemorySlot_ % memoryCacheSlots_];
  const PsramTextTransaction transaction;
  MemoryCacheEntry next;
  next.callsign = callsign;
  next.route = route;
  next.updated = now;
  next.pendingWrite = target->pendingWrite || pendingWrite;
  if (!transaction.succeeded()) return;
  *target = std::move(next);
  if (newSlot) ++nextMemorySlot_;
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

std::vector<RouteCacheEntryInfo> RouteClient::cacheEntries(uint32_t now) const {
  std::vector<RouteCacheEntryInfo> entries;
  entries.reserve(cacheSlots);

  Preferences preferences;
  preferences.begin("routecache", true);
  for (size_t slot = 0; slot < cacheSlots; ++slot) {
    const String raw = preferences.getString(cacheKey(slot).c_str(), "");
    if (raw.isEmpty()) continue;
    String callsign;
    uint32_t updated = 0;
    bool found = false;
    RouteResult route;
    if (!decodeCacheValue(raw, callsign, updated, found, route)) continue;
    RouteCacheEntryInfo* target = nullptr;
    for (RouteCacheEntryInfo& entry : entries) {
      if (entry.callsign == callsign) {
        target = &entry;
        break;
      }
    }
    if (target == nullptr) {
      entries.push_back({});
      target = &entries.back();
      target->callsign = callsign;
    }
    target->persistent = true;
    if (updated >= target->updated) {
      target->updated = updated;
      target->found = found;
      target->routeCodes = route.routeCodes;
      target->source = route.source;
      target->expired = cacheExpired(callsign, found, updated, now,
                                     route.source);
    }
  }
  preferences.end();

  for (const MemoryCacheEntry& memory : memoryCache_) {
    if (memory.callsign.isEmpty()) continue;
    RouteCacheEntryInfo* target = nullptr;
    for (RouteCacheEntryInfo& entry : entries) {
      if (entry.callsign == memory.callsign) {
        target = &entry;
        break;
      }
    }
    if (target == nullptr) {
      entries.push_back({});
      target = &entries.back();
      target->callsign = memory.callsign;
    }
    target->inMemory = true;
    target->pendingWrite = target->pendingWrite || memory.pendingWrite;
    if (memory.updated >= target->updated) {
      target->updated = memory.updated;
      target->found = memory.route.found();
      target->routeCodes = memory.route.routeCodes;
      target->source = memory.route.source;
      target->expired = cacheExpired(memory.callsign, memory.route.found(),
                                     memory.updated, now,
                                     memory.route.source);
    }
  }

  std::sort(entries.begin(), entries.end(),
            [](const RouteCacheEntryInfo& left,
               const RouteCacheEntryInfo& right) {
              return left.updated > right.updated;
            });
  return entries;
}

bool RouteClient::removeCacheEntry(const String& rawCallsign) {
  const String callsign = upperTrimmed(rawCallsign);
  if (callsign.isEmpty()) return false;
  bool removed = false;
  for (MemoryCacheEntry& entry : memoryCache_) {
    if (entry.callsign != callsign) continue;
    entry = {};
    removed = true;
  }
  for (PrimaryEntry &entry : primaryQueue_) {
    if (entry.callsign == callsign && !entry.manual) entry = {};
  }

  Preferences preferences;
  if (!preferences.begin("routecache", false)) return removed;
  for (size_t slot = 0; slot < cacheSlots; ++slot) {
    const String key = cacheKey(slot);
    const String raw = preferences.getString(key.c_str(), "");
    String storedCallsign;
    uint32_t updated = 0;
    bool found = false;
    RouteResult route;
    if (decodeCacheValue(raw, storedCallsign, updated, found, route) &&
        storedCallsign == callsign) {
      preferences.remove(key.c_str());
      removed = true;
    }
  }
  preferences.end();
  return removed;
}

void RouteClient::clearCache() {
  clearPrimaryQueue();
  for (MemoryCacheEntry& entry : memoryCache_) entry = {};
  nextMemorySlot_ = 0;
  Preferences preferences;
  if (!preferences.begin("routecache", false)) return;
  preferences.clear();
  preferences.end();
}

RouteResult RouteClient::readCache(const String& callsign,
                                   const RouteContext& context,
                                   uint32_t now) {
  Preferences preferences;
  preferences.begin("routecache", true);
  bool haveResult = false;
  uint32_t newest = 0;
  bool newestFound = false;
  RouteResult newestRoute;
  for (size_t slot = 0; slot < cacheSlots; ++slot) {
    const String raw = preferences.getString(cacheKey(slot).c_str(), "");
    if (raw.isEmpty()) continue;
    String storedCallsign;
    uint32_t updated = 0;
    bool found = false;
    RouteResult route;
    if (!decodeCacheValue(raw, storedCallsign, updated, found, route) ||
        storedCallsign != callsign ||
        cacheExpired(callsign, found, updated, now, route.source) ||
        (haveResult && updated <= newest))
      continue;
    haveResult = true;
    newest = updated;
    newestFound = found;
    newestRoute = route;
  }
  preferences.end();
  if (!haveResult) return {};
  newestRoute.status = newestFound ? "cached" : "not_found";
  fillNames(newestRoute);
  String reason;
  if (newestRoute.found() &&
      !routePlausible(newestRoute, context, reason))
    rejectRoute(newestRoute, reason);
  else
    assessRouteConfidence(newestRoute, context.hasPosition, true);
  return newestRoute;
}

void RouteClient::writeCache(const String& callsign, const RouteResult& route,
                             uint32_t now) {
  Preferences preferences;
  if (!preferences.begin("routecache", false)) return;
  int targetSlot = -1;
  for (size_t slot = 0; slot < cacheSlots; ++slot) {
    const String key = cacheKey(slot);
    const String raw = preferences.getString(key.c_str(), "");
    String storedCallsign;
    uint32_t updated = 0;
    bool found = false;
    RouteResult storedRoute;
    if (!decodeCacheValue(raw, storedCallsign, updated, found, storedRoute) ||
        storedCallsign != callsign)
      continue;
    if (targetSlot < 0) targetSlot = static_cast<int>(slot);
    else preferences.remove(key.c_str());
  }
  if (targetSlot < 0) {
    targetSlot = preferences.getUChar("next", 0) % cacheSlots;
    preferences.putUChar("next", (targetSlot + 1) % cacheSlots);
  }
  const String value = callsign + "|" + String(now) + "|" +
                       (route.found() ? "1" : "0") + "|" + route.origin +
                       "|" + route.destination + "|" + route.airlineCode +
                       "|" + route.routeCodes + "|" + route.source;
  preferences.putString(cacheKey(targetSlot).c_str(), value);
  preferences.end();
}

void RouteClient::clearPrimaryQueue() {
  for (PrimaryEntry &entry : primaryQueue_) entry = {};
  primaryWindowActive_ = false;
  manualPrimaryWaiting_ = false;
  // Retain the last POST time across configuration/cache/test actions.
}

void RouteClient::cancelTest() {
  for (PrimaryEntry &entry : primaryQueue_)
    if (entry.manual) entry = {};
  manualPrimaryWaiting_ = false;
  clearTestResponses();
}

void RouteClient::refreshPending(const String &callsign,
                                  const RouteContext &context) {
  for (PrimaryEntry &entry : primaryQueue_)
    if (entry.callsign == callsign && !entry.ready && !entry.manual)
      entry.context = context;
}

bool RouteClient::primaryResultReady(const String &callsign) const {
  for (const PrimaryEntry &entry : primaryQueue_)
    if (entry.callsign == callsign && entry.ready && !entry.manual &&
        uint32_t(millis() - entry.completedMs) < 60000U) return true;
  return false;
}

void RouteClient::processPending() {
  if (!enabled_ || !managedPrimary_ || !primaryWindowActive_) return;
  const uint32_t now = millis();
  if (uint32_t(now - primaryWindowStarted_) < primaryBatchWindowMs ||
      (primaryPosted_ && uint32_t(now - primaryLastPost_) < primaryBatchWindowMs)) return;
  PrimaryEntry *first = nullptr;
  for (PrimaryEntry &entry : primaryQueue_) {
    if (entry.callsign.isEmpty() || entry.ready) continue;
    if (!first || entry.manual) first = &entry;
  }
  if (!first) { primaryWindowActive_ = false; return; }
  // Copy before I/O: web/cache actions may clear queue entries while unlocked.
  const String callsign = first->callsign;
  const RouteContext context = first->context;
  const bool manual = first->manual;
  const bool previousCapture = captureTestResponses_;
  captureTestResponses_ = manual;
  primaryLastPost_ = now;
  primaryPosted_ = true;
  RouteResult result = requestPrimary(callsign, context, true);
  captureTestResponses_ = previousCapture;
  if (networkOperationCancelled()) { clearPrimaryQueue(); return; }
  if (manual) {
    testResponses_[0].status = result.status;
    testResponses_[0].error = result.error;
  }
  // requestPrimary fills every row on success; transport/JSON failures apply
  // to the whole batch. Completed rows remain bounded until consumed/evicted.
  for (PrimaryEntry &entry : primaryQueue_) {
    if (entry.callsign.isEmpty() || entry.ready) continue;
    const PsramTextTransaction transaction;
    entry.result = result;
    entry.storageFailed = !transaction.succeeded();
    entry.completedMs = millis();
    entry.ready = true;
  }
  primaryWindowActive_ = false;
}

RouteResult RouteClient::fetchPrimary(const String &callsign,
                                      const RouteContext &context) {
  if (!managedPrimary_) return requestPrimary(callsign, context, false);
  PrimaryEntry *target = nullptr;
  for (PrimaryEntry &entry : primaryQueue_) {
    if (entry.callsign == callsign) { target = &entry; break; }
  }
  if (target && target->ready && !target->manual &&
      uint32_t(millis() - target->completedMs) >= 60000U) {
    *target = {};
    target = nullptr;
  }
  if (target && target->ready && (!target->manual || captureTestResponses_)) {
    RouteResult result;
    if (target->storageFailed) result.error = "Routen-Batch: PSRAM allocation failed";
    else result = target->result;
    *target = {};
    String reason;
    if (result.found() && !routePlausible(result, context, reason)) rejectRoute(result, reason);
    return result;
  }
  if (!target) {
    for (PrimaryEntry &entry : primaryQueue_)
      if (entry.callsign.isEmpty()) { target = &entry; break; }
    if (!target) for (PrimaryEntry &entry : primaryQueue_)
      if (entry.ready && !entry.manual) { target = &entry; break; }
    if (target) {
      bool pendingAlready = false;
      for (const PrimaryEntry &entry : primaryQueue_)
        if (!entry.callsign.isEmpty() && !entry.ready) pendingAlready = true;
      *target = {};
      target->callsign = callsign;
      if (target->callsign.isEmpty()) {
        RouteResult failed;
        failed.error = "Routen-Batch: PSRAM allocation failed";
        return failed;
      }
      target->manual = captureTestResponses_;
      if (!primaryWindowActive_ || !pendingAlready) {
        primaryWindowStarted_ = millis();
        primaryWindowActive_ = true;
      }
    }
  }
  if (target && !target->ready) target->context = context;
  RouteResult result;
  result.source = "adsb.im";
  result.status = "pending";
  return result;
}

RouteResult RouteClient::requestPrimary(const String& callsign,
                                      const RouteContext& context, bool batch) {
  RouteResult result;
  result.source = primarySourceName(url_);
  RouteTestResponse *test = captureTestResponses_ ? &testResponses_[0] : nullptr;
  if (test) { test->attempted = true; test->source = result.source; }
  auto &diagnostic = diagnostics_.primary;
  diagnostic.begin(result.source.c_str(), callsign.c_str(), millis(), routeMemorySnapshot());
  RouteCleanupCapture cleanup(diagnostics_, diagnostic, routeMemorySnapshot);
  WiFiClient plainClient;
  WiFiClientSecure client;
  // The ESP32 has no trusted wall clock before ADS-B data arrives. HTTPS still
  // encrypts transport, but certificate verification is therefore disabled.
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(8000);
  const bool usesTls = url_.startsWith("https://");
  const bool begun = usesTls ? http.begin(client, url_) : http.begin(plainClient, url_);
  if (!begun) {
    result.error = "Ungültige Routen-Adresse";
    diagnostic.after = routeMemorySnapshot();
    diagnostic.durationMs = millis() - diagnostic.startedMs;
    diagnostics_.rememberFailure(diagnostic);
    return result;
  }

  JsonDocument request(&temporaryJsonAllocator());
  auto appendPlane = [&](const char *flight, const RouteContext &position) {
    JsonObject plane = request["planes"].add<JsonObject>();
    plane["callsign"] = flight;
    plane["lat"] = position.hasPosition ? position.latitude : 0.0;
    plane["lng"] = position.hasPosition ? position.longitude : 0.0;
  };
  if (batch) {
    for (const PrimaryEntry &entry : primaryQueue_)
      if (!entry.callsign.isEmpty() && !entry.ready)
        appendPlane(entry.callsign.c_str(), entry.context);
  } else appendPlane(callsign.c_str(), context);
  PsramBuffer body;
  serializeJson(request, body);
  if (request.overflowed() || body.failed()) {
    result.error="Routen-JSON: PSRAM allocation failed";
    return result;
  }
  http.setUserAgent(routeUserAgent);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Accept", "application/json");
  const int status = captureRouteHttp(diagnostic, client,
      [&]() { NetworkIoScope io; return http.POST(reinterpret_cast<uint8_t*>(body.data()), body.length()); }, routeMemorySnapshot, []() { return millis(); }, usesTls);
  if (networkOperationCancelled()) return result;
  if (test) test->httpCode = status;
  diagnostics_.rememberFailure(diagnostic);
  if (status != HTTP_CODE_OK) {
    result.error = "Routen-HTTP " + String(status);
    if (test && status > 0) captureRouteTestHttpError(*test, http);
    { NetworkIoScope io; http.end(); }
    return result;
  }

  JsonDocument document(&temporaryJsonAllocator());
  DeserializationError error;
  { NetworkIoScope io; error = deserializeJson(document, http.getStream()); }
  if (networkOperationCancelled()) return result;
  { NetworkIoScope io; http.end(); }
  if (networkOperationCancelled()) return result;
  if (error) {
    result.error = "Routen-JSON: " + String(error.c_str());
    return result;
  }
  if (test) captureRouteTestJson(*test, document);

  if (batch) {
    for (PrimaryEntry &entry : primaryQueue_) {
      if (entry.callsign.isEmpty() || entry.ready) continue;
      RouteResult decoded = decodePrimaryRoute(document, String(entry.callsign),
                                                entry.context, nullptr, true);
      const PsramTextTransaction transaction;
      entry.result = decoded;
      if (!transaction.succeeded()) {
        entry.result = StoredRouteResult{};
        entry.storageFailed = true;
      }
      entry.completedMs = millis();
      entry.ready = true;
    }
  }
  result = decodePrimaryRoute(document, callsign, context, test, batch);
  result.source = primarySourceName(url_);
  return result;
}

RouteResult RouteClient::fetchAdsbDb(const String& callsign,
                                     const RouteContext& context) {
  RouteResult result;
  result.source = "adsbdb";
  RouteTestResponse *test = captureTestResponses_ ? &testResponses_[1] : nullptr;
  if (test) { test->attempted = true; test->source = result.source; }
  auto &diagnostic = diagnostics_.fallback;
  diagnostic.begin("adsbdb", callsign.c_str(), millis(), routeMemorySnapshot());
  RouteCleanupCapture cleanup(diagnostics_, diagnostic, routeMemorySnapshot);
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(8000);
  if (!http.begin(client, String(adsbDbUrl) + callsign)) {
    result.error = "Ungültige ADSBDB-Adresse";
    diagnostic.after = routeMemorySnapshot();
    diagnostic.durationMs = millis() - diagnostic.startedMs;
    diagnostics_.rememberFailure(diagnostic);
    return result;
  }
  http.addHeader("Accept", "application/json");
  const int status = captureRouteHttp(diagnostic, client,
      [&]() { NetworkIoScope io; return http.GET(); }, routeMemorySnapshot, []() { return millis(); });
  if (networkOperationCancelled()) return result;
  if (test) test->httpCode = status;
  diagnostics_.rememberFailure(diagnostic);
  if (status == HTTP_CODE_NOT_FOUND) {
    result.status = "not_found";
    if (test) captureRouteTestHttpError(*test, http);
    { NetworkIoScope io; http.end(); }
    return result;
  }
  if (status != HTTP_CODE_OK) {
    result.error = "ADSBDB-HTTP " + String(status);
    if (test && status > 0) captureRouteTestHttpError(*test, http);
    { NetworkIoScope io; http.end(); }
    return result;
  }

  JsonDocument document(&temporaryJsonAllocator());
  DeserializationError error;
  { NetworkIoScope io; error = deserializeJson(document, http.getStream()); }
  if (networkOperationCancelled()) return result;
  { NetworkIoScope io; http.end(); }
  if (networkOperationCancelled()) return result;
  if (error) {
    result.error = "ADSBDB-JSON: " + String(error.c_str());
    return result;
  }
  if (test) captureRouteTestJson(*test, document);

  JsonObjectConst route =
      document["response"]["flightroute"].as<JsonObjectConst>();
  if (route.isNull()) {
    result.status = "not_found";
    return result;
  }
  if (test) {
    test->returnedCallsign = upperTrimmed(route["callsign"] | "");
    String codes;
    appendRouteCode(codes, airportCode(route["origin"]));
    JsonVariantConst midpoint = route["midpoint"];
    if (midpoint.is<JsonArrayConst>()) {
      for (JsonVariantConst item : midpoint.as<JsonArrayConst>())
        appendRouteCode(codes, airportCode(item));
    } else appendRouteCode(codes, airportCode(midpoint));
    appendRouteCode(codes, airportCode(route["destination"]));
    test->routeCodes = codes;
  }
  const String returnedCallsign = upperTrimmed(route["callsign"] | "");
  if (!returnedCallsign.isEmpty() &&
      !routeCallsignMatches(callsign.c_str(), returnedCallsign.c_str())) {
    result.source = "adsbdb-rejected";
    result.status = "rejected";
    result.error = "ADSBDB-Callsign stimmt nicht mit der Anfrage überein";
    return result;
  }
  if (context.hasPosition &&
      !adsbDbRoutePlausible(route, context.latitude, context.longitude)) {
    result.source = "adsbdb-rejected";
    result.status = "rejected";
    result.error = "ADSBDB-Route passt nicht zur Flugposition";
    return result;
  }

  appendRouteCode(result.routeCodes, airportCode(route["origin"]));
  JsonVariantConst midpoint = route["midpoint"];
  if (midpoint.is<JsonArrayConst>()) {
    for (JsonVariantConst item : midpoint.as<JsonArrayConst>())
      appendRouteCode(result.routeCodes, airportCode(item));
  } else {
    appendRouteCode(result.routeCodes, airportCode(midpoint));
  }
  appendRouteCode(result.routeCodes, airportCode(route["destination"]));
  result.airlineCode = upperTrimmed(route["airline"]["icao"] | "");
  fillNames(result);
  result.status = result.found() ? "current" : "not_found";
  String reason;
  if (result.found() && !routePlausible(result, context, reason))
    rejectRoute(result, reason);
  else
    assessRouteConfidence(result, context.hasPosition, false);
  return result;
}

RouteResult RouteClient::fetch(const String& callsign,
                               const RouteContext& context) {
  RouteResult primary = fetchPrimary(callsign, context);
  if (networkOperationCancelled()) return {};
  if (captureTestResponses_ && primary.status != "pending") {
    testResponses_[0].status = primary.status;
    testResponses_[0].error = primary.error;
  }
  if (primary.status == "pending" || primary.found() || !adsbDbFallbackEnabled_) return primary;

  RouteResult fallback = fetchAdsbDb(callsign, context);
  if (networkOperationCancelled()) return {};
  if (captureTestResponses_) {
    testResponses_[1].status = fallback.status;
    testResponses_[1].error = fallback.error;
  }
  if (fallback.found()) return fallback;
  if (primary.error.length() && fallback.error.length())
    fallback.error = primary.error + "; " + fallback.error;
  else if (primary.error.length())
    fallback.error = primary.error;
  return fallback;
}

RouteResult RouteClient::lookup(const String& callsign,
                                const RouteContext& context, uint32_t now) {
  if (!enabled_ || callsign.isEmpty()) {
    lastError_ = "";
    return {};
  }
  const bool ready = primaryResultReady(callsign);
  RouteResult cached = ready ? RouteResult{} : readMemoryCache(callsign, context, now);
  if (cached.status != "unavailable") {
    if (cached.status != "rejected") {
      lastError_ = "";
      return cached;
    }
  }
  cached = ready ? RouteResult{} : readCache(callsign, context, now);
  if (cached.status != "unavailable") {
    if (cached.status != "rejected") {
      rememberInMemory(callsign, cached, now, false);
      lastError_ = "";
      return cached;
    }
  }

  RouteResult result = fetch(callsign, context);
  if (networkOperationCancelled()) return {};
  lastError_ = result.error;
  if (result.error.isEmpty() && result.status != "pending") {
    // ADSBDB is a runtime fallback only. Keep its data in RAM and do not copy
    // it into the controller's persistent route database.
    const bool persist = !result.source.startsWith("adsbdb");
    rememberInMemory(callsign, result, now, persist);
  }
  return result;
}

RouteResult RouteClient::test(const String& callsign,
                              const RouteContext& context) {
  if (!manualPrimaryWaiting_) {
    clearTestResponses();
    // A manual test bypasses even an already completed queued response.
    for (PrimaryEntry &entry : primaryQueue_)
      if (entry.callsign == callsign) entry = {};
  }
  if (callsign.isEmpty()) {
    lastError_ = "Test-Callsign fehlt";
    RouteResult result;
    result.error = lastError_;
    return result;
  }
  captureTestResponses_ = true;
  RouteResult result = fetch(callsign, context);
  captureTestResponses_ = false;
  manualPrimaryWaiting_ = result.status == "pending";
  lastError_ = result.error;
  return result;
}
