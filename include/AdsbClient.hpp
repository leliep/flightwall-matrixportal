#pragma once

#include <Arduino.h>
#include <vector>

#include "Aircraft.hpp"
#include "AircraftProcessor.hpp"

struct AdsbResult {
  bool ok = false;
  uint32_t feedTimestamp = 0;
  size_t receivedAircraft = 0;
  std::vector<Aircraft> candidates;
  String error;
};

class AdsbClient {
 public:
  explicit AdsbClient(const String& url) : url_(url) {}
  void setUrl(const String& url) { url_ = url; }
  AdsbResult fetch(const ProcessingConfig& config);

 private:
  String url_;
};
