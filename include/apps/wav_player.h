#pragma once

#include <Arduino.h>

namespace wav_player {

// Start playing a WAV file. If path is null, the first WAV in /WAV is used.
bool start(const char* path, float volumePct);

// Pump decoding; call frequently from loop().
void loop();

// Stop playback.
void stop();

// Progress percent 0..100 based on file position.
int bufferPercent();

// Current file name (may be empty if none).
String currentName();

// True if playback finished the file.
bool finished();

// True if a failure occurred.
bool failed();

// True if playback pipeline is active.
bool isRunning();

}  // namespace wav_player
