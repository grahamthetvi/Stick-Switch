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
  if (g_fails) {
    std::fprintf(stderr, "%d failed\n", g_fails);
    return 1;
  }
  std::puts("ok");
  return 0;
}
