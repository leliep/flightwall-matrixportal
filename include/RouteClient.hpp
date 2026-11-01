#pragma once

#include <Arduino.h>
#include <vector>

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

  bool found() const { return !origin.isEmpty() || !destination.isEmpty(); }
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
  bool hasPendingWrites() const;
  void flushPendingWrites();
  std::vector<RouteCacheEntryInfo> cacheEntries(uint32_t now) const;
  bool removeCacheEntry(const String& callsign);
  void clearCache();

 private:
  struct MemoryCacheEntry {
    String callsign;
    RouteResult route;
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
  static constexpr size_t memoryCacheSlots_ = 32;
  MemoryCacheEntry memoryCache_[memoryCacheSlots_];
  size_t nextMemorySlot_ = 0;
};
