// include/infrastructure/RtcClock.h
#pragma once

#include "interfaces/IClock.h"
#include "infrastructure/NetTimeSync.h"
#include "drivers/RtcDs3231Time.h"

// ============================================================
//  RtcClock
//  ครอบ RtcDs3231Time และ NetTimeSync เข้าด้วยกัน
//  เป็น concrete implementation ของ IClock สำหรับ ESP32-S3
//
//  RtcDs3231Time — อ่าน/เขียนเวลากับ DS3231 ผ่าน I2C
//  NetTimeSync   — sync เวลาจาก NTP เมื่อ STA connected
//
//  วงจรต่อขา DS3231:
//    [DS3231 VCC] ──── [3.3V]
//    [DS3231 GND] ──── [GND]
//    [DS3231 SDA](1) ──(1)[GPIO8 (SDA)](1.1)──(1.1)[R4.7kΩ](1.2)──(1.2)[3V3]
//    [DS3231 SCL](2) ──(2)[GPIO9 (SCL)](2.1)──(2.1)[R4.7kΩ](2.2)──(2.2)[3V3]
//
//  ⚠️  R4.7kΩ ค่อมครั้งเดียวที่ bus — ไม่ต้องค่อมซ้ำถ้ามีหลาย device (BH1750, SHT40)
//  ⚠️  Wire.begin() ต้องทำใน AppBoot เท่านั้น
// ============================================================

class RtcClock : public IClock
{
public:
   explicit RtcClock(RtcDs3231Time &rtc);

   void begin() override;

   bool getMinutesOfDay(uint16_t &outMinutes) override;
   bool getTimeOfDay(TimeOfDay &out) override;
   bool setTimeOfDay(uint8_t hour, uint8_t minute, uint8_t second) override;

   // sync เวลาจาก NTP → เขียนลง DS3231
   bool syncFromNetwork() override;

private:
   RtcDs3231Time &_rtc;
};