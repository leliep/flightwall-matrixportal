#include "AdsbClient.hpp"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include <algorithm>
#include <utility>

AdsbResult AdsbClient::fetch(const ProcessingConfig& config) {
  AdsbResult result;
  HTTPClient http;
  WiFiClient plainClient;
  WiFiClientSecure secureClient;
  // ArduinoJson reads HTTPClient's underlying stream directly. HTTP/1.1
  // chunk markers would otherwise be exposed to the JSON parser by services
  // such as adsb.fi/Cloudflare. HTTP/1.0 requests a connection-delimited body
  // without transfer-encoding chunks and keeps parsing memory-efficient.
  http.useHTTP10(true);
  http.setConnectTimeout(4000);
  http.setTimeout(6000);
  const bool secure = url_.startsWith("https://");
  if (secure) {
    // The controller may not have a trusted wall clock before the first ADS-B
    // response. The connection is encrypted, but the certificate cannot be
    // validated reliably at that point.
    secureClient.setInsecure();
  }
  const bool began = secure ? http.begin(secureClient, url_)
                            : http.begin(plainClient, url_);
  if (!began) {
    result.error = "Ungültige ADS-B-Adresse";
    return result;
  }

  http.addHeader("Accept", "application/json");
  http.addHeader("User-Agent",
                 "FlightWall-MatrixPortal/3 (+https://github.com/leliep/flightwall-matrixportal)");

  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    result.error = "ADS-B HTTP " + String(status);
    http.end();
    return result;
  }

  JsonDocument filter;
  filter["now"] = true;
  const char* fields[] = {"hex",       "flight",    "r",         "t",
                          "desc",      "ownOp",     "category",  "dbFlags",
                          "lat",       "lon",       "alt_baro",  "alt_geom",
                          "gs",        "track",     "baro_rate", "geom_rate",
                          "seen",      "seen_pos", "squawk"};
  for (const char* field : fields) {
    filter["aircraft"][0][field] = true;
    filter["ac"][0][field] = true;
  }

  JsonDocument document;
  DeserializationError error = deserializeJson(
      document, http.getStream(), DeserializationOption::Filter(filter));
  if (error) {
    result.error = "ADS-B JSON: " + String(error.c_str());
    http.end();
    return result;
  }

  const bool hasReadsbAircraft = document["aircraft"].is<JsonArrayConst>();
  const bool hasAdsbFiAircraft = document["ac"].is<JsonArrayConst>();
  if (!hasReadsbAircraft && !hasAdsbFiAircraft) {
    result.error = "ADS-B JSON: Format";
    http.end();
    return result;
  }

  // readsb exposes `now` as a JSON floating-point Unix timestamp. Reading it
  // with an unsigned default can fail ArduinoJson's type check and yield zero.
  double feedTimestamp = document["now"].isNull()
                             ? 0.0
                             : document["now"].as<double>();
  // readsb uses seconds, while the adsb.fi v3 API uses milliseconds.
  if (feedTimestamp > 100000000000.0) feedTimestamp /= 1000.0;
  result.feedTimestamp = static_cast<uint32_t>(feedTimestamp);
  JsonArrayConst rawAircraft = hasReadsbAircraft
                                   ? document["aircraft"].as<JsonArrayConst>()
                                   : document["ac"].as<JsonArrayConst>();
  result.receivedAircraft = rawAircraft.size();
  result.candidates.reserve(std::min<size_t>(rawAircraft.size(), 64));
  for (JsonObjectConst raw : rawAircraft) {
    Aircraft aircraft;
    if (aircraftFromJson(raw, config, aircraft))
      result.candidates.push_back(std::move(aircraft));
  }
  result.ok = true;
  http.end();
  return result;
}
