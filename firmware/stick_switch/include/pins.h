#pragma once

#include <cstdint>

// ESP32 DevKit V1 / WROOM. ADC1 only — BLE uses the radio; ADC2 is forbidden.
// GPIO0 is the ESP32 BOOT strap; pinMode runs in setup() after the bootloader.

constexpr int PIN_SW_SOFT = 18;
constexpr int PIN_SW_HARD = 19;
constexpr int PIN_APH_BACKUP = 33;
constexpr int PIN_ADC_SENSOR = 32;  // ADC1_CH4
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;
constexpr int PIN_BTN_TARE = 4;
constexpr int PIN_BTN_CAL = 23;
constexpr int PIN_BTN_BOOT = 0;     // 1 tap soft, 2 taps hard, hold tare/cal
constexpr int PIN_LED_SOFT = 25;
constexpr int PIN_LED_HARD = 26;
constexpr int PIN_LED_GRIP = 16;    // indicator only — never HID, never jack
constexpr int PIN_JACK_OUT = 27;    // 2N7000 gate for analog modules
constexpr int PIN_ADC_BATT = 34;    // ADC1_CH6, 100k/100k from pack + (before boost)
constexpr int PIN_MODULE_ID = 35;   // ADC1_CH7, header pin 8; 47k pull-up on chassis
constexpr int PIN_TOUCH_GRIP = 14;  // T6, isolated 6061 tube through PETG collar
constexpr int PIN_BUZZER = 13;      // optional piezo; silent if unpopulated

// 8-pin module header, rear of bay, pin 1 = 3V3:
// 1 3V3 | 2 GND | 3 SOFT | 4 HARD | 5 ADC | 6 SDA | 7 SCL | 8 ID (GPIO35)
