#pragma once

#include <Arduino.h>
#include "../Config.h"
#include "interfaces/IModeSource.h"

// ============================================================
//  Esp32ModeSwitchSource
//  อ่าน rotary switch 3 ตำแหน่ง (2-bit encode) → SystemMode
//
//  วงจรต่อขา:
//
//    (1)[3V3] ── (1)[R10kΩ](2) ── (2)[GPIO6 (PIN_SW_MODE_A)]
//                                  │
//                                 (2)[A1 / A2](3) ── (3)[GND]
//
//    (1)[3V3] ── (1)[R10kΩ](2) ── (2)[GPIO7 (PIN_SW_MODE_B)]
//                                  │
//                                 (2)[B3 / B4](3) ── (3)[GND]
//
//
//  ใช้ external R10kΩ ร่วมกับ INPUT_PULLUP ภายใน ESP32-S3 (~45kΩ)
//  ต่อขนานกัน ได้ pull-up ที่แข็งแกร่งขึ้น ทนต่อ noise ในตู้ควบคุม (relay/motor/pump)
//
//  Truth table:
//    GPIO_A  GPIO_B  ตำแหน่ง     Mode
//    HIGH    HIGH    กลาง       IDLE
//    LOW     HIGH    บิดซ้าย      AUTO
//    HIGH    LOW     บิดขวา      MANUAL
//    LOW     LOW     (ผิดปกติ)    IDLE  ← fallback ปลอดภัย
// ============================================================

class Esp32ModeSwitchSource : public IModeSource
{
public:
   Esp32ModeSwitchSource(int pinA, int pinB, uint32_t debounceMs = BUTTON_DEBOUNCE_MS);

   void begin() override;
   SystemMode readMode() override;

private:
   int _pinA;
   int _pinB;
   uint32_t _debounceMs;

   SystemMode _stable = SystemMode::IDLE;
   SystemMode _lastRaw = SystemMode::IDLE;
   uint32_t _lastChangeMs = 0;

   SystemMode readRaw() const;
   SystemMode decode(int a, int b) const;
};