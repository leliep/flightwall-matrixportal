#include "SiteProfiles.hpp"
#include "AirportCoordinates.hpp"
#include "AirportNames.hpp"
#include "LookupOverrides.hpp"
#include "Geo.hpp"
#include <cmath>

namespace {
bool number(JsonVariantConst value, double minimum, double maximum) {
  return value.is<double>() && std::isfinite(value.as<double>()) &&
      value.as<double>() >= minimum && value.as<double>() <= maximum;
}
void fill(JsonObject item, const String &name, double latitude, double longitude,
          double elevation, double heading) {
  item["name"] = JsonString(name.c_str(), name.length(), false); item["latitude"] = latitude; item["longitude"] = longitude;
  item["elevation_m"] = elevation; item["view_heading"] = heading;
}
}

bool SiteProfiles::valid(JsonVariantConst value, JsonVariantConst active) {
  if (!value.is<JsonArrayConst>() || !active.is<int>()) return false;
  const auto profiles = value.as<JsonArrayConst>();
  if (!profiles.size() || profiles.size() > Limit || active.as<int>() < 0 ||
      static_cast<size_t>(active.as<int>()) >= profiles.size()) return false;
  for (JsonVariantConst raw : profiles) {
    if (!raw.is<JsonObjectConst>()) return false;
    const auto p = raw.as<JsonObjectConst>();
    if (p.size() != 5 || !p["name"].is<const char*>()) return false;
    const JsonString name = p["name"].as<JsonString>();
    if (!validSiteName(name.c_str(), name.size()) ||
        !number(p["latitude"], -90, 90) || !number(p["longitude"], -180, 180) ||
        !number(p["elevation_m"], -500, 9000) || !number(p["view_heading"], 0, 359)) return false;
  }
  return true;
}
bool SiteProfiles::assign(JsonVariantConst profiles, JsonVariantConst active) {
  if (!valid(profiles, active)) return false;
  JsonDocument staged(&temporaryJsonAllocator());
  staged["profiles"] = profiles; staged["active"] = active.as<int>();
  if (staged.overflowed() || measureJson(staged) > MaxBytes) return false;
  using std::swap; swap(data_, staged); return true;
}
bool SiteProfiles::decode(const char *json, size_t bytes) {
  if (!json || !bytes || bytes > MaxBytes) return false;
  JsonDocument staged(&temporaryJsonAllocator());
  if (deserializeJson(staged, json, bytes) || staged.overflowed()) return false;
  return assign(staged["profiles"], staged["active"]);
}
bool SiteProfiles::legacy(const String &name, double lat, double lon, double elevation, double heading) {
  JsonDocument staged(&temporaryJsonAllocator());
  fill(staged["profiles"].to<JsonArray>().add<JsonObject>(), name, lat, lon, elevation, heading);
  staged["active"] = 0; return assign(staged["profiles"], staged["active"]);
}
bool SiteProfiles::updateActive(const String &name, double lat, double lon, double elevation, double heading) {
  JsonDocument staged(&temporaryJsonAllocator());
  staged["profiles"] = profiles(); staged["active"] = active();
  fill(staged["profiles"][active()].as<JsonObject>(), name, lat, lon, elevation, heading);
  if (staged.overflowed()) return false;
  return assign(staged["profiles"], staged["active"]);
}
PsramBuffer SiteProfiles::encode() const {
  PsramBuffer json; serializeJson(data_, json); return json;
}

PsramBuffer nearbyAirportTemplates(double latitude, double longitude) {
  JsonDocument document(&temporaryJsonAllocator());
  auto airports = document.to<JsonArray>();
  auto add = [&](const String &code) {
    AirportCoordinates point;
    if (!airportCoordinates(code, point)) return;
    const float distance = distanceKm(latitude, longitude, point.latitude, point.longitude);
    if (!std::isfinite(distance) || distance > 150.0F) return;
    const String name = airportDisplayName(code);
    if (!name.length()) return;
    auto row = airports.add<JsonObject>();
    row["code"] = JsonString(code.c_str(), code.length(), false); row["name"] = JsonString(name.c_str(), name.length(), false);
    row["latitude"] = point.latitude; row["longitude"] = point.longitude;
    if (point.hasElevation) row["elevation_m"] = std::round(point.elevationFt * 0.3048);
    else row["elevation_m"] = nullptr;
    row["distance_km"] = distance;
  };
  for (size_t i = 0; i < airportCoordinateCount(); ++i) add(airportCoordinateCodeAt(i));
  for (size_t i = 0; i < lookupAirportCount(); ++i) {
    const String code = lookupAirportCodeAt(i);
    bool builtIn = false;
    for (size_t j = 0; j < airportCoordinateCount(); ++j)
      if (code == airportCoordinateCodeAt(j)) { builtIn = true; break; }
    if (!builtIn) add(code);
  }
  PsramBuffer json;
  if (document.overflowed()) json.reserve(PsramBuffer::maximumSize + 1);
  else serializeJson(document, json);
  return json;
}
