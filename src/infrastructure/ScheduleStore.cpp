// src/infrastructure/ScheduleStore.cpp
#include "infrastructure/ScheduleStore.h"

#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// แปลง "HH:MM" → นาทีของวัน (0–1439)
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

bool loadScheduleFromFS(const char *path, const char *key, ISchedule &out)
{
   out.clear(); // เคลียร์ก่อนเสมอ — ป้องกัน stale data

   // LittleFS.begin() ถูกเรียกแล้วใน AppBoot
   File f = LittleFS.open(path, "r");
   if (!f)
   {
      Serial.printf("⚠️ [Schedule] file '%s' not found\n", path);
      return false;
   }

   JsonDocument doc;
   DeserializationError err = deserializeJson(doc, f);
   f.close();

   if (err)
   {
      Serial.printf("⚠️ [Schedule] JSON parse error: %s\n", err.c_str());
      return false;
   }

   // --- ดึง object ตาม key ที่ระบุ เช่น "air_pump" ---
   JsonObject obj = doc[key];
   if (obj.isNull())
   {
      Serial.printf("⚠️ [Schedule] key '%s' not found\n", key);
      return false;
   }

   const bool enabled = obj["enabled"].is<bool>() ? obj["enabled"].as<bool>() : true;
   out.setEnabled(enabled);

   // --- windows array ---
   JsonVariant windowsVar = obj["windows"];
   if (!windowsVar.is<JsonArray>())
   {
      Serial.printf("⚠️ [Schedule] '%s.windows' missing or not array\n", key);
      return false;
   }

   JsonArray arr = windowsVar.as<JsonArray>();
   if (arr.isNull() || arr.size() == 0)
   {
      Serial.printf("⚠️ [Schedule] '%s.windows' is empty\n", key);
      return false;
   }

   uint8_t loaded = 0;
   for (JsonVariant v : arr)
   {
      if (!v.is<JsonObject>())
      {
         Serial.println("⚠️ [Schedule] window is not object, skip");
         continue;
      }

      JsonObject win = v.as<JsonObject>();

      if (!win["start"].is<const char *>() || !win["end"].is<const char *>())
      {
         Serial.println("⚠️ [Schedule] 'start' or 'end' missing, skip");
         continue;
      }

      uint16_t s = 0;
      uint16_t e = 0;

      if (!parseTimeToMinutes(win["start"].as<const char *>(), s) ||
          !parseTimeToMinutes(win["end"].as<const char *>(), e))
      {
         Serial.println("⚠️ [Schedule] bad time format, skip");
         continue;
      }

      if (s >= e)
      {
         Serial.println("⚠️ [Schedule] start >= end, skip");
         continue;
      }

      out.addWindow(s, e);
      loaded++;
   }

   if (loaded == 0)
   {
      Serial.printf("⚠️ [Schedule] '%s' no valid windows\n", key);
      return false;
   }

   Serial.printf("✅ [Schedule] '%s' loaded: %u window(s), enabled=%d\n", key, loaded, enabled ? 1 : 0);
   return true;
}