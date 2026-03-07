// src/drivers/RtcDs3231Time.cpp
#include <Arduino.h>
#include "Config.h"
#include "drivers/RtcDs3231Time.h"

RtcDs3231Time::RtcDs3231Time()
    : _isOk(false) {}

bool RtcDs3231Time::begin()
{
   if (!_rtc.begin())
   {
      Serial.println("⚠️ [RTC] ไม่พบ DS3231 บน I2C");
      _isOk = false;
      return false;
   }

   if (_rtc.lostPower())
   {
      // ไฟหาย / แบตหมด → ตั้งเวลาจากค่าตอน compile เป็น fallback
      Serial.println("⚠️ [RTC] lostPower → ตั้งเวลาจาก compile time");
      _rtc.adjust(DateTime(__DATE__, __TIME__));
   }

   _isOk = true;
   Serial.println("✅ [RTC] DS3231 ready");
   return true;
}

bool RtcDs3231Time::isOk() const
{
   return _isOk;
}

bool RtcDs3231Time::getTimeOfDay(TimeOfDay &out)
{
   if (!_isOk)
   {
#if DEBUG_TIME_LOG
      Serial.println("[RTC] not initialized");
#endif
      return false;
   }

   const DateTime now = _rtc.now();
   out.hour = now.hour();
   out.minute = now.minute();
   out.second = now.second();

#if DEBUG_TIME_LOG
   Serial.printf("[RTC] %02u:%02u:%02u\n", (unsigned)out.hour, (unsigned)out.minute, (unsigned)out.second);
#endif
   return true;
}

bool RtcDs3231Time::getMinutesOfDay(uint16_t &minutes)
{
   TimeOfDay t;
   if (!getTimeOfDay(t))
   {
      minutes = 0;
      return false;
   }

   minutes = toMinutesOfDay(t);

#if DEBUG_TIME_LOG
   Serial.printf("[RTC] minutesOfDay=%u\n", (unsigned)minutes);
#endif
   return true;
}

bool RtcDs3231Time::setTimeOfDay(uint8_t hour, uint8_t minute, uint8_t second)
{
   if (!_isOk)
   {
#if DEBUG_TIME_LOG
      Serial.println("[RTC] setTimeOfDay: not initialized");
#endif
      return false;
   }

   // คง year/month/day เดิมไว้ — เปลี่ยนแค่เวลา
   const DateTime now = _rtc.now();
   _rtc.adjust(DateTime(now.year(), now.month(), now.day(), hour, minute, second));

#if DEBUG_TIME_LOG
   Serial.printf("[RTC] setTimeOfDay → %02u:%02u:%02u\n", (unsigned)hour, (unsigned)minute, (unsigned)second);
#endif
   return true;
}