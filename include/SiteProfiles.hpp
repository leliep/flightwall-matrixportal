#pragma once
#include "TemporaryPsram.hpp"
#include "SiteName.hpp"

// Bounded PSRAM-backed profiles. Legacy scalar settings remain mirrors of active.
class SiteProfiles {
 public:
  static constexpr size_t Limit = 16;
  static constexpr size_t MaxBytes = 12288;
  SiteProfiles() : data_(&temporaryJsonAllocator()) {}
  static bool valid(JsonVariantConst profiles, JsonVariantConst active);
  bool assign(JsonVariantConst profiles, JsonVariantConst active);
  bool copyFrom(const SiteProfiles &other) { return assign(other.data_["profiles"], other.data_["active"]); }
  bool decode(const char *json, size_t bytes);
  bool legacy(const String &name, double latitude, double longitude,
              double elevation, double heading);
  bool updateActive(const String &name, double latitude, double longitude,
                    double elevation, double heading);
  JsonArrayConst profiles() const { return data_["profiles"].as<JsonArrayConst>(); }
  int active() const { return data_["active"] | 0; }
  JsonObjectConst selected() const { return profiles()[active()].as<JsonObjectConst>(); }
  PsramBuffer encode() const;
  void swap(SiteProfiles &other) { using std::swap; swap(data_, other.data_); }
 private:
  JsonDocument data_;
};
PsramBuffer nearbyAirportTemplates(double latitude, double longitude);
const char *siteProfilesEditorScript();
