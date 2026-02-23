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

   // รูปแบบที่ยอมรับ: "HH:MM" เช่น "07:00", "14:30"
   if (sscanf(str, "%d:%d", &hh, &mm) != 2)
      return false;

   if (hh < 0 || hh > 23 || mm < 0 || mm > 59)
      return false;

   minutes = static_cast<uint16_t>(hh * 60 + mm);
   return true;
}

bool loadAirScheduleFromFS(const char *path, AirPumpSchedule &out)
{
   // เคลียร์ค่าปัจจุบันก่อนเสมอ ป้องกัน state ค้าง
   out.enabled = false;
   out.windowCount = 0;

   // คาดว่า LittleFS.begin() ถูกเรียกไปแล้วใน setup()
   File f = LittleFS.open(path, "r");
   if (!f)
   {
      Serial.printf("⚠️ schedule file '%s' not found\n", path);
      return false;
   }

   // ใช้ JsonDocument (เวอร์ชันใหม่ แทน StaticJsonDocument<512>)
   JsonDocument doc;
   DeserializationError err = deserializeJson(doc, f);
   f.close();

   if (err)
   {
      Serial.printf("⚠️ schedule JSON parse error: %s\n", err.c_str());
      return false;
   }

   // ดึง object หลัก: air_pump
   JsonObject air = doc["air_pump"];
   if (air.isNull())
   {
      Serial.println("⚠️ schedule: 'air_pump' not found");
      return false;
   }

   // ตอนนี้เอา enabled มาเก็บไว้เฉย ๆ เผื่ออนาคตจะใช้ toggle
   // ถ้า field หาย จะ default เป็น true
   out.enabled = air["enabled"] | true;

   // windows ต้องเป็น array
   JsonArray arr = air["windows"].as<JsonArray>();
   if (arr.isNull())
   {
      Serial.println("⚠️ schedule: 'windows' missing or not array");
      return false;
   }

   // ไล่โหลดแต่ละ window
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

      // แปลง "HH:MM" → นาทีของวัน
      if (!parseTimeToMinutes(startStr, s) ||
          !parseTimeToMinutes(endStr, e))
      {
         Serial.println("⚠️ schedule: bad time format, skip one window");
         continue;
      }

      // ตอนนี้รองรับเฉพาะช่วงปกติ (ไม่ข้ามเที่ยงคืน)
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

   Serial.printf("✅ schedule loaded: %u window(s)\n",
                 static_cast<unsigned>(out.windowCount));

   return true;
}