#pragma once

#include <Arduino.h>

class RtcDs3231Time;

// โมดูลนี้ดูแลเรื่อง WiFi + NTP + sync RTC
namespace NetTimeSync
{
   // ถ้าจำเป็นจะเชื่อม WiFi ให้ (lazy connect)
   void connectWifiIfNeeded();

   // sync เวลา NTP → RTC DS3231
   void syncRtcFromNtp(RtcDs3231Time &rtc);
}