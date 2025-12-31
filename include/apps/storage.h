#pragma once

#include <Arduino.h>

namespace storage {

// Mount SD card (SPI mode). Returns true on success.
bool mountSd();

// Whether SD is mounted.
bool isMounted();

// Get used/total bytes (returns false if not mounted or unsupported).
bool getSpace(uint64_t &used, uint64_t &total);

}  // namespace storage
