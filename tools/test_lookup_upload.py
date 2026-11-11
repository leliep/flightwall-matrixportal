#!/usr/bin/env python3
"""Run the actual streaming upload handler with interrupted/unauthorized requests."""
from pathlib import Path
import subprocess,tempfile
root=Path(__file__).resolve().parents[1]
source=(root/'src/main.cpp').read_text()
def function(signature):
 start=source.index(signature+' {')
 return source[start:source.index('\n}\n',start)+3]
code=r'''
#include <Arduino.h>
#include <cassert>
#include <cstdlib>
#include <cstring>
constexpr size_t lookupUploadLimit=65536;
bool operator!=(const String& a,const String& b){return !(a==b);}
enum State {UPLOAD_FILE_START,UPLOAD_FILE_WRITE,UPLOAD_FILE_END,UPLOAD_FILE_ABORTED};
struct HTTPUpload {State status;String name="lookup";size_t currentSize=0;unsigned char buf[16]={1,2,3,4};} request;
String otaPassword="abcdefgh";
struct Server {
 bool authenticated=true,headerPresent=true;
 String header(const char*) {return headerPresent?"1":"";}
 bool authenticate(const char*,const char*){return authenticated;}
 HTTPUpload& upload(){return request;}
} webServer;
char* lookupUploadData=nullptr;
size_t lookupUploadBytes=0;
bool lookupUploadComplete=false,lookupUploadStarted=false;
String lookupUploadError;
int allocated=0;
void* allocateLookupUpload(){++allocated;return std::malloc(lookupUploadLimit);}
void releaseLookupUpload(void* value){if(value){--allocated;std::free(value);}}
'''+function('void discardLookupUpload()')+function('bool lookupWriteAuthorized()')+function('void handleLookupUpload()')+r'''
void event(State state,size_t size=0){request.status=state;request.currentSize=size;handleLookupUpload();}
int main(){
 webServer.authenticated=false;event(UPLOAD_FILE_START);assert(allocated==0);
 webServer.authenticated=true;webServer.headerPresent=false;event(UPLOAD_FILE_START);assert(allocated==0);
 webServer.headerPresent=true;otaPassword="";event(UPLOAD_FILE_START);assert(allocated==0);otaPassword="abcdefgh";
 event(UPLOAD_FILE_START);assert(allocated==1);event(UPLOAD_FILE_WRITE,4);event(UPLOAD_FILE_WRITE,4);
 assert(lookupUploadBytes==8);assert(memcmp(lookupUploadData,request.buf,4)==0);
 event(UPLOAD_FILE_END);assert(lookupUploadComplete);assert(lookupUploadError.isEmpty());
 event(UPLOAD_FILE_START);assert(!lookupUploadError.isEmpty());assert(allocated==1);
 discardLookupUpload();assert(allocated==0);lookupUploadError="";
 event(UPLOAD_FILE_START);event(UPLOAD_FILE_WRITE,65537);assert(!lookupUploadError.isEmpty());
 event(UPLOAD_FILE_ABORTED);assert(allocated==0);assert(!lookupUploadStarted);
 lookupUploadError="";request.name="other";event(UPLOAD_FILE_START);assert(allocated==0);assert(!lookupUploadError.isEmpty());
 event(UPLOAD_FILE_ABORTED);request.name="lookup";lookupUploadError="";
 event(UPLOAD_FILE_START);event(UPLOAD_FILE_WRITE,1);event(UPLOAD_FILE_ABORTED);assert(allocated==0);
 event(UPLOAD_FILE_START);assert(lookupUploadError.isEmpty());assert(allocated==1);discardLookupUpload();assert(allocated==0);
}
'''
with tempfile.TemporaryDirectory(prefix='flightwall-lookup-upload-') as folder:
 cpp=Path(folder)/'test.cpp';cpp.write_text(code)
 for variant in (0,1):
  binary=Path(folder)/f'upload-{variant}'
  subprocess.run(['c++','-std=c++11','-Wall','-Wextra','-pedantic','-fsanitize=address,undefined',
   f'-DFLIGHTWALL_HAS_MATRIX={variant}','-I'+str(root/'tools/test_support'),str(cpp),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True)
print('Actual upload handler passed: auth/header, streaming, multi-file rejection, size limit, abort cleanup and retry')
