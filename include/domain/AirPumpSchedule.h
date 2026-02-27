#ifndef AIR_PUMP_SCHEDULE_H
#define AIR_PUMP_SCHEDULE_H

#include <Arduino.h>

// หนึ่งช่วงเวลา (นาทีของวัน)
struct TimeWindow
{
   uint16_t startMin; // รวมตอน start
   uint16_t endMin;   // ไม่รวม end (เหมือน [start, end) )
};

// ตารางเวลาปั๊มลม
struct AirPumpSchedule
{
   static constexpr uint8_t MAX_WINDOWS = 24;

   bool enabled = false;    // true = ใช้ schedule นี้
   uint8_t windowCount = 0; // จำนวน window ที่ใช้งานจริง
   TimeWindow windows[MAX_WINDOWS];
};

#endif