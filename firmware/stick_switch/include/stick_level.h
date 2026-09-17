#pragma once

#include <cstdint>

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
