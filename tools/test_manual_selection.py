#!/usr/bin/env python3
"""Exercise transient identity selection and the actual radar POST handler."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
s=(root/'src/main.cpp').read_text()
a=s.index('void handleRadarSelection() {');handler=s[a:s.index('\n}\n',a)+3]
assert 'manualAircraftSelection.resolve(result.candidates, now,\n      hasSelection && selected.emergency, aircraftIdentity)' in s
assert 'if (manualAircraftSelection.expire(now)) pollRequested = true;' in s
code=r'''
#include <Arduino.h>
#include "ManualAircraftSelection.hpp"
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cctype>
#include <iostream>
struct Candidate { uint64_t id; bool hasPosition; };
uint64_t identify(const Candidate &p){return p.id;}
uint32_t clockMs=1000;uint32_t millis(){return clockMs;}
ManualAircraftSelection manualAircraftSelection;
int revision=0;void noteWebMutation(){++revision;}
struct Server {String id;bool has=true;int code=0;
 bool hasArg(const char*){return has;}String arg(const char*){return id;}
 void sendHeader(const char*,const char*){}void send(int n,const char*,const char*){code=n;}
}webServer;
int flightMutex=1;constexpr int pdTRUE=1;int pdMS_TO_TICKS(int v){return v;}
int xSemaphoreTake(int,int){return pdTRUE;}void xSemaphoreGive(int){}
struct Point{uint64_t identity;};Point networkRadarPoints[2]={{0x123456789abcdef1ULL},{2}};
size_t networkRadarPointCount=2;uint32_t networkLastSuccessfulPoll=1;
'''+handler+r'''
int main(){
 ManualAircraftSelection state;std::vector<Candidate> points={{1,true},{2,true}};
 assert(!state.identity(0));state.select(1,0);assert(state.remainingMs(0)==60000);
 assert(state.resolve(points,59999,false,identify)->id==1);
 assert(!state.resolve(points,60000,false,identify));assert(!state.identity(60000));
 state.select(1,0);state.select(2,50000);assert(state.resolve(points,100000,false,identify)->id==2);
 assert(state.remainingMs(100000)==10000);state.select(2,100000);assert(state.remainingMs(100000)==60000);
 assert(!state.resolve(points,100001,true,identify));assert(state.identity(100001)==2); // emergency wins; timer runs
 assert(state.resolve(points,100002,false,identify)->id==2);
 points.pop_back();assert(!state.resolve(points,100003,false,identify));assert(!state.identity(100003));
 state.select(1,UINT32_MAX-999);assert(state.remainingMs(0)==59000);assert(state.expire(59000));
 state.select(1,100);state.clear();assert(!state.resolve(points,101,false,identify));
 state.select(1,100);points[0].hasPosition=false;assert(!state.resolve(points,101,false,identify));
 webServer.has=false;handleRadarSelection();assert(webServer.code==400 && revision==0);webServer.has=true;
 for(const char *id:{"1","-123456789abcdef","123456789abcdefg","0000000000000000","123456789abcdef1x"}){
  webServer.id=id;handleRadarSelection();assert(webServer.code==400 && revision==0);
 }
 webServer.id="0000000000000003";handleRadarSelection();assert(webServer.code==409 && revision==0);
 webServer.id="123456789abcdef1";handleRadarSelection();assert(webServer.code==200 && revision==1);
 assert(manualAircraftSelection.identity(clockMs)==0x123456789abcdef1ULL);
 clockMs=16002;handleRadarSelection();assert(webServer.code==409 && revision==1); // stale radar
 webServer.id="";handleRadarSelection();assert(webServer.code==200 && revision==2 && !manualAircraftSelection.identity(clockMs));
 std::cout<<"Manual radar selection: timeout, repeat, switch, disappearance, emergency, wrap, strict/stale POST validation and cancel passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-selection-') as temp:
 p=Path(temp);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined','-I'+str(root/'include'),'-I'+str(root/'tools/test_support'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
 subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined','-I'+str(root/'include'),str(root/'tools/test_radar_storage.cpp'),'-o',str(p/'storage')],check=True)
 subprocess.run([str(p/'storage')],check=True)
