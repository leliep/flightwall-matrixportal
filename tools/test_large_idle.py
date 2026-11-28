#!/usr/bin/env python3
"""Exercise the actual idle display decision and mode/geometry validation."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];s=(root/'src/main.cpp').read_text()
a=s.index('void drawIdleDisplay() {');f=s[a:s.index('\n}\n',a)+3]
code=r'''
#include <Arduino.h>
#include "RadarIdle.hpp"
#include <atomic>
#include <cassert>
#include <cmath>
using std::isfinite;
std::atomic<uint8_t>matrixIdleRadar{RadarIdle::Lower};
bool radarFullHeight=false,environmentIdleDisplayEnabled=true,environmentSensorAvailable=true;
float ambientTemperatureC=20,relativeHumidityPercent=50,airPressureHpa=1013;
bool pageEnabled[8]={};int climate=0,radar=0,lower=0,pixel=0,shown=0;
struct Display{void fillScreen(int){climate=radar=lower=pixel=shown=0;}}matrix;
bool mqttWeatherAvailable(){return false;}
void drawCompactClimateDisplay(bool,bool){++climate;}
void drawRadarPage(){++radar;assert(radarFullHeight);}
void drawPersistentRadar(){
#if FLIGHTWALL_HAS_MATRIX && FLIGHTWALL_MATRIX_HEIGHT==64
 ++lower;
#endif
}
void drawIdleIndicator(){if(!climate&&!radar&&!lower)++pixel;}
void showMatrix(){++shown;}
'''+f+r'''
int main(){
 uint8_t mode=RadarIdle::Lower;
 assert(!RadarIdle::parse("bad",mode)&&mode==RadarIdle::Lower);
 assert(!RadarIdle::parse(nullptr,mode));
 for(uint8_t m=0;m<3;++m){assert(RadarIdle::parse(RadarIdle::name(m),mode)&&mode==m);}
 assert(RadarIdle::valid(255)==RadarIdle::Lower);
 for(uint8_t m=0;m<3;++m){
  matrixIdleRadar=m;drawIdleDisplay();assert(shown==1&&!radarFullHeight);
#if FLIGHTWALL_HAS_MATRIX && FLIGHTWALL_MATRIX_HEIGHT==64
  assert(climate==(m==RadarIdle::Lower));assert(lower==(m==RadarIdle::Lower));
  assert(radar==(m==RadarIdle::Full));assert(pixel==(m==RadarIdle::Off));
#else
  assert(climate==1&&!radar&&!lower&&!pixel);
#endif
 }
 assert(RadarIdle::project(15,15,31)==31);
 assert(RadarIdle::project(0,15,31)==16);
 assert(RadarIdle::project(30,15,31)==46);
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-large-idle-') as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 for matrix,height in ((0,32),(1,32),(1,64)):
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',f'-DFLIGHTWALL_HAS_MATRIX={matrix}',f'-DFLIGHTWALL_MATRIX_HEIGHT={height}','-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
  subprocess.run([str(p/'test')],check=True)
print('Large-panel idle: off/lower/full, weather suppression, state reset, strict modes and unscaled translated geometry passed')
