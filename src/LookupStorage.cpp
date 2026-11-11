#include "LookupStorage.hpp"
#include <Preferences.h>
#include <algorithm>
#include <cstring>
#if FLIGHTWALL_HAS_MATRIX
#include <FFat.h>
#define LOOKUP_FS FFat
#else
#include <LittleFS.h>
#define LOOKUP_FS LittleFS
#endif

namespace {
struct Metadata { uint32_t magic, slot, size, crc; };
constexpr uint32_t magic = 0x46574c31;
Metadata active{};
const char* paths[] = {"/flightwall-lookup-0.json", "/flightwall-lookup-1.json"};
bool mounted = false; // Cached availability, not a permanently mounted volume.
size_t capacity = 0;
struct Volume {
  bool opened;
  Volume() : opened(LOOKUP_FS.begin(false,
#if FLIGHTWALL_HAS_MATRIX
      "/ffat",
#else
      "/littlefs",
#endif
      1)) {} // One file at a time; never auto-format.
  void close() { if (opened) { LOOKUP_FS.end(); opened = false; } }
  ~Volume() { close(); }
};
uint32_t checksum(const char* data, size_t length) {
  uint32_t crc = ~0U;
  for (size_t i = 0; i < length; ++i) {
    crc ^= static_cast<uint8_t>(data[i]);
    for (unsigned bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
  }
  return ~crc;
}
}
bool lookupStorageMount() {
  Volume volume;
  mounted = volume.opened;
  active = {};
  if (!mounted) return false;
  capacity = LOOKUP_FS.totalBytes();
  Preferences metadata;
  if (metadata.begin("fwlookup", true)) {
    if (metadata.getBytesLength("active") == sizeof(active))
      metadata.getBytes("active", &active, sizeof(active));
    metadata.end();
  }
  if (active.magic != magic || active.slot > 1 ||
      !active.size || active.size > 65536) active = {};
  return true;
}
bool lookupStorageInitialize() {
  // Explicit first-use action only. Refuse to erase a mounted filesystem.
  if (mounted) return true;
#if FLIGHTWALL_HAS_MATRIX
  if (!LOOKUP_FS.format(false)) return false;
#else
  // begin(false) sets the partition label required by format().
  if (!LOOKUP_FS.format()) return false;
#endif
  Preferences metadata;
  if (!metadata.begin("fwlookup", false)) return false;
  metadata.remove("active"); metadata.end();
  return lookupStorageMount();
}
size_t lookupStorageCapacity() { return mounted ? capacity : 0; }
size_t lookupStorageSize() { return active.magic == magic ? active.size : 0; }
bool lookupStorageRead(char* data, size_t size) {
  if (!mounted || !size || size != lookupStorageSize()) return false;
  Volume volume;
  if (!volume.opened) return false;
  File file = LOOKUP_FS.open(paths[active.slot], FILE_READ);
  if (!file || file.size() != size) return false;
  const bool ok = file.readBytes(data, size) == size;
  file.close();
  return ok && checksum(data, size) == active.crc;
}
bool lookupStorageCommit(const char* data, size_t size) {
  if (!mounted || !size || size > 65536) return false;
  Volume volume;
  if (!volume.opened) return false;
  Metadata next{magic, active.magic == magic ? 1U - active.slot : 0U,
                static_cast<uint32_t>(size), checksum(data, size)};
  File file = LOOKUP_FS.open(paths[next.slot], FILE_WRITE);
  if (!file) return false;
  const bool written = file.write(reinterpret_cast<const uint8_t*>(data), size) == size;
  file.flush();
  file.close();
  if (!written) return false;
  // Read back every byte before committing the new active-slot pointer.
  file = LOOKUP_FS.open(paths[next.slot], FILE_READ);
  if (!file || file.size() != size) return false;
  char buffer[256];
  for (size_t offset = 0; offset < size;) {
    const size_t count = std::min(sizeof(buffer), size - offset);
    if (file.readBytes(buffer, count) != count ||
        memcmp(buffer, data + offset, count) != 0) { file.close(); return false; }
    offset += count;
  }
  file.close();
  volume.close(); // Flush/unmount filesystem metadata before the NVS commit.
  Preferences metadata;
  if (!metadata.begin("fwlookup", false)) return false;
  const bool committed = metadata.putBytes("active", &next, sizeof(next)) == sizeof(next);
  metadata.end();
  if (!committed) return false;
  active = next;
  return true;
}
