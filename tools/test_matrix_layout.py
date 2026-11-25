#!/usr/bin/env python3
"""Exercise actual width, output masking, radar projection and scrolling helpers."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/main.cpp').read_text()
def function(signature):
 a=source.rindex(signature);return source[a:source.index('\n}\n',a)+3]
funcs='\n'.join(function(s) for s in [
 'int16_t matrixLayoutWidth()', 'void showMatrix()', 'bool selectEnabledPage()',
 'bool radarPointCoordinates(float distanceKm, float bearing,',
 'int16_t airportTextX(const String &text, int16_t regionWidth,\n                     uint32_t elapsedMs)',
 'uint32_t airportScrollDuration(const String &text, int16_t regionWidth)'])
# Configuration is opt-in, matrix-only, export/restore uses the same key.
assert 'preferences.getBool("matrixNarrow", false)' in source
assert 'putBool("matrixNarrow", "matrix_reduced_width")' in source
assert 'preferences.putBool("matrixNarrow", matrixReducedWidth.load' in source
config=source[source.index('PsramBuffer configurationJson()'):source.index('void handleConfigurationDownload()')]
assert config.index('#if FLIGHTWALL_HAS_MATRIX') < config.index('settings["matrix_reduced_width"]') < config.index('#endif')
web=(root/'src/WebDisplay.cpp').read_text()
assert 'matrix_reduced_width' not in web and 'matrixLayoutWidth' not in web
assert source.count('matrix.show();')==1
assert 'dot.x + radarCenterX() - MATRIX_WIDTH / 2' in source
assert 'point.distanceKm, point.bearing, x, y, matrixLayoutWidth()' in source
code=r'''
#include <Arduino.h>
#include <atomic>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <iostream>
using std::min;using std::max;using std::isfinite;
constexpr int16_t MATRIX_WIDTH=128;
std::atomic<bool> matrixReducedWidth{false};
constexpr float PI=3.14159265358979323846F;
constexpr int16_t RADAR_RADIUS_PIXELS=15,RADAR_CENTER_Y=15;
constexpr uint32_t AIRPORT_HALF_PAUSE_MS=800,AIRPORT_HALF_PIXEL_MS=80,AIRPORT_RETURN_PIXEL_MS=20;
float displayRadiusKm=15,viewHeading=0;
struct Display{
 uint16_t pixels[FLIGHTWALL_MATRIX_HEIGHT][128]={};int calls=0;
 void fillRect(int x,int y,int w,int h,uint16_t color){assert(x>=0 && y>=0 && x+w<=128 && y+h<=FLIGHTWALL_MATRIX_HEIGHT);for(int row=y;row<y+h;++row)for(int col=x;col<x+w;++col)pixels[row][col]=color;}
 int height(){return FLIGHTWALL_MATRIX_HEIGHT;}void show(){++calls;}
}matrix;
constexpr uint8_t PAGE_COUNT=8;
uint8_t currentPageSlot=0,pageOrder[8]={0,1,2,3,4,5,6,7};
bool pageEnabled[8]={true,true,true,true,true,true,true,true};
String customPageText="test";
bool airportNamesPageHasData(){return true;}
uint32_t matrixShowLastUs=0,matrixShowMaxUs=0,matrixShowCount=0;
uint32_t micros(){static uint32_t n=0;return ++n;}
'''+funcs+r'''
int main(){
 assert(matrixLayoutWidth()==128);matrixReducedWidth=true;
 const int width=FLIGHTWALL_HAS_MATRIX?125:128;
 assert(matrixLayoutWidth()==width);
 currentPageSlot=7;assert(selectEnabledPage());
 const bool split=FLIGHTWALL_HAS_MATRIX && FLIGHTWALL_MATRIX_HEIGHT==64;
 assert(currentPageSlot==(split?0:7));
 for(int i=0;i<7;++i)pageEnabled[i]=false;
 currentPageSlot=7;assert(selectEnabledPage()==!split);
 assert(pageEnabled[7]); // selection must not mutate stored page preferences
 for(int i=0;i<7;++i)pageEnabled[i]=true;
 // Every pixel pattern/fill is masked at publication; usable pixels unchanged.
 for(int n=0;n<8;++n){
  matrix.fillRect(0,0,128,FLIGHTWALL_MATRIX_HEIGHT,n+1);showMatrix();
  for(int y=0;y<FLIGHTWALL_MATRIX_HEIGHT;++y)for(int x=0;x<128;++x)assert(matrix.pixels[y][x]==(x<width?n+1:0));
 }
 assert(matrixShowCount==8 && matrix.calls==8);
 int16_t x=0,y=0;
 assert(radarPointCoordinates(0,0,x,y,width));assert(x==width/2 && y==15);
 assert(radarPointCoordinates(15,90,x,y,width));assert(x==width/2+15 && y==15);
 // Default collection/web projection stays physically centered at 64.
 assert(radarPointCoordinates(0,0,x,y));assert(x==64);
 assert(!radarPointCoordinates(100,90,x,y,width));
 const String text="A long airport name beyond the display";
 const int travel=text.length()*6-width;
 assert(airportTextX(text,width,0)==0);
 assert(airportTextX(text,width,800+travel*80)==-travel);
 assert(airportTextX(text,width,1600+travel*80+travel*20)==0);
 assert(airportScrollDuration(text,width)==2400U+travel*100U);
 assert(airportTextX("ABC",width,9999)==0);
 if(FLIGHTWALL_HAS_MATRIX)assert(airportScrollDuration(text,125)==airportScrollDuration(text,128)+300);
 matrixReducedWidth=false;assert(matrixLayoutWidth()==128);
 matrix.fillRect(0,0,128,FLIGHTWALL_MATRIX_HEIGHT,0xabcd);showMatrix();assert(matrix.pixels[FLIGHTWALL_MATRIX_HEIGHT-1][127]==0xabcd);
 std::cout<<"Matrix layout: width, masking every frame, restoration, radar and scroll bounds passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-layout-') as folder:
 p=Path(folder);(p/'test.cpp').write_text(code)
 for variant,height in ((0,32),(1,32),(1,64)):
  binary=p/(str(variant)+'-'+str(height))
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined','-DFLIGHTWALL_HAS_MATRIX='+str(variant),'-DFLIGHTWALL_MATRIX_HEIGHT='+str(height),'-I'+str(root/'tools/test_support'),str(p/'test.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
