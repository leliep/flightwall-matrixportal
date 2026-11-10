#pragma once
// Host-only adapter for the Arduino String boundary used by PsramText.
#include <string>
class String {
 public:
  String(const char *text = "") : value_(text ? text : "") {}
  String(const char *text, unsigned int length) : value_(text, length) {}
  const char *c_str() const { return value_.c_str(); }
  unsigned int length() const { return value_.size(); }
  bool isEmpty() const { return value_.empty(); }
  bool startsWith(const char *prefix) const { return value_.rfind(prefix, 0) == 0; }
 private:
  std::string value_;
};
