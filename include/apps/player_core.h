#pragma once

#include <Arduino.h>

namespace player_core {

enum class Mode { WAV, MP3, FLAC };

// Start playback of a file with the requested mode/decoder.
bool start(const char* path, Mode mode, float volumePct);

// Pump decoding; call frequently.
void loop();

// Stop playback and free resources.
void stop();

// Approximate progress percent 0..100.
int bufferPercent();

// True if pipeline active.
bool isRunning();

// True if finished.
bool finished();

// True if failure occurred.
bool failed();

}  // namespace player_core
