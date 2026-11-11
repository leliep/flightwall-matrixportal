#pragma once

#include <cstring>

// Inputs are trimmed uppercase callsigns. Only an ICAO airline prefix followed
// entirely by digits may ignore leading zeroes; registrations/suffixes stay exact.
inline bool routeCallsignMatches(const char* requested, const char* returned) {
  if (std::strcmp(requested, returned) == 0) return true;
  const size_t requestedLength = std::strlen(requested);
  const size_t returnedLength = std::strlen(returned);
  if (requestedLength < 4 || returnedLength < 4) return false;
  for (size_t i = 0; i < 3; ++i) {
    if (requested[i] < 'A' || requested[i] > 'Z' ||
        requested[i] != returned[i]) return false;
  }
  for (size_t i = 3; i < requestedLength; ++i)
    if (requested[i] < '0' || requested[i] > '9') return false;
  for (size_t i = 3; i < returnedLength; ++i)
    if (returned[i] < '0' || returned[i] > '9') return false;
  requested += 3;
  returned += 3;
  while (requested[0] == '0' && requested[1]) ++requested;
  while (returned[0] == '0' && returned[1]) ++returned;
  return std::strcmp(requested, returned) == 0;
}
