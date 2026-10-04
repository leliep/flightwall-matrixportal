#pragma once

#include <Arduino.h>

struct RouteResult {
  String origin;
  String destination;
  String originName;
  String destinationName;
  String airlineCode;
  String status = "unavailable";
  String error;

  bool found() const { return !origin.isEmpty() || !destination.isEmpty(); }
};

class RouteClient {
 public:
  RouteClient() = default;
  void configure(bool enabled, const String& url);
  RouteResult lookup(const String& callsign, double latitude, double longitude,
                     uint32_t now);
  RouteResult test(const String& callsign, double latitude, double longitude);
  const String& lastError() const { return lastError_; }

 private:
  RouteResult readCache(const String& callsign, uint32_t now);
  void writeCache(const String& callsign, const RouteResult& route,
                  uint32_t now);
  RouteResult fetch(const String& callsign, double latitude, double longitude);

  bool enabled_ = true;
  String url_;
  String lastError_;
};
