#pragma once
#include <stdint.h>

// Network-task-owned state. No allocation, persistence or device reset.
class NetworkRecovery {
 public:
  static constexpr uint32_t failureWindowMs = 90000;
  static constexpr uint32_t cooldownMs = 300000;
  static constexpr uint8_t minimumFailures = 5;

  // HTTPClient connection/send/lost/timeout errors; not RAM/encoding errors.
  static bool isTransportFailure(int status) {
    return (status <= -1 && status >= -5) || status == -11;
  }
  void progress() { failures_ = 0; }
  void transportFailure(uint32_t now) {
    if (!failures_) firstFailure_ = now;
    if (failures_ != UINT8_MAX) ++failures_;
  }
  bool due(uint32_t now, bool connected, bool busy) const {
    return connected && !busy && failures_ >= minimumFailures &&
        uint32_t(now - firstFailure_) >= failureWindowMs &&
        (!recoveries_ || uint32_t(now - lastRecovery_) >= cooldownMs);
  }
  void recovering(uint32_t now) {
    ++recoveries_;
    lastRecovery_ = now;
    progress();
  }
  uint8_t failures() const { return failures_; }
  uint32_t recoveries() const { return recoveries_; }
  uint32_t lastRecovery() const { return lastRecovery_; }
 private:
  uint8_t failures_ = 0;
  uint32_t firstFailure_ = 0;
  uint32_t lastRecovery_ = 0;
  uint32_t recoveries_ = 0;
};
