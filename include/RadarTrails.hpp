#pragma once
#include <cmath>
#include <cstddef>
#include <cstdint>

// Fixed RAM budget; no allocation or persistent writes. Positions are local km.
class RadarTrails {
 public:
  static constexpr size_t Capacity = 64;
  static constexpr size_t Samples = 13;  // current position plus 12 old fixes
  static constexpr uint32_t LifetimeMs = 60000;
  struct Fix { float east = 0, north = 0; uint32_t time = 0; };
  struct Track {
    uint64_t identity = 0;
    Fix fixes[Samples];
    size_t count = 0;
    uint32_t lastSeen = 0;
  };
  void clear() { for (auto &track : tracks_) track = {}; }
  const Track *update(uint64_t identity, float east, float north, uint32_t now) {
    if (!identity || !std::isfinite(east) || !std::isfinite(north)) return nullptr;
    Track *slot = nullptr;
    for (auto &track : tracks_) {
      if (track.identity == identity) { slot = &track; break; }
      if (!slot || now - track.lastSeen > now - slot->lastSeen) slot = &track;
      if (!track.identity) slot = &track;
    }
    // Prefer unused/expired entries when introducing a new identity.
    if (slot->identity != identity) {
      for (auto &track : tracks_) {
        if (!track.identity || now - track.lastSeen >= LifetimeMs) {
          slot = &track; break;
        }
      }
      *slot = {}; slot->identity = identity;
    }
    if (slot->count) {
      const Fix &last = slot->fixes[slot->count - 1];
      const uint32_t gap = now - slot->lastSeen;
      const float dx = east - last.east, dy = north - last.north;
      // Break after reception gaps or implausible jumps (>1800 km/h + 0.5 km).
      const float limit = 0.5F + gap * 0.0005F;
      if (gap > 15000 || dx * dx + dy * dy > limit * limit) slot->count = 0;
    }
    slot->lastSeen = now;
    size_t kept = 0;
    for (size_t i = 0; i < slot->count; ++i)
      if (now - slot->fixes[i].time < LifetimeMs)
        slot->fixes[kept++] = slot->fixes[i];
    slot->count = kept;
    if (slot->count) {
      const Fix &last = slot->fixes[slot->count - 1];
      if (east == last.east && north == last.north) return slot;
      if (now - last.time < 4500) return slot;
    }
    if (slot->count == Samples) {
      for (size_t i = 1; i < Samples; ++i) slot->fixes[i - 1] = slot->fixes[i];
      --slot->count;
    }
    Fix &fix = slot->fixes[slot->count++];
    fix.east = east; fix.north = north; fix.time = now;
    return slot;
  }
 private:
  Track tracks_[Capacity];
};
