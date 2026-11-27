#!/usr/bin/env python3
"""Render actual 64-row page code, validate bands, scrolling and split/full rotation."""
from pathlib import Path
import subprocess,tempfile,os,re
root=Path(__file__).resolve().parents[1];s=(root/'src/main.cpp').read_text()
def function(signature):
 a=s.index(signature+' {');return s[a:s.index('\n}\n',a)+3]
render=s[s.index('struct LargePageContent {'):s.index('\n#endif\n\nuint32_t largePageDuration')]
functions='\n'.join(function(x) for x in ['bool largeFullPagesEnabled()','bool selectEnabledPage()','uint8_t currentConfiguredPage()','void advancePage()','void resetPageTransition()','void drawPersistentRadar()','uint32_t largePageDuration(uint8_t page)','void drawEmergencyPage()'])
# Exact pinned GFX font for pixel previews; tests also work before dependencies download.
fonts=list((root/'.pio/libdeps').glob('*/Adafruit GFX Library/glcdfont.c'))
font=[]
if fonts:
 text=fonts[0].read_text();text=text[text.index('static const unsigned char font[]'):];text=text[text.index('{')+1:text.index('};')];text=re.sub(r'//[^\n]*','',text)
 font=[int(x,16) for x in re.findall(r'0x[0-9A-Fa-f]+',text)]
if not font:font=[0x7f]*1280
code=r'''
#include <Arduino.h>
#include "LargeMatrixLayout.hpp"
#include <atomic>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <string>
#include <iostream>
using std::max;using std::min;using std::isnan;
const uint8_t font[]={FONT};
constexpr int AIRPORT_TEXT_CANVAS_WIDTH=768;
constexpr uint16_t white=65535,cyan=2047,yellow=65504,green=2016,orange=0xfd20,red=63488;
int width=128;int16_t matrixLayoutWidth(){return width;}
uint32_t now=1000,currentPageStarted=1000;
uint32_t millis(){return now;}
std::atomic<uint8_t>matrixLargeLayout{LargeMatrixLayout::Full};
bool largeRadarPhase=false,inside=true,radarFullHeight=false;
int radarDrawOffsetY=0,radarCalls=0,persistentCalls=0;
uint8_t transitionPercent=100;
void setTransitionPercent(uint8_t v){transitionPercent=v;}
enum class TransitionPhase{Normal};TransitionPhase transitionPhase=TransitionPhase::Normal;
int activeRenderedPage=-1;
constexpr uint8_t PAGE_COUNT=8;
uint8_t currentPageSlot=0,pageOrder[]={0,1,2,3,4,5,6,7};
bool pageEnabled[]={true,true,true,true,true,true,true,false};
String customPageText="FlightWall";
struct Flight{
 String flight="KLM877",hex="484ABC",type="B77W",routeCodes="AMS>BOM",routeNames="Amsterdam > Mumbai";
 String originName="Amsterdam",destinationName="Mumbai",origin="AMS",destination="BOM";
 String typeDescription="Boeing 777-300ER",specialType="",airline="KLM Royal Dutch",motion="approaching";
 float distanceKm=6.8f,altitudeFt=23375,speedKmh=862,cpaKm=1.1f,relativeBearing=31;
 String squawk="7700";int emergencyCount=3,emergencyIndex=2;
 bool onGround=false;
}flight;
String clipped(const String&t,int n){return t.substring(0,min<int>(n,t.length()));}
String displayAscii(const String&t){return t;}
String limitedAirportText(const String&t){return clipped(t,128);}
String compactLoopRouteCodes(){return flight.routeCodes;}
String compactLoopRouteNames(){return flight.routeNames;}
uint16_t customPixelMs=80;uint16_t customScrollPixelMs(){return customPixelMs;}
String compactRoute(){return flight.origin+">"+flight.destination;}
String compactLookDirection(float){return "rechts / NW";}
String cpaCountdownText(const Flight&){return "27s";}
String displayedAirline(){return flight.airline;}
String cpaPageDetails(){return flight.specialType.length()?flight.specialType+" | "+flight.typeDescription:flight.typeDescription;}
uint16_t aircraftColor(){return cyan;}
const char*uiText(const char*a,const char*){return a;}
bool flightHasMultiHopRoute(){return flight.routeCodes.length()>8;}
bool airportNamesPageHasData(){return flight.origin.length()||flight.routeCodes.length();}
bool aircraftIsInsideDisplayRadius(){return inside;}
struct Canvas {
 uint8_t pixels[8][768]={};
 uint8_t*getBuffer(){return &pixels[0][0];}
 void fillScreen(int){std::fill(&pixels[0][0],&pixels[0][0]+8*768,0);}
 void setFont(void*){}void setTextWrap(bool){}void setTextSize(int){}void setTextColor(int){}void setCursor(int,int){}
 void print(const String&s){for(unsigned i=0;i<s.length()&&i<128;++i)for(int x=0;x<5;++x)for(int y=0;y<8;++y)pixels[y][i*6+x]=(font[(unsigned char)s[i]*5+x]>>y)&1;}
 bool getPixel(int x,int y){assert(x>=0&&x<768&&y>=0&&y<8);return pixels[y][x];}
}airportTextCanvas;
struct Matrix {
 uint16_t pixels[64][128]={};
 void drawPixel(int x,int y,uint16_t c){assert(x>=2&&x<width-2&&y>=0&&y<64);pixels[y][x]=c;}
 void fillScreen(int){clear();}
 void clear(){std::fill(&pixels[0][0],&pixels[0][0]+64*128,0);}
}matrix;
bool isEmergencySquawk(const String&s){return s=="7700"||s=="7500"||s=="7600";}
String emergencyMeaning(const String&s){return s=="7600"?"RADIO":s=="7500"?"HIJACK":"EMERGENCY";}
void drawLine(int16_t x,int16_t y,const String&text,uint16_t color,uint8_t size=1){
 for(unsigned i=0;i<text.length();++i)for(int col=0;col<5*size;++col)for(int row=0;row<8*size;++row)
  if((font[(unsigned char)text[i]*5+col/size]>>(row/size))&1)matrix.drawPixel(x+i*6*size+col,y+row,color);
}
void drawRadarPage(){++radarCalls;if(radarFullHeight)assert(radarDrawOffsetY==0);else{assert(radarDrawOffsetY==32);++persistentCalls;}}
'''.replace('FONT',','.join(map(str,font)))+render+'\n'+functions+r'''
int main(int argc,char**argv){
 uint8_t mode=0;assert(!LargeMatrixLayout::parse(nullptr,mode));assert(!LargeMatrixLayout::parse("FULL",mode));
 assert(!LargeMatrixLayout::parse("",mode));assert(LargeMatrixLayout::valid(99)==LargeMatrixLayout::Full);
 assert(LargeMatrixLayout::parse("split",mode)&&mode==0);assert(LargeMatrixLayout::parse("full",mode)&&mode==1);
 for(int w:{128,125}){
  width=w;
  for(int p=0;p<7;++p){
   LargePageContent c=largePageContent(p);
   int bottom=-4;
   for(int r=0;r<4;++r)if(c.rows[r].length()){
    assert(c.y[r]>=bottom+4);bottom=c.y[r]+8*c.sizes[r];assert(bottom<=64);
   }
   for(uint32_t t:{0U,800U,4000U,1000000U}){now=currentPageStarted+t;matrix.clear();drawLargeConfiguredPage(p);}
   assert(!matrix.pixels[63][127]);
   if(argc>1&&w==128){now=currentPageStarted;matrix.clear();drawLargeConfiguredPage(p);std::ofstream out(std::string(argv[1])+"/page-"+std::to_string(p)+".ppm",std::ios::binary);out<<"P6\n128 64\n255\n";for(auto&row:matrix.pixels)for(uint16_t pixel:row){const char rgb[]={char(((pixel>>11)&31)*255/31),char(((pixel>>5)&63)*255/63),char((pixel&31)*255/31)};out.write(rgb,3);}}
  }
  customPageText="A very long custom text that must finish its complete forward and return scroll";
  flight.typeDescription="Robinson R44 Raven II helicopter with a long description";flight.specialType="Helicopter";
  flight.routeCodes="AMS>FRA>JFK>AMS";flight.routeNames="Amsterdam > Frankfurt > New York > Amsterdam";
  flight.originName="An extraordinarily long departure airport";flight.destinationName="An extraordinarily long arrival airport";
  for(int p=0;p<7;++p){uint32_t d=largePageDuration(p);if(p==2||p==3||p==4||p==6)assert(d>0);now=currentPageStarted+d;matrix.clear();drawLargeConfiguredPage(p);}
  auto c=largePageContent(2);assert(c.rows[3].indexOf('R')>=0);
  for(int size:{1,2}){
   const unsigned n=70;int travel=LargeMatrixLayout::travel(n,w-4,size);uint32_t d=LargeMatrixLayout::duration(n,w-4,size);
   assert(LargeMatrixLayout::offset(n,w-4,size,0)==0);
   assert(LargeMatrixLayout::offset(n,w-4,size,800+travel*80)==-travel);
   assert(LargeMatrixLayout::offset(n,w-4,size,d)==0);
  }
 }
 // Emergency owns the full frame; large squawk and identity stay within 125/128.
 for(int w:{125,128})for(const char*sq:{"7500","7600","7700"}){width=w;flight.squawk=sq;drawEmergencyPage();assert(matrix.pixels[63][127]==0);}
 customPixelMs=25;const auto fast=largePageDuration(4);customPixelMs=120;assert(largePageDuration(4)>fast);customPixelMs=80;
 width=128;matrixLargeLayout=LargeMatrixLayout::Full;currentPageSlot=0;largeRadarPhase=false;
 assert(currentConfiguredPage()==0);advancePage();assert(currentConfiguredPage()==7&&currentPageSlot==0);
 advancePage();assert(currentConfiguredPage()==1&&currentPageSlot==1);
 pageEnabled[2]=false;advancePage();assert(currentConfiguredPage()==7);advancePage();assert(currentConfiguredPage()==3);
 resetPageTransition();assert(!largeRadarPhase);assert(!pageEnabled[7]); // no mutation to saved preferences
 drawPersistentRadar();assert(radarCalls==0);drawLargeConfiguredPage(7);assert(radarCalls==1&&!radarFullHeight);
 inside=false;drawPersistentRadar();assert(persistentCalls==1);inside=true;
 matrixLargeLayout=LargeMatrixLayout::Split;drawPersistentRadar();assert(persistentCalls==2);
 currentPageSlot=0;advancePage();assert(currentPageSlot==1&&!largeRadarPhase);
 for(int i=0;i<7;++i)pageEnabled[i]=false;assert(!selectEnabledPage());
 std::cout<<"Actual 64-row pages: separated bands, 128/125 bounds, long names/custom/type/route scroll, alternating radar, split/idle fallback and settings preservation passed\n";
}
'''
for key in ['matrix_layout','largeLayout','matrixLayout']:assert key in s
assert 'if (hasLayout) preferences.putUChar("largeLayout", restoredLayout)' in s
assert 'preferences.getUChar("largeLayout", LargeMatrixLayout::Full)' in s
with tempfile.TemporaryDirectory(prefix='flightwall-large-pages-') as tmp:
 p=Path(tmp);arduino=(root/'tools/test_support/Arduino.h').read_text().replace('#include <string>','#include <string>\n#include <sstream>\n#include <iomanip>').replace('  const char *c_str()', '  String(float number, int digits) { std::ostringstream stream;stream<<std::fixed<<std::setprecision(digits)<<number;value_=stream.str(); }\n  const char *c_str()')
 arduino=arduino.replace('  bool operator==', '  bool operator!=(const String &other) const { return !(*this==other); }\n  bool operator==')
 (p/'Arduino.h').write_text(arduino);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined','-DFLIGHTWALL_HAS_MATRIX=1','-DFLIGHTWALL_MATRIX_HEIGHT=64','-I'+str(p),'-I'+str(root/'include'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 output=os.environ.get('FLIGHTWALL_PREVIEW_DIR')
 if output:Path(output).mkdir(parents=True,exist_ok=True)
 subprocess.run([str(p/'test')]+([output] if output else []),check=True)
