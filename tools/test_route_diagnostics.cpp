// Run: c++ -std=c++11 -Wall -Wextra -pedantic -Iinclude tools/test_route_diagnostics.cpp -o /tmp/flightwall-route-diagnostics && /tmp/flightwall-route-diagnostics
#include "RouteDiagnostics.hpp"
#include <cassert>
#include <cstring>
#include <iostream>

struct Client {
  int code = 0;
  bool operationFinished = false;
  int calls = 0;
  int lastError(char *text, size_t size) {
    assert(operationFinished);
    ++calls;
    std::snprintf(text, size, "%s", "SSL - Memory allocation failed");
    return code;
  }
};

int main() {
  RouteDiagnostics diagnostics;
  assert(!diagnostics.primary.available && !diagnostics.lastFailure.available);
  RouteMemorySnapshot entry; entry.freeInternal = 54000; entry.largestInternal = 20000;
  entry.minimumInternal = 16000;
  auto &d = diagnostics.primary;
  d.begin("adsb.im", "DLH2HT", 100, entry);
  Client client; client.code = -32512; // MBEDTLS_ERR_SSL_ALLOC_FAILED
  int samples = 0;
  const int result = captureRouteHttp(d, client, [&]() {
    assert(samples == 1); client.operationFinished = true; return -1;
  }, [&]() {
    assert(client.calls == 0); // memory is sampled before error decoding
    auto memory = entry;
    memory.largestInternal -= ++samples * 1000;
    return memory;
  }, []() { return 300U; });
  assert(result == -1 && d.httpAttempted && d.httpCode == -1);
  assert(d.tlsCode == -32512 && std::strstr(d.tlsMessage, "allocation"));
  assert(d.before.largestInternal == 19000 && d.after.largestInternal == 18000);
  assert(d.entry.largestInternal == 20000 && d.durationMs == 200);
  diagnostics.rememberFailure(d);
  assert(diagnostics.lastFailure.tlsCode == -32512);
  // A successful later request resets its own error but retains the last failure.
  d.begin("adsb.im", "EIN69L", 400, entry);
  assert(d.attempts == 2 && d.tlsCode == 0 && d.tlsMessage[0] == 0);
  Client success;
  captureRouteHttp(d, success, []() { return 200; }, [&]() { return entry; }, []() { return 450U; });
  assert(success.calls == 0 && !d.failed());
  diagnostics.rememberFailure(d);
  assert(diagnostics.lastFailure.tlsCode == -32512);
  // Primary and fallback records must not overwrite each other.
  auto &fallback = diagnostics.fallback;
  fallback.begin("adsbdb", "EIN69L", 500, entry);
  Client generic; generic.code = -1;
  captureRouteHttp(fallback, generic, [&]() { generic.operationFinished=true; return -1; }, [&]() { return entry; }, []() { return 600U; });
  assert(fallback.tlsCode == -1 && std::strstr(fallback.tlsMessage, "No detailed TLS"));
  assert(d.httpCode == 200 && d.tlsCode == 0);
  // DNS/HTTP failure without a secure-client error must not invent a TLS code.
  fallback.begin("adsbdb", "EIN69L", 700, entry);
  Client noDetail;
  captureRouteHttp(fallback, noDetail, [&]() { noDetail.operationFinished=true; return -1; }, [&]() { return entry; }, []() { return 750U; });
  assert(fallback.tlsCode == 0 && std::strstr(fallback.tlsMessage, "No detailed TLS"));
  fallback.begin("adsbdb", "EIN69L", UINT32_MAX - 10, entry);
  captureRouteHttp(fallback, success, []() { return 404; }, [&]() { return entry; }, []() { return 9U; });
  assert(fallback.durationMs == 20 && !fallback.failed());
  fallback.begin("adsbdb", "EIN69L", 800, entry);
  captureRouteHttp(fallback, success, []() { return 503; }, [&]() { return entry; }, []() { return 900U; });
  assert(fallback.failed() && fallback.tlsCode == 0);
  diagnostics.rememberFailure(fallback);
  assert(diagnostics.lastFailure.httpCode == 503 && std::strcmp(diagnostics.lastFailure.source, "adsbdb") == 0);
  d.begin("adsb.im", "DLH2HT", 950, entry);
  captureRouteHttp(d, success, []() { return 404; }, [&]() { return entry; }, []() { return 960U; });
  assert(d.failed()); // Only ADSBDB 404 means a normal missing route.
  d.begin("adsb.im", "DLH2HT", 970, entry);
  captureRouteHttp(d, success, []() { return 302; }, [&]() { return entry; }, []() { return 980U; });
  assert(d.failed()); // Unfollowed redirects must remain diagnosable.
  // Failed http.begin() is recorded separately from an HTTP status code of 0.
  fallback.begin("adsbdb", "A-VERY-LONG-CALLSIGN-THAT-IS-TRUNCATED", 1000, entry);
  assert(fallback.failed() && !fallback.httpAttempted);
  assert(fallback.callsign[sizeof(fallback.callsign)-1] == 0);
  diagnostics.rememberFailure(fallback);
  assert(!diagnostics.lastFailure.httpAttempted);
  std::cout << "Route diagnostic tests passed; fixed storage: " << sizeof(RouteDiagnostics) << " bytes\n";
}
