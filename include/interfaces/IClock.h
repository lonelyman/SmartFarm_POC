// include/interfaces/IClock.h
#pragma once

#include <stdint.h>
#include "Types.h"

// ============================================================
//  IClock — Interface สำหรับอ่านและตั้งเวลา
//
//  Implementation ที่มีอยู่:
//    - RtcClock : ครอบ RtcDs3231Time + NetTimeSync
//
//  ใช้งาน:
//    - clock อยู่ใน SystemContext
//    - controlTask เรียก getMinutesOfDay() ทุก loop
//    - controlTask เรียก setTimeOfDay() เมื่อมี TimeSetRequest จาก CommandTask / WebUI
//    - NetworkTask เรียก syncFromNetwork() เมื่อได้รับ SyncNtp command
// ============================================================

class IClock
{
public:
   virtual ~IClock() = default;

   // init hardware — เรียกครั้งเดียวตอน boot
   virtual void begin() = 0;

   // อ่านเวลาเป็นนาทีของวัน (0–1439) — ใช้กับตารางเวลา air pump
   virtual bool getMinutesOfDay(uint16_t &outMinutes) = 0;

   // อ่านเวลาแบบ HH:MM:SS
   virtual bool getTimeOfDay(TimeOfDay &out) = 0;

   // ตั้งเวลา — เรียกจาก controlTask เมื่อมี TimeSetRequest
   virtual bool setTimeOfDay(uint8_t hour, uint8_t minute, uint8_t second) = 0;

   // sync เวลาจาก NTP — เรียกจาก NetworkTask เมื่อ STA connected
   virtual bool syncFromNetwork() = 0;
};