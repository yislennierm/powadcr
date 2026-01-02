#pragma once

#include <Arduino.h>

namespace prefs {

// Initialize Preferences storage.
bool init();

// WiFi enabled flag (persisted).
bool wifiEnabled();
void setWifiEnabled(bool enabled);

// Persisted master volume (0..100). Optional helpers.
float volumePct();
void setVolumePct(float pct);

// Joystick calibration
int joyCenterX();
int joyCenterY();
int joyDeadzone();
void setJoyCalibration(int cx, int cy, int dz);

}  // namespace prefs
