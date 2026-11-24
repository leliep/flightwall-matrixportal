#pragma once

#include <cstddef>
#include <cstring>

// Inputs are trimmed uppercase ADS-B callsigns. Deliberately conservative:
// a missing hyphen alone is not enough to distinguish every registration from
// an ICAO operator/flight number. UP-P plus four digits is a known registration
// pattern (observed UP-P1088 / ICAO24 68328E), not a UPP operator assignment.
inline bool isRegistrationCallsign(const char* callsign) {
  const size_t length = std::strlen(callsign);
  if (length == 7 && std::strncmp(callsign, "UPP", 3) == 0) {
    bool digits = true;
    for (size_t i = 3; i < length; ++i)
      digits = digits && callsign[i] >= '0' && callsign[i] <= '9';
    if (digits) return true;
  }
  // Explicit national-prefix hyphen (D-ABCD, OE-ABC, UP-P1088, ...).
  const char* hyphen = std::strchr(callsign, '-');
  if (!hyphen || hyphen == callsign || hyphen - callsign > 3 || !hyphen[1])
    return false;
  for (const char* p = callsign; p != hyphen; ++p)
    if (!((*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9')))
      return false;
  for (const char* p = hyphen + 1; *p; ++p)
    if (!((*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9')))
      return false;
  return true;
}
