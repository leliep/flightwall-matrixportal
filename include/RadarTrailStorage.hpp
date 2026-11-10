#pragma once
#include "RadarTrails.hpp"
#include <new>

// Only live target positions stay in internal RAM. All history and its
// cross-core/web snapshots are allocated once in external PSRAM at startup.
struct RadarTrailDot {
  uint32_t time = 0;
  int8_t x = 0, y = 0;
};

struct RadarTrailSnapshot {
  RadarTrailDot trail[RadarTrails::Samples - 1];
  uint8_t trailCount = 0;
};

struct RadarTrailStorage {
  RadarTrails tracker;
  RadarTrailSnapshot network[RadarTrails::Capacity];
  RadarTrailSnapshot consumer[RadarTrails::Capacity];

  // Caller holds flightMutex. The matrix consumer is used only on the display
  // core; the headless consumer only in the network task's web handler.
  void publish(size_t count) {
    if (count > RadarTrails::Capacity) count = RadarTrails::Capacity;
    for (size_t i = 0; i < count; ++i) consumer[i] = network[i];
  }
};

template <typename Allocate>
RadarTrailStorage *allocateRadarTrailStorage(Allocate allocate) {
  void *memory = allocate(sizeof(RadarTrailStorage));
  // Deliberately no internal-heap fallback: it would reintroduce TLS failures.
  if (!memory) return nullptr;
  return new (memory) RadarTrailStorage();
}
