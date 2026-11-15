#pragma once

// Installed once before task creation. Only the data worker performs external
// HTTP requests. Its local clients/documents are never exposed to the web task.
struct NetworkIoHooks {
  void (*releaseState)() = nullptr;
  void (*acquireState)() = nullptr;
  bool (*cancelled)() = nullptr;
  NetworkIoHooks() = default;
  NetworkIoHooks(void (*release)(), void (*acquire)(), bool (*cancel)())
      : releaseState(release), acquireState(acquire), cancelled(cancel) {}
};
void configureNetworkIo(const NetworkIoHooks &hooks);
bool networkOperationCancelled();
class NetworkIoScope {
 public:
  NetworkIoScope();
  ~NetworkIoScope();
  NetworkIoScope(const NetworkIoScope &) = delete;
  NetworkIoScope &operator=(const NetworkIoScope &) = delete;
 private:
  void (*acquire_)() = nullptr;
};
