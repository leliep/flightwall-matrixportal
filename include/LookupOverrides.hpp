#pragma once
#include <Arduino.h>
#include "AirportCoordinates.hpp"
constexpr size_t lookupUploadLimit = 65536;
constexpr size_t lookupEntryLimit = 256;
void initializeLookupOverrides();
bool initializeLookupStorage();
bool installLookupOverrides(const char* data, size_t length);
bool lookupOverrideAirport(const String& code, String& name);
bool lookupOverrideAirline(const String& code, String& name);
bool lookupOverrideCoordinates(const String& code, AirportCoordinates& result);
bool lookupStorageReady();
String lookupDataVersion();
size_t lookupAirportCount();
size_t lookupAirlineCount();
size_t lookupPsramBytes();
uint32_t lookupAllocationFailures();
const String& lookupLastError();
void* allocateLookupUpload();
void releaseLookupUpload(void* data);
