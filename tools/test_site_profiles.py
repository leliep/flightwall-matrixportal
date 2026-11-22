#!/usr/bin/env python3
"""Exercise profile migration, validation, copies, restore and nearby lookup templates."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1];source=(root/'src/main.cpp').read_text()
def function(signature):
 a=source.index(signature);return source[a:source.index('\n}\n',a)+3]
helpers='\n'.join(function(x) for x in ['bool prepareSiteProfilesRestore(', 'bool prepareSiteProfilesSubmission(', 'bool applyActiveSiteProfile(', 'bool validSiteNameSetting(', 'bool siteProfilesError(', 'void handleConfigurationRestore()', 'PsramBuffer escapedProfileAttribute(', 'void fillSitePageValues('])
code=r'''
#include "SiteProfiles.hpp"
#include "AirportCoordinates.hpp"
#include "ManualAircraftSelection.hpp"
#include "Geo.hpp"
#include "RadarSweep.hpp"
#include <esp_heap_caps.h>
#include <cassert>
#include <cstdlib>
#include <cmath>
#include <map>
#include <iostream>
namespace ArduinoJson { template<> struct Converter<String> {
 static String fromJson(JsonVariantConst v){return String(v.as<const char*>());}
 static void toJson(const String&v,JsonVariant dst){dst.set(JsonString(v.c_str(),v.length(),false));}
};
inline DeserializationError deserializeJson(JsonDocument &doc,String &&input){return deserializeJson(doc,input.c_str(),input.length());}
}
bool operator!=(const String&a,const String&b){return !(a==b);}
using std::isfinite;
template<class T,class L,class H>T constrain(T n,L lo,H hi){return n<lo?lo:n>hi?hi:n;}
uint32_t millis(){return 1000;}
const char *uiText(const char*de,const char*){return de;}
bool settingsWritePreparing=false,settingsWriteInProgress=false;
int preferencesMutex=2;uint32_t manualRebootAt=0;
constexpr int BRIGHTNESS_LEVEL_COUNT=7,PAGE_COUNT=8;
struct Prefs {
 bool fail=false;int writes=0;std::string blob;std::map<std::string,double> numbers;
 size_t putBytes(const char*,const void*p,size_t n){if(fail)return 0;++writes;blob.assign(static_cast<const char*>(p),n);return n;}
 void putString(const char*,const String&){++writes;}
 void putBool(const char*,bool){++writes;}
 void putUChar(const char*k,int v){numbers[k]=v;++writes;}
 void putUShort(const char*k,int v){numbers[k]=v;++writes;}
 void putUInt(const char*k,unsigned v){numbers[k]=v;++writes;}
 void putInt(const char*k,int v){numbers[k]=v;++writes;}
 void putFloat(const char*k,float v){numbers[k]=v;++writes;}
 void putDouble(const char*k,double v){numbers[k]=v;++writes;}
}preferences;
bool failAllocation=false,relocate=false;
void *heap_caps_realloc(void*p,size_t n,uint32_t caps){assert(caps==(MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));return failAllocation?nullptr:std::realloc(p,n);}
void *heap_caps_malloc(size_t n,uint32_t caps){return heap_caps_realloc(nullptr,n,caps);}
void heap_caps_free(void*p){std::free(p);}
bool lookupOverrideCoordinates(const String&code,AirportCoordinates&out){
 if(code==String("TST")){out.latitude=51.3;out.longitude=6.7;out.elevationFt=100;out.hasElevation=true;return true;}
 if(relocate&&code==String("DUS")){out.latitude=0;out.longitude=0;return true;}
 return false;
}
bool lookupOverrideAirport(const String&code,String&name){if(code==String("TST")){name="Test Airport";return true;}return false;}
size_t lookupAirportCount(){return 1;}String lookupAirportCodeAt(size_t){return "TST";}
SiteProfiles &siteProfiles() { static SiteProfiles profiles; return profiles; }
String siteName="Zons",routedFlight="OLD";
double siteLatitude=51.1,siteLongitude=6.8;float observerElevationM=42,viewHeading=290;
ManualAircraftSelection manualAircraftSelection;
struct Radar {struct Tracker {int calls=0;void clear(){++calls;}}tracker;} radar,*radarTrailStorage=&radar;
struct Flight {int available=0;}networkFlight;
struct Route {}currentRoute;
size_t networkRadarPointCount=5;uint32_t networkLastSuccessfulPoll=100;bool pollRequested=false;
int flightMutex=1;constexpr int pdTRUE=1;int pdMS_TO_TICKS(int x){return x;}
int xSemaphoreTake(int,int){return pdTRUE;}void xSemaphoreGive(int){}
struct Server {int status=0;void send(int code,const char*,const String&){status=code;}std::map<std::string,String> args;bool hasArg(const char*k){return args.count(k);}String arg(const char*k){return args[k];}}webServer;
'''+helpers+r'''
int main(){
 assert(siteProfiles().legacy(siteName,siteLatitude,siteLongitude,observerElevationM,viewHeading));
 assert(siteProfiles().profiles().size()==1&&siteProfiles().active()==0);
 assert(siteProfiles().selected()["latitude"].as<double>()==siteLatitude);
 SiteProfiles scientific;
 const char *scientificJson=R"({"profiles":[{"name":"Field 1.2e3","latitude":5.1601362e1,"longitude":6.141228e0,"elevation_m":0.0,"view_heading":0.0}],"active":0})";
 assert(scientific.decode(scientificJson,std::strlen(scientificJson)));
 assert(std::abs(scientific.selected()["longitude"].as<double>()-6.141228)<1e-12);
 assert(!std::strcmp(scientific.selected()["name"].as<const char*>(),"Field 1.2e3"));
 auto encoded=siteProfiles().encode();SiteProfiles restored;assert(restored.decode(encoded.c_str(),encoded.length()));
 // Decode preserves short decimal coordinates across NVS/OTA round trips.
 assert(std::abs(restored.selected()["latitude"].as<double>()-siteLatitude)<1e-12);
 JsonDocument doc;assert(!deserializeJson(doc,encoded.c_str()));
 auto extra=doc["profiles"].as<JsonArray>().add<JsonObject>();
 extra["name"]="<script> & Büro";extra["latitude"]=52.0;extra["longitude"]=7.0;extra["elevation_m"]=-10;extra["view_heading"]=359;doc["active"]=1;
 assert(restored.assign(doc["profiles"],doc["active"]));assert(restored.active()==1);
 auto original=restored.encode();
 for(const char*bad:{"{}","{\"profiles\":[],\"active\":0}","{\"profiles\":[null],\"active\":0}"})assert(!restored.decode(bad,std::strlen(bad)));
 doc["active"]=2;assert(!restored.assign(doc["profiles"],doc["active"]));doc["active"]=1;
 for(const char *key:{"latitude","longitude","elevation_m","view_heading"}){
  auto prior=doc["profiles"][1][key].as<double>();doc["profiles"][1][key]=100000;assert(!restored.assign(doc["profiles"],doc["active"]));
  doc["profiles"][1][key]=true;assert(!restored.assign(doc["profiles"],doc["active"]));
  doc["profiles"][1][key]="1";assert(!restored.assign(doc["profiles"],doc["active"]));doc["profiles"][1][key]=prior;
 }
 doc["profiles"][1]["name"]=std::string(49,'a');assert(!restored.assign(doc["profiles"],doc["active"]));
 doc["profiles"][1]["name"]="New";
 while(doc["profiles"].size()<16)doc["profiles"].as<JsonArray>().add(doc["profiles"][0]);
 assert(restored.assign(doc["profiles"],doc["active"]));doc["profiles"].as<JsonArray>().add(doc["profiles"][0]);assert(!restored.assign(doc["profiles"],doc["active"]));
 auto full=restored.encode();failAllocation=true;assert(!restored.legacy("bad",0,0,0,0));failAllocation=false;auto unchanged=restored.encode();assert(!std::strcmp(full.c_str(),unchanged.c_str()));
 SiteProfiles staged;JsonDocument backup;backup["site_name"]="Legacy";backup["site_latitude"]=50.5;
 assert(prepareSiteProfilesRestore(backup.as<JsonObjectConst>(),staged));assert(staged.selected()["latitude"].as<double>()==50.5);
 assert(staged.selected()["longitude"].as<double>()==siteLongitude);
 backup["site_profiles"]=restored.profiles();backup["active_site_profile"]=restored.active();
 assert(prepareSiteProfilesRestore(backup.as<JsonObjectConst>(),staged));assert(staged.profiles().size()==16&&staged.active()==1);
 backup["active_site_profile"]=99;assert(!prepareSiteProfilesRestore(backup.as<JsonObjectConst>(),staged));
 webServer.args["siteProfiles"]=String(original.c_str());assert(prepareSiteProfilesSubmission(staged));assert(staged.active()==1);
 webServer.args["siteLat"]="52.123456";assert(prepareSiteProfilesSubmission(staged));assert(staged.selected()["latitude"].as<double>()==52.123456);webServer.args.erase("siteLat");
 webServer.args["siteProfiles"]="garbage";assert(!prepareSiteProfilesSubmission(staged));webServer.args.clear();
 webServer.args["siteLat"]="NaN";assert(!prepareSiteProfilesSubmission(staged));webServer.args["siteLat"]="51.4";assert(prepareSiteProfilesSubmission(staged));
 manualAircraftSelection.select(123,0);networkFlight.available=1;
 assert(applyActiveSiteProfile(staged.selected()));assert(!manualAircraftSelection.identity(1));assert(!networkRadarPointCount&&!networkFlight.available&&pollRequested);
 assert(observerElevationM==42&&viewHeading==290&&siteLatitude==51.4);

 PsramBuffer markup("<input value=\"%FW_SITE_PROFILES%\">%FW_SITE_NAME% %FW_ROUTE_TEST_CALLSIGN% %FW_SITE_AIRPORTS%");
 PsramBuffer raw("{\"name\":\"<script>& %FW_SITE_NAME% %FW_SITE_AIRPORTS%\"}");auto attr=escapedProfileAttribute(raw);
 fillSitePageValues(markup,"%FW_SITE_PROFILES%","DLH123",attr.c_str(),"[]");
 assert(std::strstr(markup.c_str(),"&lt;script&gt;&amp; %FW_SITE_NAME% %FW_SITE_AIRPORTS%"));
 assert(std::strstr(markup.c_str(),"%FW_SITE_PROFILES% DLH123 []"));
 // Exercise the actual restore handler: validate before NVS, canonical mirrors, legacy retention and write failures.
 assert(siteProfiles().decode(original.c_str(),original.length()));
 JsonDocument request;request["format"]="flightwall-config-v1";
 request["settings"]["site_profiles"]=siteProfiles().profiles();request["settings"]["active_site_profile"]=1;
 request["settings"]["site_latitude"]=1; // canonical active profile must win
 auto post=[&]{std::string json;serializeJson(request,json);webServer.args["plain"]=String(json.c_str());handleConfigurationRestore();};
 post();assert(webServer.status==200&&preferences.numbers["siteLat"]==52.0&&preferences.numbers["observerElevM"]==-10);
 SiteProfiles saved;assert(saved.decode(preferences.blob.c_str(),preferences.blob.size())&&saved.active()==1);
 int previousWrites=preferences.writes;manualRebootAt=0;request["settings"]["active_site_profile"]=99;
 post();assert(webServer.status==400&&preferences.writes==previousWrites&&!manualRebootAt);
 request["settings"]["active_site_profile"]=1;preferences.fail=true;
 post();assert(webServer.status==503&&preferences.writes==previousWrites&&!manualRebootAt&&!settingsWriteInProgress&&!settingsWritePreparing);preferences.fail=false;
 request["settings"].remove("site_profiles");request["settings"].remove("active_site_profile");request["settings"]["site_latitude"]=50.5;
 post();assert(webServer.status==200);assert(saved.decode(preferences.blob.c_str(),preferences.blob.size())&&saved.profiles().size()==2);
 assert(saved.selected()["latitude"].as<double>()==50.5);
 auto templates=nearbyAirportTemplates(51.3,6.7);JsonDocument airports;assert(!deserializeJson(airports,templates.c_str()));
 bool dus=false,cgn=false,test=false;for(JsonObjectConst a:airports.as<JsonArrayConst>()){
  assert(a["distance_km"].as<double>()<=150.0);const String code=a["code"]|"";
  if(code==String("DUS"))dus=true;if(code==String("CGN"))cgn=true;
  if(code==String("TST")){test=true;assert(a["elevation_m"].as<int>()==30);}
  assert(!(code==String("FRA")));
 }
 assert(dus&&cgn&&test);relocate=true;templates=nearbyAirportTemplates(51.3,6.7);assert(!deserializeJson(airports,templates.c_str()));
 for(JsonObjectConst a:airports.as<JsonArrayConst>())assert(!(String(a["code"]|"")==String("DUS")));
 std::cout<<"Site profiles: legacy migration/restore, active mirrors, strict validation, bounded PSRAM/OOM, profile changes and 150km airport/override templates passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-profiles-') as folder:
 p=Path(folder);(p/'test.cpp').write_text(code)
 args=['c++','-std=c++11','-Wall','-Wextra','-pedantic','-pthread','-fsanitize=address,undefined','-DARDUINOJSON_ENABLE_ARDUINO_STRING=0','-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0','-I'+str(root/'tools/test_support'),'-I'+str(root/'include'),'-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/ArduinoJson/src')]
 args += [str(p/'test.cpp')]+[str(root/'src'/file) for file in ['SiteProfiles.cpp','TemporaryPsram.cpp','Geo.cpp','AirportCoordinates.generated.cpp','AirportNames.generated.cpp']]+['-o',str(p/'test')]
 subprocess.run(args,check=True);subprocess.run([str(p/'test')],check=True)
