#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>

// Fixed storage: diagnostic collection must not allocate while TLS is failing.
struct RouteMemorySnapshot {
  uint32_t freeInternal = 0;
  uint32_t largestInternal = 0;
  uint32_t minimumInternal = 0;
};

struct RouteRequestDiagnostics {
  bool available = false;
  bool httpAttempted = false;
  uint32_t attempts = 0;
  uint32_t startedMs = 0;
  uint32_t durationMs = 0;
  int httpCode = 0;
  int tlsCode = 0;
  char source[8] = {};
  char callsign[16] = {};
  char tlsMessage[160] = {};
  RouteMemorySnapshot entry;
  RouteMemorySnapshot before;
  RouteMemorySnapshot after;
  RouteMemorySnapshot afterCleanup;
  bool cleanupCaptured = false;

  void begin(const char *provider, const char *flight, uint32_t now,
             const RouteMemorySnapshot &memory) {
    const uint32_t nextAttempt = attempts + 1;
    *this = {};
    available = true;
    attempts = nextAttempt;
    startedMs = now;
    std::snprintf(source, sizeof(source), "%s", provider);
    std::snprintf(callsign, sizeof(callsign), "%s", flight);
    entry = before = after = afterCleanup = memory;
  }
  bool failed() const {
    return available && (!httpAttempted ||
        (httpCode != 200 && !(httpCode == 404 && std::strcmp(source, "adsbdb") == 0)));
  }
};

// Called around the real HTTP operation, before the caller's http.end().
// HTTPClient itself may already free TLS buffers on connection failure; these
// are boundary snapshots, not the largest block during the TLS handshake.
template <typename Client, typename Operation, typename Memory, typename Clock>
int captureRouteHttp(RouteRequestDiagnostics &diagnostic, Client &client,
                     Operation operation, Memory memory, Clock clock,
                     bool usesTls = true) {
  diagnostic.before = memory();
  diagnostic.httpAttempted = true;
  const int status = operation();
  diagnostic.after = memory();
  diagnostic.durationMs = clock() - diagnostic.startedMs;
  diagnostic.httpCode = status;
  if (status < 0) {
    const int tlsCode = usesTls ? client.lastError(diagnostic.tlsMessage,
                                         sizeof(diagnostic.tlsMessage)) : 0;
    diagnostic.tlsCode = tlsCode < 0 ? tlsCode : 0;
    // -1 is the secure client's generic transport error, not an mbedTLS code.
    if (diagnostic.tlsCode >= -1)
      std::snprintf(diagnostic.tlsMessage, sizeof(diagnostic.tlsMessage), "%s",
                    "No detailed TLS error available (DNS/transport/HTTP)");
  }
  return status;
}

struct RouteDiagnostics {
  RouteRequestDiagnostics primary;
  RouteRequestDiagnostics fallback;
  RouteRequestDiagnostics lastFailure;
  void rememberFailure(const RouteRequestDiagnostics &diagnostic) {
    if (diagnostic.failed()) lastFailure = diagnostic;
  }
};

// Declare before HTTP/TLS/JSON locals so reverse destruction samples only after
// those resources have been released, including every early-return path.
class RouteCleanupCapture {
 public:
  RouteCleanupCapture(RouteDiagnostics &all, RouteRequestDiagnostics &request,
                      RouteMemorySnapshot (*memory)())
      : all_(all),request_(request),memory_(memory) {}
  ~RouteCleanupCapture() {
    request_.afterCleanup=memory_();request_.cleanupCaptured=true;
    all_.rememberFailure(request_);
  }
  RouteCleanupCapture(const RouteCleanupCapture&)=delete;
  RouteCleanupCapture &operator=(const RouteCleanupCapture&)=delete;
 private:
  RouteDiagnostics &all_;
  RouteRequestDiagnostics &request_;
  RouteMemorySnapshot (*memory_)();
};
