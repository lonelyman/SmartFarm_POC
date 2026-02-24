#include "infrastructure/ScheduleStore.h"

#include <LittleFS.h>
#include <ArduinoJson.h>

// แปลง "HH:MM" -> นาทีของวัน (0..1439)
static bool parseTimeToMinutes(const char *str, uint16_t &minutes)
{
   if (!str)
      return false;

   int hh = -1;
   int mm = -1;

   if (sscanf(str, "%d:%d", &hh, &mm) != 2)
      return false;

   if (hh < 0 || hh > 23 || mm < 0 || mm > 59)
      return false;

   minutes = static_cast<uint16_t>(hh * 60 + mm);
   return true;
}

bool loadAirScheduleFromFS(const char *path, AirPumpSchedule &out)
{
   // เคลียร์ state ก่อน
   out.enabled = false;
   out.windowCount = 0;

   File f = LittleFS.open(path, "r");
   if (!f)
   {
      Serial.printf("⚠️ schedule file '%s' not found\n", path);
      return false;
   }

   // JSON ไฟล์เล็ก ๆ ใช้ StaticJsonDocument ก็พอ
   StaticJsonDocument<512> doc;
   DeserializationError err = deserializeJson(doc, f);
   f.close();

   if (err)
   {
      Serial.printf("⚠️ schedule JSON parse error: %s\n", err.c_str());
      return false;
   }

   JsonObject air = doc["air_pump"];
   if (air.isNull())
   {
      Serial.println("⚠️ schedule: 'air_pump' not found");
      return false;
   }

   out.enabled = air["enabled"] | true;

   JsonArray arr = air["windows"].as<JsonArray>();
   if (arr.isNull())
   {
      Serial.println("⚠️ schedule: 'windows' missing or not array");
      return false;
   }

   for (JsonObject win : arr)
   {
      if (out.windowCount >= AirPumpSchedule::MAX_WINDOWS)
      {
         Serial.println("⚠️ schedule: exceed MAX_WINDOWS, ignore rest");
         break;
      }

      const char *startStr = win["start"] | nullptr;
      const char *endStr = win["end"] | nullptr;

      uint16_t s = 0;
      uint16_t e = 0;

      if (!parseTimeToMinutes(startStr, s) ||
          !parseTimeToMinutes(endStr, e))
      {
         Serial.println("⚠️ schedule: bad time format, skip one window");
         continue;
      }

      if (s >= e)
      {
         Serial.println("⚠️ schedule: start >= end, skip one window");
         continue;
      }

      TimeWindow &tw = out.windows[out.windowCount++];
      tw.startMin = s;
      tw.endMin = e;
   }

   if (out.windowCount == 0)
   {
      Serial.println("⚠️ schedule: no valid windows");
      return false;
   }

   Serial.printf("✅ schedule loaded: %u window(s), enabled=%d\n",
                 (unsigned)out.windowCount,
                 out.enabled ? 1 : 0);
   return true;
}