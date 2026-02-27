#pragma once
#include <stdint.h>
#include "Types.h" // ใช้ TimeOfDay ที่มีอยู่แล้ว

class IClock
{
public:
   virtual ~IClock() = default;

   virtual void begin() = 0;

   virtual bool getMinutesOfDay(uint16_t &outMinutes) = 0;
   virtual bool getTimeOfDay(TimeOfDay &out) = 0;

   virtual bool setTimeOfDay(uint8_t hour, uint8_t minute, uint8_t second) = 0;
};