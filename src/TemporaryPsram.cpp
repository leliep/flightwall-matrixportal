#include "TemporaryPsram.hpp"
#include <esp_heap_caps.h>
#include <algorithm>
#include <cstddef>
#include <mutex>

namespace {
TemporaryPsramStats stats;
std::mutex statsMutex;
struct alignas(std::max_align_t) Allocation { size_t size; };
constexpr size_t maximumAllocation = 2 * 1024 * 1024;
class JsonAllocator : public ArduinoJson::Allocator {
 public:
  void *allocate(size_t size) override { return allocateTemporaryPsram(size); }
  void deallocate(void *pointer) override { freeTemporaryPsram(pointer); }
  void *reallocate(void *pointer, size_t size) override {
    if (!size) { freeTemporaryPsram(pointer);return nullptr; }
    // ArduinoJson requires shrinking reallocations to succeed.
    if (pointer && size <= (static_cast<Allocation*>(pointer)-1)->size) return pointer;
    return resizeTemporaryPsram(pointer,size);
  }
} allocator;
}
TemporaryPsramStats temporaryPsramStats() {
  const std::lock_guard<std::mutex> lock(statsMutex);
  return stats;
}
ArduinoJson::Allocator &temporaryJsonAllocator() { return allocator; }
void *allocateTemporaryPsram(size_t bytes) { return resizeTemporaryPsram(nullptr,bytes); }
void *resizeTemporaryPsram(void *pointer,size_t bytes) {
  const std::lock_guard<std::mutex> lock(statsMutex);
  auto *old=pointer ? static_cast<Allocation*>(pointer)-1 : nullptr;
  const size_t oldSize=old ? old->size : 0;
  if (bytes > maximumAllocation || bytes > maximumAllocation-(stats.allocatedBytes-oldSize)) {
    ++stats.allocationFailures;return nullptr;
  }
  auto *next=static_cast<Allocation*>(heap_caps_realloc(old,sizeof(Allocation)+bytes,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!next) { ++stats.allocationFailures;return nullptr; }
  next->size=bytes;stats.allocatedBytes=stats.allocatedBytes-oldSize+bytes;
  stats.peakBytes=std::max(stats.peakBytes,stats.allocatedBytes);
  return next+1;
}
void freeTemporaryPsram(void *pointer) {
  if (!pointer) return;
  const std::lock_guard<std::mutex> lock(statsMutex);
  auto *header=static_cast<Allocation*>(pointer)-1;
  stats.allocatedBytes-=header->size;heap_caps_free(header);
}
