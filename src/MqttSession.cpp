#include "MqttSession.hpp"
#include <cstdio>

void MqttWireObserver::complete() {
  const uint8_t type=header>>4;
  if(type==2 && remaining==2)connack=prefix[1];
  else if(type==9 && remaining>=3 && remaining<=8){
    const uint16_t id=(uint16_t(prefix[0])<<8)|prefix[1];
    for(unsigned i=0;i<4;++i)if(pending[i] && pending[i]==id)subscription[i]=prefix[2];
  } else if(type==13 && !remaining)++pongs;
  phase=0;
}
void MqttWireObserver::byte(uint8_t value) {
  if(!phase){header=value;remaining=position=digits=0;multiplier=1;phase=1;return;}
  if(phase==1){
    if(++digits>4){++malformed;phase=0;return;}
    remaining+=(value&127U)*multiplier;
    if(value&128){if(digits==4){++malformed;phase=0;}else multiplier*=128;return;}
    phase=2;if(!remaining)complete();return;
  }
  if(position<sizeof(prefix))prefix[position]=value;
  if(++position==remaining)complete();
}
void MqttWireObserver::subscriptionWrite(int slot,const uint8_t *data,size_t length) {
  if(slot<0 || slot>=4 || length<4 || data[0]!=0x82)return;
  size_t at=1;unsigned digits=0;
  while(at<length && (data[at++]&128)){if(++digits==4)return;}
  if(at+2>length)return;
  pending[slot]=(uint16_t(data[at])<<8)|data[at+1];subscription[slot]=-2;
}
void mqttClientId(char *out,size_t size,const uint8_t mac[6],bool test) {
  std::snprintf(out,size,"%s%02x%02x%02x%02x%02x%02x",test?"fw-test-":"flightwall-",
      mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}
const char *mqttStateText(int code) {
  switch(code){
    case 0:return "MQTT OK";
    case -20:return "DNS lookup failed";
    case -21:return "MQTT buffer allocation failed";
    case -22:return "MQTT packet write failed";
    case -2:return "TCP connection failed";
    case -3:return "MQTT connection lost";
    case -4:return "MQTT response timeout";
    case 1:return "MQTT protocol rejected";
    case 2:return "MQTT client ID rejected";
    case 3:return "MQTT broker unavailable";
    case 4:return "MQTT credentials rejected";
    case 5:return "MQTT authorization rejected";
    default:return "MQTT disconnected";
  }
}
MqttSession::MqttSession(Validator validator):client_(transport_),validator_(validator) {
  bufferOk_=client_.setBufferSize(512);
  client_.setKeepAlive(45);
  client_.setSocketTimeout(5);
  client_.setCallback([this](char *topic,uint8_t *data,unsigned length){message(topic,data,length);});
}
bool MqttSession::connect(const char *id) {
  close();report={};transport_.wire={};transport_.writeFailed=false;
  std::snprintf(report.clientId,sizeof(report.clientId),"%s",id);
  if(!bufferOk_)bufferOk_=client_.setBufferSize(512);
  if(!bufferOk_){report.code=-21;return false;}
  IPAddress address;uint32_t started=millis();
  report.dnsOk=WiFi.hostByName(settings.host.c_str(),address)==1;
  report.dnsMs=millis()-started;
  if(!report.dnsOk){report.code=-20;return false;}
  std::snprintf(report.ip,sizeof(report.ip),"%u.%u.%u.%u",address[0],address[1],address[2],address[3]);
  started=millis();report.tcpOk=transport_.connect(address,settings.port)==1;report.tcpMs=millis()-started;
  if(!report.tcpOk){report.code=-2;return false;}
  client_.setServer(address,settings.port);
  started=millis();report.connected=settings.user.length()
      ?client_.connect(id,settings.user.c_str(),settings.password.c_str()):client_.connect(id);
  report.connackMs=millis()-started;report.code=client_.state();
  if(!report.connected && transport_.writeFailed)report.code=-22;
  if(!report.connected){transport_.stop();return false;}
  report.connectedAtMs=millis();
  for(unsigned i=0;i<4;++i)if(settings.topics[i].length()){
    transport_.subscriptionSlot=i;
    if(!client_.subscribe(settings.topics[i].c_str()))transport_.wire.subscription[i]=-3;
    transport_.subscriptionSlot=-1;
  }
  syncReport();return true;
}
void MqttSession::syncReport() {
  for(unsigned i=0;i<4;++i)report.subscription[i]=transport_.wire.subscription[i];
  report.pongs=transport_.wire.pongs;
}
bool MqttSession::step() {
  report.connected=client_.loop();report.code=client_.state();syncReport();return report.connected;
}
void MqttSession::close() { if(bufferOk_)client_.disconnect();transport_.stop();report.connected=false; }
void MqttSession::message(char *topic,uint8_t *payload,unsigned length) {
  for(unsigned i=0;i<4;++i)if(settings.topics[i].length() && settings.topics[i]==topic){
    ++report.received[i];report.lastValid[i]=false;float value=0;char text[49];
    if(length>48 || !length || std::memchr(payload,0,length)){++report.invalid[i];return;}
    std::memcpy(text,payload,length);text[length]=0;
    if(!validator_ || !validator_(i,text,value)){++report.invalid[i];return;}
    ++report.valid[i];report.lastValid[i]=true;report.values[i]=value;report.updated[i]=millis();return;
  }
}
bool MqttSession::subscriptionsReady()const {
  for(unsigned i=0;i<4;++i)if(settings.topics[i].length() && (report.subscription[i]<0 || report.subscription[i]>2))return false;
  return true;
}
bool MqttSession::subscriptionsDenied()const {
  for(unsigned i=0;i<4;++i)if(report.subscription[i]>=3 || report.subscription[i]==-3)return true;
  return false;
}
bool MqttSession::allDataValid()const {
  bool any=false;for(unsigned i=0;i<4;++i)if(settings.topics[i].length()){any=true;if(!report.valid[i] || !report.lastValid[i])return false;}return any;
}
