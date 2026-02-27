#include <Arduino.h>
#include "Config.h"
#include "drivers/RtcDs3231Time.h"

// ===================== Constructor =====================

RtcDs3231Time::RtcDs3231Time()
    : _isOk(false)
{
}

// ===================== begin() =====================

// bool RtcDs3231Time::begin()
// {
//    if (!_rtc.begin())
//    {
// #if DEBUG_TIME_LOG
//       Serial.println("⚠️ RTC DS3231: ไม่พบอุปกรณ์บน I2C");
// #endif
//       _isOk = false;
//       return false;
//    }

//    if (_rtc.lostPower())
//    {
// #if DEBUG_TIME_LOG
//       Serial.println("⚠️ RTC DS3231: เวลาเพิ่ง reset (ต้องตั้งเวลาให้มันก่อน)");
// #endif
//       // ยังถือว่าใช้งานได้ แต่เวลาอาจไม่ถูก
//    }

//    _isOk = true;
// #if DEBUG_TIME_LOG
//    Serial.println("✅ RTC DS3231: ready");
// #endif
//    return true;
// }

bool RtcDs3231Time::begin()
{
   Serial.println("[RTC] begin() called");

   bool ok = _rtc.begin();
   Serial.printf("[RTC] _rtc.begin() => %d\n", ok ? 1 : 0);

   if (!ok)
   {
      Serial.println("⚠️ RTC DS3231: ไม่พบอุปกรณ์บน I2C");
      _isOk = false;
      return false;
   }

   if (_rtc.lostPower())
   {
      Serial.println("⚠️ RTC DS3231: เวลาเพิ่ง reset → ตั้งเวลาจากค่า compile");

      // ใช้เวลาตอนคอมไพล์ตั้งให้ DS3231 ครั้งแรก
      DateTime compiled(__DATE__, __TIME__);
      _rtc.adjust(compiled);
   }

   _isOk = true;
   Serial.println("✅ RTC DS3231: ready");
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

// bool RtcDs3231Time::setTimeOfDay(uint8_t hour, uint8_t minute, uint8_t second)
// {
//    if (!_isOk)
//    {
// #if DEBUG_TIME_LOG
//       Serial.println("[RTC] setTimeOfDay: RTC not initialized");
// #endif
//       return false;
//    }

//    // อ่าน datetime ปัจจุบันมาเอา "ปี-เดือน-วัน" เดิม
//    DateTime now = _rtc.now();

//    // clamp ค่าเบื้องต้นกันพลาด
//    if (hour > 23)
//       hour = 23;
//    if (minute > 59)
//       minute = 59;
//    if (second > 59)
//       second = 59;

//    DateTime newDt(
//        now.year(),
//        now.month(),
//        now.day(),
//        hour,
//        minute,
//        second);

//    _rtc.adjust(newDt);

// #if DEBUG_TIME_LOG
//    Serial.printf("[RTC] setTimeOfDay => %04d-%02d-%02d %02u:%02u:%02u\n",
//                  newDt.year(), newDt.month(), newDt.day(),
//                  (unsigned)hour,
//                  (unsigned)minute,
//                  (unsigned)second);
// #endif
//    return true;
// }
bool RtcDs3231Time::setTimeOfDay(uint8_t hour, uint8_t minute, uint8_t second)
{
   if (!_isOk)
   {
#if DEBUG_TIME_LOG
      Serial.println("[RTC] setTimeOfDay: RTC not initialized");
#endif
      return false;
   }

   // ใช้วันที่ dummy ไปก่อน เอาเวลาเป็นหลัก
   // ปี/เดือน/วัน เลือกอะไรก็ได้ที่สมเหตุสมผล
   DateTime now = _rtc.now();
   DateTime target(
       now.year(),  // ใช้ปีเดิม
       now.month(), // ใช้เดือนเดิม
       now.day(),   // ใช้วันเดิม
       hour,
       minute,
       second);

   _rtc.adjust(target);

#if DEBUG_TIME_LOG
   Serial.printf("[RTC] setTimeOfDay -> %02u:%02u:%02u\n",
                 (unsigned)hour,
                 (unsigned)minute,
                 (unsigned)second);
#endif
   return true;
}