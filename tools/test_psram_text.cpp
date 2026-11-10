// Run with tools/test_psram_text.py to include the actual firmware journal
// record definitions and reset functions in this host-only regression test.
#include <Arduino.h>
#include <esp_heap_caps.h>
#include "PsramText.hpp"
#include "RouteClient.hpp"
#include <cassert>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <limits>
#include "JournalTypesUnderTest.hpp"
#include "CacheUnderTest.hpp"

namespace {
std::unordered_map<void *, size_t> allocated;
int successfulAllocationsBeforeFailure = -1;
unsigned attempts = 0;
}
void *heap_caps_malloc(size_t bytes, uint32_t capabilities) {
  assert(capabilities == (MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  ++attempts;
  if (successfulAllocationsBeforeFailure == 0) return nullptr;
  if (successfulAllocationsBeforeFailure > 0) --successfulAllocationsBeforeFailure;
  void *memory = std::malloc(bytes);
  if (memory) allocated[memory] = bytes;
  return memory;
}
void heap_caps_free(void *memory) {
  assert(allocated.erase(memory) == 1);
  std::free(memory);
}

int main() {
  assert(psramTextMemoryStats().blocks == 0);
  {
    PsramText empty;
    assert(empty.isEmpty() && empty == "" && empty.length() == 0);
    PsramText text("Düsseldorf > Stockholm-Bromma");
    assert(text.length() == std::strlen(text.c_str()));
    assert(allocated.count(const_cast<void *>(static_cast<const void *>(text.c_str()))) == 0); // data follows its allocation header
    auto blocks = psramTextMemoryStats().blocks;
    auto bytes = psramTextMemoryStats().allocatedBytes;
    auto copies = attempts;
    PsramText shared(text);
    assert(shared.c_str() == text.c_str() && attempts == copies);
    shared = text;
    assert(attempts == copies);
    const PsramText *alias = &shared;
    shared = *alias; // self assignment keeps ownership
    assert(shared == text);
    shared = "HAM-LPA";
    assert(text == "Düsseldorf > Stockholm-Bromma" && shared == "HAM-LPA");
    assert(psramTextMemoryStats().blocks == blocks + 1);
    shared = PsramText();
    assert(psramTextMemoryStats().allocatedBytes == bytes);
    PsramText moved(std::move(text));
    assert(text.isEmpty() && moved == "Düsseldorf > Stockholm-Bromma");
    String native = moved;
    assert(std::strcmp(native.c_str(), moved.c_str()) == 0);
    moved = String("EDWF > BMA");
    assert(moved == String("EDWF > BMA"));
    copies = attempts;
    moved = String("EDWF > BMA");
    assert(attempts == copies); // unchanged polling values never allocate
    assert(moved.assign(moved.c_str() + 7)); // overlapping source remains valid
    assert(moved == "BMA");
    auto failures = psramTextMemoryStats().allocationFailures;
    successfulAllocationsBeforeFailure = 0;
    assert(!moved.assign("A longer replacement that cannot be allocated"));
    assert(moved == "BMA" && psramTextMemoryStats().allocationFailures == failures + 1);
    successfulAllocationsBeforeFailure = -1;
    assert(!moved.assign("x", std::numeric_limits<size_t>::max()));
    assert(moved == "BMA" && psramTextMemoryStats().allocationFailures == failures + 2);
    moved = nullptr;
    assert(moved.isEmpty());
  }
  assert(psramTextMemoryStats().blocks == 0 && allocated.empty());

  // Stage a real firmware record, fail after one successful field allocation:
  // original texts and metadata must remain intact and partial work is freed.
  dailyJournal[0].flight = "KLC63V";
  dailyJournal[0].routeNames = "Amsterdam > Florenz";
  dailyJournal[0].identity = 123;
  dailyJournalCount = 1;
  const auto bytesBefore = psramTextMemoryStats().allocatedBytes;
  {
    const PsramTextTransaction transaction;
    DailyJournalEntry staged = dailyJournal[0];
    staged.identity = 456;
    successfulAllocationsBeforeFailure = 1;
    staged.flight = "NEW123";
    staged.routeNames = "Replacement route";
    assert(!transaction.succeeded());
    if (transaction.succeeded()) dailyJournal[0] = std::move(staged);
    successfulAllocationsBeforeFailure = -1;
  }
  assert(dailyJournal[0].identity == 123 && dailyJournal[0].flight == "KLC63V");
  assert(dailyJournal[0].routeNames == "Amsterdam > Florenz");
  assert(psramTextMemoryStats().allocatedBytes == bytesBefore);
  resetDailyJournal();
  assert(psramTextMemoryStats().blocks == 0); // reset really frees owned text

  // Exercise the actual firmware update functions under allocation failure.
  Aircraft aircraft; aircraft.flight="KLC63V"; aircraft.hex="ABC123";
  aircraft.type="A320"; aircraft.description="Airbus A320";
  FlightData shown; shown.flight="KLC63V"; shown.hex="ABC123";
  shown.routeNames="Amsterdam > Florenz";
  shown.selectionReason="Geringste Entfernung";
  successfulAllocationsBeforeFailure=0;
  updateDailyJournal(aircraft,shown);
  updateSpecialChronicleObservation(aircraft);
  assert(dailyJournalCount==0 && specialChronicleCount==0);
  successfulAllocationsBeforeFailure=-1;
  updateDailyJournal(aircraft,shown);
  updateSpecialChronicleObservation(aircraft);
  assert(dailyJournalCount==1 && specialChronicleCount==1);
  shown.flight="NEW123"; shown.routeNames="Different route";
  successfulAllocationsBeforeFailure=1;
  updateDailyJournal(aircraft,shown);
  assert(dailyJournal[0].flight=="KLC63V" && dailyJournal[0].routeNames=="Amsterdam > Florenz");
  assert(dailyJournalCount==1);
  shown.routeCodes="EDWF-BMA";
  successfulAllocationsBeforeFailure=1;
  markSpecialChronicleDisplayed(aircraft,shown);
  assert(!specialChronicle[0].displayed && specialChronicle[0].selectionReason.isEmpty());
  successfulAllocationsBeforeFailure=-1;
  markSpecialChronicleDisplayed(aircraft,shown);
  assert(specialChronicle[0].displayed && specialChronicle[0].routeCodes=="EDWF-BMA");
  resetDailyJournal(); resetSpecialChronicle();
  assert(psramTextMemoryStats().blocks==0);

  // Fill both actual journal structures to their firmware limits.
  for (size_t i = 0; i < DAILY_JOURNAL_LIMIT; ++i) {
    auto &e = dailyJournal[i];
    e.hex="ABC123"; e.flight="FLIGHT123"; e.airline="Example Airline";
    e.type="A320"; e.typeDescription="AIRBUS A320"; e.specialType="Military";
    e.squawk="1000"; e.routeCodes="EDWF-BMA";
    e.routeNames="Leer-Papenburg > Stockholm-Bromma";
    e.selectionReason="Geringste Entfernung unter geeigneten Flugzeugen";
  }
  dailyJournalCount = DAILY_JOURNAL_LIMIT;
  for (size_t i = 0; i < SPECIAL_CHRONICLE_LIMIT; ++i) {
    auto &e = specialChronicle[i];
    e.hex="ABC123"; e.flight="GAF123"; e.type="GL5T";
    e.typeDescription="BOMBARDIER Global 5000"; e.specialType="Military";
    e.squawk="1000"; e.routeCodes="EDWF-BMA"; e.selectionReason="Special";
  }
  specialChronicleCount = SPECIAL_CHRONICLE_LIMIT;
  assert(psramTextMemoryStats().blocks == DAILY_JOURNAL_LIMIT * 10 + SPECIAL_CHRONICLE_LIMIT * 8);
  // Exercise the actual persistent route representation and native round trip.
  StoredRouteResult cache[32];
  RouteResult route;
  route.routeCodes="EDWF-BMA"; route.routeNames="Leer-Papenburg > Stockholm-Bromma";
  route.origin="EDWF"; route.destination="BMA"; route.originName="Leer-Papenburg";
  route.destinationName="Stockholm-Bromma"; route.airlineCode="HER";
  route.source="adsb.im"; route.status="current"; route.error="";
  route.confidenceReason="Quelle adsb.im, Position plausibel"; route.confidence=95;
  for (auto &slot : cache) slot=route;
  RouteResult restored=cache[0];
  assert(std::strcmp(restored.routeNames.c_str(),route.routeNames.c_str())==0 && restored.confidence==95);
  assert(cache[0].found());
  {
    CacheUnderTest actualCache;
    for (size_t i=0;i<32;++i) {
      const std::string name="CALL"+std::to_string(i);
      actualCache.rememberInMemory(String(name.c_str()),route,100+i,true);
    }
    assert(actualCache.nextMemorySlot_==32);
    const auto retainedBytes=psramTextMemoryStats().allocatedBytes;
    successfulAllocationsBeforeFailure=1;
    actualCache.rememberInMemory(String("NEW33"),route,999,false);
    assert(actualCache.nextMemorySlot_==32 && actualCache.memoryCache_[0].callsign=="CALL0");
    assert(actualCache.memoryCache_[0].updated==100 && psramTextMemoryStats().allocatedBytes==retainedBytes);
    actualCache.rememberInMemory(String("CALL0"),route,999,false);
    assert(actualCache.memoryCache_[0].updated==100);
    successfulAllocationsBeforeFailure=-1;
    actualCache.rememberInMemory(String("NEW33"),route,999,false);
    assert(actualCache.nextMemorySlot_==33 && actualCache.memoryCache_[0].callsign=="NEW33");
  }
  // Repeated shorter replacement has no high-water retention or lost buffers.
  for (int round=0; round<100; ++round) for (auto &slot : cache) {
    slot=route;
    slot=StoredRouteResult();
  }
  resetDailyJournal(); resetSpecialChronicle();
  assert(dailyJournalCount==0 && specialChronicleCount==0);
  assert(psramTextMemoryStats().blocks==0 && psramTextMemoryStats().allocatedBytes==0 && allocated.empty());
  std::cout << "PSRAM text tests passed: external-only allocation, sharing, replacement, failure rollback, full journals/cache and real reset paths\n";
}
