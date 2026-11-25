#!/usr/bin/env python3
"""Exercise the actual blank-frame indicator with both panel heights and headless."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];s=(root/'src/main.cpp').read_text()
a=s.index('void drawIdleIndicator() {');f=s[a:s.index('\n}\n',a)+3]
idle=s[s.index('void drawIdleDisplay()'):s.index('\n\nbool selectEnabledPage()')]
assert idle.index('drawPersistentRadar();')<idle.rindex('drawIdleIndicator();')<idle.rindex('showMatrix();')
radar=s[s.index('void drawRadarPage()'):s.index('void drawPersistentRadar()')]
assert 'idleErrorPixel' not in radar
code=r'''
#include <Arduino.h>
#include <cassert>
#include <cstdint>
#include <algorithm>
constexpr int MATRIX_WIDTH=128;
constexpr uint32_t IDLE_BLINK_HALF_PERIOD_MS=3000;
bool displayEnabled=true,quiet=false;
uint32_t clockMs=0;uint32_t millis(){return clockMs;}
bool quietHoursAreActive(){return quiet;}
uint16_t idlePixel=0x2104,idleErrorPixel=0x2000;
String lastError;
int width=128;int16_t matrixLayoutWidth(){return width;}
struct Display {
 uint16_t pixels[FLIGHTWALL_MATRIX_HEIGHT*128]={};int drawn=0;
 int16_t height(){return FLIGHTWALL_MATRIX_HEIGHT;}
 uint16_t* getBuffer(){return pixels;}
 void drawPixel(int x,int y,uint16_t c){assert(x==0 && y==height()-1);pixels[y*128+x]=c;++drawn;}
 void clear(){std::fill(pixels,pixels+FLIGHTWALL_MATRIX_HEIGHT*128,0);drawn=0;}
}matrix;
'''+f+r'''
int main(){
 drawIdleIndicator();assert(matrix.drawn==(FLIGHTWALL_HAS_MATRIX?1:0));
#if FLIGHTWALL_HAS_MATRIX
 assert(matrix.pixels[(matrix.height()-1)*128]==idlePixel);
 matrix.clear();lastError="network";drawIdleIndicator();assert(matrix.pixels[(matrix.height()-1)*128]==idleErrorPixel);
 matrix.clear();clockMs=3000;drawIdleIndicator();assert(matrix.drawn==0);
 clockMs=6000;quiet=true;drawIdleIndicator();assert(matrix.drawn==0);
 quiet=false;displayEnabled=false;drawIdleIndicator();assert(matrix.drawn==0);displayEnabled=true;
 // Any content anywhere on either half suppresses the indicator.
 for(int y=0;y<matrix.height();++y){matrix.clear();matrix.pixels[y*128+64]=1;drawIdleIndicator();assert(matrix.drawn==0);}
 width=125;matrix.clear();matrix.pixels[127]=1;drawIdleIndicator();assert(matrix.drawn==1); // masked column is not visible
 matrix.clear();matrix.pixels[124]=1;drawIdleIndicator();assert(matrix.drawn==0);
#endif
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-idle-') as folder:
 p=Path(folder);(p/'test.cpp').write_text(code)
 for matrix,height in ((0,32),(1,32),(1,64)):
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',f'-DFLIGHTWALL_HAS_MATRIX={matrix}',f'-DFLIGHTWALL_MATRIX_HEIGHT={height}','-I'+str(root/'tools/test_support'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
  subprocess.run([str(p/'test')],check=True)
print('Blank-only indicator: both physical heights, full-frame content, narrow mask, blink, night/off and headless passed')
