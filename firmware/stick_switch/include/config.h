#pragma once

#include <cstdint>

enum class StickMode : uint8_t {
  Micro = 0,
  Fsr = 1,
  Hall = 2,
  Load = 3,
  Binary = 4,
};

constexpr const char* kBleName = "Stick-Switch";
constexpr char kKeySoft = '1';
constexpr char kKeyHard = '2';
constexpr char kKeyBinary = ' ';

constexpr uint32_t kDebounceMs = 15;
constexpr uint32_t kAdcHz = 200;
constexpr uint8_t kBoxcar = 8;
constexpr float kHysteresisFrac = 0.08f;

constexpr int kFsrSoftCounts = 250;
constexpr int kFsrHardCounts = 1200;
constexpr float kHallSoftMm = 5.0f;
constexpr float kHallHardMm = 11.0f;
constexpr float kHallHystMm = 0.8f;
constexpr float kLoadSoftGf = 120.0f;
constexpr float kLoadHardGf = 300.0f;
constexpr float kLoadHystGf = 25.0f;
constexpr float kLoadQuietGf = 15.0f;
constexpr uint32_t kLoadSlowTareMs = 3000;

constexpr uint8_t kHallLutPoints = 5;
constexpr uint16_t kCalSamples = 5;
constexpr uint32_t kTareWindowMs = 1000;
