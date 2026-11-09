// Run: c++ -std=c++11 -Wall -Wextra -pedantic -Iinclude tools/test_radar_trails.cpp -o /tmp/flightwall-trails && /tmp/flightwall-trails
#include "RadarTrails.hpp"
#include <cassert>
#include <iostream>

int main() {
  RadarTrails trails;
  assert(!trails.update(0, 0, 0, 0));
  assert(!trails.update(1, NAN, 0, 0));
  const RadarTrails::Track *track = nullptr;
  for (unsigned i = 0; i < 30; ++i) {
    track = trails.update(1, i * 0.5F, 0, i * 5000);
    assert(track->count <= RadarTrails::Samples);
    for (size_t j = 0; j < track->count; ++j)
      assert(i * 5000 - track->fixes[j].time < RadarTrails::LifetimeMs);
  }
  assert(track->count == 12);
  const auto *other = trails.update(2, 100, 20, 145000);
  assert(other->count == 1 && track->identity == 1);
  track = trails.update(1, 14.5F, 0, 150000);  // repeated source position
  assert(track->fixes[track->count - 1].time == 145000);
  track = trails.update(1, 14.5F, 0, 220000);  // long gap breaks history
  assert(track->count == 1);
  track = trails.update(1, 80, 0, 225000);  // implausible jump breaks history
  assert(track->count == 1);
  trails.clear();
  track = trails.update(1, 0, 0, UINT32_MAX - 2000);
  track = trails.update(1, 0.5F, 0, 2999);  // millis rollover
  assert(track->count == 2);
  track = trails.update(1, 1, 0, 8000);
  assert(track->count == 3);
  trails.clear();
  for (uint64_t i = 1; i <= 64; ++i) trails.update(i, 0, 0, i * 100);
  track = trails.update(65, 100, 0, 7000); // bounded replacement, no inherited fixes
  assert(track->identity == 65 && track->count == 1);
  track = trails.update(64, 0.5F, 0, 11400);
  assert(track->identity == 64 && track->count == 2);
  trails.clear();
  assert(trails.update(64, 0, 0, 12000)->count == 1);
  std::cout << "Radar trail tests passed; tracker bytes: " << sizeof(RadarTrails) << '\n';
}
