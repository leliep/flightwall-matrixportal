#pragma once
#include <cstdint>
#include <cstring>
#include <cstdlib>

namespace RadarSweep {
constexpr uint32_t PeriodMs = 800;
constexpr uint32_t DrawIntervalMs = 25;
constexpr uint8_t DotCount = 32;

constexpr uint16_t MinimumMs = 800, MaximumMs = 4000;
inline uint16_t duration(int value) {
  if (value < MinimumMs) value = MinimumMs;
  if (value > MaximumMs) value = MaximumMs;
  return static_cast<uint16_t>((value + 50) / 100 * 100);
}
inline bool parseSeconds(const char *value, uint16_t &result) {
  if (!value) return false;
  char *end = nullptr;
  const double seconds = std::strtod(value, &end);
  if (end == value || *end || !(seconds >= 0.8 && seconds <= 4.0)) return false;
  result = static_cast<uint16_t>(seconds * 10 + 0.5) * 100;
  return true;
}
inline uint32_t cycleMs(uint8_t intervalSeconds, uint16_t durationMs) {
  if (!intervalSeconds) return 0;
  const uint32_t intervalMs = static_cast<uint32_t>(intervalSeconds) * 1000;
  return intervalMs > durationMs ? intervalMs : durationMs;
}
// More angular samples keep slower sweeps smooth at the same frame rate.
inline uint8_t sampleCount(uint16_t durationMs = PeriodMs) {
  return duration(durationMs) / DrawIntervalMs;
}
// Include both 12-o'clock endpoints within the selected sweep duration.
inline uint8_t step(uint32_t elapsedMs, uint16_t durationMs = PeriodMs) {
  const uint32_t endpointMs = duration(durationMs) - DrawIntervalMs;
  const uint8_t count = sampleCount(durationMs);
  if (elapsedMs >= endpointMs) return count;
  const uint32_t position = (elapsedMs * count + endpointMs / 2) / endpointMs;
  return position > count ? count : static_cast<uint8_t>(position);
}
inline const char *styleName(bool line) { return line ? "line" : "dot"; }
inline bool parseStyle(const char *value, bool &line) {
  if (!value) return false;
  if (std::strcmp(value, "dot") == 0) { line = false; return true; }
  if (std::strcmp(value, "line") == 0) { line = true; return true; }
  return false;
}
}  // namespace RadarSweep
