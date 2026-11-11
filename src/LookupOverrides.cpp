#include "LookupOverrides.hpp"
#include "LookupStorage.hpp"
#include <ArduinoJson.h>
#include <esp_heap_caps.h>
#include <cmath>
#include <cstring>
#include <cstddef>
#include <algorithm>

namespace {
size_t allocated = 0;
uint32_t failures = 0;
constexpr size_t memoryLimit = 1024 * 1024;
struct alignas(std::max_align_t) Allocation { size_t size; };
class ExternalAllocator : public ArduinoJson::Allocator {
 public:
  void* allocate(size_t size) override {
    if (size > memoryLimit - allocated) { ++failures; return nullptr; }
    auto* header = static_cast<Allocation*>(heap_caps_malloc(
        sizeof(Allocation) + size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!header) { ++failures; return nullptr; }
    header->size = size; allocated += size;
    return header + 1;
  }
  void deallocate(void* pointer) override {
    if (!pointer) return;
    auto* header = static_cast<Allocation*>(pointer) - 1;
    allocated -= header->size; heap_caps_free(header);
  }
  void* reallocate(void* pointer, size_t size) override {
    if (!size) { deallocate(pointer); return nullptr; }
    if (!pointer) return allocate(size);
    auto* header = static_cast<Allocation*>(pointer) - 1;
    // ArduinoJson requires shrinking reallocations to succeed. Keep the
    // existing external block; never allocate a second buffer just to shrink.
    if (size <= header->size) return pointer;
    void* next = allocate(size);
    if (!next) return nullptr;
    memcpy(next, pointer, std::min(size, header->size));
    deallocate(pointer); return next;
  }
} allocator;
JsonDocument active(&allocator);
bool ready = false;
String error;
bool fail(const char* message) { error = message; return false; }
// Strict UTF-8, bounded display length, no controls, embedded NUL or padding.
bool text(JsonVariantConst value, size_t maximum) {
  if (!value.is<const char*>()) return false;
  JsonString string = value.as<JsonString>();
  const auto* p = reinterpret_cast<const unsigned char*>(string.c_str());
  const size_t bytes = string.size();
  if (!bytes || bytes > maximum * 4 || p[0] <= 32 || p[bytes - 1] <= 32) return false;
  size_t characters = 0;
  for (size_t i = 0; i < bytes;) {
    uint32_t cp = p[i++]; unsigned rest = 0; uint32_t minimum = 0;
    if (cp < 0x80) { if (cp < 32 || cp == 127) return false; }
    else if (cp >= 0xc2 && cp <= 0xdf) { cp &= 31; rest = 1; minimum = 0x80; }
    else if (cp >= 0xe0 && cp <= 0xef) { cp &= 15; rest = 2; minimum = 0x800; }
    else if (cp >= 0xf0 && cp <= 0xf4) { cp &= 7; rest = 3; minimum = 0x10000; }
    else return false;
    while (rest--) {
      if (i == bytes || (p[i] & 0xc0) != 0x80) return false;
      cp = (cp << 6) | (p[i++] & 63);
    }
    if (cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff) ||
        (cp >= 0x80 && cp <= 0x9f)) return false;
    if (++characters > maximum) return false;
  }
  return true;
}
bool codeValid(JsonVariantConst value, size_t minimum, size_t maximum) {
  if (!value.is<const char*>()) return false;
  const JsonString code = value.as<JsonString>();
  if (code.size() < minimum || code.size() > maximum) return false;
  for (size_t i = 0; i < code.size(); ++i) {
    const char c = code.c_str()[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) return false;
  }
  return true;
}
bool validate(const JsonDocument& document) {
  JsonObjectConst root = document.as<JsonObjectConst>();
  if (root.isNull() || root.size() != 4 ||
      strcmp(root["format"] | "", "flightwall-lookups-v1") ||
      !text(root["version"], 64) || !root["airports"].is<JsonArrayConst>() ||
      !root["airlines"].is<JsonArrayConst>()) return fail("Invalid lookup format or version");
  for (const char* table : {"airports", "airlines"}) {
    const bool airport = strcmp(table, "airports") == 0;
    JsonArrayConst rows = root[table];
    if (rows.size() > lookupEntryLimit) return fail("Too many lookup entries (maximum 256 per table)");
    size_t index = 0;
    for (JsonVariantConst value : rows) {
      JsonObjectConst row = value.as<JsonObjectConst>();
      if (row.isNull() || row.size() != (airport ? 5U : 2U) ||
          !codeValid(row["code"], airport ? 3 : 2, 4) ||
          !text(row["name"], airport ? 28 : 64)) return fail("Invalid code or display name");
      for (size_t previous = 0; previous < index; ++previous)
        if (strcmp(rows[previous]["code"], row["code"]) == 0)
          return fail("Duplicate lookup code");
      if (airport) {
        if (!row["latitude"].is<double>() || !row["longitude"].is<double>())
          return fail("Airport coordinates required");
        const double lat = row["latitude"], lon = row["longitude"];
        if (!std::isfinite(lat) || !std::isfinite(lon) || lat < -90 || lat > 90 || lon < -180 || lon > 180)
          return fail("Invalid airport coordinates");
        if (!row["elevation_ft"].isNull() && (!row["elevation_ft"].is<int>() ||
            row["elevation_ft"].as<int>() < -1500 || row["elevation_ft"].as<int>() > 30000))
          return fail("Invalid airport elevation");
        bool haveElevation = false;
        for (JsonPairConst member : row)
          if (strcmp(member.key().c_str(), "elevation_ft") == 0) haveElevation = true;
        if (!haveElevation) return fail("Airport elevation_ft must be present (number or null)");
      }
      ++index;
    }
  }
  return true;
}
bool parse(JsonDocument& document, const char* data, size_t size) {
  if (!size || size > lookupUploadLimit) return fail("Lookup file must contain 1 to 65536 bytes");
  // No writable buffer overload: strings must be copied into owned PSRAM.
  struct Input {
    const char* data; size_t size; size_t position = 0;
    Input(const char* bytes, size_t length) : data(bytes), size(length) {}
    int read() { return position < size ? static_cast<unsigned char>(data[position++]) : -1; }
    size_t readBytes(char* destination, size_t length) {
      const size_t count = std::min(length, size - position);
      memcpy(destination, data + position, count); position += count; return count;
    }
  } input{data, size};
  auto result = deserializeJson(document, input, DeserializationOption::NestingLimit(6));
  if (result || document.overflowed()) return fail("Invalid JSON or insufficient PSRAM");
  while (input.position < size) {
    const char c = data[input.position++];
    if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return fail("Trailing data after lookup JSON");
  }
  return validate(document);
}
JsonObjectConst find(const char* table, const String& code) {
  for (JsonObjectConst row : active[table].as<JsonArrayConst>())
    if (strcmp(row["code"] | "", code.c_str()) == 0) return row;
  return {};
}
}
void* allocateLookupUpload() { return allocator.allocate(lookupUploadLimit); }
void releaseLookupUpload(void* data) { allocator.deallocate(data); }
void initializeLookupOverrides() {
  active.clear(); error = "";
  ready = lookupStorageMount();
  if (!ready) { fail("Lookup storage is not initialized or could not be mounted"); return; }
  const size_t size = lookupStorageSize();
  if (!size) { error = ""; return; }
  char* data = static_cast<char*>(allocateLookupUpload());
  if (!data) { fail("Insufficient PSRAM for lookup file"); return; }
  JsonDocument staged(&allocator);
  if (lookupStorageRead(data, size) && parse(staged, data, size)) {
    swap(active, staged); error = "";
  } else if (error.isEmpty()) fail("Stored lookup file failed integrity check");
  releaseLookupUpload(data);
}
bool initializeLookupStorage() {
  if (ready) return true;
  ready = lookupStorageInitialize();
  if (!ready) return fail("Lookup storage initialization failed");
  error = ""; return true;
}
bool installLookupOverrides(const char* data, size_t length) {
  if (!ready) return fail("Initialize lookup storage first");
  JsonDocument staged(&allocator);
  if (!parse(staged, data, length)) return false;
  if (!lookupStorageCommit(data, length)) return fail("Lookup write/verification failed; previous data retained");
  swap(active, staged); error = ""; return true;
}
bool lookupOverrideAirport(const String& code, String& name) {
  auto row = find("airports", code); if (row.isNull()) return false;
  name = row["name"] | ""; return true;
}
bool lookupOverrideAirline(const String& code, String& name) {
  auto row = find("airlines", code); if (row.isNull()) return false;
  name = row["name"] | ""; return true;
}
bool lookupOverrideCoordinates(const String& code, AirportCoordinates& result) {
  auto row = find("airports", code); if (row.isNull()) return false;
  result.latitude = row["latitude"]; result.longitude = row["longitude"];
  result.hasElevation = !row["elevation_ft"].isNull();
  result.elevationFt = result.hasElevation ? row["elevation_ft"].as<int>() : 0;
  return true;
}
bool lookupStorageReady() { return ready; }
String lookupDataVersion() { return active["version"] | "built-in"; }
size_t lookupAirportCount() { return active["airports"].size(); }
size_t lookupAirlineCount() { return active["airlines"].size(); }
size_t lookupPsramBytes() { return allocated; }
uint32_t lookupAllocationFailures() { return failures; }
const String& lookupLastError() { return error; }
