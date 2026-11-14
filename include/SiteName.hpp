#pragma once
#include <stddef.h>
#include <stdint.h>

// Optional plain text: at most 48 Unicode scalar values, no controls or NUL.
inline bool validSiteName(const char* text, size_t bytes) {
  if (bytes > 48 * 4 || (!text && bytes)) return false;
  const auto* p = reinterpret_cast<const unsigned char*>(text);
  size_t count = 0;
  for (size_t i = 0; i < bytes;) {
    uint32_t cp = p[i++], minimum = 0;
    unsigned rest = 0;
    if (cp < 0x80) { if (cp < 32 || cp == 127) return false; }
    else if (cp >= 0xc2 && cp <= 0xdf) { cp &= 31; rest = 1; minimum = 0x80; }
    else if (cp >= 0xe0 && cp <= 0xef) { cp &= 15; rest = 2; minimum = 0x800; }
    else if (cp >= 0xf0 && cp <= 0xf4) { cp &= 7; rest = 3; minimum = 0x10000; }
    else return false;
    while (rest--) {
      if (i == bytes || (p[i] & 0xc0) != 0x80) return false;
      cp = (cp << 6) | (p[i++] & 63);
    }
    if (cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff) ||
        (cp >= 0x80 && cp <= 0x9f) || ++count > 48) return false;
  }
  return true;
}
