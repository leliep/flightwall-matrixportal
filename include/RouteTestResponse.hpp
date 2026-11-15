#pragma once
#include "PsramText.hpp"

// Only populated by the manual, uncached test. Never part of a route/cache.
struct RouteTestResponse {
  static constexpr size_t jsonLimit = 8192;
  PsramText json;
  PsramText source, returnedCallsign, routeCodes, status, error;
  int httpCode = 0;
  bool attempted = false;
  bool truncated = false;
  bool captureFailed = false;
};
