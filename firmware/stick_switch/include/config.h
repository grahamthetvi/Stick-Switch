#pragma once

#include <cstdint>

enum class StickMode : uint8_t {
  Micro = 0,
  Fsr = 1,
  Hall = 2,
  Load = 3,
  Binary = 4,
};

enum class PackType : uint8_t {
  Unset = 0,
  Cell2 = 2,
  Cell3 = 3,
};

enum class HidProfile : uint8_t {
  IpadOS = 0,
  Android = 1,
  Function = 2,
  Media = 3,
  Custom = 4,
};

constexpr const char* kBleName = "Stick-Switch";
constexpr const char* kBleManufacturer = "Graham Labs";
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

constexpr int kAdcVrefMv = 3300;
constexpr int kAdcFullScale = 4095;
constexpr int kModuleIdPullupOhms = 47000;
constexpr uint32_t kBattPollMs = 500;
constexpr uint32_t kBattBlinkMs = 700;
constexpr uint8_t kBattLowPct = 15;

// Alkaline-ish AA series. 2x empty ~2.0 V full ~3.2 V; 3x empty ~3.0 V full ~4.8 V.
constexpr int kPack2EmptyMv = 2000;
constexpr int kPack2FullMv = 3200;
constexpr int kPack3EmptyMv = 3000;
constexpr int kPack3FullMv = 4800;
constexpr int kPackAuto3Mv = 3300;

constexpr uint16_t kGripThreshDefault = 40;
constexpr uint32_t kBootHoldMs = 800;
constexpr uint32_t kBootTapGapMs = 350;
constexpr uint32_t kSimPulseMs = 100;
constexpr uint8_t kBuzzerChannel = 4;
