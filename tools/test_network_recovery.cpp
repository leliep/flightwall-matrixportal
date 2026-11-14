#include "NetworkRecovery.hpp"
#include <cassert>
#include <iostream>

int main() {
  for (int code : {-1,-2,-3,-4,-5,-11}) assert(NetworkRecovery::isTransportFailure(code));
  for (int code : {-6,-7,-8,-9,-10,0,200,403,404,429,500})
    assert(!NetworkRecovery::isTransportFailure(code));
  NetworkRecovery r;
  assert(!r.due(1000000, true, false));
  for (unsigned i=0; i<4; ++i) r.transportFailure(i*30000);
  assert(!r.due(120000,true,false)); // Time alone is insufficient.
  r.transportFailure(120000);
  assert(r.due(120000,true,false));
  assert(!r.due(120000,false,false));
  assert(!r.due(120000,true,true)); // OTA/flash/reboot protection.
  r.progress(); // Any HTTP response / functioning MQTT breaks the streak.
  assert(!r.due(130000,true,false));
  for (unsigned i=0;i<5;++i) r.transportFailure(140000+i);
  assert(!r.due(229999,true,false));
  assert(r.due(230000,true,false));
  r.recovering(230000);
  assert(r.recoveries()==1 && r.failures()==0);
  for (unsigned i=0;i<300;++i) r.transportFailure(240000+i);
  assert(r.failures()==255); // No counter wrap after a long outage.
  assert(!r.due(529999,true,false));
  assert(r.due(530000,true,false));
  r.progress();
  assert(!r.due(900000,true,false));
  NetworkRecovery wrap;
  const uint32_t start=UINT32_MAX-10000;
  for(unsigned i=0;i<5;++i) wrap.transportFailure(start+i);
  assert(!wrap.due(start+89999,true,false));
  assert(wrap.due(start+90000,true,false));
  wrap.recovering(start+90000);
  for(unsigned i=0;i<5;++i) wrap.transportFailure(start+100000+i);
  assert(!wrap.due(start+389999,true,false));
  assert(wrap.due(start+390000,true,false));
  std::cout << "Network recovery timing, progress, cooldown and wrap tests passed\n";
}
