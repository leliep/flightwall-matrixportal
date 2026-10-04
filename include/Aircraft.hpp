#pragma once

#include <Arduino.h>
#include <vector>

struct Aircraft {
  String hex;
  String flight;
  String registration;
  String type;
  String description;
  String airline;
  String category;
  uint32_t dbFlags = 0;

  double latitude = 0.0;
  double longitude = 0.0;
  float distanceKm = 0.0F;
  float bearing = 0.0F;
  String direction;

  bool hasAltitude = false;
  int altitudeFt = 0;
  bool hasSpeed = false;
  float speedKn = 0.0F;
  bool hasTrack = false;
  float track = 0.0F;
  bool hasVerticalRate = false;
  int verticalRateFpm = 0;

  bool hasCpa = false;
  float cpaKm = 0.0F;
  bool hasCpaMinutes = false;
  float cpaMinutes = 0.0F;
  String motion = "unknown";

  bool sportAircraft = false;
  std::vector<String> specialTypes;
};

