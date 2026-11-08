#pragma once

#include <ArduinoJson.h>

#include "Aircraft.hpp"

struct ProcessingConfig {
  double latitude = 0.0;
  double longitude = 0.0;
  float maxPositionAgeSeconds = 15.0F;
  float radiusKm = 10.0F;
  // Optional acquisition radius for views that need more surrounding
  // traffic than the selection logic. Zero retains the historic 2x radius.
  float candidateRadiusKm = 0.0F;
  bool hideSportAircraft = false;
  int maximumAltitudeFt = 0;
};

bool aircraftFromJson(JsonObjectConst raw, const ProcessingConfig& config,
                      Aircraft& aircraft);
uint64_t aircraftIdentity(const Aircraft& aircraft);
uint64_t aircraftIdentityFromJson(JsonObjectConst raw);
bool isEmergencySquawk(const String& squawk);
bool selectAircraft(const std::vector<Aircraft>& aircraft,
                    const ProcessingConfig& config, Aircraft& selected,
                    String& selectedEmergencyHex, bool advanceEmergency,
                    size_t& emergencyCount, size_t& emergencyIndex);
