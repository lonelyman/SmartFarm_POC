#ifndef AIR_PUMP_SCHEDULE_H
#define AIR_PUMP_SCHEDULE_H

#include <Arduino.h>

// หนึ่งช่วงเวลาเปิดปั๊มลม เช่น 07:00–12:00
struct TimeWindow
{
   uint16_t startMin; // นาทีตั้งแต่ 00:00
   uint16_t endMin;   // นาทีตั้งแต่ 00:00 (ต้อง > startMin)
};

// ตารางเวลาปั๊มลม ทั้งวัน
struct AirPumpSchedule
{
   static constexpr uint8_t MAX_WINDOWS = 8;

   bool enabled;                    // เผื่ออนาคตอยากปิด scheduler ทั้งก้อน
   TimeWindow windows[MAX_WINDOWS]; // fixed array
   uint8_t windowCount;             // ใช้จริงกี่ช่วง

   AirPumpSchedule()
       : enabled(true), windowCount(0)
   {
   }

   // ใช้ถามว่า นาทีของวันตอนนี้ อยู่ใน “ช่วงเปิดปั๊มลม” ใด ๆ ไหม
   bool isWithinAnyWindow(uint16_t minutesOfDay) const
   {
      for (uint8_t i = 0; i < windowCount; ++i)
      {
         const TimeWindow &tw = windows[i];
         if (minutesOfDay >= tw.startMin && minutesOfDay < tw.endMin)
         {
            return true;
         }
      }
      return false;
   }
};

#endif