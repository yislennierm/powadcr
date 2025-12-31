#include "config.h"
#include "apps/stations.h"
#include "apps/storage.h"
#include <SD.h>
#include <vector>

namespace stations {

static std::vector<Station> gStations;

static String trim(const String &s) {
  int start = 0;
  int end = s.length();
  while (start < end && isspace(static_cast<unsigned char>(s[start]))) start++;
  while (end > start && isspace(static_cast<unsigned char>(s[end - 1]))) end--;
  return s.substring(start, end);
}

bool loadFromFile(const char* path) {
  gStations.clear();
  if (!storage::isMounted()) {
    Serial.println("Stations: SD not mounted");
    return false;
  }
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.printf("Stations: cannot open %s\n", path);
    return false;
  }

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.replace("\r", "");
    line = trim(line);
    if (line.length() == 0 || line.startsWith("#")) continue;
    int sep = line.indexOf("->");
    if (sep < 0) continue;
    String name = trim(line.substring(0, sep));
    String url = trim(line.substring(sep + 2));
    if (name.length() == 0 || url.length() == 0) continue;
    Station st{ name, url };
    gStations.push_back(st);
  }
  f.close();
  Serial.printf("Stations loaded: %u\n", static_cast<unsigned>(gStations.size()));
  return !gStations.empty();
}

size_t count() {
  return gStations.size();
}

const Station* get(size_t idx) {
  if (idx >= gStations.size()) return nullptr;
  return &gStations[idx];
}

}  // namespace stations
