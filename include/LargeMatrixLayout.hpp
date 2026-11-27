#pragma once
#include <cstdint>
#include <cstring>
namespace LargeMatrixLayout {
constexpr uint8_t Split = 0, Full = 1;
inline uint8_t valid(uint8_t value) { return value <= Full ? value : Full; }
inline const char *name(uint8_t value) { return value == Split ? "split" : "full"; }
inline bool parse(const char *text, uint8_t &result) {
  if (!text) return false;
  if (!std::strcmp(text, "split")) result = Split;
  else if (!std::strcmp(text, "full")) result = Full;
  else return false;
  return true;
}
// Physical pixel travel with the same pauses/forward/return timing as 32-row pages.
inline int16_t travel(unsigned characters, int16_t width, uint8_t size) {
  const int32_t extra = static_cast<int32_t>(characters) * 6 * size - width;
  return extra > 0 ? static_cast<int16_t>(extra) : 0;
}
inline uint32_t duration(unsigned characters, int16_t width, uint8_t size, uint16_t forwardMs = 80) {
  const int16_t pixels = travel(characters, width, size);
  return pixels ? 2400U + pixels * (forwardMs + (forwardMs / 4 ? forwardMs / 4 : 1)) : 0;
}
inline int16_t offset(unsigned characters, int16_t width, uint8_t size, uint32_t elapsed, uint16_t forwardMs = 80) {
  const int16_t pixels = travel(characters, width, size);
  const uint16_t returnMs = forwardMs / 4 ? forwardMs / 4 : 1;
  if (!pixels || elapsed < 800) return 0;
  elapsed -= 800;
  if (elapsed < static_cast<uint32_t>(pixels) * forwardMs) return -static_cast<int16_t>(elapsed / forwardMs);
  elapsed -= static_cast<uint32_t>(pixels) * forwardMs;
  if (elapsed < 800) return -pixels;
  elapsed -= 800;
  if (elapsed < static_cast<uint32_t>(pixels) * returnMs) return -pixels + static_cast<int16_t>(elapsed / returnMs);
  return 0;
}
}
