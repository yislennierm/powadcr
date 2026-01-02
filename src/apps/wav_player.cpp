#include "config.h"
#include "apps/wav_player.h"
#include "apps/storage.h"
#include <FS.h>
#if USE_SD_SPI
  #include <SD.h>
#else
  #include <SD_MMC.h>
#endif
#include <AudioTools.h>
#include <AudioTools/AudioCodecs/CodecWAV.h>

using namespace audio_tools;

namespace wav_player {

#if USE_SD_SPI
  static fs::FS &fs = SD;
#else
  static fs::FS &fs = SD_MMC;
#endif

static I2SStream i2s;
static VolumeStream volume(i2s);
static WAVDecoder decoder;
static EncodedAudioStream pcm(&volume, &decoder);
static File wavFile;
static size_t totalBytes = 0;
static bool ready = false;
static bool doneFlag = false;
static bool failFlag = false;
static bool running = false;
static String currentFileName;
static uint8_t buf[8192]; // bigger chunk to reduce choppiness

static String findFirstWav() {
  File dir = fs.open("/WAV");
  if (!dir || !dir.isDirectory()) return "";
  while (true) {
    File f = dir.openNextFile();
    if (!f) break;
    if (!f.isDirectory()) {
      String fname = f.name();
      f.close();
      String lower = fname;
      lower.toLowerCase();
      if (lower.endsWith(".wav")) {
        return String("/WAV/") + fname;
      }
    } else {
      f.close();
    }
  }
  return "";
}

bool start(const char* path, float volumePct) {
  if (!storage::mountSd()) {
    failFlag = true;
    ready = false;
    running = false;
    return false;
  }

  if (wavFile) wavFile.close();
  doneFlag = false;
  failFlag = false;
   running = false;

  String target;
  if (path && strlen(path) > 0) {
    target = path;
  } else {
    target = findFirstWav();
  }
  if (target.isEmpty()) {
    Serial.println("No WAV file found.");
    failFlag = true;
    return false;
  }

  wavFile = fs.open(target.c_str(), FILE_READ);
  if (!wavFile) {
    Serial.printf("Failed to open WAV: %s\n", target.c_str());
    failFlag = true;
    running = false;
    return false;
  }

  totalBytes = wavFile.size();
  currentFileName = wavFile.name();

  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.sample_rate = 44100;
  cfg.bits_per_sample = 16;
  cfg.channels = 2;
  cfg.pin_bck = I2S_BCK_PIN;
  cfg.pin_ws  = I2S_LRCK_PIN;
  cfg.pin_data = I2S_DATA_PIN;
  cfg.pin_data_rx = -1;
  i2s.begin(cfg);

  volume.begin(cfg);
  volume.setVolume(constrain(volumePct, 0.0f, 100.0f) / 100.0f);

  if (!pcm.begin()) {
    Serial.println("WAV pipeline begin failed");
    failFlag = true;
    ready = false;
    running = false;
    return false;
  }

  ready = true;
  running = true;
  Serial.printf("Playing WAV: %s (%.2f KB)\n", currentFileName.c_str(), totalBytes / 1024.0f);
  return true;
}

void loop() {
  if (!ready) return;
  if (!wavFile || !wavFile.available()) {
    doneFlag = true;
    ready = false;
    running = false;
    pcm.end();
    i2s.end();
    if (wavFile) wavFile.close();
    return;
  }
  size_t avail = wavFile.available();
  size_t toRead = avail > sizeof(buf) ? sizeof(buf) : avail;
  size_t n = wavFile.read(buf, toRead);
  if (n > 0) {
    pcm.write(buf, n);
  } else {
    doneFlag = true;
    ready = false;
    running = false;
    pcm.end();
    i2s.end();
    wavFile.close();
  }
}

void stop() {
  ready = false;
  doneFlag = false;
  running = false;
  if (wavFile) wavFile.close();
  pcm.end();
  i2s.end();
}

int bufferPercent() {
  if (!totalBytes) return 0;
  size_t pos = wavFile ? wavFile.position() : totalBytes;
  int pct = int((pos * 100ULL) / totalBytes);
  return constrain(pct, 0, 100);
}

String currentName() {
  return currentFileName;
}

bool finished() {
  bool d = doneFlag;
  doneFlag = false;
  return d;
}

bool failed() {
  bool f = failFlag;
  failFlag = false;
  return f;
}

bool isRunning() {
  return running && ready && !failFlag && !doneFlag;
}

}  // namespace wav_player
