#!/usr/bin/env python3
"""Run real manual fetch/rejection flow and renderer with controlled HTTP replies."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
route=(root/'src/RouteClient.cpp').read_text();main=(root/'src/main.cpp').read_text()
def function(source,signature):
 start=source.index(signature);return source[start:source.index('\n}\n',start)+3]
helpers=''.join(function(route,f) for f in ['void captureRouteTestJson(', 'void captureRouteTestHttpError(', 'bool isManagedPrimaryUrl(', 'void rejectRoute(', 'RouteResult decodePrimaryRoute('])
fetch=''.join(function(route,s) for s in ['void RouteClient::configure(', 'void RouteClient::clearPrimaryQueue(', 'void RouteClient::cancelTest(', 'void RouteClient::refreshPending(', 'bool RouteClient::primaryResultReady(', 'void RouteClient::processPending(', 'RouteResult RouteClient::fetchPrimary(', 'RouteResult RouteClient::requestPrimary(', 'RouteResult RouteClient::fetchAdsbDb(', 'RouteResult RouteClient::fetch(', 'RouteResult RouteClient::test(', 'RouteResult RouteClient::lookup('])
renderer=function(main,'void appendRouteTestHtmlText(')+function(main,'void appendRouteTestResponseHtml(')
code=r'''
#include "RouteClient.hpp"
#include "NetworkIo.hpp"
#include "TemporaryPsram.hpp"
#include "RouteCallsign.hpp"
#include <esp_heap_caps.h>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
bool failAllocation=false;
bool operator!=(const String& a,const String& b){return !(a==b);}
std::timed_mutex state;
std::atomic<bool> entered(false), finish(false);
int blockedMethod=0;
bool cancelled=false;
void releaseState(){state.unlock();}
void acquireState(){state.lock();}
bool isCancelled(){return cancelled;}
void awaitWeb(int method){
 if(blockedMethod!=method)return;
 entered=true;
 while(!finish)std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
void *heap_caps_realloc(void *p,size_t n,uint32_t caps){assert(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return failAllocation?nullptr:std::realloc(p,n);}
void *heap_caps_malloc(size_t n,uint32_t caps){return heap_caps_realloc(nullptr,n,caps);}
void heap_caps_free(void *p){std::free(p);}
RouteMemorySnapshot routeMemorySnapshot(){return {};}
uint32_t clockMs=100;
uint32_t millis(){return clockMs;}
constexpr uint32_t primaryBatchWindowMs=30000;
const char *routeUserAgent="FlightWall-MatrixPortal/3.6.1-dual-target.56 (+https://github.com/leliep/flightwall-matrixportal/issues)";
struct Reply {int status;std::string json;};
std::vector<Reply> replies;size_t nextReply=0;
struct Stream {
 virtual ~Stream(){} virtual int available()=0; virtual int read()=0;
 virtual int peek()=0;virtual void flush()=0;
 virtual size_t write(uint8_t)=0;virtual size_t write(const uint8_t*,size_t)=0;
};
struct WiFiClient {};
std::string lastUrl,lastAgent,lastBody;bool usedTls=false;int posts=0;
struct WiFiClientSecure {void setInsecure(){} int lastError(char*,size_t){return 0;}};
struct HTTPClient {
 Reply reply;std::istringstream stream;
 HTTPClient():reply(replies.at(nextReply++)),stream(reply.json){}
 void setConnectTimeout(int){} void setTimeout(int){} void addHeader(const char*,const char*){}
 void setUserAgent(const char *agent){lastAgent=agent;}
 bool begin(WiFiClientSecure&,const String& url){usedTls=true;lastUrl=url.c_str();return true;}
 bool begin(WiFiClient&,const String& url){usedTls=false;lastUrl=url.c_str();return true;}
 int POST(uint8_t *data,size_t n){++posts;lastBody.assign(reinterpret_cast<char*>(data),n);awaitWeb(1);return reply.status;}
 int writeToStream(Stream *sink){awaitWeb(3);size_t n=sink->write(reinterpret_cast<const uint8_t*>(reply.json.data()),reply.json.size());return n==reply.json.size()?static_cast<int>(n):-1;} int GET(){awaitWeb(2);return reply.status;}
 std::istringstream &getStream(){return stream;} void end(){}
};
constexpr int HTTP_CODE_OK=200,HTTP_CODE_NOT_FOUND=404;
const char *adsbDbUrl="https://api.adsbdb.com/v0/callsign/";
String primarySourceName(const String&){return "adsb.im";}
String upperTrimmed(String s){s.trim();s.toUpperCase();return s;}
String airportCode(JsonVariantConst a){return a["iata_code"]|"";}
void appendRouteCode(String& codes,const String& code){if(code.isEmpty())return;if(!codes.isEmpty())codes+="-";codes+=code;}
void extractCodes(JsonObjectConst row,RouteResult& r){r.routeCodes=row["airport_codes"]|"";}
void fillNames(RouteResult& r){
 std::string s=r.routeCodes.c_str();auto n=s.find('-');
 if(n!=std::string::npos){r.origin=s.substr(0,n).c_str();r.destination=s.substr(n+1).c_str();}
}
bool plausible=true,dbPlausible=true;
bool adsbDbRoutePlausible(JsonObjectConst,double,double){return dbPlausible;}
bool routePlausible(const RouteResult&,const RouteContext&,String& reason){reason="Route verworfen: Flugposition liegt nicht auf der Route";return plausible;}
void assessRouteConfidence(RouteResult&,bool,bool){}
bool english=false;
const char *uiText(const char *de,const char *en){return english?en:de;}
'''+helpers+fetch+r'''
int remembered=0;
RouteResult RouteClient::readMemoryCache(const String&,const RouteContext&,uint32_t){return {};}
RouteResult RouteClient::readCache(const String&,const RouteContext&,uint32_t){return {};}
void RouteClient::rememberInMemory(const String&,const RouteResult&,uint32_t,bool){++remembered;}
'''+renderer+r'''
std::string html(const RouteTestResponse &r){PsramBuffer b;appendRouteTestResponseHtml(b,r,"<ADSBDB>");assert(!b.failed());return b.c_str();}
void fixtures(const char *json,int primary=403){nextReply=0;replies={{primary,"[]"},{200,json}};}
int main(){
 RouteClient client;client.configure(true,"https://adsb.im",true);RouteContext context;context.hasPosition=true;
 const char *response=R"({"response":{"flightroute":{"callsign":"DLH72","origin":{"iata_code":"LGW","name":"<script>&\""},"destination":{"iata_code":"LYS"}}}})";
 fixtures(response);replies[0].json=R"({"detail":"Use http and a contact User-Agent <admin> & batch"})";plausible=false;
 auto result=client.test("DLH072",context);
 assert(result.status==String("rejected") && !result.found() && result.routeCodes.isEmpty());
 assert(result.error==String("Routen-HTTP 403; Route verworfen: Flugposition liegt nicht auf der Route"));
 const auto &r=client.testResponse(true);assert(r.httpCode==200 && r.routeCodes=="LGW-LYS" && r.returnedCallsign=="DLH72");
 assert(r.error=="Route verworfen: Flugposition liegt nicht auf der Route");
 assert(client.testResponse(false).httpCode==403 && client.testResponse(false).error=="Routen-HTTP 403");
 assert(client.testResponse(false).rawBody);
 auto errorPage=html(client.testResponse(false));
 assert(errorPage.find("Use http")!=std::string::npos && errorPage.find("&lt;admin&gt; &amp; batch")!=std::string::npos);
 assert(errorPage.find("Fehlerantwort des Dienstes")!=std::string::npos);
 auto page=html(r);assert(page.find("LGW-LYS")!=std::string::npos && page.find("<script>")==std::string::npos);
 assert(page.find("&lt;script&gt;&amp;")!=std::string::npos && page.find("&lt;ADSBDB&gt;")!=std::string::npos);
 fixtures(response);plausible=true;dbPlausible=false;
 result=client.test("DLH072",context);assert(result.status==String("rejected"));
 assert(client.testResponse(true).routeCodes=="LGW-LYS");
 assert(client.testResponse(true).error=="ADSBDB-Route passt nicht zur Flugposition");
 fixtures(response);dbPlausible=true;
 result=client.test("BAW72",context);assert(result.status==String("rejected"));
 assert(client.testResponse(true).returnedCallsign=="DLH72");
 assert(client.testResponse(true).error=="ADSBDB-Callsign stimmt nicht mit der Anfrage überein");
 fixtures(response);result=client.test("DLH072",context);assert(result.found());
 fixtures(response,200);replies[0].json=R"([{"callsign":"DLH72","airport_codes":"LGW-LYS"}])";plausible=false;
 client.configure(true,"https://adsb.im",false);result=client.test("DLH072",context);
 assert(!result.found() && client.testResponse(false).routeCodes=="LGW-LYS");
 assert(!client.testResponse(true).attempted); // no stale fallback from earlier tests
 english=true;assert(html(client.testResponse(false)).find("Returned route")!=std::string::npos);
 fixtures(response);replies[0]={404,""};result=client.test("DLH072",context);
 assert(client.testResponse(false).json.isEmpty());
 client.clearTestResponses();assert(!client.testResponse(false).attempted && client.testResponse(false).json.isEmpty());
 {
  RouteTestResponse large;large.attempted=true;
  JsonDocument doc(&temporaryJsonAllocator());doc["large"]=std::string(20000,'x');captureRouteTestJson(large,doc);
  assert(large.json.length()==8192 && large.truncated && !large.captureFailed);
  assert(html(large).find("truncated after 8192 bytes")!=std::string::npos);
  RouteTestResponse failed;failAllocation=true;captureRouteTestJson(failed,doc);failAllocation=false;
  assert(failed.captureFailed && failed.json.isEmpty());failed.attempted=true;
  assert(html(failed).find("Could not capture")!=std::string::npos);
 }

 for(int method: {1,2,3}) {
  fixtures(response);client.configure(true,"https://adsb.im",true);
  entered=false;finish=false;cancelled=false;blockedMethod=method;
  configureNetworkIo({releaseState,acquireState,isCancelled});
  RouteResult aborted;
  std::thread worker([&]{state.lock();aborted=client.test("DLH072",context);state.unlock();});
  const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
  while(!entered && std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
  assert(entered);
  // A web request acquires the same state gate while HTTP is still blocked.
  assert(state.try_lock_for(std::chrono::milliseconds(250)));
  assert(!finish);assert(client.testResponse(false).attempted);
  cancelled=true;finish=true;state.unlock();worker.join();
  assert(!aborted.found());assert(nextReply==static_cast<size_t>(method==2?2:1));
  blockedMethod=0;cancelled=false;configureNetworkIo({});client.clearTestResponses();
 }

 {
  client.clearTestResponses();
  RouteClient batched;batched.configure(true,"https://adsb.im/api/0/routeset",false);
  clockMs=1000;int prior=posts;
  assert(batched.test("DLH072",context).status=="pending");
  RouteContext moved=context;moved.latitude=99;
  batched.refreshPending("DLH072",moved); // manual retains controller test position
  batched.refreshPending("DLH072",context); // manual position is not overwritten
  // Automatic requests join the same batch while a manual test waits.
  const int priorRemembers=remembered;
  assert(batched.lookup("BAW2",context,1).status=="pending");
  assert(batched.lookup("BAW2",context,1).status=="pending");
  assert(remembered==priorRemembers); // never persist a pending/not-yet-sent result
  RouteContext refreshed=context;refreshed.latitude=52;
  batched.refreshPending("BAW2",refreshed);
  assert(!batched.primaryResultReady("BAW2"));
  clockMs=30999;batched.processPending();assert(posts==prior);
  fixtures(response,200);replies[0].json=R"([{"callsign":"BAW2","airport_codes":"DUS-FRA"},{"callsign":"DLH72","airport_codes":"LGW-LYS"}])";
  plausible=true;clockMs=31000;batched.processPending();assert(posts==prior+1);
  assert(lastUrl=="http://adsb.im/api/0/routeset" && !usedTls);
  assert(lastAgent.find("github.com/leliep/flightwall-matrixportal/issues")!=std::string::npos);
  JsonDocument sent;assert(!deserializeJson(sent,lastBody));
  assert(sent["planes"].size()==2 && sent["planes"][1]["lat"]==52);
  assert(sent["planes"][0]["lat"]==context.latitude);
  assert(lastBody.find("DLH072")!=std::string::npos);
  assert(batched.primaryResultReady("BAW2"));
  assert(batched.lookup("BAW2",context,31).routeCodes=="DUS-FRA");
  assert(remembered==priorRemembers+1);
  assert(batched.test("DLH072",context).routeCodes=="LGW-LYS");
  assert(batched.testResponse(false).returnedCallsign=="DLH72");
  assert(batched.test("BAW2",context).status=="pending");
  clockMs=60999;batched.processPending();assert(posts==prior+1);
  fixtures(response);replies[0]={403,"<html>API denied: use HTTP & contact</html>"};
  clockMs=61000;batched.processPending();assert(posts==prior+2);
  auto denied=batched.test("BAW2",context);assert(denied.error=="Routen-HTTP 403");
  assert(html(batched.testResponse(false)).find("&lt;html&gt;API denied")!=std::string::npos);
  // New manual requests cannot bypass the 30-second window or last-POST gap.
  assert(batched.test("BAW3",context).status=="pending");
  batched.configure(false,"http://adsb.im/api/0/routeset",false);
  clockMs+=30000;batched.processPending();assert(posts==prior+2);
 }

 {
  RouteClient full;full.configure(true,"http://adsb.im/api/0/routeset",false);
  clockMs=UINT32_MAX-15000;
  for(int i=0;i<12;++i)assert(full.lookup(String("BAW")+String(i),context,1).status=="pending");
  clockMs+=29999;int prior=posts;full.processPending();assert(posts==prior);
  fixtures(response,200);replies[0].json="[]";
  ++clockMs;full.processPending();assert(posts==prior+1);
  JsonDocument sent;assert(!deserializeJson(sent,lastBody));assert(sent["planes"].size()==8);
  assert(full.lookup("BAW0",context,31).status=="not_found");
  assert(full.lookup("NEW1",context,31).status=="pending");
  full.configure(true,"https://adsb.im/api/0/routeset",false); // same effective URL retains queue
  clockMs+=29999;full.processPending();assert(posts==prior+1);
  fixtures(response,200);replies[0].json="[]";++clockMs;full.processPending();assert(posts==prior+2);
  full.configure(false,"http://adsb.im/api/0/routeset",false);
 }
 {
  RouteClient cancel;cancel.configure(true,"http://adsb.im/api/0/routeset",false);
  assert(cancel.test("DLH1",context).status=="pending");cancel.cancelTest();
  int prior=posts;clockMs+=30000;cancel.processPending();assert(posts==prior);
  failAllocation=true;auto failed=cancel.lookup("DLH2",context,1);failAllocation=false;
  assert(failed.error=="Routen-Batch: PSRAM allocation failed");
  cancel.configure(false,"http://adsb.im/api/0/routeset",false);
 }

 {
  RouteClient expiry;expiry.configure(true,"http://adsb.im/api/0/routeset",false);
  assert(expiry.lookup("DLH1",context,1).status=="pending");
  fixtures(response,200);replies[0].json=R"([{"airport_codes":"LGW-LYS"}])";
  clockMs+=30000;expiry.processPending();
  // A batch row without a callsign must never be assigned to another aircraft.
  assert(expiry.lookup("DLH1",context,31).status=="not_found");
  assert(expiry.lookup("BAW1",context,31).status=="pending");
  fixtures(response,200);replies[0].json=R"([{"callsign":"BAW1","airport_codes":"LGW-LYS"}])";
  clockMs+=30000;expiry.processPending();assert(expiry.primaryResultReady("BAW1"));
  clockMs+=60000;assert(!expiry.primaryResultReady("BAW1"));
  assert(expiry.lookup("BAW1",context,121).status=="pending");
  expiry.configure(false,"http://adsb.im/api/0/routeset",false);
 }
 {
  RouteTestResponse raw;raw.attempted=true;
  nextReply=0;replies={{503,std::string(20000,'x')}};HTTPClient http;
  captureRouteTestHttpError(raw,http);assert(raw.rawBody && raw.truncated && raw.json.length()==8192 && !raw.captureFailed);
  raw={};nextReply=0;HTTPClient failed;
  failAllocation=true;captureRouteTestHttpError(raw,failed);failAllocation=false;
  assert(raw.captureFailed && raw.json.isEmpty());
 }
 assert(temporaryPsramStats().allocatedBytes==0);
 assert(psramTextMemoryStats().blocks==0);
 std::cout<<"Route tests: provider responses, leading zeros, HTTP/UA, timed batches, deduplication, fresh positions, row matching, capacity, expiry, wrap, bounded PSRAM/OOM, HTML escaping and concurrent web/cancellation passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-route-responses-') as folder:
 p=Path(folder);(p/'test.cpp').write_text(code)
 for variant in (0,1):
  binary=p/str(variant)
  subprocess.run(['c++','-std=c++11','-pthread','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',f'-DFLIGHTWALL_HAS_MATRIX={variant}',
   '-DARDUINOJSON_ENABLE_ARDUINO_STRING=0','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0',
   '-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),'-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/ArduinoJson/src'),
   str(p/'test.cpp'),str(root/'src/TemporaryPsram.cpp'),str(root/'src/PsramText.cpp'),str(root/'src/NetworkIo.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
