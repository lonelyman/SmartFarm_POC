#include "infrastructure/NetTimeSync.h"

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"

#include "Config.h"
#include "drivers/RtcDs3231Time.h"

namespace
{
   // NOTE:
   // - ในโครงใหม่ "NetworkTask" เป็นเจ้าของการ connect WiFi
   // - ฟังก์ชันนี้คงไว้เพื่อรองรับกรณี debug/manual เท่านั้น
   // - การใช้งานจริงใน flow: NetworkTask จะ ensureConnected ก่อนเรียก syncRtcFromNtp
   void _connectWifi()
   {
      if (WiFi.status() == WL_CONNECTED)
      {
         return;
      }

      Serial.println("[NET] Connecting WiFi...");
      WiFi.mode(WIFI_STA);

#if defined(WIFI_SSID) && defined(WIFI_PASS)
      WiFi.begin(WIFI_SSID, WIFI_PASS);
#elif defined(WIFI_SSID) && defined(WIFI_PASSWORD)
      // เผื่อโปรเจคเดิมใช้ชื่อ WIFI_PASSWORD
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#else
      Serial.println("[NET] WIFI_SSID/WIFI_PASS not defined");
      return;
#endif

      uint8_t retry = 0;
      while (WiFi.status() != WL_CONNECTED && retry < 20)
      {
         delay(500);
         Serial.print(".");
         retry++;
      }
      Serial.println();

      if (WiFi.status() == WL_CONNECTED)
      {
         Serial.print("[NET] WiFi connected, IP=");
         Serial.println(WiFi.localIP());
      }
      else
      {
         Serial.println("[NET] WiFi connect FAILED");
      }
   }

   // NTP helper
   bool _fetchLocalTime(struct tm &out, uint32_t timeoutMs)
   {
      const uint32_t start = millis();
      while ((millis() - start) < timeoutMs)
      {
         if (getLocalTime(&out))
         {
            return true;
         }
         delay(250);
      }
      return false;
   }
}

void NetTimeSync::connectWifiIfNeeded()
{
   // debug/manual only (ไม่ใช่ flow หลัก)
   _connectWifi();
}

bool NetTimeSync::syncRtcFromNtp(RtcDs3231Time &rtc)
{
   // ✅ โครงใหม่: ไม่ connect เองในนี้
   // NetworkTask ต้อง ensureConnected มาก่อนแล้ว
   if (WiFi.status() != WL_CONNECTED)
   {
      Serial.println("[NTP] Cannot sync: WiFi not connected");
      return false;
   }

   // ตั้งค่า NTP
   configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

   struct tm timeinfo;
   if (!_fetchLocalTime(timeinfo, 10000))
   {
      Serial.println("[NTP] Failed to obtain time (timeout)");
      return false;
   }

   const uint8_t h = (uint8_t)timeinfo.tm_hour;
   const uint8_t m = (uint8_t)timeinfo.tm_min;
   const uint8_t s = (uint8_t)timeinfo.tm_sec;

   Serial.printf("[NTP] Time from net = %02u:%02u:%02u\n", h, m, s);

   if (!rtc.isOk())
   {
      Serial.println("[NTP] RTC not ready, cannot set");
      return false;
   }

   if (rtc.setTimeOfDay(h, m, s))
   {
      Serial.println("[NTP] RTC synced from internet");
      return true;
   }

   Serial.println("[NTP] Failed to set RTC");
   return false;
}