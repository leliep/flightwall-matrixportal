#pragma once

#include <ArduinoJson.h>

#include "Aircraft.hpp"

struct ProcessingConfig {
  double latitude = 0.0;
  double longitude = 0.0;
  float maxPositionAgeSeconds = 15.0F;
  float radiusKm = 10.0F;
  bool hideSportAircraft = false;
  int maximumAltitudeFt = 0;
};

bool aircraftFromJson(JsonObjectConst raw, const ProcessingConfig& config,
                      Aircraft& aircraft);
bool selectAircraft(const std::vector<Aircraft>& aircraft,
                    const ProcessingConfig& config, Aircraft& selected);
