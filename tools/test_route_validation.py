#!/usr/bin/env python3
"""Run the actual ADSBDB rejection branches with controlled provider/position data."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/RouteClient.cpp').read_text()
start = source.index('  const String returnedCallsign =')
branches = source[start:source.index('  appendRouteCode', start)]
assert '!routeCallsignMatches(callsign.c_str(), returned.c_str())' in source
stub = r'''
#include <Arduino.h>
#include "RouteCallsign.hpp"
#include <cassert>
struct Field { const char* value; const char* operator|(const char*) const { return value; } };
struct Route { const char* callsign; Field operator[](const char*) const { return {callsign}; } };
struct Context { bool hasPosition; double latitude=0,longitude=0; };
struct Result { String source,status,error; };
using RouteResult=Result;
bool plausible=true; int positionChecks=0;
bool adsbDbRoutePlausible(Route,double,double) { ++positionChecks;return plausible; }
String upperTrimmed(String value) { value.trim();value.toUpperCase();return value; }
Result validate(const String& callsign,Route route,Context context) {
 Result result;
'''
main = r'''
 return result;
}
int main() {
 Context context;context.hasPosition=true;
 assert(validate("DLH072",{"DLH72"},context).error.isEmpty());
 assert(positionChecks==1);
 plausible=false;
 auto position=validate("DLH072",{"DLH72"},context);
 assert(position.error==String("ADSBDB-Route passt nicht zur Flugposition"));
 assert(position.status==String("rejected"));
 positionChecks=0;
 auto callsign=validate("DLH072",{"BAW72"},context);
 assert(callsign.error==String("ADSBDB-Callsign stimmt nicht mit der Anfrage überein"));
 assert(callsign.source==String("adsbdb-rejected"));
 assert(positionChecks==0);
 context.hasPosition=false;
 assert(validate("DLH072",{"DLH72"},context).error.isEmpty());
 assert(validate("DLH072",{""},context).error.isEmpty());
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-route-validation-') as folder:
    cpp=Path(folder)/'test.cpp';cpp.write_text(stub+branches+main)
    for variant in (0,1):
        binary=Path(folder)/f'test-{variant}'
        subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',
            f'-DFLIGHTWALL_HAS_MATRIX={variant}','-I'+str(root/'tools/test_support'),
            '-I'+str(root/'include'),str(cpp),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
print('Actual ADSBDB callsign/position rejection branches passed for both variants')
