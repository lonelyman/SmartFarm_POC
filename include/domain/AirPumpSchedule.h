#ifndef AIR_PUMP_SCHEDULE_H
#define AIR_PUMP_SCHEDULE_H

#include <Arduino.h>

struct TimeWindow
{
   uint16_t startMin; // นาทีของวัน (0..1439)
   uint16_t endMin;   // นาทีของวัน (0..1439)
};

struct AirPumpSchedule
{
   static const size_t MAX_WINDOWS = 8;

   bool enabled = false;
   size_t windowCount = 0;
   TimeWindow windows[MAX_WINDOWS];
};

// โหลดตารางเวลาจาก /schedule.json (SPIFFS)
// ถ้าโหลดไม่สำเร็จ ให้คืน false แล้วเดี๋ยวเราใช้ค่า default แทน
bool loadAirPumpSchedule(AirPumpSchedule &out);

#endif