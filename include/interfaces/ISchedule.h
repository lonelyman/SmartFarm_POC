// include/interfaces/ISchedule.h
#pragma once

#include <stdint.h>
#include "domain/TimeWindow.h"

// ============================================================
//  ISchedule — Interface สำหรับตารางเวลา
//
//  Implementation ที่มีอยู่:
//    - TimeSchedule : ตารางเวลาแบบ window list (domain/TimeSchedule)
//
//  ใช้งาน:
//    - ScheduledRelay รับ ISchedule* เพื่อตัดสินใจ turnOn/turnOff
//    - ScheduleStore โหลด JSON แล้วเติมข้อมูลเข้า TimeSchedule
// ============================================================

class ISchedule
{
public:
   virtual ~ISchedule() = default;

   // ถามว่า minutesOfDay อยู่ใน window ที่ active ไหม
   virtual bool isInWindow(uint16_t minutesOfDay) const = 0;

   // เพิ่ม window — เรียกจาก ScheduleStore ตอนโหลด JSON
   virtual void addWindow(uint16_t startMin, uint16_t endMin) = 0;

   // เคลียร์ windows ทั้งหมด — ใช้ตอน reload schedule
   virtual void clear() = 0;

   // true = schedule นี้ active — false = bypass ทุก window (ปิดทั้งหมด)
   virtual bool isEnabled() const = 0;
   virtual void setEnabled(bool enabled) = 0;
};