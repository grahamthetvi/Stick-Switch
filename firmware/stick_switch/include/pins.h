#pragma once

#include <cstdint>

// ESP32 DevKit V1 / WROOM. ADC1 only — BLE uses the radio; ADC2 is forbidden.

constexpr int PIN_SW_SOFT = 18;
constexpr int PIN_SW_HARD = 19;
constexpr int PIN_APH_BACKUP = 33;
constexpr int PIN_ADC_SENSOR = 32;  // ADC1_CH4
constexpr int PIN_I2C_SDA = 21;
constexpr int PIN_I2C_SCL = 22;
constexpr int PIN_BTN_TARE = 4;
constexpr int PIN_BTN_CAL = 23;
constexpr int PIN_LED_SOFT = 25;
constexpr int PIN_LED_HARD = 26;
constexpr int PIN_JACK_OUT = 27;  // 2N7000 gate for analog modules

// 8-pin module header, rear of bay, pin 1 = 3V3:
// 1 3V3 | 2 GND | 3 SOFT | 4 HARD | 5 ADC | 6 SDA | 7 SCL | 8 ID (unused; mode is NVS)
