#include <Arduino.h>
#include "config.h"
#include <WiFi.h>
#include <AudioTools.h>
#include <AudioTools/AudioCodecs/CodecMP3Helix.h>
#include <AudioTools/Communication/AudioHttp.h>
#include "apps/radio.h"

using namespace audio_tools;

namespace radio {

// Audio pipeline components (kept internal)
static I2SStream i2s;
static VolumeStream volume(i2s);
static MP3DecoderHelix decoder;
static EncodedAudioStream player(&volume, &decoder);
static URLStream urlStream;

// State
static bool ready = false;
static float gVolumePct = 90.0f;
static String gUrl;
static const size_t NET_BUF_SZ = 4096;
static uint8_t netBuf[NET_BUF_SZ];
static int lastBufPct = 0;

bool init(float volumePct, const char* url) {
  gVolumePct = constrain(volumePct, 0.0f, 100.0f);
  gUrl = url ? url : "";

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
  volume.setVolume(gVolumePct / 100.0f);

  if (!urlStream.begin(gUrl.c_str())) {
    Serial.println("URL begin failed");
    ready = false;
    return false;
  }
  if (!player.begin()) {
    Serial.println("Player begin failed");
    ready = false;
    return false;
  }
  ready = true;
  return true;
}

void loop() {
  if (!ready) return;

  int avail = urlStream.available();
  if (avail > 0) {
    size_t toRead = avail > NET_BUF_SZ ? NET_BUF_SZ : avail;
    size_t n = urlStream.readBytes(netBuf, toRead);
    if (n > 0) {
      player.write(netBuf, n);
    }
  } else {
    vTaskDelay(pdMS_TO_TICKS(5));
  }

  // Update buffer percent based on available bytes (proxy)
  int pctRaw = constrain(map(urlStream.available(), 0, (int)NET_BUF_SZ, 0, 100), 0, 100);
  if (lastBufPct == 0) lastBufPct = pctRaw;
  lastBufPct = (lastBufPct * 7 + pctRaw * 3) / 10;
}

void setVolume(float volumePct) {
  gVolumePct = constrain(volumePct, 0.0f, 100.0f);
  volume.setVolume(gVolumePct / 100.0f);
}

int bufferPercent() {
  return lastBufPct;
}

bool isReady() {
  return ready;
}

}  // namespace radio
