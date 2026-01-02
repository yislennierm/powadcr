#include <Arduino.h>
#include "config.h"
#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "apps/radio.h"
#include "apps/storage.h"
#include "apps/stations.h"
#include "apps/prefs.h"
#include "apps/home_app.h"
#include "apps/radio_app.h"
#include "apps/settings_app.h"
#include "apps/player_app.h"
#include "apps/calib_app.h"
#include "apps/player_core.h"
#include "apps/explorer_app.h"
#include "ui/joystick.h"
#include <SD.h>
#include <OneButton.h>
#if USE_TDISPLAY
  #include "ui/widgets.h"
  #include <TFT_eSPI.h>
  TFT_eSPI tft = TFT_eSPI();
  TFT_eSprite background = TFT_eSprite(&tft);
#endif

static String gRadioUrl = "http://allrelays.rainwave.cc/covers.mp3";
static String gRadioName = "Default";
static float gMainVolPct = 50.0f;
static bool gWifiEnabled = true;
bool wifiReady = false;
static int currentStationIndex = 0;
static bool playerPaused = false;
static int playerMode = 0; // 0=WAV,1=TAP,2=MP3,3=FLAC,4=RADIO
static bool playerExplorer = false;
static int playerSelected = 0;
static int playerFileCount = 0;
static String playerEntries[8];
static String playerPaths[8];
static String playerStatus;
static bool playerViewForce = false;
static String pendingPlayPath;
static String pendingPlayName;
static bool playingRadio = false;
static String currentRadioUrl;

enum class AppId { HOME, RADIO, SETTINGS, PLAYER, EXPLORER, CALIB };
static AppId currentApp = AppId::RADIO;
static AppId lastApp = AppId::RADIO;
static bool firstLoop = true;
static TaskHandle_t audioTaskHandle = nullptr;

// Settings navigation
// Player app placeholder state
// none yet

#if HAVE_JOYSTICK
// Joystick switch (enter) and analog axes
OneButton btnEnter(JOY_SW_PIN, /*activeLow=*/true, /*pullupActive=*/false);
ui::SimpleJoystick joy(JOY_X_PIN, JOY_Y_PIN, JOY_DEADZONE);
#else
// Volume buttons (GPIOs 36 and 37 are input-only; use external pull-ups)
constexpr int PIN_VOL_UP = 36;
constexpr int PIN_VOL_DOWN = 37;
// These pins have no internal pull-ups; ensure hardware pull-ups are present.
OneButton btnVolUp(PIN_VOL_UP, /*activeLow=*/true, /*pullupActive=*/false);
OneButton btnVolDown(PIN_VOL_DOWN, /*activeLow=*/true, /*pullupActive=*/false);
#endif

// Forward decl
void startAudio();
int loadExplorerFiles();

void adjustVolume(float delta) {
  gMainVolPct = constrain(gMainVolPct + delta, 0.0f, 100.0f);
  radio::setVolume(gMainVolPct);
  prefs::setVolumePct(gMainVolPct);
  Serial.printf("Volume set to %.1f%%\n", gMainVolPct);
}

// Station navigation
void selectStation(int delta) {
  size_t total = stations::count();
  if (total == 0) return;
  int attempts = total;
  while (attempts-- > 0) {
    currentStationIndex = (currentStationIndex + delta + total) % total;
    const auto* st = stations::get(currentStationIndex);
    if (!st) continue;
    gRadioUrl = st->url;
    gRadioName = st->name;
    startAudio();
    if (radio::isReady()) {
#if USE_TDISPLAY
      ui::widgets::drawStatusLine(tft, "Playing " + gRadioName);
#endif
      break;
    } else {
      Serial.printf("Station failed: %s -> %s, trying next\n", st->name.c_str(), st->url.c_str());
    }
  }
}

void onVolUp() {
  if (currentApp == AppId::SETTINGS) {
    app_settings::selectNext();
  } else {
    adjustVolume(+5.0f);
  }
  Serial.println("[BTN] Up click");
}
void onVolDown() {
  if (currentApp == AppId::SETTINGS) {
    app_settings::selectPrev();
  } else {
    adjustVolume(-5.0f);
  }
  Serial.println("[BTN] Down click");
}
void onVolUpLong() {
  // Cycle apps
  if (currentApp == AppId::RADIO) currentApp = AppId::HOME;
  else if (currentApp == AppId::HOME) currentApp = AppId::SETTINGS;
  else if (currentApp == AppId::SETTINGS) currentApp = AppId::PLAYER;
  else if (currentApp == AppId::PLAYER) currentApp = AppId::CALIB;
  else currentApp = AppId::RADIO;
  const char* appName = (currentApp == AppId::RADIO) ? "RADIO" :
                        (currentApp == AppId::HOME) ? "HOME" :
                        (currentApp == AppId::SETTINGS) ? "SETTINGS" :
                        (currentApp == AppId::PLAYER) ? "PLAYER" : "CALIB";
  Serial.printf("Switched app to %s\n", appName);
#if USE_TDISPLAY
  ui::widgets::drawStatusLine(tft, String("App: ") + appName);
#endif
  Serial.println("[BTN] Up long");
}
void onVolDownLong() {
  if (currentApp == AppId::SETTINGS) {
    app_settings::toggleDetail();
  }
  Serial.println("[BTN] Down long");
}

void onVolUpDouble() {
  Serial.println("[BTN] Up double");
  selectStation(+1);
}

void onVolDownDouble() {
  Serial.println("[BTN] Down double");
  selectStation(-1);
}

// Cycle apps helper
void cycleApp() {
  if (currentApp == AppId::RADIO) currentApp = AppId::HOME;
  else if (currentApp == AppId::HOME) currentApp = AppId::SETTINGS;
  else if (currentApp == AppId::SETTINGS) currentApp = AppId::PLAYER;
  else if (currentApp == AppId::PLAYER) currentApp = AppId::CALIB;
  else if (currentApp == AppId::EXPLORER) currentApp = AppId::PLAYER; // explorer exits back to player on long
  else currentApp = AppId::RADIO;
  const char* appName = (currentApp == AppId::RADIO) ? "RADIO" :
                        (currentApp == AppId::HOME) ? "HOME" :
                        (currentApp == AppId::SETTINGS) ? "SETTINGS" :
                        (currentApp == AppId::PLAYER) ? "PLAYER" :
                        (currentApp == AppId::EXPLORER) ? "EXPLORER" : "CALIB";
  Serial.printf("Switched app to %s\n", appName);
#if USE_TDISPLAY
  ui::widgets::drawStatusLine(tft, String("App: ") + appName);
#endif
}

// Enter button handlers (currently log-only except explorer selection)
void onEnterClick() {
  Serial.println("[INPUT] ENTER_CLICK");
  if (currentApp == AppId::CALIB) {
    // Toggle calibration start/stop handled in loop state
  } else if (currentApp == AppId::EXPLORER) {
    if (playerFileCount > 0) {
      if (playerMode == 4) {
        // RADIO: set pending stream for player
        pendingPlayPath = playerPaths[playerSelected];
        pendingPlayName = playerEntries[playerSelected];
        currentApp = AppId::PLAYER;
      } else {
        pendingPlayPath = playerPaths[playerSelected];
        pendingPlayName = playerEntries[playerSelected];
        playerStatus = "Loading " + pendingPlayName;
        currentApp = AppId::PLAYER;
      }
    }
  } else if (currentApp == AppId::PLAYER) {
    // reserved for future; no default action
  } else {
    // no action
  }
}

void onEnterDouble() {
  Serial.println("[INPUT] ENTER_DOUBLE");
  if (currentApp == AppId::CALIB) {
    // no-op for now
  } else if (currentApp == AppId::PLAYER) {
    // Enter explorer app
    loadExplorerFiles();
    playerStatus = "Explorer";
    currentApp = AppId::EXPLORER;
  } else if (currentApp == AppId::EXPLORER) {
    // Double-click exits explorer back to player
    currentApp = AppId::PLAYER;
  } else {
    onVolDownDouble();
  }
}

void onEnterLong() {
  Serial.println("[INPUT] ENTER_LONG");
  if (currentApp == AppId::CALIB) {
    // Start/stop calibration handled in loop state
  } else if (currentApp == AppId::EXPLORER) {
    // long press cycles apps starting from player
    currentApp = AppId::PLAYER;
  } else if (currentApp == AppId::PLAYER) {
    // Cycle apps
    cycleApp();
  } else {
    // Cycle apps from any other screen
    cycleApp();
  }
}

void onJoyDir(ui::SimpleJoystick::Dir dir) {
  // Global long-hold volume control on up/down
  static ui::SimpleJoystick::Dir lastDir = ui::SimpleJoystick::Dir::Center;
  static unsigned long dirStart = 0;
  static unsigned long lastVolAdj = 0;
  unsigned long now = millis();
  if (dir != lastDir) {
    lastDir = dir;
    dirStart = now;
  }
  if ((dir == ui::SimpleJoystick::Dir::Up || dir == ui::SimpleJoystick::Dir::Down) &&
      (now - dirStart) >= 700 && (now - lastVolAdj) >= 300) {
    float delta = (dir == ui::SimpleJoystick::Dir::Up) ? +2.0f : -2.0f;
    adjustVolume(delta);
    lastVolAdj = now;
  }

  if (currentApp == AppId::EXPLORER) {
    switch (dir) {
      case ui::SimpleJoystick::Dir::Up:
        if (playerFileCount > 0) playerSelected = (playerSelected - 1 + playerFileCount) % playerFileCount;
        break;
      case ui::SimpleJoystick::Dir::Down:
        if (playerFileCount > 0) playerSelected = (playerSelected + 1) % playerFileCount;
        break;
      case ui::SimpleJoystick::Dir::Left:
        playerMode = (playerMode - 1 + 5) % 5;
        loadExplorerFiles();
        break;
      case ui::SimpleJoystick::Dir::Right:
        playerMode = (playerMode + 1) % 5;
        loadExplorerFiles();
        break;
      default:
        break;
    }
    return;
  }
  // Outside explorer: just log inputs for now, no actions
  switch (dir) {
    case ui::SimpleJoystick::Dir::Up:    Serial.println("[INPUT] JOY_UP"); break;
    case ui::SimpleJoystick::Dir::Down:  Serial.println("[INPUT] JOY_DOWN"); break;
    case ui::SimpleJoystick::Dir::Left:  Serial.println("[INPUT] JOY_LEFT"); break;
    case ui::SimpleJoystick::Dir::Right: Serial.println("[INPUT] JOY_RIGHT"); break;
    default: break;
  }
}
#if HAVE_JOYSTICK
// Map enter button; in CALIB it starts calibration, else behaves as "enter/down"
void onEnterClick();
void onEnterDouble();
void onEnterLong();
void onJoyDir(ui::SimpleJoystick::Dir dir);
int loadExplorerFiles();

static const char* kModeNames[] = {"WAV","TAP","MP3","FLAC","RADIO"};
static const char* kModePaths[] = {"/WAV","/TAP","/MP3","/FLAC","/RADIO"};

int loadExplorerFiles() {
  playerFileCount = 0;
  playerSelected = 0;
  const char* base = kModePaths[playerMode];
  if (playerMode == 4) { // RADIO stations
    size_t count = stations::count();
    for (size_t i = 0; i < count && i < 8; ++i) {
      const auto* st = stations::get(i);
      if (!st) continue;
      playerEntries[playerFileCount] = st->name;
      playerPaths[playerFileCount] = st->url;
      playerFileCount++;
    }
    playerStatus = (playerFileCount == 0) ? "No stations" : "Select station";
    return playerFileCount;
  }

  File dir = SD.open(base);
  if (!dir || !dir.isDirectory()) {
    playerStatus = "No dir";
    return 0;
  }
  while (playerFileCount < 8) {
    File f = dir.openNextFile();
    if (!f) break;
    if (f.isDirectory()) {
      f.close();
      continue;
    }
    String name = f.name();
    f.close();
    String lower = name; lower.toLowerCase();
    if (playerMode == 0) { if (!lower.endsWith(".wav")) continue; }
    else if (playerMode == 1) { if (!lower.endsWith(".tap")) continue; }
    else if (playerMode == 2) { if (!lower.endsWith(".mp3")) continue; }
    else if (playerMode == 3) { if (!lower.endsWith(".flac")) continue; }
    playerEntries[playerFileCount] = name;
    playerPaths[playerFileCount] = String(base) + "/" + name;
    playerFileCount++;
  }
  if (playerFileCount == 0) {
    playerStatus = "No files";
  } else {
    playerStatus = "Select file";
  }
  return playerFileCount;
}
#endif

// Audio task pinned to core 0 to keep playback responsive
void audioTask(void* param) {
  while (true) {
    AppId app = currentApp;
      if (playingRadio) {
        radio::loop();
      } else if (app == AppId::PLAYER) {
        player_core::loop();
      } else if (app == AppId::RADIO || app == AppId::HOME || app == AppId::SETTINGS) {
        radio::loop();
      }
    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

void connectWiFi() {
  if (!gWifiEnabled) {
    wifiReady = false;
#if USE_TDISPLAY
    ui::widgets::drawStatusLine(tft, "WiFi disabled");
#endif
    return;
  }
  const char* ssid = WIFI_SSID_DEFAULT;
  const char* pass = WIFI_PASS_DEFAULT;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
#if USE_TDISPLAY
  ui::widgets::drawStatusLine(tft, String("WiFi... ") + ssid);
#endif
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    vTaskDelay(pdMS_TO_TICKS(200));
    Serial.print(".");
  }
  Serial.println();
  wifiReady = (WiFi.status() == WL_CONNECTED);
  Serial.printf("WiFi %s, IP: %s\n", wifiReady ? "OK" : "FAIL", WiFi.localIP().toString().c_str());
#if USE_TDISPLAY
  ui::widgets::drawStatusLine(tft, wifiReady ? "WiFi OK " + WiFi.localIP().toString() : "WiFi FAIL");
#endif
}

void startAudio() {
  if (radio::init(gMainVolPct, gRadioUrl.c_str())) {
    Serial.println("Audio pipeline ready");
#if USE_TDISPLAY
    ui::widgets::drawStatusLine(tft, "Playing " + gRadioName);
#endif
  } else {
#if USE_TDISPLAY
    ui::widgets::drawStatusLine(tft, "Radio init fail");
#endif
  }
}

static bool appNeedsWiFi(AppId app) {
  if (!gWifiEnabled) return false;
  if (app == AppId::RADIO || app == AppId::HOME || app == AppId::SETTINGS) return true;
  if (app == AppId::PLAYER && playerMode == 4) return true; // radio inside player
  return false;
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) { vTaskDelay(pdMS_TO_TICKS(10)); }
  Serial.println("AudioTools radio test");

  // Load persisted prefs
  prefs::init();
  gWifiEnabled = prefs::wifiEnabled();
  #if FORCE_DEFAULT_VOLUME_ON_BOOT
    gMainVolPct = DEFAULT_VOLUME_PCT;
    prefs::setVolumePct(gMainVolPct);
  #else
    gMainVolPct = prefs::volumePct();
  #endif
  Serial.printf("Boot volume set to %.1f%%\n", gMainVolPct);

  // Set default landing app from config
#if DEFAULT_SCREEN == 0
  currentApp = AppId::HOME;
#elif DEFAULT_SCREEN == 1
  currentApp = AppId::RADIO;
#elif DEFAULT_SCREEN == 2
  currentApp = AppId::SETTINGS;
#elif DEFAULT_SCREEN == 3
  currentApp = AppId::PLAYER;
#elif DEFAULT_SCREEN == 4
  currentApp = AppId::CALIB;
#else
  currentApp = AppId::RADIO;
#endif
  lastApp = currentApp;

#if USE_TDISPLAY
  tft.init();
  tft.setRotation(1); // landscape
  background.createSprite(tft.width(), tft.height());
  background.fillSprite(TFT_BLACK);
  background.setTextColor(TFT_GREEN, TFT_BLACK);
  background.setTextSize(2);
  background.setCursor(0, 0);
  background.println("ESP-CORDER");
  background.pushSprite(0, 0);
#endif

  // Mount SD and load stations (optional)
  if (storage::mountSd()) {
    if (stations::loadFromFile()) {
      const auto* st = stations::get(0);
      if (st) {
        gRadioUrl = st->url;
        gRadioName = st->name;
        currentStationIndex = 0;
      }
    }
  }

  // Only bring up WiFi if landing app requires it
  if (appNeedsWiFi(currentApp)) {
    connectWiFi();
  }

#if HAVE_JOYSTICK
  // Buttons
  btnEnter.attachClick(onEnterClick);
  btnEnter.attachLongPressStart(onEnterLong);
  btnEnter.attachDoubleClick(onEnterDouble);
  btnEnter.setDebounceMs(BUTTON_DEBOUNCE_MS);
  btnEnter.setClickMs(BUTTON_CLICK_MS);
  btnEnter.setPressMs(BUTTON_PRESS_MS);
  joy.setRepeatMs(JOY_REPEAT_MS);
  joy.onDirection(onJoyDir);
  // Apply saved calibration if available
  int savedCx = prefs::joyCenterX();
  int savedCy = prefs::joyCenterY();
  int savedDz = prefs::joyDeadzone();
  if (savedCx >= 0 && savedCy >= 0) {
    joy.setCenter(savedCx, savedCy);
    joy.setDeadzone(savedDz);
  }
  joy.begin();
#else
  // Buttons
  btnVolUp.attachClick(onVolUp);
  btnVolDown.attachClick(onVolDown);
  btnVolUp.attachLongPressStart(onVolUpLong);
  btnVolDown.attachLongPressStart(onVolDownLong);
  btnVolUp.attachDoubleClick(onVolUpDouble);
  btnVolDown.attachDoubleClick(onVolDownDouble);
  // Tune timing for double/long press (ms)
  btnVolUp.setDebounceMs(BUTTON_DEBOUNCE_MS);
  btnVolDown.setDebounceMs(BUTTON_DEBOUNCE_MS);
  btnVolUp.setClickMs(BUTTON_CLICK_MS);   // max spacing between clicks
  btnVolDown.setClickMs(BUTTON_CLICK_MS);
  btnVolUp.setPressMs(BUTTON_PRESS_MS);   // long-press threshold
  btnVolDown.setPressMs(BUTTON_PRESS_MS);
#endif

  // Start dedicated audio task on core 0
  if (audioTaskHandle == nullptr) {
    xTaskCreatePinnedToCore(audioTask, "audioTask", TASK1_STACK_SIZE, nullptr, 2, &audioTaskHandle, 0);
  }

  // Persist initial volume setting
  prefs::setVolumePct(gMainVolPct);
}

void loop() {
#if HAVE_JOYSTICK
  btnEnter.tick();
  // Throttle joystick polling to lighten UI workload
  static unsigned long lastJoyPoll = 0;
  unsigned long nowPoll = millis();
  if ((currentApp == AppId::PLAYER || currentApp == AppId::CALIB || currentApp == AppId::EXPLORER) && (nowPoll - lastJoyPoll) > 30) {
    joy.tick(); // callback drives navigation only in the active app
    lastJoyPoll = nowPoll;
  }
#else
  btnVolUp.tick();
  btnVolDown.tick();
#endif

  // App state handling
  bool appChanged = firstLoop || (currentApp != lastApp);
  lastApp = currentApp;
  firstLoop = false;
  static String prevStationName;
  static int prevBuf = -1;
  static int prevVol = -1;
  static bool prevWifi = false;
  static String prevSsid;
  static String prevIp;
  static int prevRssi = 0;
  static bool prevSdMounted = false;
  static uint64_t prevUsed = 0, prevTotal = 0;
  static size_t prevStationCount = 0;

  // Bring up WiFi only when the active app needs it
  if (appChanged && appNeedsWiFi(currentApp) && !wifiReady) {
    connectWiFi();
  }

  switch (currentApp) {
    case AppId::RADIO:
    {
      bool radioActive = playingRadio || radio::isReady();
      if (appNeedsWiFi(currentApp) && !wifiReady) connectWiFi();
      if (radioActive && radio::failed()) {
        Serial.println("Radio stream failed, skipping to next station");
        selectStation(+1);
      }
    #if USE_TDISPLAY
      ui::view::radioViewUpdate(tft, radio::bufferPercent(), constrain(int(gMainVolPct),0,100), WiFi.RSSI(), gRadioName, appChanged);
    #endif
      break;
    }
    case AppId::HOME:
    {
      // Simple idle screen; keep audio running in background if desired
      bool radioActive = playingRadio || radio::isReady();
      if (appNeedsWiFi(currentApp) && !wifiReady) connectWiFi();
      if (radioActive && radio::failed()) {
        Serial.println("Radio stream failed, skipping to next station");
        selectStation(+1);
      }
    #if USE_TDISPLAY
      if (appChanged) {
        ui::view::HomeInfo info{};
        info.stationName = gRadioName;
        info.bufferPct = radio::bufferPercent();
        info.volPct = int(gMainVolPct + 0.5f);
        info.wifiConnected = (WiFi.status() == WL_CONNECTED);
        info.ssid = info.wifiConnected ? WiFi.SSID() : "";
        info.ip = info.wifiConnected ? WiFi.localIP().toString() : "";
        info.rssi = info.wifiConnected ? WiFi.RSSI() : 0;
        uint64_t used=0, total=0;
        info.sdMounted = storage::getSpace(used, total);
        info.sdUsedMB = used / (1024.0f * 1024.0f);
        info.sdTotalMB = total / (1024.0f * 1024.0f);
        info.stationCount = stations::count();
        ui::view::homeViewUpdate(tft, info, true);
      } else {
        static unsigned long lastHomeRefresh = 0;
        unsigned long now = millis();
        if (now - lastHomeRefresh > 800) {
          lastHomeRefresh = now;
          ui::view::HomeInfo info{};
          info.stationName = gRadioName;
          info.bufferPct = radio::bufferPercent();
          info.volPct = int(gMainVolPct + 0.5f);
          info.wifiConnected = (WiFi.status() == WL_CONNECTED);
          info.ssid = info.wifiConnected ? WiFi.SSID() : "";
          info.ip = info.wifiConnected ? WiFi.localIP().toString() : "";
          info.rssi = info.wifiConnected ? WiFi.RSSI() : 0;
          uint64_t used=0, total=0;
          info.sdMounted = storage::getSpace(used, total);
          info.sdUsedMB = used / (1024.0f * 1024.0f);
          info.sdTotalMB = total / (1024.0f * 1024.0f);
          info.stationCount = stations::count();
          ui::view::homeViewUpdate(tft, info);
        }
      }
    #endif
      break;
    }
    case AppId::SETTINGS:
      if (appNeedsWiFi(currentApp) && !wifiReady) connectWiFi();
      if ((playingRadio || radio::isReady()) && radio::failed()) {
        Serial.println("Radio stream failed, skipping to next station");
        selectStation(+1);
      }
    #if USE_TDISPLAY
      if (appChanged) app_settings::enter(true);
      app_settings::update(int(gMainVolPct + 0.5f), gWifiEnabled, WiFi.status() == WL_CONNECTED,
                           WiFi.SSID(), WiFi.localIP().toString(), WiFi.gatewayIP().toString(),
                           WiFi.dnsIP().toString(), appChanged);
    #endif
      break;
    case AppId::PLAYER:
      // Start requested content without stopping ongoing playback unless we have a new selection
      if (appChanged) {
        bool hasNewSelection = !pendingPlayPath.isEmpty();
        if (hasNewSelection) {
          // A new item was chosen (file or station) - restart pipeline accordingly
          radio::stop();
          player_core::stop();
          playingRadio = false;
          playerPaused = false;
          if (playerMode == 4) {
            if (!wifiReady || WiFi.status() != WL_CONNECTED) {
              connectWiFi();
            }
            gRadioUrl = pendingPlayPath;
            gRadioName = pendingPlayName;
            Serial.printf("Player: starting radio %s -> %s\n", gRadioName.c_str(), gRadioUrl.c_str());
            if (wifiReady && radio::init(gMainVolPct, gRadioUrl.c_str())) {
              playingRadio = true;
              playerStatus = "Playing " + gRadioName;
              playerPaused = false;
            } else {
              playingRadio = false;
              playerStatus = "Radio start failed";
              playerPaused = true;
              Serial.println("Player: radio start failed");
            }
          } else if (playerMode == 2) {
            Serial.printf("Player: starting mp3 %s\n", pendingPlayPath.c_str());
            player_core::start(pendingPlayPath.c_str(), player_core::Mode::MP3, gMainVolPct);
            playingRadio = false;
            playerStatus = "Playing " + pendingPlayName;
          } else if (playerMode == 3) {
            Serial.printf("Player: starting flac %s\n", pendingPlayPath.c_str());
            player_core::start(pendingPlayPath.c_str(), player_core::Mode::FLAC, gMainVolPct);
            playingRadio = false;
            playerStatus = "Playing " + pendingPlayName;
          } else {
            Serial.printf("Player: starting file %s\n", pendingPlayPath.c_str());
            player_core::start(pendingPlayPath.c_str(), player_core::Mode::WAV, gMainVolPct);
            playingRadio = false;
            playerStatus = "Playing " + pendingPlayName;
          }
          pendingPlayPath.clear();
          pendingPlayName.clear();
        } else {
          // No new selection: keep whatever is already playing; do not auto-start
      if (!playingRadio && !player_core::isRunning()) {
        playerPaused = true;
        playerStatus = "Select file/station";
      }
        }
      }
      if (!playerPaused && !playingRadio) {
        if (player_core::finished()) {
          playerPaused = true;
          playerStatus = "Finished";
        }
      }
      // If stream fails while in player, flag paused and status
      if (playerMode == 4 && playingRadio && radio::failed()) {
        playingRadio = false;
        playerPaused = true;
        playerStatus = "Radio failed";
      }
#if USE_TDISPLAY
      if (appChanged) app_player::enter(true);
      {
        ui::view::PlayerInfo p{};
        String modeLabel = kModeNames[playerMode];
        p.title = "PLAYER - " + modeLabel;
        p.bufferPct = playingRadio ? radio::bufferPercent() : player_core::bufferPercent();
        p.volPct = int(gMainVolPct + 0.5f);
        p.rssi = WiFi.RSSI();
        p.paused = playerPaused;
        p.status = playerStatus;
        app_player::update(p, appChanged || playerViewForce);
        playerViewForce = false;
      }
#endif
      break;
    case AppId::EXPLORER:
#if USE_TDISPLAY
    {
      if (appChanged) app_explorer::enter(true);
      app_explorer::ExplorerState st{};
      st.count = playerFileCount;
      st.selected = playerSelected;
      st.status = playerStatus;
      st.title = "EXPLORER";
      for (int i = 0; i < min(playerFileCount, 8); ++i) {
        st.entries[i] = playerEntries[i];
        st.paths[i] = playerPaths[i];
      }
      app_explorer::update(st, appChanged);
    }
#endif
      break;
    case AppId::CALIB:
#if HAVE_JOYSTICK && USE_TDISPLAY
    {
      static bool calibActive = false;
      static uint32_t calibStart = 0;
      static int minX = 4095, maxX = 0, minY = 4095, maxY = 0;
      static int centerX = 2048, centerY = 2048;
      static int runtimeDeadzone = JOY_DEADZONE;

      int rawX = analogRead(JOY_X_PIN);
      int rawY = analogRead(JOY_Y_PIN);
      int swRaw = analogRead(JOY_SW_PIN);
      bool swPressed = (swRaw < JOY_SW_THRESHOLD);

      // Toggle calibration on enter click
      if (appChanged) {
        calibActive = false;
        minX = minY = 4095;
        maxX = maxY = 0;
        // Load saved calibration (if present)
        int savedCx = prefs::joyCenterX();
        int savedCy = prefs::joyCenterY();
        int savedDz = prefs::joyDeadzone();
        if (savedCx >= 0 && savedCy >= 0) {
          centerX = savedCx;
          centerY = savedCy;
          runtimeDeadzone = savedDz;
        } else {
          runtimeDeadzone = JOY_DEADZONE;
          centerX = 2048;
          centerY = 2048;
        }
        joy.setDeadzone(runtimeDeadzone);
        joy.setCenter(centerX, centerY);
      }
      // Detect rising edge of enter click to start/stop calibration
      static bool prevSw = false;
      if (swPressed && !prevSw) {
        calibActive = !calibActive;
        if (calibActive) {
          calibStart = millis();
          minX = minY = 4095;
          maxX = maxY = 0;
        } else {
          // Finish calibration: compute center and deadzone
          centerX = (minX + maxX) / 2;
          centerY = (minY + maxY) / 2;
          int spanX = maxX - minX;
          int spanY = maxY - minY;
          int span = max(spanX, spanY);
          runtimeDeadzone = max(span / 6, 20); // ~16% of span, minimum 20
          joy.setCenter(centerX, centerY);
          joy.setDeadzone(runtimeDeadzone);
          prefs::setJoyCalibration(centerX, centerY, runtimeDeadzone);
        }
      }
      prevSw = swPressed;

      if (calibActive) {
        minX = min(minX, rawX);
        maxX = max(maxX, rawX);
        minY = min(minY, rawY);
        maxY = max(maxY, rawY);
      }

      app_calib::CalibData data{};
      data.rawX = rawX;
      data.rawY = rawY;
      data.dir = joy.sample();
      data.swRaw = swRaw;
      data.swPressed = swPressed;
      data.centerX = centerX;
      data.centerY = centerY;
      data.minX = minX;
      data.maxX = maxX;
      data.minY = minY;
      data.maxY = maxY;
      data.calibDeadzone = runtimeDeadzone;
      data.calibrating = calibActive;
      data.calibElapsedMs = calibActive ? (millis() - calibStart) : 0;
      app_calib::update(data, appChanged);
    }
#endif
      break;
  }
}
