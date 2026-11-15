#!/usr/bin/env python3
"""Run real manual fetch/rejection flow and renderer with controlled HTTP replies."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
route=(root/'src/RouteClient.cpp').read_text();main=(root/'src/main.cpp').read_text()
def function(source,signature):
 start=source.index(signature);return source[start:source.index('\n}\n',start)+3]
helpers=function(route,'void captureRouteTestJson(')+function(route,'void rejectRoute(')
fetch=''.join(function(route,s) for s in ['void RouteClient::configure(', 'RouteResult RouteClient::fetchPrimary(', 'RouteResult RouteClient::fetchAdsbDb(', 'RouteResult RouteClient::fetch(', 'RouteResult RouteClient::test('])
renderer=function(main,'void appendRouteTestHtmlText(')+function(main,'void appendRouteTestResponseHtml(')
code=r'''
#include "RouteClient.hpp"
#include "TemporaryPsram.hpp"
#include "RouteCallsign.hpp"
#include <esp_heap_caps.h>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
bool failAllocation=false;
void *heap_caps_realloc(void *p,size_t n,uint32_t caps){assert(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return failAllocation?nullptr:std::realloc(p,n);}
void *heap_caps_malloc(size_t n,uint32_t caps){return heap_caps_realloc(nullptr,n,caps);}
void heap_caps_free(void *p){std::free(p);}
RouteMemorySnapshot routeMemorySnapshot(){return {};}
uint32_t millis(){return 100;}
struct Reply {int status;std::string json;};
std::vector<Reply> replies;size_t nextReply=0;
struct WiFiClientSecure {void setInsecure(){} int lastError(char*,size_t){return 0;}};
struct HTTPClient {
 Reply reply;std::istringstream stream;
 HTTPClient():reply(replies.at(nextReply++)),stream(reply.json){}
 void setConnectTimeout(int){} void setTimeout(int){} void addHeader(const char*,const char*){}
 bool begin(WiFiClientSecure&,const String&){return true;}
 int POST(uint8_t*,size_t){return reply.status;} int GET(){return reply.status;}
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
'''+helpers+fetch+renderer+r'''
std::string html(const RouteTestResponse &r){PsramBuffer b;appendRouteTestResponseHtml(b,r,"<ADSBDB>");assert(!b.failed());return b.c_str();}
void fixtures(const char *json,int primary=403){nextReply=0;replies={{primary,"[]"},{200,json}};}
int main(){
 RouteClient client;client.configure(true,"https://adsb.im",true);RouteContext context;context.hasPosition=true;
 const char *response=R"({"response":{"flightroute":{"callsign":"DLH72","origin":{"iata_code":"LGW","name":"<script>&\""},"destination":{"iata_code":"LYS"}}}})";
 fixtures(response);plausible=false;
 auto result=client.test("DLH072",context);
 assert(result.status==String("rejected") && !result.found() && result.routeCodes.isEmpty());
 assert(result.error==String("Routen-HTTP 403; Route verworfen: Flugposition liegt nicht auf der Route"));
 const auto &r=client.testResponse(true);assert(r.httpCode==200 && r.routeCodes=="LGW-LYS" && r.returnedCallsign=="DLH72");
 assert(r.error=="Route verworfen: Flugposition liegt nicht auf der Route");
 assert(client.testResponse(false).httpCode==403 && client.testResponse(false).error=="Routen-HTTP 403");
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
 assert(temporaryPsramStats().allocatedBytes==0);
 assert(psramTextMemoryStats().blocks==0);
 std::cout<<"Manual route test: preserved replies before all rejection branches, provider separation, leading zeros, stale reset, bounded PSRAM, OOM and HTML escaping passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-route-responses-') as folder:
 p=Path(folder);(p/'test.cpp').write_text(code)
 for variant in (0,1):
  binary=p/str(variant)
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',f'-DFLIGHTWALL_HAS_MATRIX={variant}',
   '-DARDUINOJSON_ENABLE_ARDUINO_STRING=0','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0',
   '-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),'-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/ArduinoJson/src'),
   str(p/'test.cpp'),str(root/'src/TemporaryPsram.cpp'),str(root/'src/PsramText.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
