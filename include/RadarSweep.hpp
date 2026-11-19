#pragma once
#include <cstdint>
#include <cstring>

namespace RadarSweep {
constexpr uint32_t PeriodMs = 800;
constexpr uint32_t DrawIntervalMs = 25;
constexpr uint8_t DotCount = 32;

// Include both 12-o'clock endpoints in the visible 800-ms sweep.
inline uint8_t step(uint32_t elapsedMs) {
  constexpr uint32_t endpointMs = PeriodMs - DrawIntervalMs;
  if (elapsedMs >= endpointMs) return DotCount;
  const uint32_t position = (elapsedMs * DotCount + endpointMs / 2) / endpointMs;
  return position > DotCount ? DotCount : static_cast<uint8_t>(position);
}
inline const char *styleName(bool line) { return line ? "line" : "dot"; }
inline bool parseStyle(const char *value, bool &line) {
  if (!value) return false;
  if (std::strcmp(value, "dot") == 0) { line = false; return true; }
  if (std::strcmp(value, "line") == 0) { line = true; return true; }
  return false;
}
}  // namespace RadarSweep
