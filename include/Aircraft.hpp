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
  String squawk;
  uint32_t dbFlags = 0;

  bool hasPosition = false;
  double latitude = 0.0;
  double longitude = 0.0;
  float distanceKm = 0.0F;
  float bearing = 0.0F;
  String direction;

  bool hasAltitude = false;
  int altitudeFt = 0;
  bool onGround = false;
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
  bool emergency = false;
  std::vector<String> specialTypes;
};

constexpr float AIRCRAFT_STANDING_MAX_SPEED_KMH = 3.0F;

inline bool aircraftIsStanding(const Aircraft& aircraft) {
  return aircraft.onGround && aircraft.hasSpeed &&
         aircraft.speedKn * 1.852F <= AIRCRAFT_STANDING_MAX_SPEED_KMH;
}

inline bool aircraftIsTaxiing(const Aircraft& aircraft) {
  return aircraft.onGround && aircraft.hasSpeed &&
         aircraft.speedKn * 1.852F > AIRCRAFT_STANDING_MAX_SPEED_KMH;
}
