#include "AdsbClient.hpp"

#include <ArduinoJson.h>
#include <HTTPClient.h>

#include <algorithm>
#include <utility>

AdsbResult AdsbClient::fetch(const ProcessingConfig& config) {
  AdsbResult result;
  HTTPClient http;
  http.setConnectTimeout(4000);
  http.setTimeout(6000);
  if (!http.begin(url_)) {
    result.error = "Ungültige ADS-B-Adresse";
    return result;
  }

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
                          "seen_pos"};
  for (const char* field : fields) filter["aircraft"][0][field] = true;

  JsonDocument document;
  DeserializationError error = deserializeJson(
      document, http.getStream(), DeserializationOption::Filter(filter));
  if (error) {
    result.error = "ADS-B JSON: " + String(error.c_str());
    http.end();
    return result;
  }

  // readsb exposes `now` as a JSON floating-point Unix timestamp. Reading it
  // with an unsigned default can fail ArduinoJson's type check and yield zero.
  result.feedTimestamp =
      document["now"].isNull()
          ? 0U
          : static_cast<uint32_t>(document["now"].as<double>());
  JsonArrayConst rawAircraft = document["aircraft"].as<JsonArrayConst>();
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
