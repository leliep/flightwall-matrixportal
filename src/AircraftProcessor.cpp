#include "AircraftProcessor.hpp"

#include <algorithm>

#include "CallsignClassifier.hpp"
#include "Geo.hpp"

namespace {

String upperTrimmed(const char* value) {
  String result = value ? value : "";
  result.trim();
  result.toUpperCase();
  return result;
}

String normalizedRegistration(const String& value) {
  String result;
  for (size_t index = 0; index < value.length(); ++index) {
    const char character = value[index];
    if (isAlphaNumeric(character)) result += static_cast<char>(toupper(character));
  }
  return result;
}

bool containsAny(const String& text,
                 std::initializer_list<const char*> needles) {
  for (const char* needle : needles) {
    if (text.indexOf(needle) >= 0) return true;
  }
  return false;
}

void detectSpecialTypes(Aircraft& aircraft) {
  String text = aircraft.flight + " " + aircraft.registration + " " +
                aircraft.type + " " + aircraft.description + " " +
                aircraft.airline;
  text.toUpperCase();

  const bool germanAirforce = isGermanAirforceCallsign(aircraft.flight);
  if (germanAirforce)
    aircraft.specialTypes.push_back("German Airforce");
  const bool militaryCallsign = isMilitaryCallsign(aircraft.flight);
  const bool federalPolice = aircraft.flight.startsWith("BPO") ||
      containsAny(text, {"BUNDESPOLIZEI", "FEDERAL POLICE"});
  if (federalPolice) {
    aircraft.specialTypes.push_back("Bundespolizei");
  } else if (containsAny(text, {"POLICE", "POLIZEI", "POLICE AVIATION"}) ||
             aircraft.flight.startsWith("POL") ||
             aircraft.flight.startsWith("HBT") ||
             aircraft.flight.startsWith("HUM") ||
             aircraft.flight.startsWith("NRW")) {
    aircraft.specialTypes.push_back("police");
  }
  if (!germanAirforce && (militaryCallsign || (aircraft.dbFlags & 1U) ||
      containsAny(text, {"MILITARY", "LUFTWAFFE", "AIR FORCE", "ARMY",
                         "NAVY", "MARINE", "HEER"})))
    aircraft.specialTypes.push_back("military");
  if (containsAny(text, {"GOVERNMENT", "GOVT", "REGIERUNG",
                         "BUNDESREPUBLIK", "STAATSFLUG", "STATE OF"}))
    aircraft.specialTypes.push_back("government");
  if (aircraft.category == "A7" ||
      containsAny(text, {"HELICOPTER", "HELIKOPTER", "ROTORCRAFT",
                         "ROTOR CRAFT"}))
    aircraft.specialTypes.push_back("helicopter");
  if (aircraft.flight == "DLZFN" ||
      containsAny(text, {"AIRSHIP", "LUFTSCHIFF", "ZEPPELIN", "SHIP"}))
    aircraft.specialTypes.push_back("airship");
  if (aircraft.type == "A388" ||
      containsAny(text, {"AIRBUS A380", "AIRBUS A-380", "A380"}))
    aircraft.specialTypes.push_back("a380");
}

bool isSportAircraft(const Aircraft& aircraft) {
  if (aircraft.category == "A1") return true;
  if (aircraft.type == "PRIM" || aircraft.type == "C152") return true;

  const String registration = normalizedRegistration(aircraft.registration);
  const String flight = normalizedRegistration(aircraft.flight);
  const auto isGermanLight = [](const String& value) {
    return value.length() == 5 && value[0] == 'D' &&
           (value[1] == 'E' || value[1] == 'K' || value[1] == 'M');
  };
  if (isGermanLight(registration) || isGermanLight(flight)) return true;

  String text = aircraft.type + " " + aircraft.description;
  text.toUpperCase();
  return containsAny(text, {"GLIDER", "SAILPLANE", "ULTRALIGHT",
                            "MICROLIGHT", "LIGHT SPORT", "GYROCOPTER",
                            "GYROPLANE", "AMATEUR BUILT"});
}

bool readAltitude(JsonObjectConst raw, int& altitude) {
  JsonVariantConst barometric = raw["alt_baro"];
  if (barometric.is<const char*>() &&
      String(barometric.as<const char*>()) == "ground") {
    altitude = 0;
    return true;
  }
  if (barometric.is<float>() || barometric.is<int>()) {
    altitude = barometric.as<int>();
    return true;
  }
  JsonVariantConst geometric = raw["alt_geom"];
  if (geometric.is<float>() || geometric.is<int>()) {
    altitude = geometric.as<int>();
    return true;
  }
  return false;
}

}  // namespace

bool isEmergencySquawk(const String& squawk) {
  return squawk == "7500" || squawk == "7600" || squawk == "7700";
}

bool aircraftFromJson(JsonObjectConst raw, const ProcessingConfig& config,
                      Aircraft& aircraft) {
  aircraft.hex = upperTrimmed(raw["hex"] | "");
  aircraft.flight = upperTrimmed(raw["flight"] | "");
  aircraft.registration = upperTrimmed(raw["r"] | "");
  aircraft.type = upperTrimmed(raw["t"] | "");
  aircraft.description = String(raw["desc"] | "");
  aircraft.airline = String(raw["ownOp"] | "");
  aircraft.category = upperTrimmed(raw["category"] | "");
  aircraft.squawk = upperTrimmed(raw["squawk"] | "");
  aircraft.emergency = isEmergencySquawk(aircraft.squawk);
  aircraft.dbFlags = raw["dbFlags"] | 0U;

  if (aircraft.emergency && !raw["seen"].isNull() &&
      raw["seen"].as<float>() > config.maxPositionAgeSeconds)
    return false;

  const bool hasCoordinates = !raw["lat"].isNull() && !raw["lon"].isNull();
  const bool positionFresh = raw["seen_pos"].isNull() ||
      raw["seen_pos"].as<float>() <= config.maxPositionAgeSeconds;
  aircraft.hasPosition = hasCoordinates && positionFresh;
  if (!aircraft.emergency && !aircraft.hasPosition) return false;
  if (aircraft.hasPosition) {
    aircraft.latitude = raw["lat"].as<double>();
    aircraft.longitude = raw["lon"].as<double>();
    aircraft.distanceKm = distanceKm(config.latitude, config.longitude,
                                     aircraft.latitude, aircraft.longitude);
    aircraft.bearing = bearingDegrees(config.latitude, config.longitude,
                                      aircraft.latitude, aircraft.longitude);
    aircraft.direction = compassDirection(aircraft.bearing);
    if (!aircraft.emergency &&
        aircraft.distanceKm > config.radiusKm * 2.0F) return false;
  }

  aircraft.hasAltitude = readAltitude(raw, aircraft.altitudeFt);
  if (!aircraft.emergency && config.maximumAltitudeFt > 0 && aircraft.hasAltitude &&
      aircraft.altitudeFt > config.maximumAltitudeFt)
    return false;
  if (!raw["gs"].isNull()) {
    aircraft.hasSpeed = true;
    aircraft.speedKn = raw["gs"].as<float>();
  }
  if (!raw["track"].isNull()) {
    aircraft.hasTrack = true;
    aircraft.track = raw["track"].as<float>();
  }
  if (!raw["baro_rate"].isNull() || !raw["geom_rate"].isNull()) {
    aircraft.hasVerticalRate = true;
    aircraft.verticalRateFpm = !raw["baro_rate"].isNull()
                                   ? raw["baro_rate"].as<int>()
                                   : raw["geom_rate"].as<int>();
  }

  if (aircraft.hasPosition && aircraft.hasSpeed && aircraft.hasTrack) {
    const CpaResult cpa = calculateCpa(
        config.latitude, config.longitude, aircraft.latitude,
        aircraft.longitude, aircraft.speedKn, aircraft.track);
    aircraft.hasCpa = cpa.valid;
    aircraft.hasCpaMinutes = cpa.valid;
    aircraft.cpaKm = cpa.distanceKm;
    aircraft.cpaMinutes = cpa.minutes;
    aircraft.motion = cpa.motion;
  }

  detectSpecialTypes(aircraft);
  aircraft.sportAircraft = isSportAircraft(aircraft);
  return true;
}

bool selectAircraft(const std::vector<Aircraft>& aircraft,
                    const ProcessingConfig& config, Aircraft& selected,
                    String& selectedEmergencyHex, bool advanceEmergency,
                    size_t& emergencyCount, size_t& emergencyIndex) {
  std::vector<const Aircraft*> emergencies;
  for (const Aircraft& candidate : aircraft) {
    if (candidate.emergency) emergencies.push_back(&candidate);
  }
  std::sort(emergencies.begin(), emergencies.end(),
            [](const Aircraft* left, const Aircraft* right) {
              const String leftKey = left->hex.length() ? left->hex : left->flight;
              const String rightKey = right->hex.length() ? right->hex : right->flight;
              return leftKey < rightKey;
            });
  emergencyCount = emergencies.size();
  emergencyIndex = 0;
  if (!emergencies.empty()) {
    size_t index = 0;
    for (size_t candidateIndex = 0; candidateIndex < emergencies.size(); ++candidateIndex) {
      if (emergencies[candidateIndex]->hex == selectedEmergencyHex) {
        index = candidateIndex;
        if (advanceEmergency) index = (index + 1) % emergencies.size();
        break;
      }
    }
    selected = *emergencies[index];
    selectedEmergencyHex = selected.hex;
    emergencyIndex = index;
    return true;
  }

  selectedEmergencyHex = "";
  const Aircraft* nearest = nullptr;
  for (const Aircraft& candidate : aircraft) {
    if (!candidate.hasPosition) continue;
    if (config.hideSportAircraft && candidate.sportAircraft &&
        candidate.specialTypes.empty())
      continue;
    const bool special = !candidate.specialTypes.empty();
    const float allowedRadius = special ? config.radiusKm * 2.0F : config.radiusKm;
    if (candidate.distanceKm > allowedRadius) continue;
    if (!nearest || candidate.distanceKm < nearest->distanceKm) nearest = &candidate;
  }
  if (!nearest) return false;
  selected = *nearest;
  return true;
}
