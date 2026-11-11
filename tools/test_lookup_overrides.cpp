#include "LookupOverrides.hpp"
#include "LookupStorage.hpp"
#include "AirportNames.hpp"
#include "AirlineNames.hpp"
#include "UnknownCodes.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <esp_heap_caps.h>

namespace {
bool mounted = true, writeFails = false, readFails = false;
int failAfter = -1;
std::string persisted;
const char* valid = R"({"format":"flightwall-lookups-v1","version":"test-1","airports":[{"code":"ZZZ","name":"Test Airport","latitude":51.5,"longitude":6.2,"elevation_ft":null},{"code":"DUS","name":"Override Düsseldorf","latitude":50,"longitude":7,"elevation_ft":321}],"airlines":[{"code":"ZZZ","name":"Test Airline"},{"code":"DLH","name":"Override Lufthansa"}]})";
std::string changed(const char* from,const char* to) {
 std::string result=valid;auto p=result.find(from);assert(p!=std::string::npos);result.replace(p,strlen(from),to);return result;
}
void rejected(const std::string& value) {
 const auto previous=persisted;const auto memory=lookupPsramBytes();
 assert(!installLookupOverrides(value.data(),value.size()));
 assert(persisted==previous);assert(lookupDataVersion()==String("test-1"));
 assert(lookupPsramBytes()==memory);
}
}
void* heap_caps_malloc(size_t size,uint32_t caps) {
 assert(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
 if(failAfter==0)return nullptr;
 if(failAfter>0)--failAfter;
 return std::malloc(size);
}
void heap_caps_free(void* value) { std::free(value); }
bool lookupStorageMount(){return mounted;}
bool lookupStorageInitialize(){mounted=true;return true;}
size_t lookupStorageCapacity(){return 1024*1024;}
size_t lookupStorageSize(){return persisted.size();}
bool lookupStorageRead(char* data,size_t size){if(readFails||size!=persisted.size())return false;memcpy(data,persisted.data(),size);return true;}
bool lookupStorageCommit(const char* data,size_t size){if(writeFails)return false;persisted.assign(data,size);return true;}

int main(){
 initializeLookupOverrides();
 assert(airportDisplayName("DUS")==String("Düsseldorf"));
 assert(airlineDisplayName("DLH")==String("Lufthansa"));
 assert(airportDisplayName("ZZZ").isEmpty());
 rememberUnknownAirport("ZZZ");rememberUnknownAirport("QQQ");
 rememberUnknownAirline("ZZZ");rememberUnknownAirline("QQQ");
 assert(installLookupOverrides(valid,strlen(valid)));
 assert(lookupAirportCount()==2 && lookupAirlineCount()==2);
 assert(airportDisplayName("ZZZ")==String("Test Airport"));
 assert(airlineDisplayName("DLH")==String("Override Lufthansa"));
 assert(airportDisplayName("FRA")==String("Frankfurt"));
 AirportCoordinates coordinates;assert(airportCoordinates("DUS",coordinates));
 assert(coordinates.latitude==50 && coordinates.elevationFt==321 && coordinates.hasElevation);
 assert(airportCoordinates("ZZZ",coordinates) && !coordinates.hasElevation);
 forgetResolvedUnknownCodes();
 assert(unknownAirportCount()==1 && unknownAirportAt(0)==String("QQQ"));
 assert(unknownAirlineCount()==1 && unknownAirlineAt(0)==String("QQQ"));
 rejected(changed("test-1",""));
 rejected(changed("flightwall-lookups-v1","flightwall-lookups-v2"));
 rejected(changed("\"ZZZ\"","\"zzZ\""));
 rejected(changed("\"ZZZ\"","\"DUS\""));
 rejected(changed("51.5","91"));
 rejected(changed("6.2","181"));
 rejected(changed("51.5","\"51.5\""));
 rejected(changed("null","32768"));
 rejected(changed("null","1.2"));
 rejected(changed("Test Airport"," Test Airport"));
 rejected(changed("Test Airport","Test\\u0000Airport"));
 rejected(changed("Test Airport","12345678901234567890123456789"));
 rejected(changed("Test Airport","\\ud800"));
 rejected(std::string(valid)+" garbage");
 rejected(std::string(valid)+valid);
 rejected(std::string(valid).substr(0,30));
 rejected(std::string(65537,' '));
 rejected("");
 writeFails=true;rejected(valid);writeFails=false;
 const auto memory=lookupPsramBytes();
 for(int i=0;i<12;++i){failAfter=i;bool ok=installLookupOverrides(valid,strlen(valid));failAfter=-1;
  assert(lookupDataVersion()==String("test-1"));assert(persisted==valid);assert(lookupPsramBytes()==memory);
  (void)ok;
 }
 for(int i=0;i<20;++i){assert(installLookupOverrides(valid,strlen(valid)));assert(lookupPsramBytes()==memory);}
 // Simulated restart: persistent data is parsed again, independent of runtime.
 initializeLookupOverrides();assert(airportDisplayName("ZZZ")==String("Test Airport"));assert(lookupPsramBytes()==memory);
 // An empty extension document restores built-ins and removes added names.
 const char* empty=R"({"format":"flightwall-lookups-v1","version":"empty","airports":[],"airlines":[]})";
 assert(installLookupOverrides(empty,strlen(empty)));
 assert(airportDisplayName("DUS")==String("Düsseldorf"));assert(airportDisplayName("ZZZ").isEmpty());
 assert(lookupAirportCount()==0 && lookupAirlineCount()==0);
 // Full capacity and oversize table are distinct from the byte limit.
 std::string rows="{\"format\":\"flightwall-lookups-v1\",\"version\":\"limit\",\"airports\":[],\"airlines\":[";
 for(int i=0;i<256;i++){if(i)rows+=',';char row[80];snprintf(row,sizeof(row),"{\"code\":\"%04d\",\"name\":\"Operator %d\"}",i,i);rows+=row;}
 rows+="]}";assert(installLookupOverrides(rows.data(),rows.size()));assert(lookupAirlineCount()==256);
 auto tooMany=rows;tooMany.insert(tooMany.size()-2,",{\"code\":\"EXTR\",\"name\":\"Extra\"}");assert(!installLookupOverrides(tooMany.data(),tooMany.size()));assert(lookupAirlineCount()==256);
 readFails=true;initializeLookupOverrides();assert(!lookupLastError().isEmpty());assert(lookupAirlineCount()==0);readFails=false;
 persisted=valid;initializeLookupOverrides();assert(lookupAirlineCount()==2);
 failAfter=0;initializeLookupOverrides();failAfter=-1;assert(lookupAirlineCount()==0);assert(!lookupLastError().isEmpty());
 initializeLookupOverrides();assert(lookupAirlineCount()==2);
 std::cout<<"Lookup tests passed: PSRAM only, overrides/fallback, coordinates, unknown pruning, validation, failure rollback, reload, limits and stable repeated updates\n";
}
