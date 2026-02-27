#include "infrastructure/NetTimeSync.h"

#include <WiFi.h>
#include "time.h"

#include "Config.h"
#include "drivers/RtcDs3231Time.h"

namespace
{
   void _connectWifi()
   {
      if (WiFi.status() == WL_CONNECTED)
      {
         return;
      }

      Serial.println("[NET] Connecting WiFi...");
      WiFi.mode(WIFI_STA);
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

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
}

void NetTimeSync::connectWifiIfNeeded()
{
   _connectWifi();
}

void NetTimeSync::syncRtcFromNtp(RtcDs3231Time &rtc)
{
   _connectWifi();
   if (WiFi.status() != WL_CONNECTED)
   {
      Serial.println("[NTP] Cannot sync: WiFi not connected");
      return;
   }

   // ตั้งค่า NTP
   configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);

   struct tm timeinfo;
   if (!getLocalTime(&timeinfo))
   {
      Serial.println("[NTP] Failed to obtain time");
      return;
   }

   const uint8_t h = timeinfo.tm_hour;
   const uint8_t m = timeinfo.tm_min;
   const uint8_t s = timeinfo.tm_sec;

   Serial.printf("[NTP] Time from net = %02u:%02u:%02u\n", h, m, s);

   if (!rtc.isOk())
   {
      Serial.println("[NTP] RTC not ready, cannot set");
      return;
   }

   if (rtc.setTimeOfDay(h, m, s))
   {
      Serial.println("[NTP] RTC synced from internet");
   }
   else
   {
      Serial.println("[NTP] Failed to set RTC");
   }
}