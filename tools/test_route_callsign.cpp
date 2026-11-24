#include "RouteCallsign.hpp"
#include <cassert>
#include <iostream>
int main() {
  const char* equivalent[][2] = {{"DLH072","DLH72"},{"DLH72","DLH00072"},
    {"DLH000","DLH0"},{"DLH123","DLH123"},{"DLH072A","DLH072A"},
    {"UPP1088","UPP1088"},{"UP-P1088","UP-P1088"},
    {"DABCD","DABCD"},{"",""},
    {"DLH00012345678901234567890","DLH12345678901234567890"}};
  for (const auto& pair : equivalent) {
    assert(routeCallsignMatches(pair[0],pair[1]));
    assert(routeCallsignMatches(pair[1],pair[0]));
  }
  const char* different[][2] = {{"UPP1088","UPP01088"},{"UPP0088","UPP88"},
    {"UPP1088","UPP1089"},{"UP-P1088","UPP1088"},
    {"DLH072","DLH73"},{"DLH072","BAW72"},
    {"DLH072A","DLH72A"},{"DLH072","DLH72A"},{"DLH072","DLH"},
    {"D0072","D72"},{"LH0072","LH72"},{"DLH0072","DLH7 2"},
    {"DLH0072","DLH+72"},{"DLH0072","DLH72.0"},{"","DLH72"}};
  for (const auto& pair : different) {
    assert(!routeCallsignMatches(pair[0],pair[1]));
    assert(!routeCallsignMatches(pair[1],pair[0]));
  }
  std::cout << "Route callsign regression tests passed\n";
}
