// include/drivers/Esp32ManualSwitch.h
#pragma once

#include <Arduino.h>
#include "../Config.h"

// ============================================================
//  Esp32ManualSwitch
//  อ่าน toggle switch 2 ตำแหน่ง → ON / OFF
//
//  วงจรต่อขา (ตัวอย่าง PIN_SW_MANUAL_PUMP):
//
//    [3V3](1) ── (1)[R10kΩ](2) ── (2)[GPIO18 (PIN_SW_MANUAL_PUMP)]
//                                  │
//                                 (2)[ขา1 / ขา2](3) ── (3)[GND]
//
//  ใช้ external R10kΩ ร่วมกับ INPUT_PULLUP ภายใน ESP32-S3 (~45kΩ)
//  ต่อขนานกัน ได้ pull-up ที่แข็งแกร่งขึ้น ทนต่อ noise ในตู้ควบคุม
//
//  Truth table:
//    สวิตช์      วงจร    GPIO    isOn()
//    บิดซ้าย    เปิด     HIGH    false  (OFF)
//    บิดขวา     ปิด      LOW     true   (ON)
// ============================================================

class Esp32ManualSwitch
{
public:
   Esp32ManualSwitch(int pin, uint32_t debounceMs = BUTTON_DEBOUNCE_MS);

   // init pins — เรียกครั้งเดียวตอน boot
   void begin();

   // คืน true เมื่อสวิตช์อยู่ตำแหน่ง ON (บิดขวา = LOW)
   bool isOn();

private:
   int _pin;
   uint32_t _debounceMs;

   bool _stable = false;
   bool _lastRaw = false;
   uint32_t _lastChangeMs = 0;

   bool readRaw() const;
};