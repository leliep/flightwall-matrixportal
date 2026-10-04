#include "Geo.hpp"

#include <cmath>

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kEarthRadiusKm = 6371.0088;
double toRadians(double degreeValue) { return degreeValue * kPi / 180.0; }
double toDegrees(double radianValue) { return radianValue * 180.0 / kPi; }
}  // namespace

float distanceKm(double lat1, double lon1, double lat2, double lon2) {
  const double p1 = toRadians(lat1);
  const double p2 = toRadians(lat2);
  const double dp = toRadians(lat2 - lat1);
  const double dl = toRadians(lon2 - lon1);
  const double a = std::pow(std::sin(dp / 2.0), 2.0) +
                   std::cos(p1) * std::cos(p2) *
                       std::pow(std::sin(dl / 2.0), 2.0);
  return static_cast<float>(kEarthRadiusKm * 2.0 *
                            std::atan2(std::sqrt(a), std::sqrt(1.0 - a)));
}

float bearingDegrees(double lat1, double lon1, double lat2, double lon2) {
  const double p1 = toRadians(lat1);
  const double p2 = toRadians(lat2);
  const double dl = toRadians(lon2 - lon1);
  const double y = std::sin(dl) * std::cos(p2);
  const double x = std::cos(p1) * std::sin(p2) -
                   std::sin(p1) * std::cos(p2) * std::cos(dl);
  double result = std::fmod(toDegrees(std::atan2(y, x)) + 360.0, 360.0);
  return static_cast<float>(result);
}

String compassDirection(float bearing) {
  static const char* directions[] = {"N", "NE", "E", "SE",
                                     "S", "SW", "W", "NW"};
  const int index = static_cast<int>((bearing + 22.5F) / 45.0F) % 8;
  return directions[index];
}

float relativeBearing(float bearing, float viewHeading) {
  float result = std::fmod(bearing - viewHeading + 540.0F, 360.0F) - 180.0F;
  return result;
}

String lookHint(float relative) {
  const float absolute = std::fabs(relative);
  if (absolute <= 12.0F) return "Schau geradeaus.";
  if (absolute <= 50.0F)
    return relative > 0 ? "Schau nach rechts vorne."
                        : "Schau nach links vorne.";
  if (absolute <= 105.0F)
    return relative > 0 ? "Schau nach rechts." : "Schau nach links.";
  if (absolute <= 145.0F)
    return relative > 0 ? "Schau nach rechts hinten."
                        : "Schau nach links hinten.";
  if (absolute <= 170.0F)
    return relative > 0 ? "Schau über die rechte Schulter."
                        : "Schau über die linke Schulter.";
  return "Schau direkt hinter dich.";
}

CpaResult calculateCpa(double siteLat, double siteLon, double aircraftLat,
                       double aircraftLon, float speedKn, float trackDegrees) {
  CpaResult result;
  if (speedKn <= 0.0F) return result;

  const double meanLat = toRadians((siteLat + aircraftLat) / 2.0);
  const double east = (aircraftLon - siteLon) * 111.320 * std::cos(meanLat);
  const double north = (aircraftLat - siteLat) * 110.574;
  const double speedKmh = speedKn * 1.852;
  const double track = toRadians(trackDegrees);
  const double velocityEast = speedKmh * std::sin(track);
  const double velocityNorth = speedKmh * std::cos(track);
  const double velocitySquared = velocityEast * velocityEast +
                                 velocityNorth * velocityNorth;
  if (velocitySquared <= 0.0) return result;

  const double dot = east * velocityEast + north * velocityNorth;
  const double hours = -dot / velocitySquared;
  const double futureHours = hours > 0.0 ? hours : 0.0;
  const double closestEast = east + velocityEast * futureHours;
  const double closestNorth = north + velocityNorth * futureHours;

  result.valid = true;
  result.distanceKm = static_cast<float>(
      std::sqrt(closestEast * closestEast + closestNorth * closestNorth));
  result.minutes = static_cast<float>(hours * 60.0);
  result.motion = dot < 0.0 ? "approaching" : "departing";
  return result;
}
