#!/usr/bin/env python3
"""Exercise actual retry timing and lookup cache rejection/fetch control flow."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text()
start=main.index('    const bool retryRoute =')
predicate=main[start:main.index(';',start)+1]
source=(root/'src/RouteClient.cpp').read_text()
start=source.index('RouteResult RouteClient::lookup(')
lookup=source[start:source.index('\n}\n',start)+3]
code=r'''
#include <Arduino.h>
#include <cassert>
#include <cstdint>
#include <iostream>
bool operator!=(const String& a,const String& b){return !(a==b);}
struct RouteResult {String status="unavailable",source,error;};
struct RouteContext {double latitude=0;};
RouteResult currentRoute;
uint32_t clockMs=0,lastRouteAttempt=0;
uint32_t millis(){return clockMs;}
bool retryDue(){
'''+predicate+r'''
 return retryRoute;
}
class RouteClient {
 public:
 bool enabled_=true;
 String lastError_;
 RouteResult memory,disk,next;
 int fetches=0,remembers=0;
 double latitudeUsed=0;
 RouteResult readMemoryCache(const String&,const RouteContext&,uint32_t){return memory;}
 RouteResult readCache(const String&,const RouteContext&,uint32_t){return disk;}
 RouteResult fetch(const String&,const RouteContext& c){++fetches;latitudeUsed=c.latitude;return next;}
 void rememberInMemory(const String&,const RouteResult&,uint32_t,bool){++remembers;}
 RouteResult lookup(const String&,const RouteContext&,uint32_t);
};
'''+lookup+r'''
int main(){
 for(const char* status : {"unavailable","rejected"}){
  currentRoute.status=status;lastRouteAttempt=100;
  clockMs=60099;assert(!retryDue());clockMs=60100;assert(retryDue());
  lastRouteAttempt=clockMs;assert(!retryDue()); // No retry each ADS-B cycle.
  clockMs+=60000;assert(retryDue());
 }
 for(const char* status : {"current","cached","memory","not_found","disabled"}){
  currentRoute.status=status;clockMs=1000000;lastRouteAttempt=0;assert(!retryDue());
 }
 currentRoute.status="rejected";lastRouteAttempt=UINT32_MAX-10000;
 clockMs=lastRouteAttempt+59999;assert(!retryDue());++clockMs;assert(retryDue());
 RouteClient c;RouteContext context;context.latitude=51.60217;
 c.memory.status="rejected";c.disk.status="rejected";
 c.next.status="rejected";c.next.error="position mismatch";
 assert(c.lookup("RYR20JX",context,1).status==String("rejected"));
 assert(c.fetches==1 && c.remembers==0 && c.latitudeUsed==context.latitude);
 // Next minute, the provider/position can have changed; rejections are not cached.
 context.latitude=51.601362;c.next.status="current";c.next.error="";c.next.source="adsbdb";
 assert(c.lookup("RYR20JX",context,61).status==String("current"));
 assert(c.fetches==2 && c.remembers==1 && c.latitudeUsed==context.latitude);
 c.memory.status="memory";assert(c.lookup("RYR20JX",context,62).status==String("memory"));
 assert(c.fetches==2); // Valid cache remains usable.
 c.memory.status="rejected";c.disk.status="cached";
 assert(c.lookup("RYR20JX",context,63).status==String("cached"));assert(c.fetches==2);
 c.enabled_=false;assert(c.lookup("RYR20JX",context,64).status==String("unavailable"));
 assert(c.fetches==2);
 std::cout << "Route retry: timing, wrap, rejected-cache bypass, fresh context and unchanged valid cache passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-route-retry-') as folder:
 path=Path(folder);(path/'test.cpp').write_text(code)
 for variant in (0,1):
  binary=path/f'test-{variant}'
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic',
                  '-fsanitize=address,undefined',f'-DFLIGHTWALL_HAS_MATRIX={variant}',
                  '-I'+str(root/'tools/test_support'),str(path/'test.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
