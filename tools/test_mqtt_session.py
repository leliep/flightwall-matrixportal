#!/usr/bin/env python3
"""Compile real MqttSession and pinned PubSubClient against deterministic sockets."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
main=(root/'src/main.cpp').read_text()
def function(signature):
 a=main.index(signature);return main[a:main.index('\n}\n',a)+3]
a=main.index('class MqttIoScope {');io_class=main[a:main.index('\n};',a)+3]
headers={
'Arduino.h':f'''#pragma once
#include "{root/'tools/test_support/Arduino.h'}"
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdlib>
using byte=uint8_t;using boolean=bool;
unsigned long millis();void delay(unsigned long);void yield();
#define PROGMEM
#define pgm_read_byte_near(p) (*(const unsigned char*)(p))
''',
'IPAddress.h':'''#pragma once
#include <cstdint>
struct IPAddress {uint8_t b[4]={};IPAddress()=default;IPAddress(uint8_t a,uint8_t c,uint8_t d,uint8_t e):b{a,c,d,e}{};uint8_t operator[](unsigned n)const{return b[n];}};
''',
'Print.h':'''#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
class Print {public:virtual ~Print()=default;virtual size_t write(uint8_t)=0;virtual size_t write(const uint8_t*p,size_t n){for(size_t i=0;i<n;++i)write(p[i]);return n;}size_t write(const char*p){return write((const uint8_t*)p,strlen(p));}};
''',
'Stream.h':'''#pragma once
#include "Print.h"
class Stream:public Print{public:virtual int available()=0;virtual int read()=0;virtual int peek()=0;virtual void flush()=0;};
''',
'Client.h':'''#pragma once
#include "Stream.h"
#include "IPAddress.h"
class Client:public Stream{public:using Stream::read;virtual int connect(IPAddress,uint16_t)=0;virtual int connect(const char*,uint16_t)=0;virtual int read(uint8_t*,size_t)=0;virtual void stop()=0;virtual uint8_t connected()=0;virtual operator bool()=0;};
''',
'WiFi.h':'''#pragma once
#include "IPAddress.h"
extern bool dnsOk;
struct Wifi {int hostByName(const char*,IPAddress &ip){ip=IPAddress(192,168,10,50);return dnsOk?1:0;}};
extern Wifi WiFi;
''',
'WiFiClient.h':r'''#pragma once
#include "Client.h"
#include <deque>
#include <string>
#include <vector>
#include <cassert>
extern bool tcpOk,dropSuback;
extern int ackCode,grantCode;
extern unsigned long ackDelay;
extern std::vector<std::string> identities;
class WiFiClient;extern WiFiClient *latestSocket;
class WiFiClient:public Client {
 bool open=false,scheduled=false;unsigned long ackAt=0;std::deque<uint8_t> rx;
 public:
 int connect(IPAddress,uint16_t,int){latestSocket=this;open=tcpOk;return open;}
 int connect(const char*,uint16_t,int){open=tcpOk;return open;}
 int connect(IPAddress a,uint16_t p)override{return connect(a,p,4000);}
 int connect(const char*a,uint16_t p)override{return connect(a,p,4000);}
 size_t write(uint8_t c)override{return write(&c,1);}
 size_t write(const uint8_t *p,size_t n)override{
  if(!open)return 0;
  size_t a=1;while(a<n && (p[a++]&128)){}
  if(p[0]==0x10){
   size_t at=a+10;unsigned len=(unsigned(p[at])<<8)|p[at+1];
   identities.emplace_back((const char*)p+at+2,len);scheduled=true;ackAt=millis()+ackDelay;
  }else if(p[0]==0x82 && !dropSuback){rx.insert(rx.end(),{0x90,3,p[a],p[a+1],static_cast<uint8_t>(grantCode)});}
  else if(p[0]==0xc0)rx.insert(rx.end(),{0xd0,0});
  return n;
 }
 void inject(const std::string &topic,const std::string &payload){
  size_t n=2+topic.size()+payload.size();assert(n<128);rx.push_back(0x30);rx.push_back(n);rx.push_back(topic.size()>>8);rx.push_back(topic.size()&255);rx.insert(rx.end(),topic.begin(),topic.end());rx.insert(rx.end(),payload.begin(),payload.end());
 }
 int available()override{if(scheduled && millis()>=ackAt){scheduled=false;rx.insert(rx.end(),{0x20,2,0,static_cast<uint8_t>(ackCode)});}return rx.size();}
 int read()override{if(!available())return -1;int c=rx.front();rx.pop_front();return c;}
 int read(uint8_t*p,size_t n)override{size_t i=0;for(;i<n && available();++i)p[i]=read();return i;}
 int peek()override{return available()?rx.front():-1;}
 void flush()override{}
 void stop()override{open=false;rx.clear();scheduled=false;}
 uint8_t connected()override{return open;}
 operator bool()override{return open;}
};
'''}
code=r'''
#include "MqttSession.hpp"
#include "WeatherFallback.hpp"
#include "TemporaryPsram.hpp"
#include <esp_heap_caps.h>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <atomic>
#include <mutex>
#include <thread>
#include <chrono>
using std::isfinite;
bool dnsOk=true,tcpOk=true,dropSuback=false;
int ackCode=0,grantCode=0;unsigned long ackDelay=0,clockMs=100,delays=0;
std::vector<std::string> identities;Wifi WiFi;WiFiClient *latestSocket=nullptr;
std::atomic<bool> holdForWeb(false),ioEntered(false),finishWeb(false);
std::timed_mutex state;auto applicationMutex=&state;bool mqttIoActive=false;
constexpr int portMAX_DELAY=0;void xSemaphoreGive(std::timed_mutex*m){m->unlock();}void xSemaphoreTake(std::timed_mutex*m,int){m->lock();}
int xSemaphoreGetMutexHolder(std::timed_mutex*){return 1;}int xTaskGetCurrentTaskHandle(){return 1;}
#define configASSERT assert
unsigned long millis(){return clockMs;}
void delay(unsigned long ms){if(holdForWeb && !finishWeb){ioEntered=true;while(!finishWeb)std::this_thread::sleep_for(std::chrono::milliseconds(1));}clockMs+=ms;++delays;}void yield(){++clockMs;}
void *heap_caps_realloc(void*p,size_t n,uint32_t caps){assert(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return realloc(p,n);}
void *heap_caps_malloc(size_t n,uint32_t caps){return heap_caps_realloc(nullptr,n,caps);}void heap_caps_free(void*p){free(p);}
bool english=false;const char*uiText(const char*de,const char*en){return english?en:de;}
enum class ManualJobKind {None,Adsb,Route,Mqtt};
struct Job {PsramBuffer page;int status=200;bool queued=false,running=false,done=false;uint32_t id=0;ManualJobKind kind=ManualJobKind::None;} manualNetworkJob;
uint32_t applicationRevision=99,mqttTestRevision=0;void noteWebMutation(){++applicationRevision;}
MqttSettings requestedMqttTest,saved;bool mqttWeatherEnabled=false,authenticated=true;
MqttSettings storedMqttSettings(){return saved;}
struct Server {
 std::map<std::string,String> args;std::string body;int status=0;
 bool hasArg(const char*name){return args.count(name)!=0;}
 String arg(const char*name){return args[name];}
 void send(int code,const char*,const String &text){status=code;body=text.c_str();}
 void sendHeader(const char*,const String&){}
} webServer;
bool otaAuthenticated(){if(!authenticated)webServer.status=401;return authenticated;}
'''+io_class+function('bool validFlightWallHost(')+function('bool prepareManualNetworkJob(')+function('void redirectToManualNetworkJob(')+function('bool validMqttClimateTopic(')+function('void handleMqttTest(')+function('String jsonEscape(')+function('void appendMqttReportJson(')+function('bool parseMqttNumber(')+function('bool parseMqttCompassDirection(')+function('bool parseMqttWeatherValue(')+function('void appendRouteTestHtmlText(')+function('void renderMqttTestResult(')+r'''
void packet(MqttWireObserver &o,std::initializer_list<uint8_t> b){for(auto c:b)o.byte(c);}
int main(){
 uint8_t a[]={0x90,0x70,0x69,1,2,3},b[]={0x90,0x70,0x69,4,5,6};char id[32],other[32],test[32];
 mqttClientId(id,sizeof(id),a,false);mqttClientId(other,sizeof(other),b,false);mqttClientId(test,sizeof(test),a,true);
 assert(std::string(id)=="flightwall-907069010203" && std::string(id)!=other && std::string(id)!=test);
 assert(strlen(id)<=23 && strlen(test)<=23);
 MqttSession s(parseMqttWeatherValue);s.settings.host="broker";s.settings.user="sensitive-user";s.settings.password="sensitive-secret";
 for(unsigned i=0;i<4;++i)s.settings.topics[i]=String(i);
 dnsOk=false;assert(!s.connect(id) && s.report.code==-20);dnsOk=true;
 tcpOk=false;assert(!s.connect(id) && s.report.code==-2 && s.report.dnsOk);tcpOk=true;
 ackCode=4;assert(!s.connect(id) && s.report.code==4);ackCode=0;
 ackDelay=6000;assert(!s.connect(id) && s.report.code==-4);assert(delays>0);
 ackDelay=2500;assert(s.connect(id));assert(s.report.connackMs>=2500 && s.report.connackMs<5000);
 assert(identities.back()==id);for(unsigned i=0;i<4;++i)s.step();assert(s.subscriptionsReady());
 clockMs+=46000;s.step();assert(s.report.pongs==1);
 assert(s.connect(id));for(unsigned i=0;i<4;++i)s.step();assert(s.subscriptionsReady()); // reconnect really sends CONNECT
 grantCode=128;assert(s.connect(id));s.step();assert(s.subscriptionsDenied() && !s.subscriptionsReady());grantCode=0;
 dropSuback=true;assert(s.connect(id));for(unsigned i=0;i<4;++i)s.step();assert(!s.subscriptionsReady() && s.report.subscription[0]==-2);dropSuback=false;
 assert(s.connect(id));for(unsigned i=0;i<4;++i)s.step();
 for(unsigned i=0;i<4;++i){latestSocket->inject(std::to_string(i),i==0?"NW":"1.2");s.step();assert(s.report.valid[i]==1 && s.report.lastValid[i]);}
 // New climate subscriptions exercise real wire ACKs and negative temperatures.
 s.settings.topics[4]="outTemp";s.settings.topics[5]="outHum";s.settings.topics[6]="pressure";s.settings.topics[7]="dewpoint";
 assert(s.connect(id));for(unsigned i=0;i<MQTT_TOPIC_COUNT;++i)s.step();assert(s.subscriptionsReady());
 for(unsigned i=0;i<4;++i){latestSocket->inject(std::to_string(i),i==0?"NW":"1.2");s.step();}
 const char*climateTopics[]={"outTemp","outHum","pressure","dewpoint"};
 const char*climateValues[]={"-1.83333333333","79.0","1022.31999304","-1.41662900881"};
 for(unsigned i=0;i<4;++i){latestSocket->inject(climateTopics[i],climateValues[i]);s.step();assert(s.report.valid[i+4]==1&&s.report.lastValid[i+4]);}
 assert(s.report.values[4]<0&&s.report.values[7]<0);
 latestSocket->inject("outHum","101");s.step();assert(!s.report.lastValid[5]&&s.report.invalid[5]==1);
 latestSocket->inject("outHum","79.0");s.step();
 assert(s.allDataValid());assert(s.report.values[0]==315 && std::fabs(s.report.values[3]-12)<.001);
 auto validTime=s.report.updated[1];latestSocket->inject("1","bad");s.step();assert(s.report.valid[1]==1 && s.report.invalid[1]==1 && !s.report.lastValid[1] && s.report.updated[1]==validTime);
 latestSocket->inject("2",std::string("12\0x",4));s.step();assert(s.report.invalid[2]==1);
 latestSocket->inject("3",std::string(49,'1'));s.step();assert(s.report.invalid[3]==1);
 MqttWireObserver wire;const uint8_t sub[]={0x82,6,0,7,0,1,'x',0};wire.subscriptionWrite(2,sub,sizeof(sub));packet(wire,{0x90,3,0,7,0});assert(wire.subscription[2]==0);
 packet(wire,{0x30,0x80,1});for(unsigned i=0;i<128;++i)wire.byte(0);packet(wire,{0xd0,0});assert(wire.pongs==1);
 float n;assert(parseMqttWeatherValue(0,"NW",n) && n==315);assert(parseMqttWeatherValue(0,"-90",n)&&n==270);
 assert(parseMqttWeatherValue(3,"1.2",n)&&std::fabs(n-12)<.001);
 assert(!parseMqttWeatherValue(1,"-1",n));assert(!parseMqttWeatherValue(2,"nan",n));assert(!parseMqttWeatherValue(1,"12bad",n));assert(!parseMqttWeatherValue(3,"3e38",n));
 s.report.connected=true;s.report.code=0;s.report.dnsOk=s.report.tcpOk=true;
 for(unsigned i=0;i<4;++i){s.report.subscription[i]=0;s.report.valid[i]=1;s.report.received[i]=1;s.report.lastValid[i]=true;}
 assert(s.allDataValid());renderMqttTestResult(&s,false);
 std::string page=manualNetworkJob.page.c_str();assert(manualNetworkJob.status==200 && page.find("sensitive-user")==std::string::npos && page.find("sensitive-secret")==std::string::npos);
 s.settings.topics[0]="<script>&";s.report.lastValid[0]=false;++s.report.invalid[0];renderMqttTestResult(&s,false);
 page=manualNetworkJob.page.c_str();assert(manualNetworkJob.status==502 && page.find("<script>")==std::string::npos && page.find("&lt;script&gt;&amp;")!=std::string::npos);
 english=true;s.report.connected=false;s.report.code=-4;renderMqttTestResult(&s,false);assert(manualNetworkJob.status==502);

 holdForWeb=true;finishWeb=false;ioEntered=false;ackDelay=2500;bool connected=false;
 std::thread worker([&]{state.lock();{MqttIoScope io;connected=s.connect(id);}state.unlock();});
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
 while(!ioEntered && std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
 assert(ioEntered && state.try_lock_for(std::chrono::milliseconds(250)));assert(mqttIoActive && !finishWeb);
 finishWeb=true;state.unlock();worker.join();assert(connected && !mqttIoActive);holdForWeb=false;
 saved.host="saved-broker";saved.user="saved-user";saved.password="saved-secret";saved.topics[0]="weather/wind";
 manualNetworkJob=Job{};webServer.args={{"mqttHost","candidate-broker"},{"mqttPort","1884"},{"mqttUser","candidate-user"},{"mqttPassword",""}};
 handleMqttTest();assert(webServer.status==303 && manualNetworkJob.kind==ManualJobKind::Mqtt && manualNetworkJob.queued);
 assert(requestedMqttTest.host==String("candidate-broker") && requestedMqttTest.password==String("saved-secret") && requestedMqttTest.port==1884);
 assert(saved.host==String("saved-broker") && saved.user==String("saved-user") && saved.password==String("saved-secret") && !mqttWeatherEnabled);
 assert(applicationRevision==99 && mqttTestRevision==99);const auto jobId=manualNetworkJob.id;
 handleMqttTest();assert(webServer.status==409 && manualNetworkJob.id==jobId);
 manualNetworkJob=Job{};webServer.args["mqttHost"]="http://invalid";handleMqttTest();assert(webServer.status==400 && !manualNetworkJob.queued);
 webServer.args["mqttHost"]="candidate-broker";webServer.args["mqttPort"]="0";handleMqttTest();assert(webServer.status==400);
 webServer.args["mqttPort"]="1883";webServer.args["windDirectionTopic"]=String(std::string(129,'x').c_str());handleMqttTest();assert(webServer.status==400);
 webServer.args.clear();authenticated=false;handleMqttTest();assert(webServer.status==401 && !manualNetworkJob.queued);authenticated=true;
 manualNetworkJob=Job{};assert(temporaryPsramStats().allocatedBytes==0);
 {PsramBuffer json;appendMqttReportJson(json,s.report);std::cout<<"JSON:"<<json.c_str()<<'\n';}
 manualNetworkJob.page=PsramBuffer();assert(temporaryPsramStats().allocatedBytes==0);
 std::cout<<"MQTT: real PubSubClient CONNECT/reconnect, delayed ACK, DNS/TCP/auth/timeouts, real SUBACK tracking, keepalive, full-MAC IDs, payload validation and credential-safe results passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-mqtt-') as folder:
 p=Path(folder)
 for name,text in headers.items():(p/name).write_text(text)
 (p/'test.cpp').write_text(code)
 for variant in (0,1):
  binary=p/str(variant)
  subprocess.run(['c++','-std=c++11','-pthread','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined','-DESP32',f'-DFLIGHTWALL_HAS_MATRIX={variant}',
   '-DARDUINOJSON_ENABLE_PROGMEM=0','-DARDUINOJSON_ENABLE_ARDUINO_STRING=0','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0',
   '-I'+str(p),'-I'+str(root/'include'),'-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/PubSubClient/src'),'-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/ArduinoJson/src'),'-I'+str(root/'tools/test_support'),
   str(p/'test.cpp'),str(root/'src/MqttSession.cpp'),str(root/'src/TemporaryPsram.cpp'),str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/PubSubClient/src/PubSubClient.cpp'),'-o',str(binary)],check=True)
  output=subprocess.check_output([str(binary)],text=True)
  import json
  report=json.loads(next(line[5:] for line in output.splitlines() if line.startswith('JSON:')))
  assert len(report['topics'])==8 and 'sensitive-secret' not in output and 'sensitive-user' not in output
  print(next(line for line in output.splitlines() if line.startswith('MQTT:')))
