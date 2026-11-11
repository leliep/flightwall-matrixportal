#include "LookupStorage.hpp"
#include "FakeFilesystem.hpp"
#include <cassert>
#include <iostream>
std::map<std::string,std::string> fakeFiles;
std::vector<unsigned char> fakeMetadata;
bool fakeMounted=false,fakeWriteFailure=false,fakeReadFailure=false,fakeMetadataFailure=false;
FakeFilesystem FFat,LittleFS;
std::string loaded(){std::string s(lookupStorageSize(),' ');assert(lookupStorageRead(&s[0],s.size()));return s;}
int main(){
 assert(!lookupStorageMount());assert(fakeFiles.empty()); // no automatic format
 assert(lookupStorageInitialize());assert(lookupStorageCapacity()>0);
 assert(!lookupStorageCommit("",0));
 assert(lookupStorageCommit("old",3));assert(loaded()=="old");
 const auto metadata=fakeMetadata;
 fakeWriteFailure=true;assert(!lookupStorageCommit("new",3));fakeWriteFailure=false;
 assert(fakeMetadata==metadata);assert(loaded()=="old");
 fakeReadFailure=true;assert(!lookupStorageCommit("new",3));fakeReadFailure=false;
 assert(fakeMetadata==metadata);assert(loaded()=="old");
 fakeMetadataFailure=true;assert(!lookupStorageCommit("new",3));fakeMetadataFailure=false;
 assert(fakeMetadata==metadata);assert(loaded()=="old");
 assert(lookupStorageMount());assert(loaded()=="old"); // restart after failed commit
 assert(lookupStorageCommit("new",3));assert(loaded()=="new");
 assert(lookupStorageInitialize());assert(loaded()=="new"); // mounted volume never erased
 assert(lookupStorageMount());assert(loaded()=="new");
 for(auto& file:fakeFiles)file.second="bad";
 char bytes[3];assert(!lookupStorageRead(bytes,3)); // CRC rejects corrupt file
 fakeMetadata={1,2,3};assert(lookupStorageMount());assert(lookupStorageSize()==0);
 std::cout<<"Lookup storage tests passed: no auto-format, slot/CRC verification, write/read/NVS failures, reboot selection and mounted-volume preservation\n";
}
