#!/usr/bin/env python3
"""Execute the production airline classifier and route matching for both targets."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text()
start = source.index('String airlineCode(const String &callsign) {')
function = source[start:source.index('\n}\n', start) + 3]
code = r'''
#include <Arduino.h>
#include "CallsignClassifier.hpp"
#include "RouteCallsign.hpp"
#include <cassert>
''' + function + r'''
int main() {
  for (const char* text : {"UPP1088", "UPP0088", "UP-P1088", "OE-1234", "D-1234"}) {
    assert(isRegistrationCallsign(text));
    assert(airlineCode(text).isEmpty());
    assert(routeCallsignMatches(text, text));
  }
  for (const char* text : {"DLH072", "RYR20JX", "EZY91UM", "KEJ6480", "JAK123", "GAF123"})
    assert(!isRegistrationCallsign(text));
  assert(airlineCode("DLH072") == String("DLH"));
  assert(airlineCode("RYR20JX") == String("RYR"));
  assert(airlineCode("EZY91UM") == String("EZY"));
  assert(airlineCode("KEJ6480") == String("KEJ"));
  assert(airlineCode("JAK123") == String("JAK"));
  assert(airlineCode("VADER1") == String("GAF"));
  assert(airlineCode("SONIC1").isEmpty());
  assert(airlineCode("N123AB").isEmpty());
  assert(airlineCode("DABCD").isEmpty());
  for (const char* text : {"", "UPP", "UPP108", "UPP108A", "UP-", "-ABC", "ABC-", "AB-C?"})
    assert(!isRegistrationCallsign(text));
  assert(routeCallsignMatches("DLH072", "DLH72"));
  assert(!routeCallsignMatches("UPP0088", "UPP88"));
  assert(!routeCallsignMatches("UPP1088", "UPP01088"));
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-callsigns-') as folder:
    cpp = Path(folder) / 'test.cpp'
    cpp.write_text(code)
    for variant in (0, 1):
        binary = Path(folder) / ('test-' + str(variant))
        subprocess.run(['c++', '-std=c++11', '-Wall', '-Wextra', '-pedantic',
                        '-fsanitize=address,undefined', '-DFLIGHTWALL_HAS_MATRIX=' + str(variant),
                        '-I' + str(root / 'tools/test_support'), '-I' + str(root / 'include'),
                        str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    binary = Path(folder) / 'routes'
    subprocess.run(['c++', '-std=c++11', '-Wall', '-Wextra', '-pedantic',
                    '-fsanitize=address,undefined', '-I' + str(root / 'include'),
                    str(root / 'tools/test_route_callsign.cpp'), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Registration/airline classification and exact registration matching passed for both variants')
