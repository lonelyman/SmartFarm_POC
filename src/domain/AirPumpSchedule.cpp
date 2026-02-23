#include "domain/AirPumpSchedule.h"

#include <LittleFS.h>
#include <ArduinoJson.h>

static const char *SCHEDULE_FILE = "/schedule.json";

// แปลง "HH:MM" -> นาทีของวัน (0..1439)
static uint16_t parseHmToMinutes(const char *hhmm)
{
   if (!hhmm || strlen(hhmm) < 4)
      return 0;

   int h = 0;
   int m = 0;
   sscanf(hhmm, "%d:%d", &h, &m);

   if (h < 0)
      h = 0;
   if (h > 23)
      h = 23;
   if (m < 0)
      m = 0;
   if (m > 59)
      m = 59;

   return static_cast<uint16_t>(h * 60 + m);
}

bool loadAirPumpSchedule(AirPumpSchedule &out)
{
   // เคลียร์ค่าเริ่มต้น
   out.enabled = false;
   out.windowCount = 0;

   // mount LittleFS (ถ้ายังไม่ mount ที่อื่นมาก่อน)
   if (!LittleFS.begin())
   {
      Serial.println("⚠️ LittleFS mount failed, use default schedule");
      return false;
   }

   File f = LittleFS.open(SCHEDULE_FILE, "r");
   if (!f)
   {
      Serial.println("⚠️ schedule.json not found, use default schedule");
      return false;
   }

   // ใช้ JsonDocument (เวอร์ชันใหม่ของ ArduinoJson)
   JsonDocument doc;
   DeserializationError err = deserializeJson(doc, f);
   f.close();

   if (err)
   {
      Serial.print("⚠️ schedule.json parse error: ");
      Serial.println(err.c_str());
      return false;
   }

   JsonObject air = doc["air_pump"];
   if (air.isNull())
   {
      Serial.println("⚠️ schedule.json: no 'air_pump' object");
      return false;
   }

   // ถ้าไม่มี field enabled → default true
   out.enabled = air["enabled"] | true;

   JsonArray arr = air["windows"].as<JsonArray>();
   if (arr.isNull())
   {
      Serial.println("⚠️ schedule.json: no 'windows' array");
      return false;
   }

   for (JsonObject w : arr)
   {
      if (out.windowCount >= AirPumpSchedule::MAX_WINDOWS)
         break;

      const char *startStr = w["start"] | nullptr;
      const char *endStr = w["end"] | nullptr;
      if (!startStr || !endStr)
         continue;

      TimeWindow &tw = out.windows[out.windowCount++];
      tw.startMin = parseHmToMinutes(startStr);
      tw.endMin = parseHmToMinutes(endStr);

      // กันเคส start >= end
      if (tw.startMin >= tw.endMin)
      {
         // ยกเลิกช่องนี้ ทับด้วยช่องก่อนหน้า
         --out.windowCount;
         Serial.println("⚠️ schedule.json: start >= end, skip one window");
      }
   }

   if (out.windowCount == 0)
   {
      Serial.println("⚠️ schedule.json: no valid windows");
      return false;
   }

   Serial.printf("✅ AirPump schedule loaded: enabled=%d, windows=%u\n",
                 out.enabled ? 1 : 0,
                 static_cast<unsigned>(out.windowCount));

   return true;
}