#!/usr/bin/env python3
"""Test actual CPA/airport scrolling, frame scheduling and type fallback."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/main.cpp').read_text()
def function(signature):
    # Skip prototypes and find the function body.
    start=source.index(signature+' {')
    return source[start:source.index('\n}\n',start)+3]
constants=source[source.index('constexpr uint32_t AIRPORT_HALF_PAUSE_MS'):source.index('String limitedAirportText(')]
helpers='\n'.join(function(signature) for signature in [
 'uint32_t airportScrollDuration(const String &text, int16_t regionWidth)',
 'int16_t airportTextX(const String &text, int16_t regionWidth,\n                     uint32_t elapsedMs)',
 'String cpaPageDetails()', 'uint32_t cpaPageDuration()', 'uint32_t displayDrawIntervalMs()',
 'bool flightHasMultiHopRoute()', 'void drawAirportNamesPage()', 'uint32_t airportNamesPageDuration()'])
# Integration checks accompany behavioral tests of the actual called functions.
assert 'drawLine(airportTextX(details, MATRIX_WIDTH, elapsed), 24, details, aircraftColor());' in function('void drawCpaPage()')
processor=(root/'src/AircraftProcessor.cpp').read_text()
assert 'aircraft.description = resolvedAircraftTypeDescription(aircraft.type, aircraft.description);' in processor
assert processor.index('detectSpecialTypes(aircraft);') < processor.index('aircraft.description = resolvedAircraftTypeDescription')
stub=r'''
#include <Arduino.h>
#include "AircraftTypeNames.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <algorithm>
#include <cstdint>
#include <vector>
using std::min;
using std::max;
constexpr int16_t MATRIX_WIDTH=128;
struct { String specialType,typeDescription,type,routeCodes,routeNames,originName,destinationName,origin,destination; } flight;
String displayAscii(const String &text) { return text; }
const char *uiText(const char *de,const char *) { return de; }
struct DrawnLine { int16_t x,y; String text; };
std::vector<DrawnLine> lines;
uint32_t currentPageStarted=1000;
constexpr uint16_t cyan=1,white=2,yellow=3;
void drawLine(int16_t x,int16_t y,const String &text,uint16_t) { lines.push_back({x,y,text}); }
void drawCurrentSpecialIcon(int16_t) {}
bool wifiSetupAccessPointActive=false,otaPreparing=false,otaInProgress=false;
bool routeCacheWritePreparing=false,routeCacheWriteInProgress=false;
bool settingsWritePreparing=false,settingsWriteInProgress=false,displayTestActive=false;
bool environmentIdleDisplayEnabled=false,pageEnabled[8]={true,true,true,true,true,true,true,true};
enum class TransitionPhase { Normal }; TransitionPhase transitionPhase=TransitionPhase::Normal;
uint32_t brightnessMessageUntil=0,now=1000;
uint32_t millis() { return now; }
bool aircraftIsInsideDisplayRadius() { return true; }
uint8_t currentPageSlot=0,pageOrder[8]={2,0,1,3,4,5,6,7},customScrollSpeed=1;
constexpr uint8_t PAGE_COUNT=8;
uint8_t radarSweepIntervalSeconds=5;uint32_t radarSweepStarted=0;
constexpr uint32_t RADAR_SWEEP_PERIOD_MS=800,RADAR_SWEEP_DRAW_INTERVAL_MS=25;
'''
main=r'''
int main() {
 assert(resolvedAircraftTypeDescription("R44","")==String("Robinson R44"));
 assert(resolvedAircraftTypeDescription(" r22 "," Helicopter ")==String("Robinson R22"));
 assert(resolvedAircraftTypeDescription("R66","UNKNOWN")==String("Robinson R66"));
 assert(resolvedAircraftTypeDescription("R44"," R44 ")==String("Robinson R44"));
 assert(resolvedAircraftTypeDescription("R44"," Robinson R44 Raven II ")==String("Robinson R44 Raven II"));
 assert(resolvedAircraftTypeDescription("EC45","AIRBUS HELICOPTERS EC-145")==String("AIRBUS HELICOPTERS EC-145"));
 assert(resolvedAircraftTypeDescription("ZZ99","")==String("ZZ99"));
 assert(resolvedAircraftTypeDescription("","").isEmpty());
 flight.specialType="helicopter";flight.type="R44";
 flight.typeDescription=resolvedAircraftTypeDescription(flight.type,"");
 assert(cpaPageDetails()==String("helicopter | Robinson R44"));
 const String details=cpaPageDetails();
 const int travel=details.length()*6-MATRIX_WIDTH;
 assert(travel>0);
 const uint32_t forward=travel*80U,back=travel*20U;
 assert(cpaPageDuration()==2400U+forward+back);
 assert(airportTextX(details,128,799)==0);
 assert(airportTextX(details,128,800+forward)==-travel);
 assert(airportTextX(details,128,800+forward+799)==-travel);
 assert(airportTextX(details,128,1600+forward+back)==0);
 assert(airportTextX(details,128,cpaPageDuration()+10000)==0);
 int previous=0;
 for(uint32_t t=0;t<800+forward;++t) {
   int x=airportTextX(details,128,t); assert(x<=previous&&x>=-travel);previous=x;
 }
 previous=-travel;
 for(uint32_t t=1600+forward;t<=1600+forward+back;++t) {
   int x=airportTextX(details,128,t);assert(x>=previous&&x<=0);previous=x;
 }
 flight.type="A320";flight.specialType="";flight.typeDescription="AIRBUS A-320";
 assert(cpaPageDetails()==String("AIRBUS A-320")&&cpaPageDuration()==0);
 flight.type="R44";flight.typeDescription="";
 assert(cpaPageDetails()==String("R44"));
 // Single-hop: the longer line controls the full page cycle; short line stays.
 flight.routeCodes="AAA-BBB";flight.origin="AAA";flight.destination="BBB";
 flight.originName="A very long origin airport name with more than twenty characters";
 flight.destinationName="Berlin";
 const String originText=String("S ")+flight.originName;
 const String destinationText=String("Z ")+flight.destinationName;
 const uint32_t airportDuration=airportNamesPageDuration();
 assert(airportDuration==max(airportScrollDuration(originText,128),airportScrollDuration(destinationText,128)));
 assert(airportDuration>10000); // configured 10 seconds must be extended
 for(uint32_t elapsed=0;elapsed<=airportDuration+1000;elapsed+=50) {
   now=currentPageStarted+elapsed;lines.clear();drawAirportNamesPage();
   assert(lines.size()==3);
   assert(lines[1].text==originText && lines[2].text==destinationText);
   assert(lines[1].x==airportTextX(originText,128,elapsed));
   assert(lines[2].x==0);
 }
 assert(lines[1].x==0); // returned to the beginning before page completion
 // Missing names use codes; no unnecessary scrolling for short texts.
 flight.originName="";flight.destinationName="";
 assert(airportNamesPageDuration()==0);
 now=currentPageStarted;lines.clear();drawAirportNamesPage();
 assert(lines[1].text==String("S AAA") && lines[2].text==String("Z BBB"));
 // Multi-hop: both complete code chain and full names use the return cycle.
 flight.routeCodes="AAAA-BBBB-CCCC-DDDD-EEEE";
 flight.routeNames="First Airport > Second Very Long Airport Name > Third Airport";
 String codes=flight.routeCodes;codes.replace("-",">");
 const uint32_t multiDuration=airportNamesPageDuration();
 assert(multiDuration==max(airportScrollDuration(codes,128),airportScrollDuration(flight.routeNames,128)));
 assert(multiDuration>10000);
 for(uint32_t elapsed=0;elapsed<=multiDuration+1000;elapsed+=50) {
   now=currentPageStarted+elapsed;lines.clear();drawAirportNamesPage();
   assert(lines.size()==3 && lines[1].text==codes && lines[2].text==flight.routeNames);
   assert(lines[1].x==airportTextX(codes,128,elapsed));
   assert(lines[2].x==airportTextX(flight.routeNames,128,elapsed));
 }
 assert(lines[1].x==0 && lines[2].x==0);
 now=1000;
 pageOrder[0]=3;assert(displayDrawIntervalMs()==50);
 pageOrder[0]=2;assert(displayDrawIntervalMs()==50);
 pageOrder[0]=6;assert(displayDrawIntervalMs()==50);
 pageOrder[0]=4;customScrollSpeed=2;assert(displayDrawIntervalMs()==25);
 pageOrder[0]=0;assert(displayDrawIntervalMs()==250);
 std::cout<<"CPA/airport tests passed: source descriptions, Robinson fallback, normal/multi-hop airport return cycles, duration and 50-ms scheduling\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-cpa-') as temporary:
    folder=Path(temporary);cpp=folder/'test.cpp';cpp.write_text(stub+constants+helpers+main)
    for variant in [0,1]:
        binary=folder/('test-'+str(variant))
        subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',
            '-DFLIGHTWALL_HAS_MATRIX='+str(variant),'-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),
            str(cpp),str(root/'src/AircraftTypeNames.cpp'),str(root/'src/AircraftTypeNames.generated.cpp'),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
