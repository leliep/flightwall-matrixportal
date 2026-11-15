#include "NetworkIo.hpp"
namespace { NetworkIoHooks callbacks; }
void configureNetworkIo(const NetworkIoHooks &hooks) { callbacks = hooks; }
bool networkOperationCancelled() { return callbacks.cancelled && callbacks.cancelled(); }
NetworkIoScope::NetworkIoScope() : acquire_(callbacks.acquireState) {
  if (callbacks.releaseState) callbacks.releaseState();
}
NetworkIoScope::~NetworkIoScope() { if (acquire_) acquire_(); }
