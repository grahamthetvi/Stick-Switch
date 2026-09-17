#pragma once

#include "config.h"

#include <cstdint>

struct StickSettings {
  StickMode mode = StickMode::Micro;
  bool invert_switches = false;
  bool invert_analog = false;
  bool invert_load = false;
  char key_soft = kKeySoft;
  char key_hard = kKeyHard;
  char key_binary = kKeyBinary;
  int32_t tare_adc = 0;
  int32_t fsr_soft_on = kFsrSoftCounts;
  int32_t fsr_hard_on = kFsrHardCounts;
  float hall_soft_mm = kHallSoftMm;
  float hall_hard_mm = kHallHardMm;
  float hall_adc[kHallLutPoints] = {1800, 2000, 2300, 2700, 3100};
  float hall_mm[kHallLutPoints] = {0, 3, 6, 9, 12};
  int32_t load_tare = 0;
  float load_scale = 1.0f;
  float load_soft_gf = kLoadSoftGf;
  float load_hard_gf = kLoadHardGf;
};
