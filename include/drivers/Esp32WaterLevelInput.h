// include/drivers/Esp32WaterLevelInput.h
#pragma once

#include <Arduino.h>
#include "../Config.h"

// ============================================================
//  Esp32WaterLevelInput
//  อ่านเซนเซอร์ระดับน้ำ XKC-Y25 แบบ digital 2 ช่อง (CH1, CH2)
//
//  วงจรต่อขา (ใช้เหมือนกันทั้ง CH1 และ CH2):
//
//    [VCC 5V/12V] ── [XKC-Y25 VCC]
//    [GND]        ── [XKC-Y25 GND]
//    (1)[3V3] ──(1)[R10kΩ](2)──(2)[GPIO]
//                               │
//                              (2)[XKC-Y25 OUT](3)──(3)[GND]
//
//  ⚠️  XKC-Y25 มี 2 output type — ตรวจสอบรุ่นก่อนใช้:
//    - Push-pull      : ไม่ต้องการ pull-up ภายนอก
//    - Open-collector : ต้องการ pull-up → ใช้ INPUT_PULLUP หรือ R ภายนอก
//  ✅ code ใช้ INPUT_PULLUP เป็น default (ปลอดภัยทั้ง 2 กรณี)
//
//  Pins (ยึดตาม Config.h):
//    CH1 : GPIO1
//    CH2 : GPIO2
//
//  Enable/Disable แต่ละช่องด้วย ENABLE_WATER_LEVEL_CH1/CH2 ใน Config.h
//  ปิดช่องที่ไม่มีเซนเซอร์เพื่อลด false alarm
//
//  ขั้วสัญญาณ alarm ปรับได้ด้วย WATER_LEVEL_ALARM_LOW ใน Config.h:
//    true  = LOW  = น้ำต่ำ (alarm)
//    false = HIGH = น้ำต่ำ (alarm)
// ============================================================

struct WaterLevelReadings
{
      bool ch1Low = false; // true = น้ำต่ำกว่าระดับ CH1
      bool ch2Low = false; // true = น้ำต่ำกว่าระดับ CH2
};

class Esp32WaterLevelInput
{
public:
      Esp32WaterLevelInput(int ch1Pin, int ch2Pin)
          : _ch1Pin(ch1Pin), _ch2Pin(ch2Pin) {}

      // init pins — เรียกครั้งเดียวตอน boot
      void begin()
      {
            pinMode(_ch1Pin, INPUT_PULLUP);
            pinMode(_ch2Pin, INPUT_PULLUP);
      }

      // อ่านสถานะทั้ง 2 ช่อง — ch1Low/ch2Low = true เมื่อน้ำต่ำ
      WaterLevelReadings read() const
      {
            WaterLevelReadings r{};

#if ENABLE_WATER_LEVEL_CH1
            const bool rawLow1 = (digitalRead(_ch1Pin) == LOW);
            r.ch1Low = WATER_LEVEL_ALARM_LOW ? rawLow1 : !rawLow1;
#else
            r.ch1Low = false; // ปิดช่อง → ไม่มี alarm
#endif

#if ENABLE_WATER_LEVEL_CH2
            const bool rawLow2 = (digitalRead(_ch2Pin) == LOW);
            r.ch2Low = WATER_LEVEL_ALARM_LOW ? rawLow2 : !rawLow2;
#else
            r.ch2Low = false; // ปิดช่อง → ไม่มี alarm
#endif

            return r;
      }

private:
      int _ch1Pin;
      int _ch2Pin;
};