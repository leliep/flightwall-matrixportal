#!/usr/bin/env python3
"""Run production scan coordinator against controlled ESP/Arduino events."""
from pathlib import Path
import subprocess
import tempfile
import sys
root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text()
fake = r'''
#pragma once
#include <cstdint>
#include <functional>
#include <cassert>
#include <vector>
#include <cstring>
using esp_err_t=int;
constexpr int ESP_OK=0,ESP_FAIL=-1,ESP_ERR_TIMEOUT=0x107;
constexpr int ESP_ERR_WIFI_STATE=0x3006,ESP_ERR_WIFI_NOT_CONNECT=0x300f;
constexpr int WL_CONNECTED=3,WIFI_SCAN_RUNNING=-1,WIFI_SCAN_FAILED=-2;
constexpr int WIFI_SCAN_TYPE_ACTIVE=0,WIFI_SCAN_DONE_BIT=1,WIFI_SCANNING_BIT=2;
constexpr int ARDUINO_EVENT_WIFI_SCAN_DONE=1;
using WiFiEvent_t=int;
struct WiFiEventInfo_t{struct{uint32_t status;}wifi_scan_done;};
uint32_t nowMs=100;
uint32_t millis(){return nowMs;}
struct {template<class... A>void printf(const char*,A...) {}}Serial;
const char*esp_err_to_name(int e){return e?"ERROR":"ESP_OK";}
struct WiFiGenericClass {
 static int bits;
 static void clearStatusBits(int b){bits&=~b;}
 static void setStatusBits(int b){bits|=b;}
};
int WiFiGenericClass::bits=0;
struct WiFiScanClass {
 static bool _scanAsync;
 static uint32_t _scanTimeout,_scanStarted;
 static int result;
 static int deletes;
 void scanDelete(){++deletes;WiFiGenericClass::clearStatusBits(WIFI_SCAN_DONE_BIT);}
 int scanComplete(){return WiFiGenericClass::bits&WIFI_SCANNING_BIT?WIFI_SCAN_RUNNING:result;}
};
bool WiFiScanClass::_scanAsync=false;
uint32_t WiFiScanClass::_scanTimeout=0,WiFiScanClass::_scanStarted=0;
int WiFiScanClass::result=0,WiFiScanClass::deletes=0;
struct FakeWifi {
 int statusValue=0,mode=3,apClients=1,registrations=0;
 bool reconnect=true;
 std::function<void(WiFiEvent_t,WiFiEventInfo_t)>callback;
 int status(){return statusValue;}
 bool getAutoReconnect(){return reconnect;}
 bool setAutoReconnect(bool b){reconnect=b;return true;}
 void onEvent(std::function<void(WiFiEvent_t,WiFiEventInfo_t)>cb,int){callback=cb;++registrations;}
 int getMode(){return mode;}
 int softAPgetStationNum(){return apClients;}
 void done(int count,uint32_t status=0){
  WiFiScanClass::result=count;
  WiFiGenericClass::clearStatusBits(WIFI_SCANNING_BIT);
  WiFiGenericClass::setStatusBits(WIFI_SCAN_DONE_BIT);
  callback(1,WiFiEventInfo_t{{status}});
 }
}WiFi;
struct wifi_scan_config_t {
 void*ssid;void*bssid;uint8_t channel;bool show_hidden;int scan_type;
 struct{struct{uint32_t min,max;}active;}scan_time;
};
int disconnectError=0,disconnects=0,stops=0,startCalls=0;
std::vector<int>startErrors;
int esp_wifi_disconnect(){++disconnects;return disconnectError;}
int esp_wifi_scan_stop(){++stops;return ESP_OK;}
int esp_wifi_scan_start(wifi_scan_config_t*c,bool block){
 assert(!block&&!c->ssid&&!c->bssid&&!c->channel&&!c->show_hidden);
 assert(c->scan_time.active.min==100&&c->scan_time.active.max==160);
 assert(!WiFi.reconnect);++startCalls;
 if(startErrors.empty())return 0;
 int e=startErrors.front();startErrors.erase(startErrors.begin());return e;
}
'''
code = r'''
#include "WifiScan.hpp"
#include <iostream>
void start(WifiSurvey&s){assert(s.request());assert(s.busy());s.service();assert(!WiFi.reconnect);nowMs+=300;s.service();}
void reset(){WiFi.statusValue=0;WiFi.reconnect=true;disconnectError=0;startErrors.clear();}
int main(){
 WifiSurvey s;assert(!s.busy());assert(!strcmp(s.stateName(),"idle"));
 start(s);assert(s.state==WifiSurvey::State::Scanning);assert(disconnects==1);
 assert(!s.request());s.service();assert(s.busy());
 WiFi.done(4);s.service();assert(s.count==4&&!s.busy()&&WiFi.reconnect);
 assert(s.error==ESP_OK&&s.duration==300&&WiFi.mode==3&&WiFi.apClients==1);
 assert(WiFi.registrations==1);
 // A successful station is never disconnected; preserve auto-reconnect=false.
 WiFi.statusValue=WL_CONNECTED;WiFi.reconnect=false;int n=disconnects;
 start(s);assert(disconnects==n);WiFi.done(0);s.service();assert(!WiFi.reconnect&&s.count==0);
 assert(WiFi.registrations==1);reset();
 // Transient connecting/busy driver error is bounded and then succeeds.
 startErrors={ESP_ERR_WIFI_STATE,ESP_OK};start(s);assert(s.busy()&&s.attempts==1);
 nowMs+=299;s.service();assert(s.attempts==1);++nowMs;s.service();
 assert(s.attempts==2);WiFi.done(2);s.service();assert(WiFi.reconnect&&s.count==2);
 // Persistent SDK error preserves its exact value and failure phase.
 startErrors={ESP_ERR_WIFI_STATE,ESP_ERR_WIFI_STATE,ESP_ERR_WIFI_STATE,ESP_ERR_WIFI_STATE};
 start(s);for(int i=0;i<3;++i){nowMs+=300;s.service();}
 assert(!s.busy()&&s.error==ESP_ERR_WIFI_STATE&&s.attempts==4);
 assert(!strcmp(s.phase,"start")&&WiFi.reconnect);
 startErrors={0x3001};start(s);assert(s.error==0x3001&&!s.busy()&&WiFi.reconnect);
 // Scan-done failure is not presented as a start failure or success.
 start(s);WiFi.done(0,7);s.service();assert(s.error==ESP_FAIL&&s.eventStatus==7);
 assert(!strcmp(s.phase,"scan_done")&&WiFi.reconnect);
 // No browser polling: timeout still restores radio/reconnect, including wrap.
 nowMs=UINT32_MAX-500;start(s);nowMs+=14700;s.service();
 assert(!s.busy()&&s.error==ESP_ERR_TIMEOUT&&WiFi.reconnect&&stops>0);
 assert(!strcmp(s.phase,"timeout"));
 disconnectError=0x3002;assert(s.request());s.service();
 assert(s.error==0x3002&&!strcmp(s.phase,"disconnect")&&WiFi.reconnect);
 disconnectError=ESP_ERR_WIFI_NOT_CONNECT;start(s);WiFi.done(1);s.service();assert(s.count==1);
 // Don't consume Arduino results until its user event has also supplied status.
 reset();start(s);WiFiGenericClass::clearStatusBits(WIFI_SCANNING_BIT);s.service();assert(s.busy());
 WiFi.done(5);s.service();assert(!s.busy()&&s.count==5);
 std::cout<<"Scan coordinator: AP+STA, connected STA, no browser, exact errors, event failure, retries, timeout/wrap, reconnect restoration passed\n";
}
'''
assert 'if (wifiSurvey.busy()) return false;' in source
assert 'networkOperationCancelled() || wifiSurvey.busy()' in source
assert '!wifiConnected && !wifiSurvey.busy() &&' in source
assert 'wifiSurvey.service();' in source[source.index('void webTask('):source.index('void networkTask(')]
assert 'sendPsramResponse("application/json; charset=utf-8", json, 0, SIZE_MAX, 500)' in source
assert 'wifiScanDiagnosticJson()' in source and '\\"wifi_scan\\"' in source
with tempfile.TemporaryDirectory(prefix='flightwall-wifi-scan-') as folder:
 p=Path(folder)
 (p/'WiFi.h').write_text(fake)
 (p/'esp_wifi.h').write_text('#pragma once\n')
 (p/'esp_err.h').write_text('#pragma once\n')
 (p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',
                 '-I'+str(p),'-I'+str(root/'include'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
 # Exercise the actual UI script: exact server errors must survive both POST/GET.
 js=source[source.index('const wifiScanButton='):source.index("wifiScanResults.addEventListener('change'")]
 harness=r'''
const vm=require('node:vm');const assert=require('node:assert/strict');
const nodes={};let click;const messages=[];
const document={getElementById(id){return nodes[id]||(nodes[id]={disabled:false,textContent:'',addEventListener(_,fn){click=fn;}});}};
let replies=[];const sandbox={document,fetch:async()=>replies.shift(),setTimeout(){}};
vm.createContext(sandbox);vm.runInContext(SCRIPT,sandbox);
(async()=>{
 replies=[{ok:false,json:async()=>({phase:'start',error_name:'ESP_ERR_WIFI_STATE',error_code:12294})}];
 await click();assert.match(nodes.wifiScanStatus.textContent,/start.*ESP_ERR_WIFI_STATE.*12294/);assert.equal(nodes.wifiScanButton.disabled,false);
 replies=[{ok:true,json:async()=>({state:'running'})},{ok:false,json:async()=>({state:'failed',phase:'scan_done',error_name:'ESP_FAIL',error_code:-1,scan_done_status:7})}];
 await click();await new Promise(resolve=>setImmediate(resolve));
 assert.match(nodes.wifiScanStatus.textContent,/scan_done.*ESP_FAIL.*event 7/);
 assert.equal(nodes.wifiScanButton.disabled,false);console.log('Actual scan UI: POST/GET exact errors and button recovery passed');
})().catch(e=>{console.error(e);process.exitCode=1;});
'''
 import json
 (p/'test.cjs').write_text(harness.replace('SCRIPT',json.dumps(js)))
 if '--cpp-only' not in sys.argv:
  subprocess.run(['node',str(p/'test.cjs')],check=True)
