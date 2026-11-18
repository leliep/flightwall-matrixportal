#!/usr/bin/env python3
"""Check actual boot timing, interruptions and rendered information without hardware."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];source=(root/'src/main.cpp').read_text()
def function(signature):
 a=source.index(signature+' {');return source[a:source.index('\n}\n',a)+3]
display=function('void drawDisplay()');loop=function('void loop()');setup=function('void setup()')
assert display.index('wifiSetupAccessPointActive')<display.index('if (bootScreenVisible)')
assert display.index('if (!displayEnabled)')<display.index('if (bootScreenVisible)')
assert 'flight.emergency' in loop and 'displayEnabled && !quietHoursAreActive()' in loop
assert loop.index('bootScreen.update')<loop.index('drawDisplay()')
assert '#if FLIGHTWALL_HAS_MATRIX\n  const bool bootWasVisible' in loop
assert 'bootWasVisible != bootScreenVisible || now - lastDraw' in loop
assert setup.index('bootSiteText = displayAscii')<setup.index('xTaskCreatePinnedToCore(mqttTask')
assert 'wifiSetupAccessPointActive || bootScreenVisible' in function('uint32_t displayDrawIntervalMs()')
assert 'preferences.getBool("bootIp", true)' in setup
assert 'putBool("bootIp", "boot_show_ip")' in source
assert 'preferences.putBool("bootIp", bootShowIp.load' in source
config=function('PsramBuffer configurationJson()')
assert config.index('#if FLIGHTWALL_HAS_MATRIX') < config.index('settings["boot_show_ip"]') < config.index('#endif')

code=r'''
#include <Arduino.h>
#include "BootScreen.hpp"
#include <cassert>
#include <atomic>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>
using std::min;
void states(){
 BootScreenState waiting;
 assert(waiting.update(0,false,false,false,true));
 assert(waiting.update(20000,false,false,false,true)); // No IP: timer has not started.
 assert(waiting.update(30000,true,false,false,true));assert(waiting.elapsed(30000)==0);
 assert(waiting.update(35999,true,false,false,true));assert(waiting.elapsed(35999)==5999);
 assert(!waiting.update(36000,true,false,false,true));assert(!waiting.update(40000,true,false,false,true));
 BootScreenState zero;assert(zero.update(0,true,false,false,true));assert(!zero.update(6000,true,false,false,true));
 BootScreenState wrap;assert(wrap.update(UINT32_MAX-2000,true,false,false,true));
 assert(wrap.update(3998,true,false,false,true));assert(!wrap.update(3999,true,false,false,true));
 BootScreenState emergency;assert(emergency.update(1,true,false,false,true));
 assert(!emergency.update(2,true,false,true,true));assert(!emergency.update(3,true,false,false,true));
 BootScreenState ap;assert(!ap.update(1,false,true,false,true));assert(!ap.update(3,true,false,false,true));
 BootScreenState off;assert(!off.update(1,true,false,false,false));assert(!off.update(2,true,false,false,true));
 BootScreenState lost;assert(lost.update(1,true,false,false,true));assert(!lost.update(2,false,false,false,true));assert(!lost.update(3,true,false,false,true));
}
#if FLIGHTWALL_HAS_MATRIX
uint32_t now=100;uint32_t millis(){return now;}
constexpr char APP_VERSION[]="3.6.1-dual-target.54";
BootScreenState bootScreen;
std::atomic<bool> bootShowIp{true};
String bootSiteText="Ein besonders langer Standortname im Wintergarten";
int16_t width=128;int16_t matrixLayoutWidth(){return width;}
constexpr uint16_t cyan=1,white=2,yellow=3,green=4;
constexpr int WL_CONNECTED=3;
struct {int connected=0;int status(){return connected;}struct IP{String toString(){return "192.168.10.196";}};IP localIP(){return {};}}WiFi;
struct {int clears=0;void fillScreen(int color){assert(color==0);++clears;}}matrix;
int shows=0;void showMatrix(){++shows;}
const char* uiText(const char* de,const char*){return de;}
struct Line{int16_t x,y;String text;uint16_t color;};std::vector<Line>lines;
void drawLine(int16_t x,int16_t y,const String& text,uint16_t color){lines.push_back({x,y,text,color});}
'''+function('int16_t bootTextX(const String &text, uint32_t elapsedMs)')+function('void drawBootScreen()')+r'''
void rendering(){
 assert(bootScreen.update(now,false,false,false,true));drawBootScreen();
 assert(lines.size()==4 && shows==1);assert(lines[0].text==String("FLIGHTWALL START"));
 assert(lines[1].text==String("FW 3.6.1-dual-target.54"));assert(lines[2].text==bootSiteText);
 assert(lines[3].text==String("WLAN VERBINDET..."));
 WiFi.connected=WL_CONNECTED;now=20000;assert(bootScreen.update(now,true,false,false,true));
 lines.clear();drawBootScreen();assert(lines[3].text==String("IP 192.168.10.196"));
 for(int w:{128,125}){
  width=w;const int travel=bootSiteText.length()*6-w;
  assert(travel>0);assert(bootTextX(bootSiteText,0)==0);assert(bootTextX(bootSiteText,500)==0);
  assert(bootTextX(bootSiteText,5500)==-travel);assert(bootTextX(bootSiteText,5999)==-travel);
  assert(bootTextX("Kleve",5999)==0);
  now=25500;lines.clear();drawBootScreen();assert(lines[2].x==-travel);
  assert(lines[1].y==8 && lines[2].y==16 && lines[3].y==24);
 }
 const uint32_t elapsedBefore=bootScreen.elapsed(now);
 bootShowIp=false;lines.clear();drawBootScreen();
 assert(lines.size()==3 && lines[0].text==String("FLIGHTWALL START"));
 assert(lines[1].text==String("FW 3.6.1-dual-target.54") && lines[2].text==bootSiteText);
 assert(bootScreen.elapsed(now)==elapsedBefore); // Hiding IP never alters the timer.
 bootShowIp=true;lines.clear();drawBootScreen();assert(lines.size()==4);
 assert(lines[3].text==String("IP 192.168.10.196"));
}
#endif
int main(){states();
#if FLIGHTWALL_HAS_MATRIX
 rendering();
#endif
 std::cout<<"Boot screen: delayed IP, six seconds, rollover, one-shot/cancellation and matrix rendering passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-boot-') as folder:
 p=Path(folder);(p/'test.cpp').write_text(code)
 for variant in [0,1]:
  binary=p/str(variant)
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined','-DFLIGHTWALL_HAS_MATRIX='+str(variant),'-I'+str(root/'include'),'-I'+str(root/'tools/test_support'),str(p/'test.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
