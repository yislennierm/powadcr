#include <Arduino.h>
#include "config.h"
#include <WiFi.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "apps/radio.h"
#include "apps/storage.h"
#include "apps/stations.h"
#include "apps/prefs.h"
#include <OneButton.h>
#if USE_TDISPLAY
  #include "ui/widgets.h"
  #include <TFT_eSPI.h>
  TFT_eSPI tft = TFT_eSPI();
#endif

static String gRadioUrl = "http://allrelays.rainwave.cc/covers.mp3";
static String gRadioName = "Default";
static float gMainVolPct = 90.0f;
static bool gWifiEnabled = true;
bool wifiReady = false;
bool playerReady = false;

enum class AppId { HOME, RADIO, SETTINGS };
static AppId currentApp = AppId::RADIO;
static AppId lastApp = AppId::RADIO;

// Settings navigation
enum class SettingsView { LIST, WIFI, BLUETOOTH };
static SettingsView settingsView = SettingsView::LIST;
static int settingsSelection = 0;
constexpr const char* SETTINGS_ITEMS[] = {"WiFi", "Bluetooth"};
constexpr int SETTINGS_COUNT = sizeof(SETTINGS_ITEMS) / sizeof(SETTINGS_ITEMS[0]);
static bool settingsDirty = true;
static bool settingsDetailDirty = true;
static SettingsView prevSettingsView = SettingsView::LIST;
static int prevSettingsSelection = -1;
static bool prevWifiConnected = false;
static String prevWifiIp;
static String prevWifiGw;
static String prevWifiDns;

// Volume buttons (GPIOs 36 and 37 are input-only; use external pull-ups)
constexpr int PIN_VOL_UP = 36;
constexpr int PIN_VOL_DOWN = 37;
OneButton btnVolUp(PIN_VOL_UP, /*activeLow=*/true, /*pullupActive=*/false);
OneButton btnVolDown(PIN_VOL_DOWN, /*activeLow=*/true, /*pullupActive=*/false);

void adjustVolume(float delta) {
  gMainVolPct = constrain(gMainVolPct + delta, 0.0f, 100.0f);
  radio::setVolume(gMainVolPct);
  prefs::setVolumePct(gMainVolPct);
  Serial.printf("Volume set to %.1f%%\n", gMainVolPct);
}

void onVolUp() {
  if (currentApp == AppId::SETTINGS) {
    settingsSelection = (settingsSelection + 1) % SETTINGS_COUNT;
    settingsDirty = true;
  } else {
    adjustVolume(+5.0f);
  }
}
void onVolDown() {
  if (currentApp == AppId::SETTINGS) {
    settingsSelection = (settingsSelection - 1 + SETTINGS_COUNT) % SETTINGS_COUNT;
    settingsDirty = true;
  } else {
    adjustVolume(-5.0f);
  }
}
void onVolUpLong() {
  // Cycle apps
  if (currentApp == AppId::RADIO) currentApp = AppId::HOME;
  else if (currentApp == AppId::HOME) currentApp = AppId::SETTINGS;
  else currentApp = AppId::RADIO;
  const char* appName = (currentApp == AppId::RADIO) ? "RADIO" : (currentApp == AppId::HOME) ? "HOME" : "SETTINGS";
  Serial.printf("Switched app to %s\n", appName);
#if USE_TDISPLAY
  ui::widgets::drawStatusLine(tft, String("App: ") + appName);
#endif
}
void onVolDownLong() {
  if (currentApp == AppId::SETTINGS) {
    // Enter or exit detail view
    if (settingsView == SettingsView::LIST) {
      settingsView = (settingsSelection == 0) ? SettingsView::WIFI : SettingsView::BLUETOOTH;
    } else {
      settingsView = SettingsView::LIST;
    }
    settingsDirty = true;
    settingsDetailDirty = true;
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
    playerReady = true;
    Serial.println("Audio pipeline ready");
#if USE_TDISPLAY
    ui::widgets::drawStatusLine(tft, "Playing " + gRadioName);
#endif
  } else {
    playerReady = false;
#if USE_TDISPLAY
    ui::widgets::drawStatusLine(tft, "Radio init fail");
#endif
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 2000) { vTaskDelay(pdMS_TO_TICKS(10)); }
  Serial.println("AudioTools radio test");

  // Load persisted prefs
  prefs::init();
  gWifiEnabled = prefs::wifiEnabled();
  gMainVolPct = prefs::volumePct();

  // Set default landing app from config
#if DEFAULT_SCREEN == 0
  currentApp = AppId::HOME;
#elif DEFAULT_SCREEN == 1
  currentApp = AppId::RADIO;
#elif DEFAULT_SCREEN == 2
  currentApp = AppId::SETTINGS;
#else
  currentApp = AppId::RADIO;
#endif
  lastApp = currentApp;

#if USE_TDISPLAY
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
  tft.println("ESP-CORDER");
#endif

  // Mount SD and load stations (optional)
  if (storage::mountSd()) {
    if (stations::loadFromFile()) {
      const auto* st = stations::get(0);
      if (st) {
        gRadioUrl = st->url;
        gRadioName = st->name;
      }
    }
  }

  connectWiFi();
  if (wifiReady) {
    startAudio();
  }

  // Buttons
  btnVolUp.attachClick(onVolUp);
  btnVolDown.attachClick(onVolDown);
  btnVolUp.attachLongPressStart(onVolUpLong);
  btnVolDown.attachLongPressStart(onVolDownLong);

  // Persist initial volume setting
  prefs::setVolumePct(gMainVolPct);
}

void loop() {
  btnVolUp.tick();
  btnVolDown.tick();

  if (!wifiReady || !playerReady) {
    vTaskDelay(pdMS_TO_TICKS(500));
    return;
  }

  // App state handling
  bool appChanged = (currentApp != lastApp);
  lastApp = currentApp;
  static unsigned long lastHomeRefresh = 0;
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
static SettingsView prevSettingsView = SettingsView::LIST;
static int prevSettingsSelection = -1;

  switch (currentApp) {
    case AppId::RADIO:
      radio::loop();
    #if USE_TDISPLAY
      {
        int pct = radio::bufferPercent();
        ui::widgets::drawBufferBar(tft, pct);
        if (WiFi.status() == WL_CONNECTED) {
          ui::widgets::drawWifiBars(tft, WiFi.RSSI());
          int volPct = constrain(int(gMainVolPct), 0, 100);
          ui::widgets::drawVolumeArc(tft, volPct);
        }
      }
    #endif
      break;
    case AppId::HOME:
    {
      // Simple idle screen; keep audio running in background if desired
      radio::loop(); // keep pumping audio even on HOME
    #if USE_TDISPLAY
      if (appChanged) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.setTextSize(2);
        tft.println("HOME");
        tft.setTextSize(1);
        lastHomeRefresh = 0;
        prevStationName = "";
        prevBuf = prevVol = -1;
        prevWifi = false;
        prevSsid = prevIp = "";
        prevRssi = 0;
        prevSdMounted = false;
        prevUsed = prevTotal = 0;
        prevStationCount = 0;
      }
      unsigned long now = millis();
      if (now - lastHomeRefresh > 800) { // slower updates to reduce flicker
        lastHomeRefresh = now;
        auto clearLine = [&](int y){ tft.fillRect(0, y, tft.width(), 10, TFT_BLACK); };
        int y = 18;
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);

        if (gRadioName != prevStationName) {
          clearLine(y);
          tft.setCursor(0, y); tft.printf("Station: %s", gRadioName.c_str());
          prevStationName = gRadioName;
        }
        y += 10;

        int bufNow = radio::bufferPercent();
        int volNow = int(gMainVolPct + 0.5f);
        if (bufNow != prevBuf || volNow != prevVol) {
          clearLine(y);
          tft.setCursor(0, y); tft.printf("Vol: %d%% Buf: %d%%", volNow, bufNow);
          prevBuf = bufNow; prevVol = volNow;
        }
        y += 10;

        bool wifiNow = (WiFi.status() == WL_CONNECTED);
        String ssidNow = wifiNow ? WiFi.SSID() : "";
        String ipNow = wifiNow ? WiFi.localIP().toString() : "";
        int rssiNow = wifiNow ? WiFi.RSSI() : 0;
        if (wifiNow != prevWifi || ssidNow != prevSsid) {
          clearLine(y);
          tft.setCursor(0, y); tft.printf("WiFi: %s", wifiNow ? ssidNow.c_str() : "disconnected");
          prevWifi = wifiNow; prevSsid = ssidNow;
        }
        y += 10;
        if (wifiNow && (ipNow != prevIp)) {
          clearLine(y);
          tft.setCursor(0, y); tft.printf("IP: %s", ipNow.c_str());
          prevIp = ipNow;
        }
        y += 10;
        if (wifiNow && (rssiNow != prevRssi)) {
          clearLine(y);
          tft.setCursor(0, y); tft.printf("RSSI: %d dBm", rssiNow);
          prevRssi = rssiNow;
        }
        y += 10;

        uint64_t used=0, total=0;
        bool sdOk = storage::getSpace(used, total);
        if (sdOk != prevSdMounted || used != prevUsed || total != prevTotal) {
          clearLine(y);
          if (sdOk) {
            float usedMB = used / (1024.0f * 1024.0f);
            float totalMB = total / (1024.0f * 1024.0f);
            tft.setCursor(0, y); tft.printf("SD: %.1f/%.1f MB", usedMB, totalMB);
          } else {
            tft.setCursor(0, y); tft.print("SD: not mounted");
          }
          prevSdMounted = sdOk; prevUsed = used; prevTotal = total;
        }
        y += 10;

        size_t countNow = stations::count();
        if (countNow != prevStationCount) {
          clearLine(y);
          tft.setCursor(0, y); tft.printf("Stations: %u", (unsigned)countNow);
          prevStationCount = countNow;
        }
        y += 10;

        clearLine(y);
        tft.setCursor(0, y); tft.print("Hold + to switch");
      }
    #endif
      break;
    }
    case AppId::SETTINGS:
      radio::loop(); // keep audio flowing
    #if USE_TDISPLAY
      static int prevSettingsVol = -1;
      if (appChanged) {
        settingsView = SettingsView::LIST;
        settingsSelection = 0;
        settingsDirty = true;
        settingsDetailDirty = true;
        prevSettingsVol = -1;
      }

      // Redraw header and reset when view/selection changes
      if (settingsDirty || settingsView != prevSettingsView) {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0);
        tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        tft.setTextSize(2);
        tft.println("SETTINGS");
        tft.setTextSize(1);
        settingsDirty = false;
        settingsDetailDirty = true;
        prevSettingsView = settingsView;
      }

      // Render based on view
      if (settingsView == SettingsView::LIST) {
        if (settingsSelection != prevSettingsSelection || settingsDirty) {
          int y = 18;
          for (int i = 0; i < SETTINGS_COUNT; ++i) {
            uint16_t bg = (i == settingsSelection) ? TFT_DARKGREY : TFT_BLACK;
            uint16_t fg = (i == settingsSelection) ? TFT_WHITE : TFT_LIGHTGREY;
            tft.fillRect(0, y, tft.width(), 12, bg);
            // Draw small circle indicator on selected row
            if (i == settingsSelection) {
              tft.fillCircle(6, y + 5, 3, fg); // nudge up slightly
            }
            tft.setCursor(12, y + 2);
            tft.setTextColor(fg, bg);
            tft.print(SETTINGS_ITEMS[i]);
            y += 12;
          }
          // Footer hint
          tft.fillRect(0, 18 + SETTINGS_COUNT * 12, tft.width(), 12, TFT_BLACK);
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          tft.setCursor(0, 18 + SETTINGS_COUNT * 12 + 2);
          tft.print("Long - to enter / exit");
          prevSettingsSelection = settingsSelection;
          settingsDetailDirty = true; // entering selection redraw will need detail clear if switching to detail
        }
      } else {
        // Detail screens
        // Detect changes in WiFi info to refresh detail
        bool wifiConnNow = (WiFi.status() == WL_CONNECTED);
        String ipNow = WiFi.localIP().toString();
        String gwNow = WiFi.gatewayIP().toString();
        String dnsNow = WiFi.dnsIP().toString();
        if (wifiConnNow != prevWifiConnected || ipNow != prevWifiIp || gwNow != prevWifiGw || dnsNow != prevWifiDns) {
          settingsDetailDirty = true;
          prevWifiConnected = wifiConnNow;
          prevWifiIp = ipNow;
          prevWifiGw = gwNow;
          prevWifiDns = dnsNow;
        }

        if (settingsDetailDirty) {
          tft.fillRect(0, 18, tft.width(), tft.height() - 18, TFT_BLACK);
          int y = 18;
          tft.setTextColor(TFT_WHITE, TFT_BLACK);
          if (settingsView == SettingsView::WIFI) {
            // Header line: label left, toggle right
            tft.fillRect(0, y, tft.width(), 16, TFT_BLACK);
            tft.setCursor(0, y + 2); tft.print("WiFi");
            int toggleX = tft.width() - 40;
            ui::widgets::drawToggle(tft, toggleX, y, gWifiEnabled, true);
            y += 22; // extra spacing below header
            if (gWifiEnabled && WiFi.status() == WL_CONNECTED) {
              // IP line with DHCP/manual toggle aligned to the right
              tft.fillRect(0, y, tft.width(), 16, TFT_BLACK);
              tft.setCursor(0, y + 2); tft.print("IP: "); tft.print(WiFi.localIP().toString());
              int ipToggleX = tft.width() - 40;
              ui::widgets::drawToggle(tft, ipToggleX, y, true /*DHCP placeholder*/, true);
              y += 20;
              tft.fillRect(0, y, tft.width(), 16, TFT_BLACK);
              tft.setCursor(0, y + 2); tft.print("GW: "); tft.print(WiFi.gatewayIP().toString()); 
              y += 20;
              tft.fillRect(0, y, tft.width(), 16, TFT_BLACK);
              tft.setCursor(0, y + 2); tft.print("DNS: "); tft.print(WiFi.dnsIP().toString()); 
              y += 20;
            } else if (gWifiEnabled) {
              tft.setCursor(0, y); tft.print("WiFi enabled, not connected"); y += 14;
            } else {
              tft.setCursor(0, y); tft.print("WiFi disabled"); y += 14;
            }
            y += 6;
            tft.setCursor(0, y); tft.print("(Stub) Add edit/connect here"); y += 12;
          } else if (settingsView == SettingsView::BLUETOOTH) {
            tft.setCursor(0, y); tft.print("Bluetooth Settings"); y += 12;
            tft.setCursor(0, y); tft.print("(Stub) add BT options"); y += 12;
          }
          tft.setCursor(0, tft.height() - 24); tft.print("Long - to go back");
          settingsDetailDirty = false;
        }
      }

      // Show current volume line at bottom of settings
      int volNow = int(gMainVolPct + 0.5f);
      if (volNow != prevSettingsVol) {
        prevSettingsVol = volNow;
        tft.fillRect(0, tft.height() - 12, tft.width(), 12, TFT_BLACK);
        tft.setCursor(0, tft.height() - 12);
        tft.setTextColor(TFT_CYAN, TFT_BLACK);
        tft.printf("Volume: %d%%", volNow);
      }
    #endif
      break;
  }
}
