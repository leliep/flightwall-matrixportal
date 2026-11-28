#pragma once
#include <cmath>
#include <cstdint>
namespace WeatherFallback {
enum Source : uint8_t { Missing, Bme280, MqttOutdoor };
struct Samples {
  float values[4] = {NAN, NAN, NAN, NAN}; // temperature, humidity, pressure, dew point
  uint32_t updated[4] = {};
};
struct Resolved {
  float temperature=NAN, humidity=NAN, stationPressure=NAN, seaPressure=NAN, dewPoint=NAN;
  Source sources[4] = {Missing, Missing, Missing, Missing};
  uint32_t updated[4]={};
  bool moldEligible=false;
  bool outdoorClimate() const { return sources[0]==MqttOutdoor || sources[1]==MqttOutdoor; }
};
inline const char *name(Source source) {
  return source==Bme280?"bme280":source==MqttOutdoor?"mqtt_outdoor":"unavailable";
}
inline bool valid(unsigned index,float n) {
  if(!std::isfinite(n))return false;
  if(index==0 || index==3)return n>=-100 && n<=70;
  if(index==1)return n>=0 && n<=100;
  return index==2 && n>=300 && n<=1100;
}
inline bool fresh(uint32_t stamp,uint32_t now,uint32_t ttl) {return stamp && now-stamp<=ttl;}
inline float seaLevel(float pressure,float elevation) {
  const float factor=1-elevation/44330.0f;
  return std::isfinite(pressure)&&std::isfinite(elevation)&&factor>0?pressure/std::pow(factor,5.255f):NAN;
}
inline float dewPoint(float temperature,float humidity) {
  if(!valid(0,temperature)||!valid(1,humidity)||humidity<=0)return NAN;
  const float gamma=std::log(humidity/100.0f)+17.62f*temperature/(243.12f+temperature);
  return 243.12f*gamma/(17.62f-gamma);
}
inline Resolved resolve(const Samples &local,const Samples &remote,uint32_t now,
                        bool sensor,bool mqtt,float localElevation,float remoteElevation,
                        uint32_t localTtl=15000,uint32_t remoteTtl=900000) {
  Resolved result;float values[3]={NAN,NAN,NAN};
  for(unsigned i=0;i<3;++i){
    if(sensor && valid(i,local.values[i]) && fresh(local.updated[i],now,localTtl)){
      values[i]=local.values[i];result.sources[i]=Bme280;result.updated[i]=local.updated[i];
    }else if(mqtt && valid(i,remote.values[i]) && fresh(remote.updated[i],now,remoteTtl)){
      values[i]=remote.values[i];result.sources[i]=MqttOutdoor;result.updated[i]=remote.updated[i];
    }
  }
  result.temperature=values[0];result.humidity=values[1];result.stationPressure=values[2];
  result.seaPressure=seaLevel(values[2],result.sources[2]==Bme280?localElevation:remoteElevation);
  result.moldEligible=result.sources[0]==Bme280 && result.sources[1]==Bme280 && values[1]>0;
  if(result.moldEligible){result.dewPoint=dewPoint(values[0],values[1]);result.sources[3]=Bme280;result.updated[3]=(now-local.updated[0]>=now-local.updated[1])?local.updated[0]:local.updated[1];}
  else if(mqtt && valid(3,remote.values[3]) && fresh(remote.updated[3],now,remoteTtl)){
    result.dewPoint=remote.values[3];result.sources[3]=MqttOutdoor;result.updated[3]=remote.updated[3];
  }else if(result.sources[0]==MqttOutdoor && result.sources[1]==MqttOutdoor){
    result.dewPoint=dewPoint(values[0],values[1]);result.sources[3]=MqttOutdoor;result.updated[3]=(now-remote.updated[0]>=now-remote.updated[1])?remote.updated[0]:remote.updated[1];
  }
  return result;
}
}
