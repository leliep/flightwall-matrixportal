#!/usr/bin/env python3
"""Check UTF-8 bounds and real HTML escaping for the optional site description."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
source = (root / 'src/main.cpp').read_text()
start = source.index('String htmlEscape(String value) {')
escape = source[start:source.index('\n}\n', start) + 3]
start = source.index('bool validSiteNameSetting(JsonVariantConst value) {')
setting = source[start:source.index('\n}\n', start) + 3]
code = r'''
#include <Arduino.h>
#include "SiteName.hpp"
#include <ArduinoJson.h>
#include <cassert>
#include <string>
#include <iostream>
'''+escape+setting+r'''
bool valid(const std::string& name){return validSiteName(name.data(),name.size());}
int main(){
 JsonDocument backup;
 assert(validSiteNameSetting(backup["site_name"]));
 backup["site_name"]="";assert(validSiteNameSetting(backup["site_name"]));
 backup["site_name"]="Büro München";assert(validSiteNameSetting(backup["site_name"]));
 backup["site_name"]=42;assert(!validSiteNameSetting(backup["site_name"]));
 backup["site_name"]=nullptr;assert(!validSiteNameSetting(backup["site_name"]));
 assert(!deserializeJson(backup,"{\"site_name\":\"A\\u0000B\"}"));
 assert(!validSiteNameSetting(backup["site_name"]));
 assert(valid(""));assert(valid("Dachstation 2"));
 assert(valid("Büro München / Süd-West"));
 assert(valid(std::string(48,'A')));assert(!valid(std::string(49,'A')));
 std::string umlauts;for(unsigned i=0;i<48;++i)umlauts+="ä";
 assert(valid(umlauts));assert(!valid(umlauts+"ä"));
 assert(!valid(std::string("A\0B",3)));
 assert(!valid("A\nB"));assert(!valid("A\tB"));assert(!valid("A\x7f"));
 assert(!valid("\xc0\xaf"));assert(!valid("\xed\xa0\x80"));
 assert(!valid("\xf4\x90\x80\x80"));assert(!valid("\xc2\x85"));
 assert(!valid("\xe2\x82"));assert(!valid("\x80"));
 assert(!validSiteName(nullptr,1));assert(validSiteName(nullptr,0));
 String payload="O'Brien \"Nord\" <script>&";
 assert(htmlEscape(payload)==String("O'Brien &quot;Nord&quot; &lt;script&gt;&amp;"));
 std::cout << "Site name: Unicode bounds, invalid encodings, controls, empty value and HTML escaping passed\n";
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-site-name-') as folder:
    path = Path(folder)
    (path/'test.cpp').write_text(code)
    subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic',
                    '-fsanitize=address,undefined','-DARDUINOJSON_ENABLE_ARDUINO_STRING=0',
                    '-DARDUINOJSON_ENABLE_ARDUINO_STREAM=0','-DARDUINOJSON_ENABLE_ARDUINO_PRINT=0',
                    '-I'+str(root/'.pio/libdeps/adafruit_matrixportal_esp32s3/ArduinoJson/src'),
                    '-I'+str(root/'include'),
                    '-I'+str(root/'tools/test_support'),str(path/'test.cpp'),
                    '-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
