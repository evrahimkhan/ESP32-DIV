#include <ArduinoJson.h>
#include <Preferences.h>
#include <SD.h>
#include "SettingsStore.h"
#include "utils.h"


static AppSettings g_settings;
AppSettings& settings() { return g_settings; }

static const AccentOption kAccentPresets[] = {
  {"Orange", 0xFBE4},
  {"Green",  0xB721},
  {"Red",    0xF800},
  {"Cyan",   0x07FF},
  {"Purple", 0xF81F},
  {"Yellow", 0xFFE0},
  {"White",  0xFFFF},
};

uint8_t accentPresetClamp(uint8_t preset) {
  if (preset >= ACCENT_PRESET_COUNT) return 0;
  return preset;
}

uint16_t accentColor565(uint8_t preset) {
  return kAccentPresets[accentPresetClamp(preset)].color565;
}

const char* accentPresetName(uint8_t preset) {
  return kAccentPresets[accentPresetClamp(preset)].name;
}

const char* settingsBoardProfileId() {
  return TOUCH_PROFILE_ID;
}

void settingsApplyBoardTouchDefaults() {
  auto& s = g_settings;
  s.touchXMin = TOUCH_X_MIN;
  s.touchXMax = TOUCH_X_MAX;
  s.touchYMin = TOUCH_Y_MIN;
  s.touchYMax = TOUCH_Y_MAX;
}

/* Touch calibration in NVS, under a per-board namespace so flashing two boards
 * from the same sketch cannot swap each other's calibration. The SD card keeps
 * the full settings file; NVS exists so the calibration survives on a board
 * with no card in the slot, where settingsSave() cannot write at all. */
static const char* TOUCH_NVS_NAMESPACE = "divtouch";
static bool s_touchInNvs = false;

bool settingsTouchInNvs() { return s_touchInNvs; }

bool settingsLoadTouchFromNvs() {
  Preferences prefs;
  if (!prefs.begin(TOUCH_NVS_NAMESPACE, true)) {
    return false;
  }
  String board = prefs.getString("board", "");
  bool ok = (board == TOUCH_PROFILE_ID) && prefs.isKey("xMin") && prefs.isKey("yMax");
  if (ok) {
    auto& s = g_settings;
    s.touchXMin = prefs.getUShort("xMin", s.touchXMin);
    s.touchXMax = prefs.getUShort("xMax", s.touchXMax);
    s.touchYMin = prefs.getUShort("yMin", s.touchYMin);
    s.touchYMax = prefs.getUShort("yMax", s.touchYMax);
  }
  prefs.end();
  return ok;
}

bool settingsSaveTouchToNvs() {
  Preferences prefs;
  if (!prefs.begin(TOUCH_NVS_NAMESPACE, false)) {
    s_touchInNvs = false;
    return false;
  }
  auto& s = g_settings;
  prefs.putString("board", TOUCH_PROFILE_ID);
  prefs.putUShort("xMin", s.touchXMin);
  prefs.putUShort("xMax", s.touchXMax);
  prefs.putUShort("yMin", s.touchYMin);
  prefs.putUShort("yMax", s.touchYMax);
  prefs.end();
  s_touchInNvs = true;
  return true;
}

static bool settingsTouchSavedForBoard(const StaticJsonDocument<512>& doc) {
  JsonObjectConst touch = doc["touch"];
  if (touch.isNull()) {
    return false;
  }
  if (!touch["xMin"].is<uint16_t>() || !touch["xMax"].is<uint16_t>() ||
      !touch["yMin"].is<uint16_t>() || !touch["yMax"].is<uint16_t>()) {
    return false;
  }
  const char* savedBoard = doc["board"] | "";
  if (savedBoard[0] == '\0') {
    return true;
  }
  return strcmp(savedBoard, TOUCH_PROFILE_ID) == 0;
}

static bool sd_mounted = false;
static bool mountSD() {

  if (sd_mounted) {
    if (SD.cardType() != CARD_NONE) return true;
    sd_mounted = false;
  }

  sd_mounted = isSDCardAvailable();
  return sd_mounted;
}

static bool ensureDir(const char* dirPath) {
  if (!mountSD()) return false;
  if (!SD.exists(dirPath)) {
    if (SD.mkdir(dirPath)) return true;

    if (dirPath && dirPath[0] == '/') {
      return SD.mkdir(dirPath + 1);
    }
    return false;
  }
  return true;
}

/** Nothing on the SD card to load (or no card at all): NVS still has the
 *  calibration from the last Tools -> Touch Calibrate run. */
static void settingsLoadTouchFallback() {
  settingsApplyBoardTouchDefaults();
  settingsLoadTouchFromNvs();
}

bool settingsLoad() {
  settingsApplyBoardTouchDefaults();
  sdRetryMount();
  if (!mountSD()) {
    settingsLoadTouchFallback();
    return false;
  }
  if (!SD.exists(SETTINGS_PATH)) {
    settingsLoadTouchFallback();
    return true;
  }

  File f = SD.open(SETTINGS_PATH, FILE_READ);
  if (!f) return false;

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;

  auto& s = g_settings;
  s.brightness      = doc["brightness"]      | s.brightness;
  s.theme           = (Theme)(uint8_t)(doc["theme"] | (uint8_t)s.theme);
  s.accentColor     = accentPresetClamp(doc["accentColor"] | s.accentColor);
  s.neopixelEnabled = doc["neopixelEnabled"] | s.neopixelEnabled;

  s.autoWifiScan    = doc["autoWifiScan"]    | s.autoWifiScan;
  s.autoBleScan     = doc["autoBleScan"]     | s.autoBleScan;

  if (s.autoWifiScan != s.autoBleScan) {
    bool en = (s.autoWifiScan || s.autoBleScan);
    s.autoWifiScan = en;
    s.autoBleScan  = en;
  }

  if (settingsTouchSavedForBoard(doc)) {
    JsonObjectConst touch = doc["touch"];
    s.touchXMin = touch["xMin"] | s.touchXMin;
    s.touchXMax = touch["xMax"] | s.touchXMax;
    s.touchYMin = touch["yMin"] | s.touchYMin;
    s.touchYMax = touch["yMax"] | s.touchYMax;
  } else {
    settingsApplyBoardTouchDefaults();
    settingsLoadTouchFromNvs();
  }

  return true;
}


bool settingsSave() {
  // NVS always, so a calibration survives on a board with no card. The SD copy
  // is the fuller record (brightness, theme, accent), so try it too and report
  // success when either one kept the settings.
  const bool nvsOk = settingsSaveTouchToNvs();

  sdRetryMount();

  if (!ensureDir("/config")) {
    sd_mounted = false;
    if (!ensureDir("/config")) return nvsOk;
  }

  File f = SD.open(SETTINGS_PATH, FILE_WRITE);
  if (!f) {
    sd_mounted = false;
    if (!mountSD()) return nvsOk;
    f = SD.open(SETTINGS_PATH, FILE_WRITE);
    if (!f) return nvsOk;
  }

  auto& s = g_settings;
  StaticJsonDocument<512> doc;
  doc["board"]           = TOUCH_PROFILE_ID;
  doc["brightness"]      = s.brightness;
  doc["theme"]           = (uint8_t)s.theme;
  doc["accentColor"]     = s.accentColor;
  doc["neopixelEnabled"] = s.neopixelEnabled;

  doc["autoWifiScan"]    = s.autoWifiScan;
  doc["autoBleScan"]     = s.autoBleScan;

  JsonObject t = doc.createNestedObject("touch");
  t["xMin"] = s.touchXMin;
  t["xMax"] = s.touchXMax;
  t["yMin"] = s.touchYMin;
  t["yMax"] = s.touchYMax;

  bool ok = serializeJson(doc, f) > 0;
  f.close();
  return ok;
}
