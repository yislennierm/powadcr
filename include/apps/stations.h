#pragma once

#include <Arduino.h>
#include <vector>

namespace stations {

struct Station {
  String name;
  String url;
};

// Load stations from a text file (default: /RADIO/radio.txt).
// Expected line format: Name -> http://url
bool loadFromFile(const char* path = "/RADIO/radio.txt");

size_t count();
const Station* get(size_t idx);

}  // namespace stations
