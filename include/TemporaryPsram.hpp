#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <cstddef>
#include <cstring>
#include <utility>

struct TemporaryPsramStats {
  size_t allocatedBytes = 0; // requested payload bytes; allocator headers excluded
  size_t peakBytes = 0;
  uint32_t allocationFailures = 0;
};
TemporaryPsramStats temporaryPsramStats();
void *allocateTemporaryPsram(size_t bytes);
void *resizeTemporaryPsram(void *pointer, size_t bytes);
void freeTemporaryPsram(void *pointer);
ArduinoJson::Allocator &temporaryJsonAllocator();
class __FlashStringHelper;

// Mutable response/serialization buffer. No implicit conversion to String and
// no internal-heap fallback; a failed response must be discarded, never sent.
class PsramBuffer {
 public:
  static constexpr size_t maximumSize = 512 * 1024;
  PsramBuffer() = default;
  PsramBuffer(const char *text) { append(text, text ? std::strlen(text) : 0); }
  PsramBuffer(const String &text) { append(text.c_str(), text.length()); }
  PsramBuffer(const PsramBuffer&) = delete;
  PsramBuffer &operator=(const PsramBuffer&) = delete;
  PsramBuffer(PsramBuffer &&other) noexcept { swap(other); }
  PsramBuffer &operator=(PsramBuffer &&other) noexcept {
    if (this != &other) { PsramBuffer empty; swap(empty); swap(other); }
    return *this;
  }
  ~PsramBuffer() { freeTemporaryPsram(data_); }
  bool failed() const { return failed_; }
  size_t length() const { return length_; }
  const char *c_str() const { return data_ ? data_ : ""; }
  char *data() { return data_; }
  bool reserve(size_t size) {
    if (failed_) return false;
    if (size > maximumSize) { failed_ = true; return false; }
    if (size <= capacity_) return true;
    size_t grown = capacity_ ? capacity_ + capacity_/2 : 256;
    if (grown < size) grown = size;
    if (grown > maximumSize) grown = maximumSize;
    void *next = resizeTemporaryPsram(data_, grown + 1);
    if (!next) { failed_ = true; return false; }
    data_ = static_cast<char*>(next); capacity_ = grown; data_[length_] = 0;
    return true;
  }
  bool append(const char *text, size_t size) {
    if (!text && size) { failed_=true;return false; }
    const uintptr_t address=reinterpret_cast<uintptr_t>(text);
    const uintptr_t begin=reinterpret_cast<uintptr_t>(data_);
    const bool aliases=data_ && address>=begin && address<=begin+length_;
    const size_t offset=aliases ? address-begin : 0;
    if (size > maximumSize - length_ || !reserve(length_ + size)) {
      failed_ = true; return false;
    }
    if (aliases) text=data_+offset;
    if (size) std::memmove(data_ + length_, text, size);
    length_ += size;
    if (data_) data_[length_] = 0;
    return true;
  }
  size_t write(uint8_t value) { return append(reinterpret_cast<const char*>(&value),1) ? 1 : 0; }
  size_t write(const uint8_t *text, size_t size) { return append(reinterpret_cast<const char*>(text),size) ? size : 0; }
  PsramBuffer &operator+=(const PsramBuffer &value) {
    if (value.failed()) failed_=true;
    else append(value.c_str(),value.length());
    return *this;
  }
  PsramBuffer &operator+=(const String &value) { append(value.c_str(),value.length()); return *this; }
  PsramBuffer &operator+=(const char *value) { if (value) append(value,std::strlen(value)); return *this; }
  PsramBuffer &operator+=(const __FlashStringHelper *value) { return *this += reinterpret_cast<const char*>(value); }
  PsramBuffer &operator+=(char value) { append(&value,1); return *this; }
  PsramBuffer &operator=(const String &value) { length_=0; *this += value; return *this; }
  PsramBuffer &operator=(const char *value) { length_=0; *this += value; return *this; }
  friend PsramBuffer operator+(PsramBuffer left,const String &right) { left += right; return left; }
  friend PsramBuffer operator+(PsramBuffer left,const char *right) { left += right; return left; }
  friend PsramBuffer operator+(PsramBuffer left,char right) { left += right; return left; }
  void remove(size_t start) { if (start < length_) { length_=start;data_[length_]=0; } }
  int indexOf(const String &token, size_t start=0) const { return indexOf(token.c_str(),start); }
  int indexOf(const char *token, size_t start=0) const {
    if (start > length_) return -1;
    const char *found=std::strstr(c_str()+start,token);
    return found ? static_cast<int>(found-c_str()) : -1;
  }
  void replace(const char *from, const String &to) { replace(from,to.c_str()); }
  void replace(const char *from, const char *to) {
    const size_t a=std::strlen(from),b=std::strlen(to);
    if (!a || failed_) return;
    size_t at=0;
    for (;;) {
      int found=indexOf(from,at); if (found < 0) return;
      const size_t pos=static_cast<size_t>(found);
      if (b>a && (b-a > maximumSize-length_ || !reserve(length_+b-a))) { failed_=true; return; }
      std::memmove(data_+pos+b,data_+pos+a,length_-pos-a+1);
      if (b) std::memcpy(data_+pos,to,b);
      length_=length_-a+b;at=pos+b;
    }
  }
 private:
  char *data_ = nullptr;
  size_t length_ = 0,capacity_ = 0;
  bool failed_ = false;
  void swap(PsramBuffer &other) {
    std::swap(data_,other.data_); std::swap(length_,other.length_);
    std::swap(capacity_,other.capacity_); std::swap(failed_,other.failed_);
  }
};
