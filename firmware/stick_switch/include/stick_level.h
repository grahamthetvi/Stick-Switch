#pragma once

#include "config.h"

#include <cstdint>

enum class ModuleId : uint8_t {
  Empty = 0,
  Micro = 1,
  Fsr = 2,
  Hall = 3,
  Load = 4,
};

// Soft->hard UPGRADE, never dual-fire, never mid-press downgrade.
// Idle --soft--> Soft (press 1)
// Soft --hard--> Hard (release 1, press 2)
// Hard stays until full release (both inputs inactive).
// Full release returns to Idle and releases whichever key is held.

enum class StickLevel : uint8_t { Idle = 0, Soft = 1, Hard = 2 };

struct HidAction {
  bool release_soft = false;
  bool press_soft = false;
  bool release_hard = false;
  bool press_hard = false;
  bool release_binary = false;
  bool press_binary = false;

  bool empty() const {
    return !release_soft && !press_soft && !release_hard && !press_hard &&
           !release_binary && !press_binary;
  }
};

class StickMachine {
 public:
  StickLevel level() const { return level_; }

  // Dual-level path. `soft` / `hard` are already debounced, active-high.
  HidAction updateDual(bool soft, bool hard) {
    HidAction a;
    const StickLevel prev = level_;

    if (level_ == StickLevel::Hard) {
      if (!soft && !hard) {
        level_ = StickLevel::Idle;
      }
    } else if (hard) {
      level_ = StickLevel::Hard;
    } else if (level_ == StickLevel::Idle && soft) {
      level_ = StickLevel::Soft;
    } else if (level_ == StickLevel::Soft && !soft) {
      level_ = StickLevel::Idle;
    }

    fillDualKeys(prev, level_, a);
    return a;
  }

  // Binary path: any activation is a single key. APH and wired-OR share this.
  HidAction updateBinary(bool active) {
    HidAction a;
    if (active && !binary_held_) {
      binary_held_ = true;
      a.press_binary = true;
    } else if (!active && binary_held_) {
      binary_held_ = false;
      a.release_binary = true;
    }
    return a;
  }

  void reset() {
    level_ = StickLevel::Idle;
    binary_held_ = false;
  }

  static void fillDualKeys(StickLevel prev, StickLevel now, HidAction& a) {
    if (prev == now) {
      return;
    }
    if (prev == StickLevel::Soft) {
      a.release_soft = true;
    }
    if (prev == StickLevel::Hard) {
      a.release_hard = true;
    }
    if (now == StickLevel::Soft) {
      a.press_soft = true;
    }
    if (now == StickLevel::Hard) {
      a.press_hard = true;
    }
  }

 private:
  StickLevel level_ = StickLevel::Idle;
  bool binary_held_ = false;
};

class Debounce {
 public:
  explicit Debounce(uint32_t ms) : ms_(ms) {}

  bool update(bool raw, uint32_t now_ms) {
    if (raw != candidate_) {
      candidate_ = raw;
      since_ms_ = now_ms;
    }
    if (candidate_ != stable_ && (now_ms - since_ms_) >= ms_) {
      stable_ = candidate_;
    }
    return stable_;
  }

  bool value() const { return stable_; }

 private:
  uint32_t ms_;
  bool candidate_ = false;
  bool stable_ = false;
  uint32_t since_ms_ = 0;
};

class BoxcarFilter {
 public:
  explicit BoxcarFilter(float denom) : denom_(denom) {}

  float push(float sample) {
    filt_ += (sample - filt_) / denom_;
    return filt_;
  }

  float value() const { return filt_; }
  void set(float v) { filt_ = v; }

 private:
  float denom_;
  float filt_ = 0.0f;
};

inline bool analogActive(float value, bool currently, float on, float hyst) {
  if (currently) {
    return value >= (on - hyst);
  }
  return value >= on;
}

// 100k/100k divider from pack +. Approximate; ESP32 ADC is uncalibrated.
inline int battMvFromAdc(int adc, int vref_mv = 3300, int full = 4095) {
  if (adc < 0) {
    return 0;
  }
  return static_cast<int>((static_cast<long>(adc) * vref_mv * 2) / full);
}

inline int packEmptyMv(PackType p) {
  return p == PackType::Cell3 ? kPack3EmptyMv : kPack2EmptyMv;
}

inline int packFullMv(PackType p) {
  return p == PackType::Cell3 ? kPack3FullMv : kPack2FullMv;
}

inline int battPctFromMv(int mv, PackType p) {
  if (p == PackType::Usb) {
    return 100;
  }
  const int empty = packEmptyMv(p);
  const int full = packFullMv(p);
  if (mv <= empty) {
    return 0;
  }
  if (mv >= full) {
    return 100;
  }
  return (mv - empty) * 100 / (full - empty);
}

inline bool battIsLow(int mv, PackType p) {
  if (p == PackType::Usb) {
    return false;
  }
  return battPctFromMv(mv, p) <= kBattLowPct;
}

inline PackType autoPickPack(int mv) {
  return mv >= kPackAuto3Mv ? PackType::Cell3 : PackType::Cell2;
}

inline uint8_t packCells(PackType p) {
  if (p == PackType::Usb) {
    return 0;
  }
  return p == PackType::Cell3 ? 3 : 2;
}

// Capacitive grip: touchRead falls when holding. Never feeds StickMachine.
inline bool gripHolding(uint16_t raw, uint16_t thresh) {
  return raw < thresh;
}

// 47k pull-up to 3V3 on the chassis; module resistor to GND.
// MICRO 10k, FSR 22k, HALL 47k, LOAD 100k, empty = open.
inline ModuleId decodeModuleId(int adc, int* ohms_out = nullptr, int rpu = 47000,
                               int full = 4095) {
  int ohms = -1;
  if (adc < 1) {
    ohms = 0;
  } else if (adc >= full - 8) {
    ohms = -1;
  } else {
    ohms = static_cast<int>((static_cast<long>(rpu) * adc) / (full - adc));
  }
  if (ohms_out != nullptr) {
    *ohms_out = ohms;
  }

  if (adc >= 3600) {
    return ModuleId::Empty;
  }
  if (ohms < 16000) {
    return ModuleId::Micro;
  }
  if (ohms < 34500) {
    return ModuleId::Fsr;
  }
  if (ohms < 73500) {
    return ModuleId::Hall;
  }
  return ModuleId::Load;
}

inline const char* moduleIdName(ModuleId id) {
  switch (id) {
    case ModuleId::Empty:
      return "EMPTY";
    case ModuleId::Micro:
      return "MICRO";
    case ModuleId::Fsr:
      return "FSR";
    case ModuleId::Hall:
      return "HALL";
    case ModuleId::Load:
      return "LOAD";
    default: {
      const ModuleId unused = id;
      (void)unused;
      return "EMPTY";
    }
  }
}

// 5-point LUT: adc[i] -> mm[i], both strictly increasing after invert.
inline float lutInterp(const float* x, const float* y, int n, float v) {
  if (n < 2) {
    return y[0];
  }
  if (v <= x[0]) {
    return y[0];
  }
  if (v >= x[n - 1]) {
    return y[n - 1];
  }
  for (int i = 0; i < n - 1; ++i) {
    if (v <= x[i + 1]) {
      const float t = (v - x[i]) / (x[i + 1] - x[i]);
      return y[i] + t * (y[i + 1] - y[i]);
    }
  }
  return y[n - 1];
}
