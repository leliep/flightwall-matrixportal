#pragma once
// Host-only adapter for the Arduino String boundary used by PsramText.
#include <string>
#include <algorithm>
#include <cctype>
class String {
 public:
  String(const char *text = "") : value_(text ? text : "") {}
  String(const char *text, unsigned int length) : value_(text, length) {}
  const char *c_str() const { return value_.c_str(); }
  unsigned int length() const { return value_.size(); }
  bool isEmpty() const { return value_.empty(); }
  bool startsWith(const char *prefix) const { return value_.rfind(prefix, 0) == 0; }
  void trim() {
    const auto first=value_.find_first_not_of(" \t\r\n");
    if (first==std::string::npos) { value_.clear(); return; }
    value_=value_.substr(first,value_.find_last_not_of(" \t\r\n")-first+1);
  }
  void toUpperCase() {
    std::transform(value_.begin(),value_.end(),value_.begin(),[](unsigned char c){return std::toupper(c);});
  }
  int compareTo(const char *other) const { return value_.compare(other); }
  bool operator==(const String &other) const { return value_==other.value_; }
  String &operator+=(const String &other) { value_+=other.value_; return *this; }

 private:
  std::string value_;
};
