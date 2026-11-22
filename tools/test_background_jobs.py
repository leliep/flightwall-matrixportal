#!/usr/bin/env python3
"""Exercise real bounded job transitions and HTTP result polling during IO."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];s=(root/'src/main.cpp').read_text()
def function(signature):
 a=s.index(signature);return s[a:s.index('\n}\n',a)+3]
a=s.index('enum class ManualJobKind');jobs=s[a:s.index('} manualNetworkJob;',a)+len('} manualNetworkJob;')]
scheduler=s[s.index('      if (wifiConnected && !otaPreparing && !otaInProgress && !otaRebootAt && !manualRebootAt) {'):s.index('\n      {',s.index('      if (wifiConnected && !otaPreparing && !otaInProgress && !otaRebootAt && !manualRebootAt) {'))]
code=r'''
#include "TemporaryPsram.hpp"
#include "NetworkIo.hpp"
#include <esp_heap_caps.h>
#include <cassert>
#include <cstdlib>
#include <string>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <iostream>
void *heap_caps_realloc(void *p,size_t n,uint32_t caps){assert(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return std::realloc(p,n);}
void *heap_caps_malloc(size_t n,uint32_t caps){return heap_caps_realloc(nullptr,n,caps);}
void heap_caps_free(void *p){std::free(p);}
std::timed_mutex state;
std::atomic<bool> entered(false),finish(false);
void release(){state.unlock();} void acquire(){state.lock();}
bool english=false;
const char *uiText(const char *de,const char *en){return english?en:de;}
uint32_t applicationRevision=9,operationRevision=0;
void noteWebMutation(){++applicationRevision;}
String networkLastError;
uint32_t manualRebootAt=0;bool requireRestart=false;
bool applyWebSettings(){if(requireRestart)manualRebootAt=500;return true;}
int adsbCalls=0;
bool fetchLocalFlight(){++adsbCalls;return true;}
struct Server {
 String id;int status=0;std::string body;size_t contentLength=0;std::map<std::string,std::string> headers;
 String arg(const char*){return id;}
 void sendHeader(const char *key,const String &v){headers[key]=v.c_str();}
 void send(int c,const char*,const String &v){status=c;body=v.c_str();}
 void setContentLength(size_t n){contentLength=n;}
 void sendContent(const char *text,size_t n){body.assign(text,n);}
} webServer;
'''+jobs+function('void sendPsramResponse(')+function('String htmlEscape(String value)')+r'''
bool collectOnly=false;
bool runRouteTestJob(){
 if(collectOnly)return false;
 {NetworkIoScope io;entered=true;while(!finish)std::this_thread::sleep_for(std::chrono::milliseconds(1));}
 manualNetworkJob.page="<!doctype html><html><body>rejected: LGW-LYS</body></html>";
 manualNetworkJob.status=502;return true;
}
'''+''.join(function(f) for f in ['bool prepareManualNetworkJob(', 'void redirectToManualNetworkJob(', 'void handleManualNetworkJobResult(', 'void runManualNetworkJob(', 'void handleAdsbTest('])+r'''
bool wifiConnected=true,otaPreparing=false,otaInProgress=false,pollRequested=false;
uint32_t otaRebootAt=0,lastPoll=0;constexpr uint32_t POLL_INTERVAL_MS=500;
void networkCycle(uint32_t now){
'''+scheduler+r'''
}

int main(){
 configureNetworkIo({release,acquire,nullptr});
 assert(prepareManualNetworkJob(ManualJobKind::Route));assert(manualNetworkJob.queued);
 redirectToManualNetworkJob();assert(webServer.status==303 && webServer.headers["Location"]=="/request-result?id=1");
 webServer.id="1";
 state.lock();handleManualNetworkJobResult();state.unlock();assert(webServer.status==202);
 assert(!prepareManualNetworkJob(ManualJobKind::Adsb));assert(webServer.status==409);
 collectOnly=true;runManualNetworkJob();
 assert(manualNetworkJob.queued && manualNetworkJob.running && !manualNetworkJob.done);
 assert(manualNetworkJob.revision==applicationRevision);
 collectOnly=false;
 std::thread worker([]{state.lock();runManualNetworkJob();state.unlock();});
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
 while(!entered && std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
 assert(entered);assert(state.try_lock_for(std::chrono::milliseconds(250)));
 assert(manualNetworkJob.running && manualNetworkJob.queued);
 handleManualNetworkJobResult();assert(webServer.status==202 && webServer.body.find("setTimeout")!=std::string::npos);
 assert(!prepareManualNetworkJob(ManualJobKind::Adsb));assert(webServer.status==409);
 finish=true;state.unlock();worker.join();
 assert(manualNetworkJob.done && !manualNetworkJob.running && operationRevision==10);
 handleManualNetworkJobResult();assert(webServer.status==502 && webServer.body.find("LGW-LYS")!=std::string::npos);
 assert(webServer.body.size()==webServer.contentLength);
 assert(prepareManualNetworkJob(ManualJobKind::Adsb));assert(manualNetworkJob.page.length()==0);
 webServer.id="1";handleManualNetworkJobResult();assert(webServer.status==410);
 webServer.id="2";english=true;handleManualNetworkJobResult();assert(webServer.status==202 && webServer.body.find("Other pages remain available")!=std::string::npos);
 runManualNetworkJob();assert(adsbCalls==1);handleManualNetworkJobResult();assert(webServer.status==200);
 manualNetworkJob.id=UINT32_MAX;assert(prepareManualNetworkJob(ManualJobKind::Adsb));assert(manualNetworkJob.id==1);
 manualNetworkJob={};requireRestart=true;handleAdsbTest();
 assert(manualNetworkJob.done && !manualNetworkJob.queued && manualRebootAt==500);
 webServer.id=String(manualNetworkJob.id);handleManualNetworkJobResult();
 assert(webServer.status==200 && webServer.body.find("after restart")!=std::string::npos);
 manualNetworkJob={};manualRebootAt=0;requireRestart=false;
 assert(prepareManualNetworkJob(ManualJobKind::Route));collectOnly=true;
 auto before=adsbCalls;networkCycle(1000);
 assert(manualNetworkJob.running && manualNetworkJob.queued && adsbCalls==before+1);
 networkCycle(1500);assert(adsbCalls==before+2); // waiting test never starves ADS-B
 manualNetworkJob={};assert(temporaryPsramStats().allocatedBytes==0);
 std::cout<<"Background jobs: bounded slot, running/queued conflict, concurrent polling, result HTTP status, expiry, replacement, wrap and PSRAM cleanup passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-background-jobs-') as folder:
 p=Path(folder);(p/'test.cpp').write_text(code)
 for variant in (0,1):
  binary=p/str(variant)
  subprocess.run(['c++','-std=c++11','-pthread','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',f'-DFLIGHTWALL_HAS_MATRIX={variant}',
   '-DARDUINOJSON_ENABLE_ARDUINO_STRING=0','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0',
   '-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),'-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/ArduinoJson/src'),
   str(p/'test.cpp'),str(root/'src/TemporaryPsram.cpp'),str(root/'src/NetworkIo.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
