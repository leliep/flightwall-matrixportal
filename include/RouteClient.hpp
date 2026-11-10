#pragma once

#include <Arduino.h>
#include <vector>
#include "RouteDiagnostics.hpp"
#include "PsramText.hpp"

RouteMemorySnapshot routeMemorySnapshot();

struct RouteResult {
  String routeCodes;
  String routeNames;
  String origin;
  String destination;
  String originName;
  String destinationName;
  String airlineCode;
  String source;
  String status = "unavailable";
  String error;
  uint8_t confidence = 0;
  String confidenceReason;

  bool found() const { return !origin.isEmpty() || !destination.isEmpty(); }
};

// The cache owns PSRAM texts; conversion to RouteResult creates only the
// short-lived/current Arduino strings needed by existing network/display code.
struct StoredRouteResult {
  PsramText routeCodes;
  PsramText routeNames;
  PsramText origin;
  PsramText destination;
  PsramText originName;
  PsramText destinationName;
  PsramText airlineCode;
  PsramText source;
  PsramText status;
  PsramText error;
  PsramText confidenceReason;
  uint8_t confidence = 0;
  bool found() const { return !origin.isEmpty() || !destination.isEmpty(); }
  StoredRouteResult &operator=(const RouteResult &value) {
    routeCodes = value.routeCodes;
    routeNames = value.routeNames;
    origin = value.origin;
    destination = value.destination;
    originName = value.originName;
    destinationName = value.destinationName;
    airlineCode = value.airlineCode;
    source = value.source;
    status = value.status;
    error = value.error;
    confidenceReason = value.confidenceReason;
    confidence = value.confidence;
    return *this;
  }
  operator RouteResult() const {
    RouteResult value;
    value.routeCodes = String(routeCodes);
    value.routeNames = String(routeNames);
    value.origin = String(origin);
    value.destination = String(destination);
    value.originName = String(originName);
    value.destinationName = String(destinationName);
    value.airlineCode = String(airlineCode);
    value.source = String(source);
    value.status = String(status);
    value.error = String(error);
    value.confidenceReason = String(confidenceReason);
    value.confidence = confidence;
    return value;
  }
};

struct RouteContext {
  bool hasPosition = false;
  double latitude = 0.0;
  double longitude = 0.0;
  bool hasAltitude = false;
  float altitudeFt = 0.0F;
  bool hasTrack = false;
  float trackDegrees = 0.0F;
  bool hasVerticalRate = false;
  float verticalRateFpm = 0.0F;
};

struct RouteCacheEntryInfo {
  String callsign;
  String routeCodes;
  String source;
  uint32_t updated = 0;
  bool found = false;
  bool inMemory = false;
  bool persistent = false;
  bool pendingWrite = false;
  bool expired = false;
};

class RouteClient {
 public:
  RouteClient() = default;
  void configure(bool enabled, const String& url,
                 bool adsbDbFallbackEnabled = true);
  RouteResult lookup(const String& callsign, const RouteContext& context,
                     uint32_t now);
  RouteResult test(const String& callsign, const RouteContext& context);
  const String& lastError() const { return lastError_; }
  const RouteDiagnostics &diagnostics() const { return diagnostics_; }
  bool hasPendingWrites() const;
  void flushPendingWrites();
  std::vector<RouteCacheEntryInfo> cacheEntries(uint32_t now) const;
  bool removeCacheEntry(const String& callsign);
  void clearCache();

 private:
  struct MemoryCacheEntry {
    PsramText callsign;
    StoredRouteResult route;
    uint32_t updated = 0;
    bool pendingWrite = false;
  };

  RouteResult readMemoryCache(const String& callsign,
                              const RouteContext& context, uint32_t now);
  void rememberInMemory(const String& callsign, const RouteResult& route,
                        uint32_t now, bool pendingWrite);
  RouteResult readCache(const String& callsign, const RouteContext& context,
                        uint32_t now);
  void writeCache(const String& callsign, const RouteResult& route,
                  uint32_t now);
  RouteResult fetchPrimary(const String& callsign,
                           const RouteContext& context);
  RouteResult fetchAdsbDb(const String& callsign,
                          const RouteContext& context);
  RouteResult fetch(const String& callsign, const RouteContext& context);

  bool enabled_ = true;
  bool adsbDbFallbackEnabled_ = true;
  String url_;
  String lastError_;
  RouteDiagnostics diagnostics_;
  static constexpr size_t memoryCacheSlots_ = 32;
  MemoryCacheEntry memoryCache_[memoryCacheSlots_];
  size_t nextMemorySlot_ = 0;
};
