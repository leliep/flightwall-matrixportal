#include "AircraftTypeNames.hpp"

String resolvedAircraftTypeDescription(const String &code, const String &provided) {
  String description = provided;
  description.trim();
  String normalized = description;
  normalized.toUpperCase();
  String type = code;
  type.trim();
  type.toUpperCase();
  const bool generic = normalized.isEmpty() || normalized == type ||
      normalized == "UNKNOWN" || normalized == "N/A" || normalized == "HELICOPTER" ||
      normalized == "HELIKOPTER" || normalized == "HUBSCHRAUBER" ||
      normalized == "ROTORCRAFT" || normalized == "ROTOR CRAFT";
  if (!generic) return description;
  const String fallback = aircraftTypeDisplayName(type);
  if (fallback.length()) return fallback;
  return type.length() ? type : description;
}
