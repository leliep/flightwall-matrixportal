#pragma once
#include <cstdint>
#include <cstring>
namespace RadarIdle {
constexpr uint8_t Off = 0, Lower = 1, Full = 2;
inline uint8_t valid(uint8_t value) { return value <= Full ? value : Lower; }
inline const char* name(uint8_t value) {
  return value == Off ? "off" : value == Full ? "full" : "lower";
}
inline bool parse(const char* value, uint8_t& result) {
  if (!value) return false;
  if (!std::strcmp(value,"off")) result=Off;
  else if (!std::strcmp(value,"lower")) result=Lower;
  else if (!std::strcmp(value,"full")) result=Full;
  else return false;
  return true;
}
// Uniform 31/15 zoom preserves round geometry, including stored trail pixels.
inline int16_t project(int16_t value, int16_t origin, int16_t center) {
  const int32_t n = static_cast<int32_t>(value - origin) * 31;
  return center + (n >= 0 ? n + 7 : n - 7) / 15;
}
}
