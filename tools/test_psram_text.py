#!/usr/bin/env python3
"""Host regression tests of actual PSRAM text code and journal record/reset code."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root/'src/main.cpp').read_text()
records = source[source.index('struct DailyJournalEntry {'):source.index('String customPageText;')]
def function(name):
    start = source.index('void '+name+'(')
    end = source.index('\n}\n',start)+3
    return source[start:end]
with tempfile.TemporaryDirectory(prefix='flightwall-psram-text-') as temporary:
    folder=Path(temporary)
    flight_start=source.index('struct FlightData {')
    flight=source[flight_start:source.index('\n};',flight_start)+3]
    dependencies="""
struct Aircraft {
 uint64_t identity=123; String flight, hex, type, description, squawk;
 bool hasPosition=false, hasCpa=false;
 float distanceKm=0, cpaKm=0;
};
uint64_t aircraftIdentity(const Aircraft &value) { return value.identity; }
String specialCategoryName(const Aircraft &) { return "Military"; }
using std::isnan;
"""
    actual_functions=''.join(function(name) for name in ['resetDailyJournal','resetSpecialChronicle',
        'updateDailyJournal','updateSpecialChronicleObservation','markSpecialChronicleDisplayed'])
    (folder/'JournalTypesUnderTest.hpp').write_text(records+flight+dependencies+actual_functions)
    header=(root/'include/RouteClient.hpp').read_text()
    entry_start=header.index('  struct MemoryCacheEntry {')
    entry=header[entry_start:header.index('\n  };',entry_start)+5]
    routes=(root/'src/RouteClient.cpp').read_text()
    remember_start=routes.index('void RouteClient::rememberInMemory(')
    remember=routes[remember_start:routes.index('\n}\n',remember_start)+3].replace('RouteClient::','')
    cache='class CacheUnderTest { public:\n'+entry+'\nstatic constexpr size_t memoryCacheSlots_=32;\nMemoryCacheEntry memoryCache_[memoryCacheSlots_];\nsize_t nextMemorySlot_=0;\n'+remember+'\n};\n'
    (folder/'CacheUnderTest.hpp').write_text(cache)
    subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',
                    '-I'+str(folder),'-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),
                    str(root/'tools/test_psram_text.cpp'),str(root/'src/PsramText.cpp'),'-o',str(folder/'test')],check=True)
    subprocess.run([str(folder/'test')],check=True)
