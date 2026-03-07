// src/infrastructure/NetTimeSync.cpp
#include "infrastructure/NetTimeSync.h"

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"

#include "Config.h"
#include "drivers/RtcDs3231Time.h"

namespace
{
   bool fetchLocalTime(struct tm &out, uint32_t timeoutMs)
   {
      const uint32_t start = millis();
      while (millis() - start < timeoutMs)
      {
         if (getLocalTime(&out))
            return true;
         delay(250);
      }
      return false;
   }
}

bool NetTimeSync::syncRtcFromNtp(RtcDs3231Time &rtc)
{
   // NetworkTask ต้อง ensureConnected ก่อนเรียกฟังก์ชันนี้
   if (WiFi.status() != WL_CONNECTED)
   {
      Serial.println("⚠️ [NTP] WiFi not connected");
      return false;
   }

   configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

   struct tm timeinfo;
   if (!fetchLocalTime(timeinfo, 10000))
   {
      Serial.println("⚠️ [NTP] timeout — no time from server");
      return false;
   }

   const uint8_t h = static_cast<uint8_t>(timeinfo.tm_hour);
   const uint8_t m = static_cast<uint8_t>(timeinfo.tm_min);
   const uint8_t s = static_cast<uint8_t>(timeinfo.tm_sec);

   Serial.printf("[NTP] got %02u:%02u:%02u\n", h, m, s);

   if (!rtc.isOk())
   {
      Serial.println("⚠️ [NTP] RTC not ready");
      return false;
   }

   if (rtc.setTimeOfDay(h, m, s))
   {
      Serial.println("✅ [NTP] RTC synced");
      return true;
   }

   Serial.println("⚠️ [NTP] RTC set failed");
   return false;
}