#pragma once

#include <Arduino.h>

constexpr size_t unknownCodeLimit = 32;

void rememberUnknownAirport(const String& code);
void rememberUnknownAirline(const String& code);
size_t unknownAirportCount();
size_t unknownAirlineCount();
const String& unknownAirportAt(size_t index);
const String& unknownAirlineAt(size_t index);
size_t droppedUnknownAirports();
size_t droppedUnknownAirlines();
