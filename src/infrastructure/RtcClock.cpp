// src/infrastructure/RtcClock.cpp
#include "infrastructure/RtcClock.h"

RtcClock::RtcClock(RtcDs3231Time &rtc)
    : _rtc(rtc) {}

void RtcClock::begin()
{
   _rtc.begin();
}

bool RtcClock::getMinutesOfDay(uint16_t &outMinutes)
{
   return _rtc.getMinutesOfDay(outMinutes);
}

bool RtcClock::getTimeOfDay(TimeOfDay &out)
{
   return _rtc.getTimeOfDay(out);
}

bool RtcClock::setTimeOfDay(uint8_t hour, uint8_t minute, uint8_t second)
{
   return _rtc.setTimeOfDay(hour, minute, second);
}

bool RtcClock::syncFromNetwork()
{
   // NetworkTask ensure STA connected ก่อนส่ง SyncNtp command
   return NetTimeSync::syncRtcFromNtp(_rtc);
}