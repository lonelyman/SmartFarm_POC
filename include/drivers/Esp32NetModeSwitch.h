// include/drivers/Esp32NetModeSwitch.h
#pragma once

#include <Arduino.h>
#include "../Config.h"
#include "interfaces/INetModeSource.h"

// ============================================================
//  Esp32NetModeSwitch
//  อ่าน toggle switch 2 ตำแหน่ง → NetDesiredMode
//
//  วงจรต่อขา:
//
//    (1)[3V3] ── (1)[R10kΩ](2) ── (2)[GPIO39 (PIN_SW_NET)]
//                                  │
//                                 (2)[ขา1 / ขา2](3) ── (3)[GND]
//
//  ใช้ external R10kΩ ร่วมกับ INPUT_PULLUP ภายใน ESP32-S3 (~45kΩ)
//  ต่อขนานกัน ได้ pull-up ที่แข็งแกร่งขึ้น ทนต่อ noise ในตู้ควบคุม
//
//  ⚠️  GPIO39 เป็น input-only — ห้ามใช้เป็น output
//
//  Truth table:
//    สวิตช์       วงจร    GPIO39   Mode
//    บิดซ้าย     เปิด     HIGH     AP_PRIMARY
//    บิดขวา      ปิด      LOW      STA_PREFERRED
// ============================================================

class Esp32NetModeSwitch : public INetModeSource
{
public:
   Esp32NetModeSwitch(int pin, uint32_t debounceMs = NET_SWITCH_DEBOUNCE_MS);

   void begin() override;
   NetDesiredMode read() override;

private:
   int _pin;
   uint32_t _debounceMs;

   NetDesiredMode _stable = NetDesiredMode::AP_PRIMARY;
   NetDesiredMode _lastRaw = NetDesiredMode::AP_PRIMARY;
   uint32_t _lastChangeMs = 0;

   NetDesiredMode readRaw() const;
};