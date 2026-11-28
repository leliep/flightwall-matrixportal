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
restore='\n'.join(line for line in s.splitlines() if line.startswith('  if (!s["radar_sweep_interval_seconds"]') or line.startswith('  if (hasSweepStyle) preferences.') or line.startswith('  if (!s["radar_sweep_duration_ms"]'))
a=s.index('    if (webServer.hasArg("radarSweepInterval"))');saveform=s[a:s.index('    radarTrailsEnabled =',a)]
write='\n'.join(line for line in s.splitlines() if line.startswith('    preferences.put') and 'radarSweep' in line)
assert s.index('settings["radar_sweep_style"]')<s.index('#if FLIGHTWALL_HAS_MATRIX',s.index('PsramBuffer configurationJson()'))
a=s.index('  bool firstRadarPoint = true;',s.index('void handleDisplayApi()'));web_fragment=s[a:s.index('  const uint32_t remaining =',a)]
code=r'''
#include <Arduino.h>
#include <ArduinoJson.h>
#include "RadarSweep.hpp"
#include "RadarIdle.hpp"
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
constexpr uint32_t RADAR_SWEEP_SESSION_GAP_MS=750,IDLE_BLINK_HALF_PERIOD_MS=500;
constexpr uint8_t RADAR_POINT_SELECTED=1,RADAR_POINT_SPECIAL=2,RADAR_POINT_EMERGENCY=4,RADAR_POINT_GROUND=8;
uint16_t red=0xf800,yellow=0xffe0,violet=0xf81f,orange=0xfd20,cyan=0x07ff,blue=0x001f,white=0xffff,green=0x07e0,idleErrorPixel=0x1234;
int16_t radarCenterX();
int16_t radarDrawOffsetY=0;bool radarFullHeight=false;
bool fullPageMode=false,insideDisplayRadius=true;
bool largeFullPagesEnabled(){return fullPageMode;}
bool aircraftIsInsideDisplayRadius(){return insideDisplayRadius;}
int width=128;int16_t matrixLayoutWidth(){return width;}
float displayRadiusKm=15,viewHeading=0;
uint8_t radarSweepIntervalSeconds=5;std::atomic<bool>radarSweepLine{false};std::atomic<uint16_t>radarSweepDurationMs{800};
uint32_t radarSweepStarted=0,radarSweepLastDraw=0,clockMs=1000;
uint32_t millis(){return clockMs;}
void setTransitionPercent(uint8_t);
int transitionPercent=100;bool displayedRadarTrailsEnabled=true;
String lastError;
struct Display{
 uint16_t pixels[64][128]={};int lines=0;
 void clear(){std::fill(&pixels[0][0],&pixels[0][0]+8192,0);lines=0;}
 int height(){return FLIGHTWALL_MATRIX_HEIGHT;}
 uint16_t color565(uint8_t r,uint8_t g,uint8_t b){return ((r&0xf8)<<8)|((g&0xfc)<<3)|(b>>3);}
 void drawPixel(int x,int y,uint16_t color){if(x>=0&&x<128&&y>=0&&y<64)pixels[y][x]=color;}
 void drawLine(int x,int y,int x1,int y1,uint16_t color){
  ++lines;int dx=std::abs(x1-x),dy=-std::abs(y1-y),sx=x<x1?1:-1,sy=y<y1?1:-1,error=dx+dy;
  for(;;){drawPixel(x,y,color);if(x==x1&&y==y1)break;int twice=2*error;if(twice>=dy){error+=dy;x+=sx;}if(twice<=dx){error+=dx;y+=sy;}}
 }
}matrix;
RadarTrailStorage storage;RadarTrailStorage *radarTrailStorage=&storage;
'''+point+r'''
RadarPoint radarPoints[64];size_t radarPointCount=0;
'''+''.join(function(f) for f in ['bool radarCollectionPass(uint8_t pass, bool selected, bool relevant, bool addedRowsOnly)', 'int16_t radarDisplayX(int16_t x)', 'int16_t radarDisplayY(int16_t y)', 'void drawRadarPixel(int16_t x, int16_t y, uint16_t color)', 'uint16_t radarPointColor(const RadarPoint &point)','int16_t radarCenterX()', 'void drawRadarRing()','bool radarPointCoordinates(float distanceKm, float bearing,\n                           int16_t &x, int16_t &y, int16_t width = MATRIX_WIDTH, bool fullViewport = false)','void drawRadarPage()', 'void drawPersistentRadar()'])+r'''
void setTransitionPercent(uint8_t percent){transitionPercent=percent;}
struct Prefs{
 std::map<std::string,bool>bools;std::map<std::string,int>ints;
 void putBool(const char*k,bool v){bools[k]=v;}void putUChar(const char*k,int v){ints[k]=v;}void putUShort(const char*k,int v){ints[k]=v;}
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
 uint16_t durationValue=800;
 assert(RadarSweep::duration(-1)==800 && RadarSweep::duration(9000)==4000 && RadarSweep::duration(1550)==1600);
 assert(RadarSweep::parseSeconds("4.0",durationValue) && durationValue==4000);
 assert(RadarSweep::parseSeconds("0.8",durationValue) && durationValue==800);
 for(const char*bad:{"", "0", "0.7", "4.1", "nan", "inf", "bad", "2x"})assert(!RadarSweep::parseSeconds(bad,durationValue));
 assert(RadarSweep::cycleMs(2,4000)==4000 && RadarSweep::cycleMs(5,4000)==5000 && RadarSweep::cycleMs(0,4000)==0);
 assert(RadarSweep::sampleCount(4000)==160 && RadarSweep::step(3975,4000)==160);
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
 for(bool asLine:{false,true})for(uint16_t duration:{800,1500,4000}){
  radarSweepDurationMs=duration;radarSweepLine=asLine;radarSweepIntervalSeconds=2;
  radarSweepStarted=radarSweepLastDraw=0;radarPointCount=0;
  const uint32_t cycle=RadarSweep::cycleMs(2,duration);
  for(uint32_t t=0;t<=cycle;t+=25){
   draw(10000+t);
   if(t<duration || t==cycle){
    if(asLine)assert(matrix.lines==1);
    if(t==0 || t==duration-25 || t==cycle)assert(matrix.pixels[0][width/2]==(asLine?matrix.color565(0,85,0):white));
   }else assert(matrix.lines==0 && matrix.pixels[0][width/2]!=white);
   if(duration==4000 && t==2000)assert(matrix.pixels[30][width/2-1]==(asLine?matrix.color565(0,85,0):white) || matrix.pixels[30][width/2]==(asLine?matrix.color565(0,85,0):white) || matrix.pixels[30][width/2+1]==(asLine?matrix.color565(0,85,0):white));
  }
 }
 radarSweepDurationMs=800;
 radarSweepLine=false;
 assert(std::string(renderForm().c_str()).find("value='dot' selected")!=std::string::npos);
 radarSweepLine=true;
 assert(std::string(renderForm().c_str()).find("value='line' selected")!=std::string::npos);
 webServer.args={{"radarSweepStyle","line"},{"radarSweepInterval","7"},{"radarSweepDuration","4.0"}};save();
 assert(radarSweepLine && radarSweepIntervalSeconds==7 && preferences.bools["radarSweepLine"] && preferences.ints["radarSweepSec"]==7);
 assert(radarSweepDurationMs==4000 && preferences.ints["radarSweepMs"]==4000);
 auto doc=backup();assert(doc["radar_sweep_duration_ms"]==4000);assert(std::string(doc["radar_sweep_style"].as<const char*>())=="line" && doc["radar_sweep_interval_seconds"]==7);
 JsonDocument empty;restore(empty.as<JsonObjectConst>());assert(preferences.bools["radarSweepLine"] && preferences.ints["radarSweepMs"]==4000); // legacy backup preserves style/duration
 JsonDocument invalid;invalid["radar_sweep_style"]="invalid";restore(invalid.as<JsonObjectConst>());assert(webServer.status==400 && preferences.bools["radarSweepLine"]);
 invalid["radar_sweep_style"]=42;restore(invalid.as<JsonObjectConst>());assert(webServer.status==400);
 doc["radar_sweep_duration_ms"]=4000;
 doc["radar_sweep_style"]="dot";doc["radar_sweep_interval_seconds"]=0;restore(doc.as<JsonObjectConst>());
 assert(webServer.status==200 && !preferences.bools["radarSweepLine"] && preferences.ints["radarSweepSec"]==0);
 assert(preferences.ints["radarSweepMs"]==4000);
 webServer.args={{"radarSweepDuration","bad"}};save();assert(radarSweepDurationMs==4000);
 webServer.args.clear();save();assert(radarSweepLine && radarSweepDurationMs==4000); // missing form key also preserves style
#if FLIGHTWALL_HAS_MATRIX && FLIGHTWALL_MATRIX_HEIGHT == 64
 matrix.clear();width=125;radarPointCount=1;
 radarPoints[0].distanceKm=0;radarPoints[0].bearing=0;radarPoints[0].flags=RADAR_POINT_SELECTED;
 radarSweepLine=true;radarSweepIntervalSeconds=5;radarSweepStarted=radarSweepLastDraw=0;
 for(int y=0;y<32;++y)for(int x=0;x<128;++x)matrix.pixels[y][x]=0x1111;
 transitionPercent=40;drawPersistentRadar();
 assert(transitionPercent==40 && radarDrawOffsetY==0);
 for(int y=0;y<32;++y)for(int x=0;x<128;++x)assert(matrix.pixels[y][x]==0x1111);
 assert(matrix.pixels[47][62]==yellow);
 fullPageMode=true;matrix.clear();drawPersistentRadar();
 for(int y=0;y<64;++y)for(int x=0;x<128;++x)assert(matrix.pixels[y][x]==0);
 insideDisplayRadius=false;drawPersistentRadar();assert(matrix.pixels[47][62]==yellow);
 fullPageMode=false;insideDisplayRadius=true;
 for(int y=0;y<32;++y)for(int x=0;x<128;++x)matrix.pixels[y][x]=0x1111;

 drawRadarPixel(62,-1,white);drawRadarPixel(62,32,white);
 assert(matrix.pixels[31][62]==0x1111);
 matrix.clear();radarFullHeight=true;radarPointCount=0;
 drawRadarPage();assert(matrix.pixels[31][width/2]==green);
 assert(radarDisplayY(0)==16 && radarDisplayY(15)==31 && radarDisplayY(30)==46);
 assert(radarDisplayX(width/2+15)==width/2+15);
 assert(radarDisplayX(width/2-15)==width/2-15);
 // Live targets and their signed trail samples beyond the old 32-row window.
 for(int w:{125,128}){
  width=w;radarFullHeight=true;radarPointCount=2;radarSweepIntervalSeconds=0;
  radarPoints[0].distanceKm=20;radarPoints[0].bearing=0;radarPoints[0].flags=RADAR_POINT_SELECTED;
  radarPoints[1].distanceKm=20;radarPoints[1].bearing=180;radarPoints[1].flags=RADAR_POINT_SPECIAL;
  storage.consumer[0].trailCount=1;storage.consumer[0].trail[0].x=64+4;storage.consumer[0].trail[0].y=-3;storage.consumer[0].trail[0].time=millis();
  matrix.clear();drawRadarPage();assert(matrix.pixels[11][w/2]==yellow);assert(matrix.pixels[51][w/2]==violet);
  assert(matrix.pixels[13][w/2+4]!=0);assert(matrix.pixels[31][w/2+15]==blue);
  assert(radarDisplayY(-16)==0&&radarDisplayY(47)==63);
  drawRadarPixel(w/2,-16,white);drawRadarPixel(w/2,47,white);assert(matrix.pixels[0][w/2]==white&&matrix.pixels[63][w/2]==white);
  matrix.clear();drawRadarPixel(w/2,-17,white);drawRadarPixel(w/2,48,white);
  assert(!matrix.pixels[0][w/2]&&!matrix.pixels[63][w/2]);
  radarFullHeight=false;radarDrawOffsetY=32;matrix.clear();drawRadarPage();
  assert(!matrix.pixels[43][w/2]&&!matrix.pixels[63][w/2]);radarDrawOffsetY=0;
 }
 radarFullHeight=false;
 int16_t x=0,y=0;
 assert(radarPointCoordinates(20,0,x,y,125,true)&&y==-5);
 assert(radarPointCoordinates(20,180,x,y,125,true)&&y==35);
 assert(!radarPointCoordinates(33,0,x,y,125,true));
 for(bool selected:{false,true})for(bool relevant:{false,true})for(bool added:{false,true}){
  int hits=0;for(uint8_t pass=0;pass<4;++pass)if(radarCollectionPass(pass,selected,relevant,added)){++hits;assert(pass==(selected?0:added?3:relevant?1:2));}assert(hits==1);
 }

 assert(!radarPointCoordinates(20,180,x,y,125)); // 64-row canvas must not expand shared projection
#endif
 std::cout<<"Radar sweep: dot/beam, cardinal endpoints, off/timing/rollover, 125/128, target/trail priority, shared form/save/backup/restore passed\n";
}
'''
web_function = 'String exportWebRadar(){String json="[";auto*webRadarPoints=radarPoints;size_t webRadarPointCount=radarPointCount;bool radarTrailsEnabled=true;\n'+web_fragment+'json+=\']\';return json;}\n'
code=code.replace('int main(){',web_function+'int main(){',1)
code=code.replace(' std::cout<<"Radar sweep:',r'''
 // The actual API loop keeps its old 32-row window and valid JSON after filtering.
 radarPointCount=3;clockMs=10000;
 radarPoints[0].distanceKm=20;radarPoints[0].bearing=0;
 radarPoints[1].distanceKm=0;radarPoints[1].bearing=0;
 radarPoints[2].distanceKm=20;radarPoints[2].bearing=180;
 auto&wt=storage.webConsumer[1];wt.trailCount=3;
 for(int j=0;j<3;++j){wt.trail[j].time=clockMs;wt.trail[j].x=64;}
 wt.trail[0].y=-5;wt.trail[1].y=15;wt.trail[2].y=40;
 JsonDocument exported;assert(!deserializeJson(exported,exportWebRadar().c_str()));
 assert(exported.size()==1&&exported[0][3].size()==1&&exported[0][3][0][1]==15);
 radarPoints[1].distanceKm=20;assert(!deserializeJson(exported,exportWebRadar().c_str()));assert(exported.size()==0);
 std::cout<<"Radar sweep:''',1)
with tempfile.TemporaryDirectory(prefix='flightwall-radar-sweep-') as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 arduino=(root/'tools/test_support/Arduino.h').read_text().replace('#include <string>','#include <string>\n#include <sstream>\n#include <iomanip>')
 arduino=arduino.replace('  const char *c_str()', '  String(float number,int digits){std::ostringstream out;out<<std::fixed<<std::setprecision(digits)<<number;value_=out.str();}\n  String&operator+=(char c){value_+=c;return *this;}\n  friend String operator+(char c,const String&s){return String(std::string(1,c).c_str())+s;}\n  friend String operator+(const String&s,char c){return s+String(std::string(1,c).c_str());}\n  const char *c_str()')
 (p/'Arduino.h').write_text(arduino)
 for variant,height in ((0,32),(1,32),(1,64)):
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',f'-DFLIGHTWALL_HAS_MATRIX={variant}',f'-DFLIGHTWALL_MATRIX_HEIGHT={height}','-DARDUINOJSON_ENABLE_ARDUINO_STRING=0','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0','-I'+str(p),'-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),'-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/ArduinoJson/src'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
  subprocess.run([str(p/'test')],check=True)
