#pragma once
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
extern std::map<std::string,std::string> fakeFiles;
extern std::vector<unsigned char> fakeMetadata;
extern bool fakeMounted, fakeWriteFailure, fakeReadFailure, fakeMetadataFailure;
#define FILE_READ "r"
#define FILE_WRITE "w"
class File {
 std::string path_;size_t position_=0;bool open_=false;
 public:
 File()=default;
 File(const char* path,const char* mode):path_(path) {
  if(!fakeMounted)return;
  if(*mode=='w'){fakeFiles[path_].clear();open_=true;}
  else open_=fakeFiles.count(path_)!=0;
 }
 explicit operator bool()const{return open_;}
 size_t size()const{return fakeFiles[path_].size();}
 size_t write(const unsigned char* bytes,size_t size){if(fakeWriteFailure)size/=2;fakeFiles[path_].append(reinterpret_cast<const char*>(bytes),size);return size;}
 size_t readBytes(char* out,size_t size){if(fakeReadFailure)return 0;size=std::min(size,fakeFiles[path_].size()-position_);memcpy(out,fakeFiles[path_].data()+position_,size);position_+=size;return size;}
 void flush(){}void close(){open_=false;}
};
struct FakeFilesystem {
 bool begin(bool formatOnFail,const char* ="",unsigned=1){if(formatOnFail)std::abort();return fakeMounted;}
 bool format(bool=false){fakeMounted=true;fakeFiles.clear();return true;}
 void end(){}
 size_t totalBytes(){return 1024*1024;}
 File open(const char* path,const char* mode){return File(path,mode);}
};
extern FakeFilesystem FFat,LittleFS;
