#pragma once

#include <Arduino.h>

enum class LedStatus : uint8_t {
  Off,
  Starting,
  Setup,
  Connecting,
  Healthy,
  Warning,
  Updating,
  Error,
  Emergency,
};

void setStatusLedBrightness(uint8_t mode);
uint8_t statusLedBrightness();
void setStatusLed(LedStatus status);
LedStatus statusLedState();
const char *statusLedStateName(LedStatus status);
void pulseHttpActivity();
void updateStatusLed(bool nightDimmed = false);

