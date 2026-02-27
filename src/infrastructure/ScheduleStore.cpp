#include "infrastructure/ScheduleStore.h"

#include <Arduino.h>
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
   // เคลียร์ค่าเริ่มต้นก่อน
   out.enabled = false;
   out.windowCount = 0;

   // *** สมมติว่า LittleFS.begin() ถูกเรียกไปแล้วในที่อื่น (เช่น setup()) ***
   File f = LittleFS.open(path, "r");
   if (!f)
   {
      Serial.printf("⚠️ schedule file '%s' not found\n", path);
      return false;
   }

   // ---------------- JSON Parse ด้วย JsonDocument (ArduinoJson v7) ----------------
   JsonDocument doc; // ใช้ตัวใหม่ แทน StaticJsonDocument
   DeserializationError err = deserializeJson(doc, f);
   f.close();

   if (err)
   {
      Serial.printf("⚠️ schedule JSON parse error: %s\n", err.c_str());
      return false;
   }

   // ---- DEBUG: แสดง JSON ที่อ่านได้จริงจาก LittleFS ----
   Serial.println(F("----- Parsed schedule JSON (from FS) -----"));
   serializeJsonPretty(doc, Serial);
   Serial.println();
   Serial.println(F("----- END Parsed JSON -----"));

   // ---------------- ดึง object "air_pump" ----------------
   JsonObject air = doc["air_pump"];
   if (air.isNull())
   {
      Serial.println(F("⚠️ schedule: 'air_pump' not found"));
      return false;
   }

   // enabled: ถ้าไม่มี key นี้ ให้ default = true
   if (air["enabled"].is<bool>())
   {
      out.enabled = air["enabled"].as<bool>();
   }
   else
   {
      out.enabled = true;
   }

   // ---------------- ดึง array "windows" ----------------
   JsonVariant windowsVar = air["windows"];
   if (!windowsVar.is<JsonArray>())
   {
      Serial.println(F("⚠️ schedule: 'windows' missing or not array"));
      return false;
   }

   JsonArray arr = windowsVar.as<JsonArray>();
   if (arr.isNull() || arr.size() == 0)
   {
      Serial.println(F("⚠️ schedule: 'windows' is empty"));
      return false;
   }

   // ---------------- วนลูปอ่านแต่ละ window ----------------
   for (JsonVariant v : arr)
   {
      if (!v.is<JsonObject>())
      {
         Serial.println(F("⚠️ schedule: window is not object, skip"));
         continue;
      }

      if (out.windowCount >= AirPumpSchedule::MAX_WINDOWS)
      {
         Serial.println(F("⚠️ schedule: exceed MAX_WINDOWS, ignore rest"));
         break;
      }

      JsonObject win = v.as<JsonObject>();

      // แทน containsKey() -> ใช้ is<T>() ตามที่ lib แนะนำ
      JsonVariant startVar = win["start"];
      JsonVariant endVar = win["end"];

      if (!startVar.is<const char *>() || !endVar.is<const char *>())
      {
         Serial.println(F("⚠️ schedule: 'start' or 'end' missing/not string, skip one window"));
         continue;
      }

      const char *startStr = startVar.as<const char *>();
      const char *endStr = endVar.as<const char *>();

      Serial.printf("DEBUG window: start='%s', end='%s'\n",
                    startStr ? startStr : "NULL",
                    endStr ? endStr : "NULL");

      uint16_t s = 0;
      uint16_t e = 0;

      if (!parseTimeToMinutes(startStr, s) ||
          !parseTimeToMinutes(endStr, e))
      {
         Serial.println(F("⚠️ schedule: bad time format, skip one window"));
         continue;
      }

      if (s >= e)
      {
         Serial.println(F("⚠️ schedule: start >= end, skip one window"));
         continue;
      }

      TimeWindow &tw = out.windows[out.windowCount++];
      tw.startMin = s;
      tw.endMin = e;
   }

   if (out.windowCount == 0)
   {
      Serial.println(F("⚠️ schedule: no valid windows"));
      return false;
   }

   Serial.printf("✅ schedule loaded: %u window(s), enabled=%d\n",
                 static_cast<unsigned>(out.windowCount),
                 out.enabled ? 1 : 0);
   return true;
}