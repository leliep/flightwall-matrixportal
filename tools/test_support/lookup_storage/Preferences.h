#pragma once
#include "FakeFilesystem.hpp"
#include <cassert>
class Preferences {
 public:
 bool begin(const char* name,bool){assert(std::string(name)=="fwlookup");return true;}
 size_t getBytesLength(const char*){return fakeMetadata.size();}
 size_t getBytes(const char*,void* out,size_t size){if(size!=fakeMetadata.size())return 0;memcpy(out,fakeMetadata.data(),size);return size;}
 size_t putBytes(const char*,const void* data,size_t size){if(fakeMetadataFailure)return 0;auto* p=static_cast<const unsigned char*>(data);fakeMetadata.assign(p,p+size);return size;}
 bool remove(const char*){fakeMetadata.clear();return true;}
 void end(){}
};
