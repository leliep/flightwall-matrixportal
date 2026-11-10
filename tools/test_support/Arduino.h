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
  int indexOf(char value, unsigned int from=0) const {
    auto position=value_.find(value,from);
    return position==std::string::npos ? -1 : static_cast<int>(position);
  }
  void replace(const char *from,const char *to) {
    if (!*from) return;
    size_t position=0;
    while ((position=value_.find(from,position))!=std::string::npos) {
      value_.replace(position,std::char_traits<char>::length(from),to);
      position+=std::char_traits<char>::length(to);
    }
  }
  friend String operator+(const String &a,const String &b) {
    return String((a.value_+b.value_).c_str());
  }
  int compareTo(const char *other) const { return value_.compare(other); }
  bool operator==(const String &other) const { return value_==other.value_; }
  String &operator+=(const String &other) { value_+=other.value_; return *this; }

 private:
  std::string value_;
};
