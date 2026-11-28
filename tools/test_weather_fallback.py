#!/usr/bin/env python3
"""Exercise actual source selection, N/A risk gate and sensorless weather drawing."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];s=(root/'src/main.cpp').read_text()
def function(signature):
 a=s.index(signature+' {');return s[a:s.index('\n}\n',a)+3]
code=r'''
#include <Arduino.h>
#include "WeatherFallback.hpp"
#include <cassert>
#include <atomic>
#include <cmath>
#include <vector>
#include <iostream>
using std::isfinite;
using WeatherFallback::Samples;using WeatherFallback::resolve;
std::atomic<bool>outdoorClimateInUse{false};
float ambientTemperatureC=NAN,relativeHumidityPercent=NAN,stationPressureHpa=NAN,airPressureHpa=NAN,dewPointC=NAN,moldCriticalSurfaceTemperatureC=NAN;
WeatherFallback::Resolved resolvedClimate;
enum class MoldRiskLevel{Unavailable,Normal,Elevated,High};MoldRiskLevel moldRiskLevel=MoldRiskLevel::Unavailable;
int assessed=0,cleared=0;
void updateMoldRisk(float,float,uint32_t){++assessed;moldRiskLevel=MoldRiskLevel::High;moldCriticalSurfaceTemperatureC=22;}
void resetMoldRisk(){++cleared;moldRiskLevel=MoldRiskLevel::Unavailable;moldCriticalSurfaceTemperatureC=dewPointC=NAN;}
const char*uiText(const char*de,const char*){return de;}
int weatherMutex=0;
int pdMS_TO_TICKS(int n){return n;}constexpr int pdTRUE=1;
int xSemaphoreTake(int,int){return 1;}void xSemaphoreGive(int){}
bool weatherValueFresh(uint32_t){return false;}
String weatherCompassDirection(float){return "N";}
float windSpeedKph=NAN,windGustKph=NAN,rain24Mm=NAN,windDirectionDegrees=NAN;
uint32_t windDirectionUpdated=0,windSpeedUpdated=0,windGustUpdated=0,rainUpdated=0;
constexpr uint16_t yellow=1,red=2,cyan=3,blue=4,orange=5;
struct{void drawCircle(int,int,int,int){}}matrix;
std::vector<String>texts;
void drawLine(int16_t,int16_t,const String&t,uint16_t){texts.push_back(t);}
int16_t matrixLayoutWidth(){return 128;}
'''+function('void applyEnvironmentWeather(const WeatherFallback::Resolved &weather, uint32_t now)')+function('const char *moldRiskText()')+function('const char *moldRiskCssClass()')+function('void drawCompactClimateDisplay(bool bmeAvailable, bool mqttAvailable)')+r'''
int main(){
 Samples local,remote;uint32_t now=100000;
 remote.values[0]=1.83333333333f;remote.values[1]=79;remote.values[2]=1022.31999304f;remote.values[3]=-1.41662900881f;
 for(auto&t:remote.updated)t=now;
 auto r=resolve(local,remote,now,false,true,900,40);
 assert(r.outdoorClimate()&&!r.moldEligible&&r.sources[0]==WeatherFallback::MqttOutdoor);
 assert(std::fabs(r.seaPressure-1027.2f)<.1f);assert(r.stationPressure==remote.values[2]);
 assert(std::fabs(r.dewPoint-remote.values[3])<.0001f);
 assert(resolve(local,remote,now,false,true,0,40).seaPressure==r.seaPressure);
 applyEnvironmentWeather(r,now);assert(assessed==0&&cleared==1&&String(moldRiskText())==String("N/A"));
 assert(String(moldRiskCssClass())==String("neutral"));assert(std::isnan(moldCriticalSurfaceTemperatureC));
 drawCompactClimateDisplay(false,false);bool temperature=false,humidity=false,pressure=false;
 for(auto&t:texts){temperature|=t==String("1.8");humidity|=t==String("79%");pressure|=t==String("1027hPa");}
 assert(temperature&&humidity&&pressure);
 // Real local paired climate permits the existing indoor risk assessment.
 local.values[0]=20;local.values[1]=50;local.values[2]=1000;for(auto&t:local.updated)t=now;
 r=resolve(local,remote,now,true,true,200,40);assert(r.moldEligible&&!r.outdoorClimate());
 assert(r.temperature==20&&r.humidity==50&&r.stationPressure==1000);assert(r.seaPressure>1020);
 applyEnvironmentWeather(r,now);assert(assessed==1&&moldRiskLevel==MoldRiskLevel::High);
 // Mixed indoor/outdoor T/RH must clear even an existing high-risk timer/state.
 local.values[1]=NAN;r=resolve(local,remote,now,true,true,200,40);applyEnvironmentWeather(r,now);
 assert(assessed==1&&moldRiskLevel==MoldRiskLevel::Unavailable&&String(moldRiskText())==String("N/A"));
 assert(std::isnan(moldCriticalSurfaceTemperatureC));
 // Pressure fallback alone never disables an otherwise valid local T/RH pair.
 local.values[1]=50;local.values[2]=NAN;r=resolve(local,remote,now,true,true,900,40);
 assert(r.moldEligible&&r.sources[2]==WeatherFallback::MqttOutdoor);assert(std::fabs(r.seaPressure-1027.2f)<.1f);
 auto disabled=resolve({},remote,now,false,false,0,40);assert(std::isnan(disabled.temperature)&&!disabled.moldEligible);
 auto stale=resolve({},remote,now+900001,false,true,0,40);assert(std::isnan(stale.temperature)&&std::isnan(stale.dewPoint));
 auto fresh=resolve({},remote,now+900000,false,true,0,40);assert(std::isfinite(fresh.temperature));
 remote.updated[0]=now-900001;r=resolve({},remote,now,false,true,0,40);assert(std::isnan(r.temperature)&&r.humidity==79);
 remote.updated[0]=UINT32_MAX-100;r=resolve({},remote,200,false,true,0,40);assert(std::isfinite(r.temperature));
 remote.values[0]=-10;remote.values[1]=101;remote.values[2]=0;remote.values[3]=NAN;remote.updated[0]=now;
 r=resolve({},remote,now,false,true,0,40);assert(r.temperature==-10&&std::isnan(r.humidity)&&std::isnan(r.seaPressure));
 remote.values[1]=79;remote.updated[1]=now;r=resolve({},remote,now,false,true,0,40);assert(std::isfinite(r.dewPoint)&&!r.moldEligible);
 assert(!WeatherFallback::valid(0,INFINITY)&&!WeatherFallback::valid(2,1101));
 std::cout<<"Weather fallback: local priority, missing/mixed/stale/disabled values, signed temperatures, independent 40m pressure correction, sensorless display and actual N/A assessment gate passed\n";
}
'''
assert 'serviceEnvironmentSensor();\n  serviceEnvironmentWeather();' in s
assert 'updateMoldRisk(temperature, humidity, now)' not in s
assert 'if(weather.moldEligible)updateMoldRisk' in s
for key in ['mqtt_temperature_topic','mqtt_humidity_topic','mqtt_pressure_topic','mqtt_dew_point_topic','mqtt_weather_elevation_m']:assert key in s
with tempfile.TemporaryDirectory(prefix='flightwall-weather-') as folder:
 p=Path(folder);a=(root/'tools/test_support/Arduino.h').read_text().replace('#include <string>','#include <string>\n#include <sstream>\n#include <iomanip>')
 a=a.replace('  const char *c_str()','  String(float n,int digits){std::ostringstream out;out<<std::fixed<<std::setprecision(digits)<<n;value_=out.str();}\n  const char *c_str()')
 (p/'Arduino.h').write_text(a);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined','-I'+str(p),'-I'+str(root/'include'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
