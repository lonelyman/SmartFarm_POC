#include <Arduino.h>
#include "Config.h"
#include "drivers/RtcDs3231Time.h"

// ===================== Constructor =====================

RtcDs3231Time::RtcDs3231Time()
    : _isOk(false)
{
}

// ===================== begin() =====================

bool RtcDs3231Time::begin()
{
   if (!_rtc.begin())
   {
#if DEBUG_TIME_LOG
      Serial.println("⚠️ RTC DS3231: ไม่พบอุปกรณ์บน I2C");
#endif
      _isOk = false;
      return false;
   }

   if (_rtc.lostPower())
   {
#if DEBUG_TIME_LOG
      Serial.println("⚠️ RTC DS3231: เวลาเพิ่ง reset (ต้องตั้งเวลาให้มันก่อน)");
#endif
      // ยังถือว่าใช้งานได้ แต่เวลาอาจไม่ถูก
   }

   _isOk = true;
#if DEBUG_TIME_LOG
   Serial.println("✅ RTC DS3231: ready");
#endif
   return true;
}

// ===================== isOk() =====================

bool RtcDs3231Time::isOk() const
{
   return _isOk;
}

// ===================== getTimeOfDay() =====================

bool RtcDs3231Time::getTimeOfDay(TimeOfDay &out)
{
   if (!_isOk)
   {
#if DEBUG_TIME_LOG
      Serial.println("[RTC] not initialized");
#endif
      return false;
   }

   DateTime now = _rtc.now();

   out.hour = now.hour();
   out.minute = now.minute();
   out.second = now.second();

#if DEBUG_TIME_LOG
   Serial.printf("[RTC] %02u:%02u:%02u\n",
                 (unsigned)out.hour,
                 (unsigned)out.minute,
                 (unsigned)out.second);
#endif
   return true;
}

// ===================== getMinutesOfDay() =====================

bool RtcDs3231Time::getMinutesOfDay(uint16_t &minutes)
{
   TimeOfDay t;
   if (!getTimeOfDay(t))
   {
#if DEBUG_TIME_LOG
      Serial.println("[RTC] getTimeOfDay failed, use 0");
#endif
      minutes = 0;
      return false;
   }

   minutes = toMinutesOfDay(t);

#if DEBUG_TIME_LOG
   Serial.printf("[RTC] minutesOfDay = %u\n", (unsigned)minutes);
#endif
   return true;
}