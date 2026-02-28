#pragma once

#include "interfaces/IClock.h"

#include "infrastructure/NetTimeSync.h"

#include "drivers/RtcDs3231Time.h"

class RtcClock : public IClock
{
public:
   explicit RtcClock(RtcDs3231Time &rtc) : _rtc(rtc) {}

   void begin() override { _rtc.begin(); }

   bool getMinutesOfDay(uint16_t &outMinutes) override
   {
      return _rtc.getMinutesOfDay(outMinutes);
   }

   bool getTimeOfDay(TimeOfDay &out) override
   {
      return _rtc.getTimeOfDay(out);
   }

   bool setTimeOfDay(uint8_t hour, uint8_t minute, uint8_t second) override
   {
      // RtcDs3231Time ของคุณมี setTimeOfDay(h,m,s) อยู่แล้วจากเดิม
      return _rtc.setTimeOfDay(hour, minute, second);
   }

   bool syncFromNetwork() override
   {
      // NetworkTask จะ ensureConnected มาแล้ว
      return NetTimeSync::syncRtcFromNtp(_rtc);
   }

private:
   RtcDs3231Time &_rtc;
};