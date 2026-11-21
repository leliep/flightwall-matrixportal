#pragma once
#include <cstdint>

// Application-state mutex protects this transient selection. No NVS writes.
class ManualAircraftSelection {
 public:
  static constexpr uint32_t TimeoutMs = 60000;
  void select(uint64_t identity, uint32_t now) { identity_ = identity; started_ = now; }
  void clear() { identity_ = 0; }
  uint64_t identity(uint32_t now) const { return remainingMs(now) ? identity_ : 0; }
  uint32_t remainingMs(uint32_t now) const {
    const uint32_t age = now - started_;
    return identity_ && age < TimeoutMs ? TimeoutMs - age : 0;
  }
  bool expire(uint32_t now) {
    if (!identity_ || remainingMs(now)) return false;
    clear(); return true;
  }
  template <typename Range, typename Identity>
  const typename Range::value_type *resolve(const Range &aircraft, uint32_t now,
                                           bool emergency, Identity identify) {
    expire(now);
    if (!identity_) return nullptr;
    for (const auto &candidate : aircraft)
      if (candidate.hasPosition && identify(candidate) == identity_)
        return emergency ? nullptr : &candidate;
    clear(); // Fresh successful snapshot no longer contains the target.
    return nullptr;
  }
 private:
  uint64_t identity_ = 0;
  uint32_t started_ = 0;
};
