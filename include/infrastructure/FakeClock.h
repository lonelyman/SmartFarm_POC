#pragma once

#include "interfaces/IClock.h"

class FakeClock : public IClock
{
public:
   explicit FakeClock(uint16_t fixedMinutesOfDay)
       : _minutes(fixedMinutesOfDay) {}

   void begin() override {}

   bool getMinutesOfDay(uint16_t &outMinutes) override
   {
      outMinutes = _minutes;
      return true;
   }

   bool getTimeOfDay(TimeOfDay &out) override
   {
      out.hour = (uint8_t)(_minutes / 60);
      out.minute = (uint8_t)(_minutes % 60);
      return true;
   }

   bool setTimeOfDay(uint8_t hour, uint8_t minute, uint8_t /*second*/) override
   {
      if (hour > 23 || minute > 59)
         return false;
      _minutes = (uint16_t)hour * 60 + (uint16_t)minute;
      return true;
   }

   bool syncFromNetwork() override { return false; }

private:
   uint16_t _minutes;
};