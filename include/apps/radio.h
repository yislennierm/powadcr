#pragma once

#include <Arduino.h>

namespace radio {

// Initialize the audio pipeline and open the given stream URL.
// volumePct: 0..100
bool init(float volumePct, const char* url);

// Periodic pump; call from loop().
void loop();

// Adjust master volume (0..100).
void setVolume(float volumePct);

// Returns last known buffer fullness percentage (0..100).
int bufferPercent();

// Returns true if the pipeline was started successfully.
bool isReady();

}  // namespace radio
