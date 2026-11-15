#!/usr/bin/env python3
"""Compile real ADS-B fetch with delayed HTTP/stream boundaries and a web peer."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
fake=r'''
#pragma once
#include <Arduino.h>
#include <istream>
#include <streambuf>
#include <string>
extern int blockAt,endCalls;
void awaitWeb(int);
class WiFiClient {};
class WiFiClientSecure {public:void setInsecure(){}};
struct Buffer:std::streambuf {
 std::string body=R"({"now":123,"aircraft":[{"hex":"abc123","flight":"DLH072","seen":0}]})";
 size_t pos=0;char c=0;bool first=true;
 int_type underflow()override{
  if(pos>=body.size())return traits_type::eof();
  if(first){first=false;awaitWeb(2);}
  c=body[pos++];setg(&c,&c,&c+1);return traits_type::to_int_type(c);
 }
};
class HTTPClient {
 Buffer buffer;std::istream stream{&buffer};
 public:
 void useHTTP10(bool){} void setConnectTimeout(int){} void setTimeout(int){}
 bool begin(WiFiClient&,const String&){return true;}
 bool begin(WiFiClientSecure&,const String&){return true;}
 void addHeader(const char*,const char*){}
 int GET(){awaitWeb(1);return 200;}
 std::istream &getStream(){return stream;}
 void end(){++endCalls;}
};
constexpr int HTTP_CODE_OK=200;
'''
code=r'''
#include "AdsbClient.hpp"
#include "TemporaryPsram.hpp"
#include "NetworkIo.hpp"
#include "FakeHttp.hpp"
#include <esp_heap_caps.h>
#include <cassert>
#include <cstdlib>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>
int blockAt=0,endCalls=0,conversions=0;
std::timed_mutex state;
std::atomic<bool> entered(false),finish(false);
bool cancelled=false;
void release(){state.unlock();} void acquire(){state.lock();} bool isCancelled(){return cancelled;}
void awaitWeb(int phase){if(blockAt!=phase)return;entered=true;while(!finish)std::this_thread::sleep_for(std::chrono::milliseconds(1));}
void *heap_caps_realloc(void *p,size_t n,uint32_t caps){assert(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return std::realloc(p,n);}
void *heap_caps_malloc(size_t n,uint32_t caps){return heap_caps_realloc(nullptr,n,caps);}
void heap_caps_free(void *p){std::free(p);}
bool aircraftFromJson(JsonObjectConst,const ProcessingConfig&,Aircraft&){++conversions;return true;}
uint64_t aircraftIdentityFromJson(JsonObjectConst){return 1;}
int main(){
 for(int phase:{1,2})for(bool abort:{false,true}){
  entered=false;finish=false;cancelled=false;blockAt=phase;endCalls=0;conversions=0;
  AdsbResult result;AdsbClient client("https://example.invalid/aircraft.json");ProcessingConfig config;
  configureNetworkIo({release,acquire,isCancelled});
  std::thread worker([&]{state.lock();result=client.fetch(config);state.unlock();});
  auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(!entered && std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
  assert(entered && state.try_lock_for(std::chrono::milliseconds(250)));
  assert(!finish);cancelled=abort;finish=true;state.unlock();worker.join();
  assert(result.ok==!abort && result.candidates.size()==(abort?0:1));
  assert(conversions==(abort?0:1));assert(endCalls==1);
  assert(temporaryPsramStats().allocatedBytes==0);configureNetworkIo({});
 }
 std::cout<<"Real ADS-B fetch: web access during HTTP and JSON stream stalls, cancellation before conversion/publication, cleanup and successful completion passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-adsb-io-') as folder:
 p=Path(folder);(p/'FakeHttp.hpp').write_text(fake);(p/'test.cpp').write_text(code)
 for name in ['HTTPClient.h','WiFiClient.h','WiFiClientSecure.h']:(p/name).write_text('#include "FakeHttp.hpp"\n')
 for variant in (0,1):
  binary=p/str(variant)
  subprocess.run(['c++','-std=c++11','-pthread','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',f'-DFLIGHTWALL_HAS_MATRIX={variant}',
   '-DARDUINOJSON_ENABLE_ARDUINO_STRING=0','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0',
   '-I'+str(p),'-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),'-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/ArduinoJson/src'),
   str(p/'test.cpp'),str(root/'src/AdsbClient.cpp'),str(root/'src/TemporaryPsram.cpp'),str(root/'src/NetworkIo.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
