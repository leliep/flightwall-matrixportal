#include "PsramText.hpp"
#include <Arduino.h>
#include <esp_heap_caps.h>

namespace {
PsramTextMemoryStats stats;
}
const PsramTextMemoryStats &psramTextMemoryStats() { return stats; }

void recordPsramTextAllocationFailure() { ++stats.allocationFailures; }

void *allocatePsramTextBytes(size_t bytes) {
  void *memory = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!memory) { recordPsramTextAllocationFailure(); return nullptr; }
  stats.allocatedBytes += bytes;
  ++stats.blocks;
  return memory;
}
void releasePsramTextBytes(void *memory, size_t bytes) {
  if (!memory) return;
  stats.allocatedBytes -= bytes;
  --stats.blocks;
  heap_caps_free(memory);
}
PsramText::PsramText(const String &text) { assign(text.c_str(), text.length()); }
PsramText &PsramText::operator=(const String &text) {
  assign(text.c_str(), text.length());
  return *this;
}
PsramText::operator String() const { return String(c_str(), length()); }
bool PsramText::operator==(const String &other) const {
  return length() == other.length() && std::memcmp(c_str(), other.c_str(), length()) == 0;
}
bool PsramText::operator!=(const String &other) const { return !(*this == other); }
