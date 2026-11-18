#pragma once
#include <cstdint>

constexpr uint32_t BOOT_SCREEN_DURATION_MS = 6000;

// Owned by the display loop. Show once per boot; never restart on reconnect.
class BootScreenState {
 public:
  bool update(uint32_t now, bool connected, bool setupActive,
              bool emergency, bool displayAllowed) {
    if (finished_) return false;
    if (setupActive || emergency || !displayAllowed || (started_ && !connected)) {
      finished_ = true;
      return false;
    }
    if (connected && !started_) {
      started_ = true;
      connectedAt_ = now;
    }
    if (started_ && static_cast<uint32_t>(now - connectedAt_) >= BOOT_SCREEN_DURATION_MS) {
      finished_ = true;
      return false;
    }
    return true;
  }
  uint32_t elapsed(uint32_t now) const {
    return started_ ? static_cast<uint32_t>(now - connectedAt_)
                    : now % BOOT_SCREEN_DURATION_MS;
  }
 private:
  uint32_t connectedAt_ = 0;
  bool started_ = false;
  bool finished_ = false;
};
