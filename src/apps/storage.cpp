#include "config.h"
#include "apps/storage.h"
#include <SD.h>
#include <SPI.h>

namespace storage {

static bool sdMounted = false;

bool mountSd() {
  if (sdMounted) return true;

#if USE_SD_SPI
  // Ensure SPI uses the configured pins.
  SPI.begin(SDSPI_SCK_PIN, SDSPI_MISO_PIN, SDSPI_MOSI_PIN, SDSPI_CS_PIN);
  sdMounted = SD.begin(SDSPI_CS_PIN, SPI, SDSPI_FREQ_HZ, "/sdcard");
#else
  sdMounted = SD_MMC.begin("/sdcard", true);  // 1-bit mode if no explicit pins
#endif

  if (!sdMounted) {
    Serial.println("SD mount failed");
  } else {
    Serial.println("SD card mounted");
  }
  return sdMounted;
}

bool isMounted() {
  return sdMounted;
}

bool getSpace(uint64_t &used, uint64_t &total) {
  if (!sdMounted) return false;
#if USE_SD_SPI
  total = SD.totalBytes();
  used  = SD.usedBytes();
  return true;
#else
  total = SD_MMC.totalBytes();
  used  = SD_MMC.usedBytes();
  return true;
#endif
}

}  // namespace storage
