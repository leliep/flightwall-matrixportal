#include "TemporaryPsram.hpp"
#include "RouteDiagnostics.hpp"
#include <esp_heap_caps.h>
#include <cassert>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <iostream>

bool failAllocation=false;
unsigned calls=0;
void *heap_caps_realloc(void *pointer,size_t bytes,uint32_t caps) {
  assert(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));++calls;
  if(failAllocation)return nullptr;
  return std::realloc(pointer,bytes);
}
void heap_caps_free(void *p){std::free(p);}
void *heap_caps_malloc(size_t n,uint32_t caps){return heap_caps_realloc(nullptr,n,caps);}
struct Server {
 int status=0;size_t length=0;const char *pointer=nullptr;std::string body;
 void setContentLength(size_t n){length=n;}
 void send(int code,const char*,const char *text){status=code;body=text;pointer=nullptr;}
 void sendContent(const char *text,size_t n){pointer=text;body.assign(text,n);}
} webServer;
#include "ResponseUnderTest.hpp"
unsigned tlsResources=0;
RouteMemorySnapshot cleanedMemory(){assert(tlsResources==0);assert(temporaryPsramStats().allocatedBytes==0);RouteMemorySnapshot m;m.freeInternal=70000;m.largestInternal=50000;return m;}
struct Tls {Tls(){++tlsResources;}~Tls(){--tlsResources;}};
void earlyReturn(RouteDiagnostics &all){
 all.primary.begin("adsb.im","RYR20JX",0,{});
 RouteCleanupCapture cleanup(all,all.primary,cleanedMemory);
 Tls tls;
 JsonDocument doc(&temporaryJsonAllocator());doc["cleanup"]="test";
 PsramBuffer response;response.reserve(42000);response+="temporary response";
 all.primary.httpAttempted=true;all.primary.httpCode=403;
 all.rememberFailure(all.primary);
 assert(!all.primary.cleanupCaptured && !all.lastFailure.cleanupCaptured);
 return;
}
int main(){
 static_assert(!std::is_convertible<PsramBuffer,String>::value,"No large internal String copies");
 {
  PsramBuffer b;b.reserve(34000);b+="Hallo ";b+=String("München");b+='!';
  assert(std::string(b.c_str())=="Hallo München!");
  b.replace("München","Kleve am Niederrhein");b.replace("Hallo ","");
  assert(std::string(b.c_str())=="Kleve am Niederrhein!");
  b.replace("!","?!");assert(b.indexOf("?!")==static_cast<int>(std::strlen("Kleve am Niederrhein")));
  b.remove(5);assert(std::string(b.c_str())=="Kleve");
  PsramBuffer moved=std::move(b);assert(b.length()==0 && moved.length()==5);
  PsramBuffer joined=PsramBuffer("<")+String("Kleve")+">";moved+=joined;
  assert(std::string(moved.c_str())=="Kleve<Kleve>");
  PsramBuffer alias(std::string(240,'A').c_str());alias.append(alias.c_str(),alias.length());
  assert(alias.length()==480 && !alias.failed());
  failAllocation=true;
  const auto old=std::string(moved.c_str());assert(!moved.reserve(100000));
  assert(moved.failed() && std::string(moved.c_str())==old);
  failAllocation=false;PsramBuffer good("good");failAllocation=true;good+=moved;assert(good.failed());
  assert(!good.write(static_cast<uint8_t>('x')));
  failAllocation=false;
 }
 assert(temporaryPsramStats().allocatedBytes==0);
 {
  PsramBuffer html;html.reserve(34000);
  std::string expected;
  for(unsigned i=0;i<5000;++i){html+="<p>Standort</p>";expected+="<p>Standort</p>";}
  html.replace("Standort","Site description");
  expected.clear();for(unsigned i=0;i<5000;++i)expected+="<p>Site description</p>";
  assert(html.length()>73033 && std::strlen(html.c_str())==html.length());
  assert(std::string(html.c_str())==expected && !html.failed());
 }
 {
  PsramBuffer large;assert(!large.reserve(PsramBuffer::maximumSize+1));assert(large.failed());
  JsonDocument doc(&temporaryJsonAllocator());assert(!deserializeJson(doc,"{\"site\":\"Kleve\",\"values\":[1,2,3]}"));
  PsramBuffer json;serializeJson(doc,json);assert(!json.failed());
  assert(std::string(json.c_str()).find("Kleve")!=std::string::npos);
  assert(temporaryPsramStats().allocatedBytes>0);
 }
 assert(temporaryPsramStats().allocatedBytes==0);
 {
  failAllocation=true;JsonDocument doc(&temporaryJsonAllocator());
  assert(deserializeJson(doc,"{\"site\":\"Kleve\"}")==DeserializationError::NoMemory);
  PsramBuffer b;b+="not sent";assert(b.failed());failAllocation=false;
 }
 assert(temporaryPsramStats().allocatedBytes==0 && temporaryPsramStats().allocationFailures>0);
 // Shrinking JSON reallocations must succeed even under allocation failure.
 auto &a=temporaryJsonAllocator();void *p=a.allocate(1024);assert(p);
 failAllocation=true;assert(a.reallocate(p,100)==p);failAllocation=false;a.deallocate(p);
 {
  PsramBuffer page("<status>__DATA__</status>");
  const size_t bytes=temporaryPsramStats().allocatedBytes;
  sendPsramResponse("text/html",page,page.indexOf("__DATA__"),8);
  assert(webServer.status==200 && webServer.body=="__DATA__" && webServer.length==8);
  assert(webServer.pointer==page.c_str()+page.indexOf("__DATA__"));
  assert(temporaryPsramStats().allocatedBytes==bytes); // No second response buffer.
  failAllocation=true;page.reserve(100000);failAllocation=false;
  sendPsramResponse("text/html",page);assert(webServer.status==503 && webServer.pointer==nullptr);
 }
 RouteDiagnostics all;earlyReturn(all);
 assert(all.primary.cleanupCaptured && all.lastFailure.cleanupCaptured);
 assert(all.lastFailure.afterCleanup.freeInternal==70000);
 assert(temporaryPsramStats().allocatedBytes==0 && calls>0);
 std::cout<<"PSRAM-only buffer/JSON, OOM, moves, bounded growth, aliases and post-TLS early-return cleanup passed\n";
}
