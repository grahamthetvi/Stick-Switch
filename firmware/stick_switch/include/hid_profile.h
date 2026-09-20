#pragma once

#include "config.h"
#include "settings.h"

#include <cstdint>
#include <cstring>

// BleKeyboard.h KEY_* / MediaKeyReport values (native tests have no Arduino).
constexpr uint8_t kHidTab = 0xB3;
constexpr uint8_t kHidReturn = 0xB0;
constexpr uint8_t kHidF1 = 0xC2;
constexpr uint8_t kHidF2 = 0xC3;
constexpr uint8_t kHidF3 = 0xC4;
constexpr uint8_t kMediaVolDown = 64;
constexpr uint8_t kMediaVolUp = 32;
constexpr uint8_t kMediaPlayPause = 8;

enum class HidKeyKind : uint8_t { Keyboard = 0, Media = 1 };

struct HidKey {
  HidKeyKind kind = HidKeyKind::Keyboard;
  uint8_t kbd = 0;
  uint8_t media0 = 0;
  uint8_t media1 = 0;

  static HidKey keyboard(uint8_t c) {
    HidKey k;
    k.kind = HidKeyKind::Keyboard;
    k.kbd = c;
    return k;
  }

  static HidKey media(uint8_t b0, uint8_t b1 = 0) {
    HidKey k;
    k.kind = HidKeyKind::Media;
    k.media0 = b0;
    k.media1 = b1;
    return k;
  }
};

struct HidKeyset {
  HidKey soft;
  HidKey hard;
  HidKey binary;
};

inline const char* profileName(HidProfile p) {
  switch (p) {
    case HidProfile::IpadOS:
      return "IPADOS";
    case HidProfile::Android:
      return "ANDROID";
    case HidProfile::Function:
      return "FUNCTION";
    case HidProfile::Media:
      return "MEDIA";
    case HidProfile::Custom:
      return "CUSTOM";
    default: {
      const HidProfile unused = p;
      (void)unused;
      return "IPADOS";
    }
  }
}

inline bool parseProfile(const char* s, HidProfile* out) {
  if (s == nullptr || out == nullptr) {
    return false;
  }
  if (strcmp(s, "IPADOS") == 0) {
    *out = HidProfile::IpadOS;
    return true;
  }
  if (strcmp(s, "ANDROID") == 0) {
    *out = HidProfile::Android;
    return true;
  }
  if (strcmp(s, "FUNCTION") == 0) {
    *out = HidProfile::Function;
    return true;
  }
  if (strcmp(s, "MEDIA") == 0) {
    *out = HidProfile::Media;
    return true;
  }
  if (strcmp(s, "CUSTOM") == 0) {
    *out = HidProfile::Custom;
    return true;
  }
  return false;
}

inline HidKeyset keysetFor(HidProfile p, const StickSettings& s) {
  switch (p) {
    case HidProfile::IpadOS:
      return {HidKey::keyboard(static_cast<uint8_t>(kKeySoft)),
              HidKey::keyboard(static_cast<uint8_t>(kKeyHard)),
              HidKey::keyboard(static_cast<uint8_t>(kKeyBinary))};
    case HidProfile::Android:
      return {HidKey::keyboard(kHidTab), HidKey::keyboard(kHidReturn),
              HidKey::keyboard(kHidReturn)};
    case HidProfile::Function:
      return {HidKey::keyboard(kHidF1), HidKey::keyboard(kHidF2),
              HidKey::keyboard(kHidF3)};
    case HidProfile::Media:
      return {HidKey::media(kMediaVolDown), HidKey::media(kMediaVolUp),
              HidKey::media(kMediaPlayPause)};
    case HidProfile::Custom:
      return {HidKey::keyboard(static_cast<uint8_t>(s.key_soft)),
              HidKey::keyboard(static_cast<uint8_t>(s.key_hard)),
              HidKey::keyboard(static_cast<uint8_t>(s.key_binary))};
    default: {
      const HidProfile unused = p;
      (void)unused;
      return {HidKey::keyboard(static_cast<uint8_t>(kKeySoft)),
              HidKey::keyboard(static_cast<uint8_t>(kKeyHard)),
              HidKey::keyboard(static_cast<uint8_t>(kKeyBinary))};
    }
  }
}
