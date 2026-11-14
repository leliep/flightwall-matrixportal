#!/usr/bin/env python3
"""Exercise the actual reconnect function with controlled network/OTA outcomes."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text()
start = source.index('bool recoverStalledWiFi() {')
function = source[start:source.index('\n}\n', start) + 3]
code = r'''
#include "NetworkRecovery.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
NetworkRecovery networkRecovery;
uint32_t clockMs=200000, lastWifiDisconnect=0, lastWiFiAttempt=0;
uint32_t otaRebootAt=0, manualRebootAt=0;
bool otaPreparing=false, otaInProgress=false, routeCacheWritePreparing=false;
bool wifiWasConnected=true, connectResult=true;
int stops=0, starts=0, connects=0, disconnects=0;
constexpr int WL_CONNECTED=3;
uint32_t millis(){return clockMs;}
struct {template<class... Args> void printf(const char*,Args...) {}} Serial;
struct {
 int state=WL_CONNECTED;
 int status(){return state;}
 void disconnect(bool off,bool erase){assert(!off && !erase);++disconnects;state=0;}
} WiFi;
void stopNetworkServices(){++stops;}
void setNetworkError(const char*){}
uint32_t pdMS_TO_TICKS(uint32_t ms){return ms;}
void vTaskDelay(uint32_t ms){clockMs+=ms;}
enum class NetworkPhase {Wifi};
struct NetworkPhaseScope {explicit NetworkPhaseScope(NetworkPhase) {}};
bool connectWiFi(uint32_t timeout){assert(timeout==10000);++connects;
 WiFi.state=connectResult?WL_CONNECTED:0;return connectResult;}
void startSetupAccessPoint(){++starts;}
'''+function+r'''
void failing(){for(unsigned i=0;i<5;++i)networkRecovery.transportFailure(clockMs-100000+i);}
int main(){
 failing();
 otaPreparing=true; assert(!recoverStalledWiFi()); otaPreparing=false;
 otaInProgress=true; assert(!recoverStalledWiFi()); otaInProgress=false;
 otaRebootAt=clockMs+1000; assert(!recoverStalledWiFi()); otaRebootAt=0;
 manualRebootAt=clockMs+1000; assert(!recoverStalledWiFi()); manualRebootAt=0;
 routeCacheWritePreparing=true; assert(!recoverStalledWiFi());routeCacheWritePreparing=false;
 WiFi.state=0;assert(!recoverStalledWiFi());WiFi.state=WL_CONNECTED;
 assert(!stops && !disconnects && !connects);
 assert(recoverStalledWiFi());
 assert(stops==1 && disconnects==1 && connects==1 && starts==0);
 assert(WiFi.state==WL_CONNECTED && !wifiWasConnected);
 assert(lastWifiDisconnect==200000 && lastWiFiAttempt==200100);
 failing();assert(!recoverStalledWiFi()); // Cooldown even when reconnect succeeded.
 clockMs=600000;connectResult=false;
 assert(recoverStalledWiFi());assert(starts==1 && connects==2 && disconnects==2);
 assert(WiFi.state==0 && networkRecovery.recoveries()==2);
 std::cout << "Actual reconnect function: OTA guards, cooldown, retained credentials, success/failure passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-network-recovery-') as folder:
    path = Path(folder)
    (path / 'test.cpp').write_text(code)
    subprocess.run(['c++', '-std=c++11', '-Wall', '-Wextra', '-pedantic',
                    '-I' + str(root / 'include'), str(path / 'test.cpp'),
                    '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
