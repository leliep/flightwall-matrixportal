// Run: c++ -std=c++11 -Wall -Wextra -pedantic -Iinclude tools/test_radar_storage.cpp -o /tmp/flightwall-radar-storage && /tmp/flightwall-radar-storage
#include "RadarTrailStorage.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

int main() {
  unsigned attempts = 0;
  auto *missing = allocateRadarTrailStorage([&](size_t bytes) -> void * {
    ++attempts;
    assert(bytes == sizeof(RadarTrailStorage));
    return nullptr;
  });
  assert(!missing && attempts == 1); // no internal-RAM fallback allocation

  alignas(RadarTrailStorage) unsigned char external[sizeof(RadarTrailStorage)];
  std::memset(external, 0xA5, sizeof(external));
  auto *storage = allocateRadarTrailStorage([&](size_t bytes) -> void * {
    ++attempts;
    assert(bytes == sizeof(external));
    return external;
  });
  assert(storage && attempts == 2);
  for (size_t i = 0; i < RadarTrails::Capacity; ++i) {
    assert(storage->network[i].trailCount == 0 && storage->consumer[i].trailCount == 0);
    const auto *track = storage->tracker.update(i + 1, 0, 0, 1000);
    assert(track && track->count == 1); // construction cleared uninitialized memory
    auto &network = storage->network[i];
    network.trailCount = RadarTrails::Samples - 1;
    for (uint8_t j = 0; j < network.trailCount; ++j) {
      network.trail[j].x = i;
      network.trail[j].y = j;
      network.trail[j].time = 1000 + j * 5000;
    }
  }
  storage->publish(RadarTrails::Capacity + 100); // capacity clamp
  for (size_t i = 0; i < RadarTrails::Capacity; ++i) {
    const auto &snapshot = storage->consumer[i];
    assert(snapshot.trailCount == RadarTrails::Samples - 1);
    for (uint8_t j = 0; j < snapshot.trailCount; ++j) {
      assert(snapshot.trail[j].x == static_cast<int8_t>(i) && snapshot.trail[j].y == static_cast<int8_t>(j));
      assert(snapshot.trail[j].time == 1000 + j * 5000U);
    }
  }
  storage->network[0].trail[0].x = 127;
  storage->network[0].trailCount = 0;
  assert(storage->consumer[0].trailCount == RadarTrails::Samples - 1);
  assert(storage->consumer[0].trail[0].x == 0); // independent display/web copy
  storage->publish(1);
  assert(storage->consumer[0].trailCount == 0);
  assert(storage->consumer[1].trailCount == RadarTrails::Samples - 1);
  storage->tracker.clear();
  assert(storage->tracker.update(1, 0, 0, 2000)->count == 1);
  storage->~RadarTrailStorage();
  std::cout << "Radar storage tests passed: allocation failure, initialization, full capacity, independent snapshots; external bytes: " << sizeof(RadarTrailStorage) << '\n';
}
