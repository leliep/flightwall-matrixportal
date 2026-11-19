#!/usr/bin/env python3
"""Exercise real radar drawing and shared form/backup settings on both builds."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];s=(root/'src/main.cpp').read_text()
def function(signature):
 a=s.index(signature+' {');b=s.index('{',a);depth=1;c=b+1
 while depth:
  depth+=(s[c]=='{')-(s[c]=='}');c+=1
 return s[a:c]
a=s.index('struct RadarPoint {');point=s[a:s.index('\n};',a)+3]
a=s.index('  html += F("<label>Radar-Umlauf<select');b=s.index('  if (!radarTrailStorage)',a);form=s[a:b]
assert s.rfind('#endif',0,a)>s.rfind('#if FLIGHTWALL_HAS_MATRIX',0,a)
export='\n'.join(line for line in s.splitlines() if line.strip().startswith('settings["radar_sweep_'))
a=s.index('  bool restoredSweepLine = false;');validate=s[a:s.index('  settingsWritePreparing = true;',a)]
restore='\n'.join(line for line in s.splitlines() if line.startswith('  if (!s["radar_sweep_interval_seconds"]') or line.startswith('  if (hasSweepStyle) preferences.'))
a=s.index('    if (webServer.hasArg("radarSweepInterval"))');saveform=s[a:s.index('    radarTrailsEnabled =',a)]
write='\n'.join(line for line in s.splitlines() if line.startswith('    preferences.put') and 'radarSweep' in line)
assert s.index('settings["radar_sweep_style"]')<s.index('#if FLIGHTWALL_HAS_MATRIX',s.index('PsramBuffer configurationJson()'))
code=r'''
#include <Arduino.h>
#include <ArduinoJson.h>
#include "RadarSweep.hpp"
#include "RadarTrailStorage.hpp"
#include <atomic>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <map>
#include <string>
#include <iostream>
using std::min;using std::max;using std::isfinite;
#define F(x) x
template<class T,class L,class H> T constrain(T n,L lo,H hi){return std::max(static_cast<T>(lo),std::min(static_cast<T>(hi),n));}
constexpr float PI=3.14159265358979323846F;
constexpr int16_t MATRIX_WIDTH=128,RADAR_CENTER_Y=15,RADAR_RADIUS_PIXELS=15;
constexpr uint8_t RADAR_RADIUS_DOT_COUNT=32;
constexpr uint32_t RADAR_SWEEP_PERIOD_MS=800,RADAR_SWEEP_SESSION_GAP_MS=750,IDLE_BLINK_HALF_PERIOD_MS=500;
constexpr uint8_t RADAR_POINT_SELECTED=1,RADAR_POINT_SPECIAL=2,RADAR_POINT_EMERGENCY=4,RADAR_POINT_GROUND=8;
uint16_t red=0xf800,yellow=0xffe0,violet=0xf81f,orange=0xfd20,cyan=0x07ff,blue=0x001f,white=0xffff,green=0x07e0,idleErrorPixel=0x1234;
int width=128;int16_t matrixLayoutWidth(){return width;}
float displayRadiusKm=15,viewHeading=0;
uint8_t radarSweepIntervalSeconds=5;std::atomic<bool>radarSweepLine{false};
uint32_t radarSweepStarted=0,radarSweepLastDraw=0,clockMs=1000;
uint32_t millis(){return clockMs;}
int transitionPercent=100;bool displayedRadarTrailsEnabled=true;
String lastError;
struct Display{
 uint16_t pixels[32][128]={};int lines=0;
 void clear(){std::fill(&pixels[0][0],&pixels[0][0]+4096,0);lines=0;}
 int height(){return 32;}
 uint16_t color565(uint8_t r,uint8_t g,uint8_t b){return ((r&0xf8)<<8)|((g&0xfc)<<3)|(b>>3);}
 void drawPixel(int x,int y,uint16_t color){if(x>=0&&x<128&&y>=0&&y<32)pixels[y][x]=color;}
 void drawLine(int x,int y,int x1,int y1,uint16_t color){
  ++lines;int dx=std::abs(x1-x),dy=-std::abs(y1-y),sx=x<x1?1:-1,sy=y<y1?1:-1,error=dx+dy;
  for(;;){drawPixel(x,y,color);if(x==x1&&y==y1)break;int twice=2*error;if(twice>=dy){error+=dy;x+=sx;}if(twice<=dx){error+=dx;y+=sy;}}
 }
}matrix;
RadarTrailStorage storage;RadarTrailStorage *radarTrailStorage=&storage;
'''+point+r'''
RadarPoint radarPoints[64];size_t radarPointCount=0;
'''+''.join(function(f) for f in ['uint16_t radarPointColor(const RadarPoint &point)','int16_t radarCenterX()', 'void drawRadarRing()','bool radarPointCoordinates(float distanceKm, float bearing,\n                           int16_t &x, int16_t &y, int16_t width = MATRIX_WIDTH)','void drawRadarPage()'])+r'''
struct Prefs{
 std::map<std::string,bool>bools;std::map<std::string,int>ints;
 void putBool(const char*k,bool v){bools[k]=v;}void putUChar(const char*k,int v){ints[k]=v;}
 bool getBool(const char*k,bool def){return bools.count(k)?bools[k]:def;}
}preferences;
struct Server{
 std::map<std::string,String>args;int status=0;
 bool hasArg(const char*k){return args.count(k);}String arg(const char*k){return args[k];}
 void send(int n,const char*,const char*){status=n;}
}webServer;
const char*uiText(const char*de,const char*){return de;}
String selected(bool v){return v?" selected":"";}
String renderForm(){String html;
'''+form+r'''
 return html;
}
void save(){
'''+saveform+write+r'''
}
void restore(JsonObjectConst s){
'''+validate+restore+r'''
 webServer.status=200;
}
JsonDocument backup(){JsonDocument doc;JsonObject settings=doc.to<JsonObject>();
'''+export+r'''
 return doc;
}
void draw(uint32_t now){clockMs=now;matrix.clear();drawRadarPage();}
int main(){
 assert(RadarSweep::step(UINT32_MAX)==32);
 assert(RadarSweep::step(0)==0 && RadarSweep::step(194)==8 && RadarSweep::step(388)==16 && RadarSweep::step(581)==24 && RadarSweep::step(775)==32);
 bool style=false;assert(!RadarSweep::parseStyle("invalid",style));assert(!RadarSweep::parseStyle(nullptr,style));
 assert(RadarSweep::parseStyle("line",style)&&style);assert(RadarSweep::parseStyle("dot",style)&&!style);
 assert(!preferences.getBool("radarSweepLine",false));
 for(int w:{128,125}){
  width=w;radarPointCount=0;radarSweepLine=false;radarSweepIntervalSeconds=5;radarSweepStarted=radarSweepLastDraw=0;
  draw(1000);assert(matrix.pixels[0][w/2]==white && matrix.lines==0);
  draw(1194);assert(matrix.pixels[15][w/2+15]==white);
  draw(1388);assert(matrix.pixels[30][w/2]==white);
  draw(1581);assert(matrix.pixels[15][w/2-15]==white);
  draw(1775);assert(matrix.pixels[0][w/2]==white);
  draw(1800);assert(matrix.pixels[0][w/2]!=white);
  radarSweepLine=true;radarSweepStarted=1000;radarSweepLastDraw=1194;
  draw(1194);assert(matrix.lines==1 && matrix.pixels[15][w/2+3]!=0 && matrix.pixels[15][w/2]==green);
  radarPointCount=1;radarPoints[0].distanceKm=10;radarPoints[0].bearing=90;radarPoints[0].flags=RADAR_POINT_SELECTED;
  storage.consumer[0].trailCount=1;storage.consumer[0].trail[0].x=72;storage.consumer[0].trail[0].y=15;storage.consumer[0].trail[0].time=1000;
  draw(1194);assert(matrix.pixels[15][w/2+10]==yellow); // live aircraft is above beam
  const auto trailColor=matrix.pixels[15][w/2+8];assert(trailColor!=matrix.pixels[15][w/2+7]); // trail above beam
  assert(matrix.pixels[15][w/2+15]==matrix.pixels[15][w/2+7]); // visible beam endpoint over ring
  radarSweepIntervalSeconds=0;draw(1200);assert(matrix.lines==0 && radarSweepStarted==0);
  radarPointCount=0;radarSweepIntervalSeconds=1;draw(2000);draw(2500);draw(2775);assert(matrix.lines==1);
  draw(2800);assert(matrix.lines==0);draw(3000);assert(matrix.lines==1);
  // The original unsigned clock arithmetic remains valid over millis rollover.
  radarSweepStarted=UINT32_MAX-100;radarSweepLastDraw=radarSweepStarted;draw(93);assert(matrix.lines==1);
 }
 radarSweepLine=false;
 assert(std::string(renderForm().c_str()).find("value='dot' selected")!=std::string::npos);
 radarSweepLine=true;
 assert(std::string(renderForm().c_str()).find("value='line' selected")!=std::string::npos);
 webServer.args={{"radarSweepStyle","line"},{"radarSweepInterval","7"}};save();
 assert(radarSweepLine && radarSweepIntervalSeconds==7 && preferences.bools["radarSweepLine"] && preferences.ints["radarSweepSec"]==7);
 auto doc=backup();assert(std::string(doc["radar_sweep_style"].as<const char*>())=="line" && doc["radar_sweep_interval_seconds"]==7);
 JsonDocument empty;restore(empty.as<JsonObjectConst>());assert(preferences.bools["radarSweepLine"]); // legacy backup preserves style
 JsonDocument invalid;invalid["radar_sweep_style"]="invalid";restore(invalid.as<JsonObjectConst>());assert(webServer.status==400 && preferences.bools["radarSweepLine"]);
 invalid["radar_sweep_style"]=42;restore(invalid.as<JsonObjectConst>());assert(webServer.status==400);
 doc["radar_sweep_style"]="dot";doc["radar_sweep_interval_seconds"]=0;restore(doc.as<JsonObjectConst>());
 assert(webServer.status==200 && !preferences.bools["radarSweepLine"] && preferences.ints["radarSweepSec"]==0);
 webServer.args.clear();save();assert(radarSweepLine); // missing form key also preserves style
 std::cout<<"Radar sweep: dot/beam, cardinal endpoints, off/timing/rollover, 125/128, target/trail priority, shared form/save/backup/restore passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-radar-sweep-') as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 for variant in [0,1]:
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',f'-DFLIGHTWALL_HAS_MATRIX={variant}','-DARDUINOJSON_ENABLE_ARDUINO_STRING=0','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0','-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),'-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/ArduinoJson/src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
  subprocess.run([str(p/'test')],check=True)
