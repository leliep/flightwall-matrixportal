#pragma once

#include <Arduino.h>
#include <vector>
#include "RouteDiagnostics.hpp"
#include "PsramText.hpp"
#include "RouteTestResponse.hpp"

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
  // Called by the data task; collecting a batch never waits or blocks ADS-B.
  void processPending();
  void refreshPending(const String& callsign, const RouteContext& context);
  bool primaryResultReady(const String& callsign) const;
  void cancelTest();
  const String& lastError() const { return lastError_; }
  const RouteDiagnostics &diagnostics() const { return diagnostics_; }
  const RouteTestResponse &testResponse(bool fallback) const {
    return testResponses_[fallback ? 1 : 0];
  }
  void clearTestResponses() {
    testResponses_[0] = {};
    testResponses_[1] = {};
  }
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
  RouteResult requestPrimary(const String& callsign,
                             const RouteContext& context, bool batch);
  void clearPrimaryQueue();
  struct PrimaryEntry {
    PsramText callsign;
    RouteContext context;
    StoredRouteResult result;
    uint32_t completedMs = 0;
    bool ready = false;
    bool manual = false;
    bool storageFailed = false;
  };
  static constexpr size_t primarySlots_ = 8;
  PrimaryEntry primaryQueue_[primarySlots_];
  uint32_t primaryWindowStarted_ = 0;
  uint32_t primaryLastPost_ = 0;
  bool primaryWindowActive_ = false;
  bool primaryPosted_ = false;
  bool manualPrimaryWaiting_ = false;
  bool managedPrimary_ = false;
  RouteResult fetchAdsbDb(const String& callsign,
                          const RouteContext& context);
  RouteResult fetch(const String& callsign, const RouteContext& context);

  bool enabled_ = true;
  bool adsbDbFallbackEnabled_ = true;
  String url_;
  String lastError_;
  RouteDiagnostics diagnostics_;
  RouteTestResponse testResponses_[2];
  bool captureTestResponses_ = false;
  static constexpr size_t memoryCacheSlots_ = 32;
  MemoryCacheEntry memoryCache_[memoryCacheSlots_];
  size_t nextMemorySlot_ = 0;
};
