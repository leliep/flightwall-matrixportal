#pragma once

#include <Arduino.h>

struct AirportCoordinates {
  double latitude = 0.0;
  double longitude = 0.0;
  int16_t elevationFt = 0;
  bool hasElevation = false;
};

bool airportCoordinates(const String& code, AirportCoordinates& result);
