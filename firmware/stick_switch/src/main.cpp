#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_NAU7802.h>

#include <NimBLEDevice.h>
#include <BleKeyboard.h>
#include <esp_mac.h>

#include "config.h"
#include "hid_profile.h"
#include "pins.h"
#include "settings.h"
#include "stick_level.h"

namespace {

Preferences prefs;
char ble_name[20] = "Stick-Switch";
BleKeyboard ble(kBleName, kBleManufacturer, 100);
Adafruit_NAU7802 nau;
StickMachine machine;
StickSettings settings;
HidKeyset keys;

Debounce deb_soft(kDebounceMs);
Debounce deb_hard(kDebounceMs);
Debounce deb_aph(kDebounceMs);
Debounce deb_tare(kDebounceMs);
Debounce deb_cal(kDebounceMs);
Debounce deb_boot(kDebounceMs);
BoxcarFilter adc_filt(static_cast<float>(kBoxcar));
BoxcarFilter load_filt(static_cast<float>(kBoxcar));
BoxcarFilter grip_filt(4.0f);

bool nau_ok = false;
bool plot = false;
bool cal_collecting = false;
bool cal_is_hard = false;
float cal_acc[kCalSamples];
uint8_t cal_count = 0;
uint32_t last_adc_ms = 0;
uint32_t last_cal_sample_ms = 0;
uint32_t quiet_since_ms = 0;
bool analog_soft_held = false;
bool analog_hard_held = false;
char serial_buf[96];
uint8_t serial_len = 0;
int last_batt_mv = 0;
int last_batt_pct = -1;
int last_id_ohms = -1;
ModuleId last_module_id = ModuleId::Empty;
uint32_t last_batt_ms = 0;
bool last_ble_connected = false;
bool last_batt_low = false;
uint16_t last_grip_raw = 0;
bool last_grip = false;

bool sim_soft = false;
bool sim_hard = false;
uint32_t sim_until_ms = 0;

bool boot_was = false;
bool boot_held_fired = false;
uint8_t boot_taps = 0;
uint32_t boot_down_ms = 0;
uint32_t boot_wait_until = 0;

void handleLine(const String& line);

struct Earcon {
  uint16_t freq;
  uint16_t ms;
};

Earcon earcon_q[6];
uint8_t earcon_n = 0;
uint8_t earcon_i = 0;
uint32_t earcon_until = 0;
bool earcon_on = false;

const char* modeName(StickMode m) {
  switch (m) {
    case StickMode::Micro:
      return "MICRO";
    case StickMode::Fsr:
      return "FSR";
    case StickMode::Hall:
      return "HALL";
    case StickMode::Load:
      return "LOAD";
    case StickMode::Binary:
      return "BINARY";
    default: {
      const StickMode unused = m;
      (void)unused;
      return "MICRO";
    }
  }
}

StickMode parseMode(const String& s) {
  if (s == "MICRO") {
    return StickMode::Micro;
  }
  if (s == "FSR") {
    return StickMode::Fsr;
  }
  if (s == "HALL") {
    return StickMode::Hall;
  }
  if (s == "LOAD") {
    return StickMode::Load;
  }
  if (s == "BINARY") {
    return StickMode::Binary;
  }
  return settings.mode;
}

StickMode modeFromModuleId(ModuleId id) {
  switch (id) {
    case ModuleId::Micro:
      return StickMode::Micro;
    case ModuleId::Fsr:
      return StickMode::Fsr;
    case ModuleId::Hall:
      return StickMode::Hall;
    case ModuleId::Load:
      return StickMode::Load;
    case ModuleId::Empty:
      return settings.mode;
    default: {
      const ModuleId unused = id;
      (void)unused;
      return StickMode::Micro;
    }
  }
}

void refreshKeys() {
  keys = keysetFor(settings.profile, settings);
}

void saveSettings() {
  prefs.begin("stick", false);
  prefs.putUChar("mode", static_cast<uint8_t>(settings.mode));
  prefs.putUChar("prof", static_cast<uint8_t>(settings.profile));
  prefs.putUChar("pack", static_cast<uint8_t>(settings.pack));
  prefs.putBool("inv_sw", settings.invert_switches);
  prefs.putBool("inv_an", settings.invert_analog);
  prefs.putBool("inv_ld", settings.invert_load);
  prefs.putChar("ksoft", settings.key_soft);
  prefs.putChar("khard", settings.key_hard);
  prefs.putChar("kbin", settings.key_binary);
  prefs.putInt("tare", settings.tare_adc);
  prefs.putInt("fsr_s", settings.fsr_soft_on);
  prefs.putInt("fsr_h", settings.fsr_hard_on);
  prefs.putFloat("h_s", settings.hall_soft_mm);
  prefs.putFloat("h_h", settings.hall_hard_mm);
  prefs.putBytes("h_adc", settings.hall_adc, sizeof(settings.hall_adc));
  prefs.putBytes("h_mm", settings.hall_mm, sizeof(settings.hall_mm));
  prefs.putInt("ld_tare", settings.load_tare);
  prefs.putFloat("ld_sc", settings.load_scale);
  prefs.putFloat("ld_s", settings.load_soft_gf);
  prefs.putFloat("ld_h", settings.load_hard_gf);
  prefs.putUShort("gth", settings.grip_thresh);
  prefs.end();
}

void persist(bool announce) {
  saveSettings();
  if (announce) {
    Serial.println(F("{\"saved\":true}"));
  }
}

void loadSettings() {
  prefs.begin("stick", true);
  settings.mode = static_cast<StickMode>(prefs.getUChar("mode", 0));
  settings.profile = static_cast<HidProfile>(prefs.getUChar("prof", 0));
  settings.pack = static_cast<PackType>(prefs.getUChar("pack", 0));
  settings.invert_switches = prefs.getBool("inv_sw", false);
  settings.invert_analog = prefs.getBool("inv_an", false);
  settings.invert_load = prefs.getBool("inv_ld", false);
  settings.key_soft = prefs.getChar("ksoft", kKeySoft);
  settings.key_hard = prefs.getChar("khard", kKeyHard);
  settings.key_binary = prefs.getChar("kbin", kKeyBinary);
  settings.tare_adc = prefs.getInt("tare", 0);
  settings.fsr_soft_on = prefs.getInt("fsr_s", kFsrSoftCounts);
  settings.fsr_hard_on = prefs.getInt("fsr_h", kFsrHardCounts);
  settings.hall_soft_mm = prefs.getFloat("h_s", kHallSoftMm);
  settings.hall_hard_mm = prefs.getFloat("h_h", kHallHardMm);
  prefs.getBytes("h_adc", settings.hall_adc, sizeof(settings.hall_adc));
  prefs.getBytes("h_mm", settings.hall_mm, sizeof(settings.hall_mm));
  settings.load_tare = prefs.getInt("ld_tare", 0);
  settings.load_scale = prefs.getFloat("ld_sc", 1.0f);
  settings.load_soft_gf = prefs.getFloat("ld_s", kLoadSoftGf);
  settings.load_hard_gf = prefs.getFloat("ld_h", kLoadHardGf);
  settings.grip_thresh = prefs.getUShort("gth", kGripThreshDefault);
  prefs.end();
  if (settings.hall_adc[kHallLutPoints - 1] <= settings.hall_adc[0]) {
    settings.hall_adc[0] = 1800;
    settings.hall_adc[1] = 2000;
    settings.hall_adc[2] = 2300;
    settings.hall_adc[3] = 2700;
    settings.hall_adc[4] = 3100;
    settings.hall_mm[0] = 0;
    settings.hall_mm[1] = 3;
    settings.hall_mm[2] = 6;
    settings.hall_mm[3] = 9;
    settings.hall_mm[4] = 12;
  }
  if (settings.grip_thresh == 0) {
    settings.grip_thresh = kGripThreshDefault;
  }
  refreshKeys();
}

void enqueueTone(uint16_t freq, uint16_t ms) {
  if (earcon_n >= 6) {
    return;
  }
  earcon_q[earcon_n].freq = freq;
  earcon_q[earcon_n].ms = ms;
  earcon_n++;
}

void earconStartup() {
  enqueueTone(880, 80);
  enqueueTone(0, 40);
  enqueueTone(1175, 80);
}

void earconPaired() {
  enqueueTone(1200, 60);
  enqueueTone(0, 30);
  enqueueTone(1600, 80);
}

void earconClick() {
  enqueueTone(2000, 18);
}

void earconLowBatt() {
  enqueueTone(400, 120);
  enqueueTone(0, 80);
  enqueueTone(400, 120);
}

void pollBuzzer(uint32_t now) {
  if (!earcon_on) {
    if (earcon_i >= earcon_n) {
      earcon_n = 0;
      earcon_i = 0;
      return;
    }
    const Earcon& t = earcon_q[earcon_i];
    if (t.freq > 0) {
      ledcWriteTone(kBuzzerChannel, t.freq);
    } else {
      ledcWriteTone(kBuzzerChannel, 0);
    }
    earcon_until = now + t.ms;
    earcon_on = true;
    return;
  }
  if (now >= earcon_until) {
    ledcWriteTone(kBuzzerChannel, 0);
    earcon_on = false;
    earcon_i++;
  }
}

void hidPress(const HidKey& k) {
  switch (k.kind) {
    case HidKeyKind::Keyboard:
      ble.press(k.kbd);
      break;
    case HidKeyKind::Media: {
      MediaKeyReport r;
      r[0] = k.media0;
      r[1] = k.media1;
      ble.press(r);
      break;
    }
    default: {
      const HidKeyKind unused = k.kind;
      (void)unused;
      break;
    }
  }
}

void hidRelease(const HidKey& k) {
  switch (k.kind) {
    case HidKeyKind::Keyboard:
      ble.release(k.kbd);
      break;
    case HidKeyKind::Media: {
      MediaKeyReport r;
      r[0] = k.media0;
      r[1] = k.media1;
      ble.release(r);
      break;
    }
    default: {
      const HidKeyKind unused = k.kind;
      (void)unused;
      break;
    }
  }
}

void applyHid(const HidAction& a) {
  if (!ble.isConnected() || a.empty()) {
    return;
  }
  if (a.release_soft) {
    hidRelease(keys.soft);
  }
  if (a.release_hard) {
    hidRelease(keys.hard);
  }
  if (a.release_binary) {
    hidRelease(keys.binary);
  }
  if (a.press_soft) {
    hidPress(keys.soft);
  }
  if (a.press_hard) {
    hidPress(keys.hard);
  }
  if (a.press_binary) {
    hidPress(keys.binary);
  }
  if (a.press_soft || a.press_hard || a.press_binary) {
    earconClick();
  }
}

void setLeds(StickLevel level, bool binary) {
  const bool soft = level == StickLevel::Soft || binary;
  const bool hard = level == StickLevel::Hard;
  digitalWrite(PIN_LED_SOFT, soft ? HIGH : LOW);
  digitalWrite(PIN_LED_HARD, hard ? HIGH : LOW);
}

void setJack(bool closed) {
  digitalWrite(PIN_JACK_OUT, closed ? HIGH : LOW);
}

int analogReadFilt() {
  analogSetPinAttenuation(PIN_ADC_SENSOR, ADC_11db);
  const int raw = analogRead(PIN_ADC_SENSOR);
  const float v = adc_filt.push(static_cast<float>(raw));
  return static_cast<int>(v);
}

int readBattMv() {
  analogSetPinAttenuation(PIN_ADC_BATT, ADC_11db);
  int acc = 0;
  for (int i = 0; i < 8; ++i) {
    acc += analogRead(PIN_ADC_BATT);
  }
  return battMvFromAdc(acc / 8, kAdcVrefMv, kAdcFullScale);
}

void readModuleId() {
  analogSetPinAttenuation(PIN_MODULE_ID, ADC_11db);
  int acc = 0;
  for (int i = 0; i < 8; ++i) {
    acc += analogRead(PIN_MODULE_ID);
  }
  last_module_id = decodeModuleId(acc / 8, &last_id_ohms, kModuleIdPullupOhms, kAdcFullScale);
}

void pollGrip() {
  last_grip_raw = static_cast<uint16_t>(grip_filt.push(static_cast<float>(touchRead(PIN_TOUCH_GRIP))));
  last_grip = gripHolding(last_grip_raw, settings.grip_thresh);
  digitalWrite(PIN_LED_GRIP, last_grip ? HIGH : LOW);
}

void maybeLowBattLeds(uint32_t now, bool active) {
  if (active || last_batt_mv == 0 || !battIsLow(last_batt_mv, settings.pack)) {
    return;
  }
  const bool on = ((now / kBattBlinkMs) % 2) == 0;
  digitalWrite(PIN_LED_SOFT, on ? HIGH : LOW);
  digitalWrite(PIN_LED_HARD, on ? HIGH : LOW);
}

void publishBattery(bool force) {
  const int pct = battPctFromMv(last_batt_mv, settings.pack);
  if (!ble.isConnected()) {
    last_batt_pct = -1;
    return;
  }
  if (force || pct != last_batt_pct) {
    last_batt_pct = pct;
    ble.setBatteryLevel(static_cast<uint8_t>(pct));
  }
}

float hallMm(int adc) {
  float v = static_cast<float>(adc);
  if (settings.invert_analog) {
    v = 4095.0f - v;
  }
  return lutInterp(settings.hall_adc, settings.hall_mm, kHallLutPoints, v);
}

float loadGrams() {
  if (!nau_ok) {
    return 0.0f;
  }
  const int32_t raw = nau.read();
  int32_t centered = raw - settings.load_tare;
  if (settings.invert_load) {
    centered = -centered;
  }
  const float g = load_filt.push(static_cast<float>(centered) * settings.load_scale);
  return g;
}

void printKeyJson(const HidKey& k) {
  if (k.kind == HidKeyKind::Media) {
    Serial.print("\"M");
    Serial.print(k.media0);
    if (k.media1) {
      Serial.print(",");
      Serial.print(k.media1);
    }
    Serial.print("\"");
    return;
  }
  if (k.kbd == ' ') {
    Serial.print("\"SPACE\"");
    return;
  }
  if (k.kbd >= 32 && k.kbd < 127) {
    Serial.print("\"");
    Serial.print(static_cast<char>(k.kbd));
    Serial.print("\"");
    return;
  }
  Serial.print(k.kbd);
}

void dumpJson() {
  Serial.print("{\"mode\":\"");
  Serial.print(modeName(settings.mode));
  Serial.print("\",\"profile\":\"");
  Serial.print(profileName(settings.profile));
  Serial.print("\",\"ble_name\":\"");
  Serial.print(ble_name);
  Serial.print("\",\"level\":");
  Serial.print(static_cast<int>(machine.level()));
  Serial.print(",\"batt_mv\":");
  Serial.print(last_batt_mv);
  Serial.print(",\"batt_pct\":");
  Serial.print(battPctFromMv(last_batt_mv, settings.pack));
  Serial.print(",\"pack\":");
  Serial.print(packCells(settings.pack));
  Serial.print(",\"grip\":");
  Serial.print(last_grip ? "true" : "false");
  Serial.print(",\"grip_raw\":");
  Serial.print(last_grip_raw);
  Serial.print(",\"grip_thresh\":");
  Serial.print(settings.grip_thresh);
  Serial.print(",\"module_id\":\"");
  Serial.print(moduleIdName(last_module_id));
  Serial.print("\",\"module_ohms\":");
  Serial.print(last_id_ohms);
  Serial.print(",\"tare_adc\":");
  Serial.print(settings.tare_adc);
  Serial.print(",\"fsr_soft\":");
  Serial.print(settings.fsr_soft_on);
  Serial.print(",\"fsr_hard\":");
  Serial.print(settings.fsr_hard_on);
  Serial.print(",\"hall_soft_mm\":");
  Serial.print(settings.hall_soft_mm);
  Serial.print(",\"hall_hard_mm\":");
  Serial.print(settings.hall_hard_mm);
  Serial.print(",\"load_tare\":");
  Serial.print(settings.load_tare);
  Serial.print(",\"load_scale\":");
  Serial.print(settings.load_scale, 6);
  Serial.print(",\"load_soft_gf\":");
  Serial.print(settings.load_soft_gf);
  Serial.print(",\"load_hard_gf\":");
  Serial.print(settings.load_hard_gf);
  Serial.print(",\"keys\":[");
  printKeyJson(keys.soft);
  Serial.print(",");
  printKeyJson(keys.hard);
  Serial.print(",");
  printKeyJson(keys.binary);
  Serial.print("],\"hall_lut\":[");
  for (uint8_t i = 0; i < kHallLutPoints; ++i) {
    if (i) {
      Serial.print(",");
    }
    Serial.print("[");
    Serial.print(settings.hall_adc[i]);
    Serial.print(",");
    Serial.print(settings.hall_mm[i]);
    Serial.print("]");
  }
  Serial.println("]}");
}

void help() {
  Serial.println(F("MODE MICRO|FSR|HALL|LOAD|BINARY"));
  Serial.println(F("PROFILE IPADOS|ANDROID|FUNCTION|MEDIA|CUSTOM"));
  Serial.println(F("PACK 2|3"));
  Serial.println(F("TARE"));
  Serial.println(F("CAL START SOFT|HARD"));
  Serial.println(F("CAL STOP"));
  Serial.println(F("LUT adc0,mm0,adc1,mm1,adc2,mm2,adc3,mm3,adc4,mm4"));
  Serial.println(F("SCALE <gf_per_count>"));
  Serial.println(F("INVERT SW|AN|LD 0|1"));
  Serial.println(F("KEYS <soft><hard>   example: KEYS 12"));
  Serial.println(F("BINARY_KEY SPACE|1|2"));
  Serial.println(F("GRIP THRESH <n>"));
  Serial.println(F("GRIP CAL"));
  Serial.println(F("SAVE  DUMP  PLOT 0|1  HELP"));
}

void finishCal() {
  if (cal_count == 0) {
    Serial.println(F("{\"cal\":\"empty\"}"));
    cal_collecting = false;
    return;
  }
  float sum = 0;
  for (uint8_t i = 0; i < cal_count; ++i) {
    sum += cal_acc[i];
  }
  const float mean = sum / cal_count;
  if (settings.mode == StickMode::Fsr) {
    if (cal_is_hard) {
      const float span = mean - static_cast<float>(settings.tare_adc);
      settings.fsr_hard_on = static_cast<int32_t>(0.70f * span);
      settings.fsr_soft_on = static_cast<int32_t>(0.35f * span);
    } else {
      settings.fsr_soft_on = static_cast<int32_t>(mean - static_cast<float>(settings.tare_adc));
    }
  } else if (settings.mode == StickMode::Hall) {
    if (cal_is_hard) {
      settings.hall_hard_mm = mean;
    } else {
      settings.hall_soft_mm = mean;
    }
  } else if (settings.mode == StickMode::Load) {
    if (cal_is_hard) {
      settings.load_hard_gf = mean;
      settings.load_soft_gf = 0.40f * mean;
    } else {
      settings.load_soft_gf = mean;
    }
  }
  cal_collecting = false;
  cal_count = 0;
  persist(false);
  Serial.print(F("{\"cal\":\"ok\",\"mean\":"));
  Serial.print(mean);
  Serial.println("}");
}

void doTare() {
  if (settings.mode == StickMode::Load && nau_ok) {
    int64_t acc = 0;
    for (int i = 0; i < 16; ++i) {
      while (!nau.available()) {
        delay(2);
      }
      acc += nau.read();
    }
    settings.load_tare = static_cast<int32_t>(acc / 16);
    load_filt.set(0);
  } else {
    int64_t acc = 0;
    const uint32_t start = millis();
    int n = 0;
    while (millis() - start < kTareWindowMs) {
      acc += analogRead(PIN_ADC_SENSOR);
      n++;
      delay(5);
    }
    settings.tare_adc = static_cast<int32_t>(acc / max(n, 1));
    adc_filt.set(static_cast<float>(settings.tare_adc));
  }
  Serial.println(F("{\"tare\":true}"));
  persist(false);
  dumpJson();
}

void doCalToggle() {
  if (!cal_collecting) {
    handleLine(analog_hard_held ? String("CAL START HARD") : String("CAL START SOFT"));
  } else {
    handleLine("CAL STOP");
  }
}

void simulateLevel(bool hard) {
  sim_soft = !hard;
  sim_hard = hard;
  sim_until_ms = millis() + kSimPulseMs;
}

void bootHoldAction() {
  if (cal_collecting || analog_soft_held || analog_hard_held) {
    doCalToggle();
  } else {
    doTare();
  }
}

void pollBoot(uint32_t now) {
  const bool down = deb_boot.update(digitalRead(PIN_BTN_BOOT) == LOW, now);
  if (down && !boot_was) {
    boot_down_ms = now;
    boot_held_fired = false;
  }
  if (down && !boot_held_fired && (now - boot_down_ms) >= kBootHoldMs) {
    boot_held_fired = true;
    boot_taps = 0;
    bootHoldAction();
  }
  if (!down && boot_was && !boot_held_fired) {
    boot_taps++;
    boot_wait_until = now + kBootTapGapMs;
  }
  if (!down && boot_taps > 0 && now >= boot_wait_until) {
    if (boot_taps == 1) {
      simulateLevel(false);
    } else {
      simulateLevel(true);
    }
    boot_taps = 0;
  }
  boot_was = down;
}

void handleLine(const String& line) {
  if (line == "HELP") {
    help();
    return;
  }
  if (line == "DUMP") {
    dumpJson();
    return;
  }
  if (line == "SAVE") {
    persist(true);
    return;
  }
  if (line == "TARE") {
    doTare();
    return;
  }
  if (line.startsWith("MODE ")) {
    settings.mode = parseMode(line.substring(5));
    machine.reset();
    analog_soft_held = false;
    analog_hard_held = false;
    persist(false);
    Serial.print(F("{\"mode\":\""));
    Serial.print(modeName(settings.mode));
    Serial.println("\"}");
    return;
  }
  if (line.startsWith("PROFILE ")) {
    HidProfile p = settings.profile;
    const String name = line.substring(8);
    if (parseProfile(name.c_str(), &p)) {
      settings.profile = p;
      refreshKeys();
      if (ble.isConnected()) {
        ble.releaseAll();
      }
      persist(false);
      Serial.print(F("{\"profile\":\""));
      Serial.print(profileName(settings.profile));
      Serial.println("\"}");
    } else {
      Serial.println(F("{\"err\":\"profile\"}"));
    }
    return;
  }
  if (line.startsWith("PACK ")) {
    const int n = line.substring(5).toInt();
    if (n == 2) {
      settings.pack = PackType::Cell2;
    } else if (n == 3) {
      settings.pack = PackType::Cell3;
    } else {
      Serial.println(F("{\"err\":\"pack\"}"));
      return;
    }
    persist(false);
    last_batt_pct = -1;
    Serial.print(F("{\"pack\":"));
    Serial.print(packCells(settings.pack));
    Serial.println("}");
    return;
  }
  if (line.startsWith("PLOT ")) {
    plot = line.substring(5).toInt() != 0;
    return;
  }
  if (line.startsWith("SCALE ")) {
    settings.load_scale = line.substring(6).toFloat();
    persist(false);
    return;
  }
  if (line.startsWith("INVERT ")) {
    const String rest = line.substring(7);
    const int sp = rest.indexOf(' ');
    if (sp > 0) {
      const String which = rest.substring(0, sp);
      const bool v = rest.substring(sp + 1).toInt() != 0;
      if (which == "SW") {
        settings.invert_switches = v;
      } else if (which == "AN") {
        settings.invert_analog = v;
      } else if (which == "LD") {
        settings.invert_load = v;
      }
      persist(false);
    }
    return;
  }
  if (line.startsWith("KEYS ") && line.length() >= 7) {
    settings.key_soft = line.charAt(5);
    settings.key_hard = line.charAt(6);
    settings.profile = HidProfile::Custom;
    refreshKeys();
    persist(false);
    return;
  }
  if (line.startsWith("BINARY_KEY ")) {
    const String k = line.substring(11);
    settings.key_binary = (k == "SPACE") ? ' ' : k.charAt(0);
    settings.profile = HidProfile::Custom;
    refreshKeys();
    persist(false);
    return;
  }
  if (line.startsWith("GRIP THRESH ")) {
    const int n = line.substring(12).toInt();
    if (n > 0 && n < 1000) {
      settings.grip_thresh = static_cast<uint16_t>(n);
      persist(false);
      Serial.print(F("{\"grip_thresh\":"));
      Serial.print(settings.grip_thresh);
      Serial.println("}");
    } else {
      Serial.println(F("{\"err\":\"grip\"}"));
    }
    return;
  }
  if (line == "GRIP CAL") {
    uint32_t acc = 0;
    for (int i = 0; i < 32; ++i) {
      acc += touchRead(PIN_TOUCH_GRIP);
      delay(8);
    }
    const uint16_t mean = static_cast<uint16_t>(acc / 32);
    uint16_t th = static_cast<uint16_t>((static_cast<uint32_t>(mean) * 3) / 4);
    if (th < 8) {
      th = kGripThreshDefault;
    }
    settings.grip_thresh = th;
    grip_filt.set(static_cast<float>(mean));
    persist(false);
    Serial.print(F("{\"grip_cal\":true,\"mean\":"));
    Serial.print(mean);
    Serial.print(F(",\"grip_thresh\":"));
    Serial.print(settings.grip_thresh);
    Serial.println("}");
    return;
  }
  if (line.startsWith("LUT ")) {
    String rest = line.substring(4);
    rest.replace(" ", "");
    int idx = 0;
    while (rest.length() && idx < kHallLutPoints) {
      const int comma = rest.indexOf(',');
      const float adc = rest.substring(0, comma).toFloat();
      rest = rest.substring(comma + 1);
      const int comma2 = rest.indexOf(',');
      const float mm = (comma2 < 0) ? rest.toFloat() : rest.substring(0, comma2).toFloat();
      if (comma2 >= 0) {
        rest = rest.substring(comma2 + 1);
      } else {
        rest = "";
      }
      settings.hall_adc[idx] = adc;
      settings.hall_mm[idx] = mm;
      idx++;
    }
    persist(false);
    Serial.println(F("{\"lut\":true}"));
    return;
  }
  if (line == "CAL START SOFT") {
    cal_collecting = true;
    cal_is_hard = false;
    cal_count = 0;
    Serial.println(F("{\"cal\":\"soft\"}"));
    return;
  }
  if (line == "CAL START HARD") {
    cal_collecting = true;
    cal_is_hard = true;
    cal_count = 0;
    Serial.println(F("{\"cal\":\"hard\"}"));
    return;
  }
  if (line == "CAL STOP") {
    finishCal();
    return;
  }
  Serial.println(F("{\"err\":\"unknown\"}"));
}

void pollSerial() {
  while (Serial.available()) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      serial_buf[serial_len] = 0;
      if (serial_len > 0) {
        handleLine(String(serial_buf));
      }
      serial_len = 0;
    } else if (serial_len < sizeof(serial_buf) - 1) {
      serial_buf[serial_len++] = c;
    }
  }
}

bool readSoftHard(uint32_t now, bool* soft, bool* hard, bool* aph) {
  const bool raw_soft = digitalRead(PIN_SW_SOFT) == LOW;
  const bool raw_hard = digitalRead(PIN_SW_HARD) == LOW;
  const bool raw_aph = digitalRead(PIN_APH_BACKUP) == LOW;
  bool s = deb_soft.update(settings.invert_switches ? !raw_soft : raw_soft, now);
  bool h = deb_hard.update(settings.invert_switches ? !raw_hard : raw_hard, now);
  const bool a = deb_aph.update(raw_aph, now);

  auto maybeCal = [&](float sample) {
    if (cal_collecting && cal_count < kCalSamples && now - last_cal_sample_ms >= 200) {
      cal_acc[cal_count++] = sample;
      last_cal_sample_ms = now;
    }
  };

  if (settings.mode == StickMode::Fsr || settings.mode == StickMode::Hall) {
    if (now - last_adc_ms >= (1000 / kAdcHz)) {
      last_adc_ms = now;
      const int adc = analogReadFilt();
      if (settings.mode == StickMode::Fsr) {
        const float hyst =
            kHysteresisFrac * static_cast<float>(settings.fsr_hard_on - settings.fsr_soft_on);
        const float v = static_cast<float>(adc - settings.tare_adc);
        analog_soft_held = analogActive(v, analog_soft_held, static_cast<float>(settings.fsr_soft_on), hyst);
        analog_hard_held = analogActive(v, analog_hard_held, static_cast<float>(settings.fsr_hard_on), hyst);
        maybeCal(static_cast<float>(adc));
      } else {
        const float mm = hallMm(adc);
        analog_soft_held = analogActive(mm, analog_soft_held, settings.hall_soft_mm, kHallHystMm);
        analog_hard_held = analogActive(mm, analog_hard_held, settings.hall_hard_mm, kHallHystMm);
        maybeCal(mm);
      }
      s = analog_soft_held;
      h = analog_hard_held;
      if (plot) {
        Serial.print(F("adc="));
        Serial.println(adc);
      }
    } else {
      s = analog_soft_held;
      h = analog_hard_held;
    }
  } else if (settings.mode == StickMode::Load) {
    if (nau_ok && nau.available()) {
      const float g = loadGrams();
      analog_soft_held = analogActive(g, analog_soft_held, settings.load_soft_gf, kLoadHystGf);
      analog_hard_held = analogActive(g, analog_hard_held, settings.load_hard_gf, kLoadHystGf);
      s = analog_soft_held;
      h = analog_hard_held;
      if (!s && !h && fabsf(g) < kLoadQuietGf) {
        if (quiet_since_ms == 0) {
          quiet_since_ms = now;
        } else if (now - quiet_since_ms >= kLoadSlowTareMs) {
          settings.load_tare += static_cast<int32_t>(0.01f * (g / max(settings.load_scale, 1e-9f)));
          quiet_since_ms = now;
        }
      } else {
        quiet_since_ms = 0;
      }
      maybeCal(g);
      if (plot) {
        Serial.print(F("gf="));
        Serial.println(g);
      }
    } else if (!nau_ok) {
      s = false;
      h = false;
    }
  }

  *soft = s;
  *hard = h;
  *aph = a;
  return true;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  pinMode(PIN_SW_SOFT, INPUT_PULLUP);
  pinMode(PIN_SW_HARD, INPUT_PULLUP);
  pinMode(PIN_APH_BACKUP, INPUT_PULLUP);
  pinMode(PIN_BTN_TARE, INPUT_PULLUP);
  pinMode(PIN_BTN_CAL, INPUT_PULLUP);
  pinMode(PIN_BTN_BOOT, INPUT_PULLUP);
  pinMode(PIN_LED_SOFT, OUTPUT);
  pinMode(PIN_LED_HARD, OUTPUT);
  pinMode(PIN_LED_GRIP, OUTPUT);
  pinMode(PIN_JACK_OUT, OUTPUT);
  digitalWrite(PIN_LED_GRIP, LOW);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_ADC_SENSOR, ADC_11db);
  analogSetPinAttenuation(PIN_ADC_BATT, ADC_11db);
  analogSetPinAttenuation(PIN_MODULE_ID, ADC_11db);

  ledcSetup(kBuzzerChannel, 2000, 8);
  ledcAttachPin(PIN_BUZZER, kBuzzerChannel);
  ledcWriteTone(kBuzzerChannel, 0);

  uint8_t mac[6] = {};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(ble_name, sizeof(ble_name), "Stick-Switch-%02X%02X", mac[4], mac[5]);
  ble.setName(ble_name);

  loadSettings();
  last_batt_mv = readBattMv();
  if (settings.pack == PackType::Unset) {
    settings.pack = autoPickPack(last_batt_mv);
    persist(false);
  }
  grip_filt.set(static_cast<float>(touchRead(PIN_TOUCH_GRIP)));
  pollGrip();
  readModuleId();
  if (last_module_id != ModuleId::Empty) {
    settings.mode = modeFromModuleId(last_module_id);
  }
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  nau_ok = nau.begin();
  if (nau_ok) {
    nau.setLDO(NAU7802_3V0);
    nau.setGain(NAU7802_GAIN_128);
    nau.setRate(NAU7802_RATE_80SPS);
    nau.calibrate(NAU7802_CALMOD_INTERNAL);
  }

  ble.begin();
  earconStartup();
  help();
  dumpJson();
}

void loop() {
  const uint32_t now = millis();
  pollSerial();
  pollBuzzer(now);
  pollGrip();
  pollBoot(now);

  if (now - last_batt_ms >= kBattPollMs) {
    last_batt_ms = now;
    last_batt_mv = readBattMv();
    publishBattery(false);
    const bool low = battIsLow(last_batt_mv, settings.pack);
    if (low && !last_batt_low) {
      earconLowBatt();
    }
    last_batt_low = low;
  }

  const bool connected = ble.isConnected();
  if (connected && !last_ble_connected) {
    publishBattery(true);
    earconPaired();
  }
  last_ble_connected = connected;

  const bool tare_btn = deb_tare.update(digitalRead(PIN_BTN_TARE) == LOW, now);
  static bool tare_was = false;
  if (tare_btn && !tare_was) {
    handleLine("TARE");
  }
  tare_was = tare_btn;

  const bool cal_btn = deb_cal.update(digitalRead(PIN_BTN_CAL) == LOW, now);
  static bool cal_was = false;
  if (cal_btn && !cal_was) {
    doCalToggle();
  }
  cal_was = cal_btn;

  if (sim_until_ms != 0 && now >= sim_until_ms) {
    sim_soft = false;
    sim_hard = false;
    sim_until_ms = 0;
  }

  bool soft = false;
  bool hard = false;
  bool aph = false;
  readSoftHard(now, &soft, &hard, &aph);
  soft = soft || sim_soft;
  hard = hard || sim_hard;

  HidAction action;
  bool jack = false;
  if (settings.mode == StickMode::Binary) {
    action = machine.updateBinary(soft || hard || aph);
    jack = soft || hard || aph;
    setLeds(StickLevel::Idle, jack);
  } else {
    action = machine.updateDual(soft, hard);
    const HidAction aph_action = machine.updateBinary(aph);
    action.release_binary = aph_action.release_binary;
    action.press_binary = aph_action.press_binary;
    jack = machine.level() != StickLevel::Idle || aph;
    setLeds(machine.level(), aph);
  }
  applyHid(action);
  setJack(jack);
  maybeLowBattLeds(now, jack);

  static uint32_t last_plot = 0;
  if (plot && now - last_plot > 50) {
    last_plot = now;
    Serial.print(F("lvl="));
    Serial.print(static_cast<int>(machine.level()));
    Serial.print(F(" s="));
    Serial.print(soft);
    Serial.print(F(" h="));
    Serial.print(hard);
    Serial.print(F(" g="));
    Serial.println(last_grip);
  }
}
