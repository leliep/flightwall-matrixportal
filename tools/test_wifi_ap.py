#!/usr/bin/env python3
"""Exercise actual AP shutdown: keep STA/credentials, report real radio state."""
from pathlib import Path
import tempfile,subprocess
root=Path(__file__).resolve().parents[1];s=(root/'src/main.cpp').read_text()
def function(signature):
 a=s.index(signature+' {');return s[a:s.index('\n}\n',a)+3]
code=r'''
#include <Arduino.h>
#include <cassert>
constexpr int WIFI_STA=1,WIFI_AP=2,WIFI_AP_STA=3;
bool wifiSetupAccessPointActive=false;
struct Wifi {int mode=1,calls=0;bool fail=false;String credentials="preserved";
 int getMode(){return mode;}bool enableAP(bool enable){assert(!enable);++calls;if(fail)return false;mode&=~WIFI_AP;return true;}
}WiFi;
struct Dns{int stopped=0;void stop(){++stopped;}}setupDnsServer;
struct Log{void println(const char*){}}Serial;
'''+function('bool wifiAccessPointEnabled()')+function('const char* wifiModeName()')+function('void stopSetupAccessPoint()')+r'''
int main(){
 assert(String(wifiModeName())==String("STA"));stopSetupAccessPoint();assert(WiFi.calls==0);
 WiFi.mode=WIFI_AP_STA;wifiSetupAccessPointActive=true;assert(wifiAccessPointEnabled());
 stopSetupAccessPoint();assert(WiFi.mode==WIFI_STA&&!wifiSetupAccessPointActive&&!wifiAccessPointEnabled());assert(WiFi.credentials==String("preserved"));
 WiFi.mode=WIFI_AP_STA; // catch an actual AP despite a stale inactive flag
 stopSetupAccessPoint();assert(WiFi.mode==WIFI_STA);
 WiFi.mode=WIFI_AP_STA;wifiSetupAccessPointActive=true;WiFi.fail=true;
 stopSetupAccessPoint();assert(wifiAccessPointEnabled()&&wifiSetupAccessPointActive);
 assert(String(wifiModeName())==String("AP+STA"));
 WiFi.mode=WIFI_AP;assert(String(wifiModeName())==String("AP"));
 WiFi.mode=0;assert(String(wifiModeName())==String("OFF"));
}
'''
for field in ['wifi_mode','wifi_ap_active','wifi_ap_ssid','wifi_ap_mac_address','wifi_ap_ip']:assert '\\"'+field+'\\"' in s
assert 'htmlEscape(WiFi.softAPSSID())' in s and 'jsonEscape(WiFi.softAPSSID())' in s
with tempfile.TemporaryDirectory(prefix='flightwall-wifi-ap-') as d:
 p=Path(d);(p/'test.cpp').write_text(code)
 subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined','-I'+str(root/'tools/test_support'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('Actual AP shutdown, STA/credential preservation, stale flag, failure and real-mode diagnostics passed')
