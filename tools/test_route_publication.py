#!/usr/bin/env python3
"""Test the actual flight-loop publication branch against a concurrent mutation."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];s=(root/'src/main.cpp').read_text()
a=s.index('    if (selected.flight != routedFlight || retryRoute || newRouteSession) {')
branch=s[a:s.index('\n\n    next.available =',a)]
code=r'''
#include <Arduino.h>
#include <cassert>
#include <cstdint>
#include <utility>
#include <iostream>
bool operator!=(const String &a,const String &b){return !(a==b);}
struct RouteResult {String status="unavailable",source,error;};
struct RouteContext {bool hasPosition=false,hasAltitude=false,hasTrack=false,hasVerticalRate=false;double latitude=0,longitude=0;float altitudeFt=0,trackDegrees=0,verticalRateFpm=0;};
struct Selected {String flight="DLH072";bool hasPosition=true,hasAltitude=false,hasTrack=false,hasVerticalRate=false;double latitude=51,longitude=6;float altitudeFt=0,track=0,verticalRateFpm=0;} selected;
RouteResult currentRoute;String routedFlight,lastRouteError;
uint32_t lastRouteAttempt=77;
uint32_t millis(){return 100;}
bool cancelled=false,mutate=false,retryRoute=false,newRouteSession=false;
bool networkOperationCancelled(){return cancelled;}
double siteLatitude=50,siteLongitude=7;
struct {uint32_t feedTimestamp=123;} result;
enum class NetworkPhase {Route};struct NetworkPhaseScope {NetworkPhaseScope(NetworkPhase){}};
struct Client {
 int calls=0;String error;
 RouteResult lookup(const String&,const RouteContext& context,uint32_t){
  ++calls;assert(context.latitude==51 && context.longitude==6);
  if(mutate){routedFlight="NEW-ACTION";currentRoute.status="not_found";currentRoute.error="fresh state";cancelled=true;}
  RouteResult fetched;fetched.status="current";fetched.source="old query";return fetched;
 }
 const String &lastError(){return error;}
} routeClient;
bool process(){
'''+branch+r'''
 return true;
}
int main(){
 routedFlight="OLD";currentRoute.status="memory";mutate=true;
 assert(!process());assert(routedFlight==String("NEW-ACTION"));
 assert(currentRoute.status==String("not_found") && currentRoute.error==String("fresh state"));
 assert(lastRouteAttempt==77); // even the old attempt timestamp is not published
 mutate=false;cancelled=false;assert(process());
 assert(routedFlight==String("DLH072") && currentRoute.status==String("current"));
 assert(lastRouteAttempt==100);
 selected.flight="TEST1";const int calls=routeClient.calls;assert(process());
 assert(routeClient.calls==calls && currentRoute.status==String("unavailable"));
 std::cout<<"Actual flight-loop publication: newer config/cache state survives cancelled IO; valid and test routes commit correctly\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-route-publication-') as folder:
 p=Path(folder);(p/'test.cpp').write_text(code)
 for variant in (0,1):
  binary=p/str(variant)
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',f'-DFLIGHTWALL_HAS_MATRIX={variant}','-I'+str(root/'tools/test_support'),str(p/'test.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
