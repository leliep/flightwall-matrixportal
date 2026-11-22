#pragma once
#include <cmath>
#include <cstdio>
#include <cstddef>

// Number inputs reject padded values such as Arduino String(0.0f, 0) == " 0".
// Fixed formatting without field width preserves precision and produces valid HTML.
inline bool numericInputText(char *out, size_t size, double value, unsigned decimals) {
  if (!out || !size || !std::isfinite(value) || decimals > 9) return false;
  if (value == 0) value = 0; // Normalize negative zero.
  const int count = std::snprintf(out, size, "%.*f", static_cast<int>(decimals), value);
  return count > 0 && static_cast<size_t>(count) < size;
}
