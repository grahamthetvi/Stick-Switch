#include <Arduino.h>
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_NAU7802.h>

#include <BleKeyboard.h>

#include "config.h"
#include "pins.h"
#include "settings.h"
#include "stick_level.h"

namespace {

Preferences prefs;
BleKeyboard ble(kBleName, "Stick-Switch", 100);
Adafruit_NAU7802 nau;
StickMachine machine;
StickSettings settings;

Debounce deb_soft(kDebounceMs);
Debounce deb_hard(kDebounceMs);
Debounce deb_aph(kDebounceMs);
Debounce deb_tare(kDebounceMs);
Debounce deb_cal(kDebounceMs);
BoxcarFilter adc_filt(static_cast<float>(kBoxcar));
BoxcarFilter load_filt(static_cast<float>(kBoxcar));

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
int last_id_ohms = -1;
ModuleId last_module_id = ModuleId::Empty;
uint32_t last_batt_ms = 0;

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

void saveSettings() {
  prefs.begin("stick", false);
  prefs.putUChar("mode", static_cast<uint8_t>(settings.mode));
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
}

void applyHid(const HidAction& a) {
  if (!ble.isConnected() || a.empty()) {
    return;
  }
  if (a.release_soft) {
    ble.release(settings.key_soft);
  }
  if (a.release_hard) {
    ble.release(settings.key_hard);
  }
  if (a.release_binary) {
    ble.release(settings.key_binary);
  }
  if (a.press_soft) {
    ble.press(settings.key_soft);
  }
  if (a.press_hard) {
    ble.press(settings.key_hard);
  }
  if (a.press_binary) {
    ble.press(settings.key_binary);
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

void maybeLowBattLeds(uint32_t now, bool active) {
  if (active || last_batt_mv == 0 || last_batt_mv >= kBattLowMv) {
    return;
  }
  const bool on = ((now / kBattBlinkMs) % 2) == 0;
  digitalWrite(PIN_LED_SOFT, on ? HIGH : LOW);
  digitalWrite(PIN_LED_HARD, on ? HIGH : LOW);
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

void dumpJson() {
  Serial.print("{\"mode\":\"");
  Serial.print(modeName(settings.mode));
  Serial.print("\",\"level\":");
  Serial.print(static_cast<int>(machine.level()));
  Serial.print(",\"batt_mv\":");
  Serial.print(last_batt_mv);
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
  Serial.print(",\"keys\":[\"");
  Serial.print(settings.key_soft);
  Serial.print("\",\"");
  Serial.print(settings.key_hard);
  Serial.print("\",\"");
  if (settings.key_binary == ' ') {
    Serial.print("SPACE");
  } else {
    Serial.print(settings.key_binary);
  }
  Serial.print("\"],\"hall_lut\":[");
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
  Serial.println(F("TARE"));
  Serial.println(F("CAL START SOFT|HARD"));
  Serial.println(F("CAL STOP"));
  Serial.println(F("LUT adc0,mm0,adc1,mm1,adc2,mm2,adc3,mm3,adc4,mm4"));
  Serial.println(F("SCALE <gf_per_count>"));
  Serial.println(F("INVERT SW|AN|LD 0|1"));
  Serial.println(F("KEYS <soft><hard>   example: KEYS 12"));
  Serial.println(F("BINARY_KEY SPACE|1|2"));
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
    persist(false);
    return;
  }
  if (line.startsWith("BINARY_KEY ")) {
    const String k = line.substring(11);
    settings.key_binary = (k == "SPACE") ? ' ' : k.charAt(0);
    persist(false);
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
  pinMode(PIN_LED_SOFT, OUTPUT);
  pinMode(PIN_LED_HARD, OUTPUT);
  pinMode(PIN_JACK_OUT, OUTPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_ADC_SENSOR, ADC_11db);
  analogSetPinAttenuation(PIN_ADC_BATT, ADC_11db);
  analogSetPinAttenuation(PIN_MODULE_ID, ADC_11db);

  loadSettings();
  last_batt_mv = readBattMv();
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
  help();
  dumpJson();
}

void loop() {
  const uint32_t now = millis();
  pollSerial();

  if (now - last_batt_ms >= kBattPollMs) {
    last_batt_ms = now;
    last_batt_mv = readBattMv();
  }

  const bool tare_btn = deb_tare.update(digitalRead(PIN_BTN_TARE) == LOW, now);
  static bool tare_was = false;
  if (tare_btn && !tare_was) {
    handleLine("TARE");
  }
  tare_was = tare_btn;

  const bool cal_btn = deb_cal.update(digitalRead(PIN_BTN_CAL) == LOW, now);
  static bool cal_was = false;
  if (cal_btn && !cal_was) {
    if (!cal_collecting) {
      handleLine(analog_hard_held ? String("CAL START HARD") : String("CAL START SOFT"));
    } else {
      handleLine("CAL STOP");
    }
  }
  cal_was = cal_btn;

  bool soft = false;
  bool hard = false;
  bool aph = false;
  readSoftHard(now, &soft, &hard, &aph);

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
    Serial.println(hard);
  }
}
