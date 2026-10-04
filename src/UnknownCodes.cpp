#include "UnknownCodes.hpp"

namespace {
String airports[unknownCodeLimit];
String airlines[unknownCodeLimit];
size_t airportCount = 0;
size_t airlineCount = 0;
size_t airportDrops = 0;
size_t airlineDrops = 0;

bool isUsefulCode(const String& code, size_t minimumLength,
                  size_t maximumLength) {
  if (code.length() < minimumLength || code.length() > maximumLength) return false;
  for (size_t index = 0; index < code.length(); ++index) {
    if (!isAlphaNumeric(code[index])) return false;
  }
  return true;
}

void remember(String code, String* values, size_t& count, size_t& drops,
              size_t minimumLength, size_t maximumLength) {
  code.trim();
  code.toUpperCase();
  if (!isUsefulCode(code, minimumLength, maximumLength)) return;
  for (size_t index = 0; index < count; ++index) {
    if (values[index] == code) return;
  }
  if (count < unknownCodeLimit) values[count++] = code;
  else ++drops;
}
}  // namespace

void rememberUnknownAirport(const String& code) {
  remember(code, airports, airportCount, airportDrops, 3, 4);
}

void rememberUnknownAirline(const String& code) {
  remember(code, airlines, airlineCount, airlineDrops, 2, 4);
}

size_t unknownAirportCount() { return airportCount; }
size_t unknownAirlineCount() { return airlineCount; }
const String& unknownAirportAt(size_t index) { return airports[index]; }
const String& unknownAirlineAt(size_t index) { return airlines[index]; }
size_t droppedUnknownAirports() { return airportDrops; }
size_t droppedUnknownAirlines() { return airlineDrops; }
