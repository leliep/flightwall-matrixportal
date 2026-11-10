#!/usr/bin/env python3
"""Compile the actual status-API diagnostic serializers and verify their JSON.

Requires a host C++ compiler, no ESP32 or network. A small String adapter keeps
this test independent of the Arduino runtime.
"""
import json
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text()
escape = source[source.index('String jsonEscape('):source.index('String configurationJson()')]
serializers = source[source.index('void appendRouteMemoryJson('):source.index('void appendRouteRequestHtml(')]
append = source[source.index('  // Append without building a second full JSON'):source.index('  webServer.sendHeader', source.index('  // Append without building a second full JSON'))]
adapter = r'''
#include "RouteDiagnostics.hpp"
#include "RadarTrailStorage.hpp"
#include <string>
#include <iostream>
#include <type_traits>
#include <cstring>
class String {
  std::string value;
 public:
  String() = default;
  String(const char *text):value(text) {}
  String(char c):value(1,c) {}
  String(const std::string &text):value(text) {}
  template<typename T, typename std::enable_if<std::is_integral<T>::value,int>::type = 0>
  String(T number):value(std::to_string(number)) {}
  String &operator+=(const String &other) { value += other.value; return *this; }
  friend String operator+(const String &a,const String &b) { return a.value+b.value; }
  void remove(size_t start) { value.erase(start); }
  size_t length() const { return value.size(); }
  void replace(const char *from,const char *to) {
    size_t pos=0;
    while((pos=value.find(from,pos))!=std::string::npos) {
      value.replace(pos,std::strlen(from),to); pos+=std::strlen(to);
    }
  }
  const char *c_str() const { return value.c_str(); }
};
uint32_t millis() { return 10000; }
RadarTrailStorage *radarTrailStorage = nullptr;
struct { uint32_t getFreePsram() const { return 123456; } } ESP;
struct { RouteDiagnostics data; const RouteDiagnostics &diagnostics() const { return data; } } routeClient;
'''
main = r'''
int main() {
 RouteMemorySnapshot statusMemory;
 statusMemory.freeInternal=54000;statusMemory.largestInternal=17000;statusMemory.minimumInternal=16000;
 String json="{\"version\":\"test\"}";
'''+append+r'''
 std::cout << json.c_str() << '\n';
 RadarTrailStorage external; radarTrailStorage=&external;
 auto &d=routeClient.data.primary;
 d.begin("adsb.im","DLH2HT",5000,statusMemory);
 d.httpAttempted=true;d.httpCode=-1;d.tlsCode=-32512;
 std::snprintf(d.tlsMessage,sizeof(d.tlsMessage),"%s","SSL - \"allocation\" failed\n");
 d.before=statusMemory;d.after=statusMemory;d.durationMs=500;
 routeClient.data.rememberFailure(d);
 routeClient.data.fallback.begin("adsbdb","DLH2HT",6000,statusMemory);
 routeClient.data.fallback.httpAttempted=true;routeClient.data.fallback.httpCode=200;
 json="{\"version\":\"test\"}";
'''+append+r'''
 std::cout << json.c_str() << '\n';
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-diagnostic-test-') as temporary:
    cpp = Path(temporary)/'test.cpp'
    binary = Path(temporary)/'test'
    cpp.write_text(adapter+escape+serializers+main)
    subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-I'+str(root/'include'),str(cpp),'-o',str(binary)],check=True)
    empty, populated = map(json.loads, subprocess.check_output([str(binary)],text=True).splitlines())
assert empty['version'] == 'test'
assert empty['radar_trails_storage_bytes'] == 0
assert populated['radar_trails_storage_bytes'] > 20000
assert populated['free_psram_bytes'] == 123456
assert empty['route_memory']['largest_internal_block_bytes'] == 17000
assert all(value is None for value in empty['route_diagnostics'].values())
d = populated['route_diagnostics']
assert d['primary']['tls_error_code'] == -32512
assert d['primary']['tls_error_hex'] == '-0x7F00'
assert d['primary']['tls_error_detail_available'] is True
assert '"allocation"' in d['primary']['tls_error_text']
assert d['primary']['age_seconds'] == 5
assert d['primary']['before_request']['free_internal_bytes'] == 54000
assert d['fallback']['http_code'] == 200 and not d['fallback']['tls_error_detail_available']
assert d['last_failure'] == d['primary']
print('Status-API diagnostic output passed: valid JSON, empty state, TLS details, escaping, independent providers, retained failure')
