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
// Translate the viewport center without scaling points, ring, sweep or trails.
inline int16_t project(int16_t value, int16_t origin, int16_t center) {
  return center + value - origin;
}
}
