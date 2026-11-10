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

  void begin(const char *provider, const char *flight, uint32_t now,
             const RouteMemorySnapshot &memory) {
    const uint32_t nextAttempt = attempts + 1;
    *this = {};
    available = true;
    attempts = nextAttempt;
    startedMs = now;
    std::snprintf(source, sizeof(source), "%s", provider);
    std::snprintf(callsign, sizeof(callsign), "%s", flight);
    entry = before = after = memory;
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
                     Operation operation, Memory memory, Clock clock) {
  diagnostic.before = memory();
  diagnostic.httpAttempted = true;
  const int status = operation();
  diagnostic.after = memory();
  diagnostic.durationMs = clock() - diagnostic.startedMs;
  diagnostic.httpCode = status;
  if (status < 0) {
    const int tlsCode = client.lastError(diagnostic.tlsMessage,
                                         sizeof(diagnostic.tlsMessage));
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
