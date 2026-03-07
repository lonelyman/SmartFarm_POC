// include/application/ScheduledRelay.h
#pragma once

#include "interfaces/ISchedule.h"
#include "interfaces/IActuator.h"

// ============================================================
//  ScheduledRelay — Application Layer
//  เอา ISchedule + IActuator มาต่อกัน
//
//  ไม่แตะ hardware ตรง — ตัดสินใจผ่าน interface เท่านั้น
//  ทดสอบได้โดย mock ISchedule และ IActuator
//
//  ใช้งาน:
//    static TimeSchedule   airSchedule;
//    static ScheduledRelay scheduledAirPump(airPump, airSchedule);
//
//    // ใน FarmManager AUTO mode:
//    scheduledAirPump.update(minutesOfDay);
//
//  อนาคต — เพิ่ม device ใหม่โดยไม่แก้ FarmManager:
//    static TimeSchedule   lightSchedule;
//    static ScheduledRelay scheduledLight(lightRelay, lightSchedule);
// ============================================================

class ScheduledRelay
{
public:
   ScheduledRelay(IActuator &relay, ISchedule &schedule);

   // เรียกทุก loop ใน AUTO mode — turnOn/turnOff ตาม schedule
   void update(uint16_t minutesOfDay);

   bool isOn() const;

   // ให้ AppBoot / ScheduleStore โหลดข้อมูลเข้า schedule โดยตรง
   ISchedule &getSchedule();

private:
   IActuator &_relay;
   ISchedule &_schedule;
};