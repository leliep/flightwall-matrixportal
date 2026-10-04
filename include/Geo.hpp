#pragma once

#include <Arduino.h>

float distanceKm(double lat1, double lon1, double lat2, double lon2);
float bearingDegrees(double lat1, double lon1, double lat2, double lon2);
String compassDirection(float bearing);
float relativeBearing(float bearing, float viewHeading);
String lookHint(float relative);

struct CpaResult {
  bool valid = false;
  float distanceKm = 0.0F;
  float minutes = 0.0F;
  String motion = "unknown";
};

CpaResult calculateCpa(double siteLat, double siteLon, double aircraftLat,
                       double aircraftLon, float speedKn, float trackDegrees);

