#include "config.h"
#include "apps/player_core.h"
#include "apps/storage.h"
#include <FS.h>
#if USE_SD_SPI
  #include <SD.h>
#else
  #include <SD_MMC.h>
#endif
#include <AudioTools.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/AudioCodecs/CodecWAV.h>

using namespace audio_tools;

namespace player_core {

#if USE_SD_SPI
  static fs::FS &fs = SD;
#else
  static fs::FS &fs = SD_MMC;
#endif

static I2SStream i2s;
static VolumeStream volume(i2s);
static MP3DecoderHelix mp3Dec;
static WAVDecoder wavDec;
static EncodedAudioStream* pipeline = nullptr; // created per start with chosen decoder
static File audioFile;
static size_t totalBytes = 0;
static bool ready = false;
static bool doneFlag = false;
static bool failFlag = false;
static bool running = false;
static Mode currentMode = Mode::WAV;
static uint8_t buf[1024];

static void endPipeline() {
  if (pipeline) {
    pipeline->end();
    delete pipeline;
    pipeline = nullptr;
  }
}

bool start(const char* path, Mode mode, float volumePct) {
  stop();
  if (!path || strlen(path) == 0) {
    failFlag = true;
    return false;
  }
  if (!storage::mountSd()) {
    failFlag = true;
    return false;
  }

  audioFile = fs.open(path, FILE_READ);
  if (!audioFile) {
    Serial.printf("Audio open failed: %s\n", path);
    failFlag = true;
    return false;
  }
  totalBytes = audioFile.size();
  currentMode = mode;

  auto cfg = i2s.defaultConfig(TX_MODE);
  cfg.sample_rate = 44100;
  cfg.bits_per_sample = 16;
  cfg.channels = 2;
  cfg.pin_bck = I2S_BCK_PIN;
  cfg.pin_ws  = I2S_LRCK_PIN;
  cfg.pin_data = I2S_DATA_PIN;
  cfg.pin_data_rx = -1;
  cfg.buffer_size = 512;
  cfg.buffer_count = 4;
  i2s.begin(cfg);

  volume.begin(cfg);
  volume.setVolume(constrain(volumePct, 0.0f, 100.0f) / 100.0f);

  if (mode == Mode::MP3) {
    pipeline = new EncodedAudioStream(&volume, &mp3Dec);
  } else if (mode == Mode::FLAC) {
    Serial.println("FLAC not supported on this build");
    failFlag = true;
    audioFile.close();
    i2s.end();
    return false;
  } else {
    pipeline = new EncodedAudioStream(&volume, &wavDec);
  }
  if (!pipeline || !pipeline->begin()) {
    Serial.println("Audio pipeline begin failed");
    failFlag = true;
    ready = false;
    running = false;
    endPipeline();
    audioFile.close();
    i2s.end();
    return false;
  }

  ready = true;
  running = true;
  doneFlag = false;
  failFlag = false;
  Serial.printf("Playing %s: %s (%.2f KB)\n", (mode == Mode::MP3) ? "MP3" : "WAV", path, totalBytes / 1024.0f);
  return true;
}

void loop() {
  if (!ready || !pipeline || !audioFile) return;
  if (!audioFile.available()) {
    doneFlag = true;
    ready = false;
    running = false;
    endPipeline();
    i2s.end();
    audioFile.close();
    return;
  }
  size_t avail = audioFile.available();
  size_t toRead = avail > sizeof(buf) ? sizeof(buf) : avail;
  size_t n = audioFile.read(buf, toRead);
  if (n > 0) {
    pipeline->write(buf, n);
  } else {
    doneFlag = true;
    ready = false;
    running = false;
    endPipeline();
    i2s.end();
    audioFile.close();
  }
}

void stop() {
  ready = false;
  running = false;
  doneFlag = false;
  if (audioFile) audioFile.close();
  endPipeline();
  i2s.end();
}

int bufferPercent() {
  if (!totalBytes) return 0;
  size_t pos = audioFile ? audioFile.position() : totalBytes;
  int pct = int((pos * 100ULL) / totalBytes);
  return constrain(pct, 0, 100);
}

bool isRunning() {
  return running && ready && !failFlag && !doneFlag;
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

}  // namespace player_core
