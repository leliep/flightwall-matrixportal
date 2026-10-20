#pragma once

#include <Arduino.h>

inline bool isGermanAirforceCallsign(const String& callsign) {
  // Standard and already observed abbreviated Luftwaffe prefixes.
  if (callsign.startsWith("GAF") || callsign.startsWith("VAD") ||
      callsign.startsWith("SHO")) {
    return true;
  }

  // Tactical formation and mission callsigns. Match the complete codeword
  // to avoid confusing civil ICAO operator codes such as HAM and BUR.
  static const char* const tacticalCallsigns[] = {
      "BURRO", "HAMMER", "SMASH", "VADER",
      "CONAN", "FALCON", "THOR", "VIRUS"};
  for (const char* prefix : tacticalCallsigns) {
    if (callsign.startsWith(prefix)) return true;
  }
  return false;
}
