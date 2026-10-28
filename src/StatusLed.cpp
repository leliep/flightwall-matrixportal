#include "StatusLed.hpp"

#ifndef FLIGHTWALL_HAS_MATRIX
#define FLIGHTWALL_HAS_MATRIX 1
#endif

namespace {
constexpr uint32_t HTTP_PULSE_MS = 180;
volatile LedStatus baseStatus = LedStatus::Off;
volatile uint8_t brightnessMode = 1;
volatile uint32_t httpPulseUntil = 0;
uint32_t lastColor = UINT32_MAX;

uint8_t scaleChannel(uint8_t value, bool nightDimmed) {
  if (brightnessMode == 0 || value == 0) return 0;
  const uint8_t maximum = brightnessMode == 1 ? 8 : 24;
  uint8_t scaled = static_cast<uint16_t>(value) * maximum / 24;
  if (nightDimmed) {
    scaled /= 3;
    if (scaled == 0) scaled = 1;
  }
  return scaled;
}

void writeLed(uint8_t red, uint8_t green, uint8_t blue,
              bool nightDimmed) {
  red = scaleChannel(red, nightDimmed);
  green = scaleChannel(green, nightDimmed);
  blue = scaleChannel(blue, nightDimmed);
  const uint32_t color = (static_cast<uint32_t>(red) << 16) |
                         (static_cast<uint32_t>(green) << 8) | blue;
  if (color == lastColor) return;
  lastColor = color;
#if defined(RGB_BUILTIN)
  neopixelWrite(RGB_BUILTIN, red, green, blue);
#endif
}

bool blinkOn(uint32_t now, uint32_t periodMs, uint32_t onMs) {
  return now % periodMs < onMs;
}
}  // namespace

void setStatusLedBrightness(uint8_t mode) {
  brightnessMode = mode <= 2 ? mode : 1;
  lastColor = UINT32_MAX;
}

uint8_t statusLedBrightness() { return brightnessMode; }

void setStatusLed(LedStatus status) { baseStatus = status; }

LedStatus statusLedState() { return baseStatus; }

const char *statusLedStateName(LedStatus status) {
  switch (status) {
    case LedStatus::Off: return "off";
    case LedStatus::Starting: return "starting";
    case LedStatus::Setup: return "setup";
    case LedStatus::Connecting: return "connecting";
    case LedStatus::Healthy: return "healthy";
    case LedStatus::Warning: return "warning";
    case LedStatus::Updating: return "updating";
    case LedStatus::Error: return "error";
    case LedStatus::Emergency: return "emergency";
  }
  return "unknown";
}

void pulseHttpActivity() {
  const LedStatus current = baseStatus;
  if (current == LedStatus::Emergency || current == LedStatus::Updating ||
      current == LedStatus::Error || brightnessMode == 0) return;
  httpPulseUntil = millis() + HTTP_PULSE_MS;
}

void updateStatusLed(bool nightDimmed) {
  const uint32_t now = millis();
  const LedStatus current = baseStatus;
  if (brightnessMode == 0 || current == LedStatus::Off) {
    writeLed(0, 0, 0, false);
    return;
  }
  if (static_cast<int32_t>(httpPulseUntil - now) > 0 &&
      current != LedStatus::Emergency && current != LedStatus::Updating &&
      current != LedStatus::Error) {
    writeLed(24, 14, 0, nightDimmed);
    return;
  }
  switch (current) {
    case LedStatus::Off:
      writeLed(0, 0, 0, false);
      break;
    case LedStatus::Starting:
      writeLed(24, 8, 0, nightDimmed);
      break;
    case LedStatus::Setup:
      if (blinkOn(now, 1600, 500)) writeLed(0, 18, 24, nightDimmed);
      else writeLed(0, 0, 0, false);
      break;
    case LedStatus::Connecting:
      if (blinkOn(now, 1200, 400)) writeLed(0, 5, 24, nightDimmed);
      else writeLed(0, 0, 0, false);
      break;
    case LedStatus::Healthy:
      // A short brighter beat proves that the main loop is still alive.
      if (blinkOn(now, 2500, 140)) writeLed(0, 24, 0, nightDimmed);
      else writeLed(0, 8, 0, nightDimmed);
      break;
    case LedStatus::Warning:
      if (blinkOn(now, 2000, 450)) writeLed(24, 7, 0, nightDimmed);
      else writeLed(2, 1, 0, nightDimmed);
      break;
    case LedStatus::Updating:
      writeLed(18, 0, 24, nightDimmed);
      break;
    case LedStatus::Error:
      if (blinkOn(now, 1000, 350)) writeLed(24, 0, 0, nightDimmed);
      else writeLed(0, 0, 0, false);
      break;
    case LedStatus::Emergency:
      if (blinkOn(now, 300, 150)) writeLed(24, 0, 0, false);
      else writeLed(0, 0, 0, false);
      break;
  }
}
