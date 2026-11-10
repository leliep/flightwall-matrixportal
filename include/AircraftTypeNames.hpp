#pragma once
#include <Arduino.h>

String aircraftTypeDisplayName(const String &code);
// Keep a real source description; replace missing/generic labels by a known
// local model name, or at least the current ICAO type code.
String resolvedAircraftTypeDescription(const String &code, const String &provided);
