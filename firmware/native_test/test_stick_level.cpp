#include "hid_profile.h"
#include "stick_level.h"

#include <cstdio>
#include <cstdlib>

static int g_fails = 0;

#define CHECK(cond, msg)                                                       \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, msg);        \
      g_fails++;                                                               \
    }                                                                          \
  } while (0)

static void test_upgrade_no_dual_fire() {
  StickMachine m;
  HidAction a = m.updateDual(true, false);
  CHECK(m.level() == StickLevel::Soft, "idle->soft");
  CHECK(a.press_soft && !a.press_hard, "press 1 only");

  a = m.updateDual(true, true);
  CHECK(m.level() == StickLevel::Hard, "soft->hard upgrade");
  CHECK(a.release_soft && a.press_hard, "release 1 press 2");
  CHECK(!a.press_soft, "must not dual-fire");
}

static void test_no_downgrade() {
  StickMachine m;
  m.updateDual(true, false);
  m.updateDual(true, true);
  const HidAction a = m.updateDual(true, false);
  CHECK(m.level() == StickLevel::Hard, "hard stays while still in stroke");
  CHECK(a.empty(), "no extra HID on easing off hard");
}

static void test_full_release() {
  StickMachine m;
  m.updateDual(true, false);
  m.updateDual(false, true);
  CHECK(m.level() == StickLevel::Hard, "hard from idle if hard asserted");
  const HidAction a = m.updateDual(false, false);
  CHECK(m.level() == StickLevel::Idle, "full release");
  CHECK(a.release_hard && !a.release_soft, "release 2 only");
}

static void test_soft_release() {
  StickMachine m;
  m.updateDual(true, false);
  const HidAction a = m.updateDual(false, false);
  CHECK(m.level() == StickLevel::Idle, "soft release");
  CHECK(a.release_soft && !a.press_hard, "release 1");
}

static void test_hard_priority() {
  StickMachine m;
  const HidAction a = m.updateDual(true, true);
  CHECK(m.level() == StickLevel::Hard, "slam goes hard");
  CHECK(a.press_hard && !a.press_soft, "skip soft on slam");
}

static void test_binary() {
  StickMachine m;
  HidAction a = m.updateBinary(true);
  CHECK(a.press_binary && !a.press_soft, "binary press");
  a = m.updateBinary(true);
  CHECK(a.empty(), "hold is not a new event");
  a = m.updateBinary(false);
  CHECK(a.release_binary, "binary release");
}

static void test_debounce() {
  Debounce d(15);
  CHECK(!d.update(true, 0), "not yet");
  CHECK(!d.update(true, 14), "still bouncing");
  CHECK(d.update(true, 15), "stable after 15 ms");
  CHECK(d.update(false, 16), "new candidate not immediate");
  CHECK(!d.update(false, 31), "released");
}

static void test_hysteresis() {
  CHECK(!analogActive(9, false, 10, 2), "below on");
  CHECK(analogActive(10, false, 10, 2), "on threshold");
  CHECK(analogActive(8, true, 10, 2), "hold inside hyst");
  CHECK(!analogActive(7.9f, true, 10, 2), "drop below hyst");
}

static void test_lut() {
  const float x[] = {0, 10, 20, 30, 40};
  const float y[] = {0, 3, 6, 9, 12};
  CHECK(lutInterp(x, y, 5, -5) == 0, "clamp low");
  CHECK(lutInterp(x, y, 5, 40) == 12, "clamp high");
  const float mid = lutInterp(x, y, 5, 5);
  CHECK(mid > 1.4f && mid < 1.6f, "linear mid-segment");
}

static void test_boxcar() {
  BoxcarFilter f(8);
  f.set(0);
  float v = 0;
  for (int i = 0; i < 40; ++i) {
    v = f.push(100);
  }
  CHECK(v > 90, "converges toward step");
}

static void test_batt_divider() {
  CHECK(battMvFromAdc(0) == 0, "zero");
  const int full = battMvFromAdc(4095);
  CHECK(full >= 6590 && full <= 6610, "full scale ~2 * 3.3 V");
  const int mid = battMvFromAdc(2048);
  CHECK(mid >= 3290 && mid <= 3310, "mid ~3.3 V cell");
}

static void test_pack_curves() {
  CHECK(battPctFromMv(2000, PackType::Cell2) == 0, "2cell empty");
  CHECK(battPctFromMv(3200, PackType::Cell2) == 100, "2cell full");
  CHECK(battPctFromMv(2600, PackType::Cell2) == 50, "2cell mid");
  CHECK(battPctFromMv(3000, PackType::Cell3) == 0, "3cell empty");
  CHECK(battPctFromMv(4800, PackType::Cell3) == 100, "3cell full");
  CHECK(battPctFromMv(3900, PackType::Cell3) == 50, "3cell mid");
  CHECK(battIsLow(2100, PackType::Cell2), "2cell low");
  CHECK(!battIsLow(3000, PackType::Cell2), "2cell ok");
  CHECK(battIsLow(3100, PackType::Cell3), "3cell low");
  CHECK(!battIsLow(4000, PackType::Cell3), "3cell ok");
  CHECK(autoPickPack(2500) == PackType::Cell2, "auto 2cell");
  CHECK(autoPickPack(4000) == PackType::Cell3, "auto 3cell");
  CHECK(packCells(PackType::Cell2) == 2, "cells 2");
  CHECK(packCells(PackType::Cell3) == 3, "cells 3");
  CHECK(packCells(PackType::Usb) == 0, "usb cells");
  CHECK(battPctFromMv(0, PackType::Usb) == 100, "usb pct");
  CHECK(!battIsLow(0, PackType::Usb), "usb never low");
}

static void test_profiles() {
  StickSettings s;
  const HidKeyset ipad = keysetFor(HidProfile::IpadOS, s);
  CHECK(ipad.soft.kind == HidKeyKind::Keyboard && ipad.soft.kbd == '1', "ipad soft 1");
  CHECK(ipad.hard.kbd == '2', "ipad hard 2");
  CHECK(ipad.binary.kbd == ' ', "ipad binary space");

  const HidKeyset andr = keysetFor(HidProfile::Android, s);
  CHECK(andr.soft.kbd == kHidTab, "android tab");
  CHECK(andr.hard.kbd == kHidReturn, "android return");
  CHECK(andr.binary.kbd == kHidReturn, "android binary return");

  const HidKeyset fn = keysetFor(HidProfile::Function, s);
  CHECK(fn.soft.kbd == kHidF1 && fn.hard.kbd == kHidF2 && fn.binary.kbd == kHidF3, "F1 F2 F3");

  const HidKeyset media = keysetFor(HidProfile::Media, s);
  CHECK(media.soft.kind == HidKeyKind::Media && media.soft.media0 == kMediaVolDown, "vol down");
  CHECK(media.hard.media0 == kMediaVolUp, "vol up");
  CHECK(media.binary.media0 == kMediaPlayPause, "play pause");

  s.key_soft = 'a';
  s.key_hard = 'b';
  s.key_binary = 'c';
  const HidKeyset custom = keysetFor(HidProfile::Custom, s);
  CHECK(custom.soft.kbd == 'a' && custom.hard.kbd == 'b' && custom.binary.kbd == 'c',
        "custom uses NVS keys");

  HidProfile p = HidProfile::IpadOS;
  CHECK(parseProfile("ANDROID", &p) && p == HidProfile::Android, "parse android");
  CHECK(parseProfile("MEDIA", &p) && p == HidProfile::Media, "parse media");
  CHECK(!parseProfile("HUB", &p), "reject unknown profile");
}

static void test_grip_does_not_fire_hid() {
  CHECK(gripHolding(20, 40), "touch below thresh is hold");
  CHECK(!gripHolding(70, 40), "idle above thresh");
  StickMachine m;
  const bool grip = gripHolding(20, 40);
  CHECK(grip, "holding");
  const HidAction a = m.updateDual(false, false);
  CHECK(a.empty(), "grip is not a StickMachine input");
  CHECK(m.level() == StickLevel::Idle, "grip never becomes a stick level");
  const HidAction jack_path = m.updateBinary(false);
  CHECK(jack_path.empty(), "grip does not close binary/jack HID");
}

static int adcFromDivider(int r_gnd, int rpu = 47000, int full = 4095) {
  return static_cast<int>((static_cast<long>(full) * r_gnd) / (rpu + r_gnd));
}

static void test_module_id() {
  int ohms = 0;
  CHECK(decodeModuleId(4095, &ohms) == ModuleId::Empty, "open");
  CHECK(ohms == -1, "open ohms");
  CHECK(decodeModuleId(adcFromDivider(10000), &ohms) == ModuleId::Micro, "10k MICRO");
  CHECK(ohms > 8000 && ohms < 12000, "10k ohms");
  CHECK(decodeModuleId(adcFromDivider(22000), &ohms) == ModuleId::Fsr, "22k FSR");
  CHECK(decodeModuleId(adcFromDivider(47000), &ohms) == ModuleId::Hall, "47k HALL");
  CHECK(decodeModuleId(adcFromDivider(100000), &ohms) == ModuleId::Load, "100k LOAD");
  CHECK(decodeModuleId(0, &ohms) == ModuleId::Micro, "shorted reads MICRO");
}

static void test_analog_machine_together() {
  const float on_s = 10.0f;
  const float on_h = 20.0f;
  const float hyst = 2.0f;

  auto drive = [](StickMachine& m, bool& soft_held, bool& hard_held, float v, float on_s,
                  float on_h, float hyst) {
    soft_held = analogActive(v, soft_held, on_s, hyst);
    hard_held = analogActive(v, hard_held, on_h, hyst);
    return m.updateDual(soft_held, hard_held);
  };

  StickMachine m;
  bool soft_held = false;
  bool hard_held = false;

  HidAction a = drive(m, soft_held, hard_held, 0, on_s, on_h, hyst);
  CHECK(m.level() == StickLevel::Idle, "rest idle");
  CHECK(a.empty(), "rest no hid");

  a = drive(m, soft_held, hard_held, 10, on_s, on_h, hyst);
  CHECK(m.level() == StickLevel::Soft, "rest->soft");
  CHECK(a.press_soft && !a.press_hard, "press 1 only");

  a = drive(m, soft_held, hard_held, 20, on_s, on_h, hyst);
  CHECK(m.level() == StickLevel::Hard, "soft->hard upgrade");
  CHECK(a.release_soft && a.press_hard && !a.press_soft, "release 1 press 2");

  a = drive(m, soft_held, hard_held, 18.5f, on_s, on_h, hyst);
  CHECK(hard_held, "hard analog still held");
  CHECK(m.level() == StickLevel::Hard, "ease off hard stays Hard");
  CHECK(a.empty(), "no extra HID on ease");

  a = drive(m, soft_held, hard_held, 0, on_s, on_h, hyst);
  CHECK(m.level() == StickLevel::Idle, "full release");
  CHECK(a.release_hard && !a.release_soft, "release 2 only");

  StickMachine slam;
  bool s2 = false;
  bool h2 = false;
  a = drive(slam, s2, h2, 25, on_s, on_h, hyst);
  CHECK(slam.level() == StickLevel::Hard, "slam goes hard");
  CHECK(a.press_hard && !a.press_soft, "slam skips soft");

  StickMachine chatter;
  bool s3 = false;
  bool h3 = false;
  drive(chatter, s3, h3, 10, on_s, on_h, hyst);
  CHECK(chatter.level() == StickLevel::Soft, "at soft on");
  a = drive(chatter, s3, h3, 8.5f, on_s, on_h, hyst);
  CHECK(s3, "hyst holds analog soft");
  CHECK(chatter.level() == StickLevel::Soft, "hyst does not drop");
  CHECK(a.empty(), "hyst does not chatter");
  a = drive(chatter, s3, h3, 10.2f, on_s, on_h, hyst);
  CHECK(chatter.level() == StickLevel::Soft, "still soft");
  CHECK(a.empty(), "no extra press");
  a = drive(chatter, s3, h3, 7.5f, on_s, on_h, hyst);
  CHECK(chatter.level() == StickLevel::Idle, "below hyst releases");
  CHECK(a.release_soft, "release 1 after hyst drop");
}

int main() {
  test_upgrade_no_dual_fire();
  test_no_downgrade();
  test_full_release();
  test_soft_release();
  test_hard_priority();
  test_binary();
  test_debounce();
  test_hysteresis();
  test_lut();
  test_boxcar();
  test_batt_divider();
  test_pack_curves();
  test_profiles();
  test_grip_does_not_fire_hid();
  test_module_id();
  test_analog_machine_together();
  if (g_fails) {
    std::fprintf(stderr, "%d failed\n", g_fails);
    return 1;
  }
  std::puts("ok");
  return 0;
}
