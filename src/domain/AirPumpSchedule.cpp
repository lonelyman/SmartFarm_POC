#include "domain/AirPumpSchedule.h"
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

static const char *SCHEDULE_FILE = "/schedule.json";

// แปลง "HH:MM" -> นาทีของวัน
static uint16_t parseHmToMinutes(const char *hhmm)
{
   if (!hhmm || strlen(hhmm) < 4)
      return 0;

   int h = 0, m = 0;
   sscanf(hhmm, "%d:%d", &h, &m);
   if (h < 0)
      h = 0;
   if (h > 23)
      h = 23;
   if (m < 0)
      m = 0;
   if (m > 59)
      m = 59;
   return (uint16_t)(h * 60 + m);
}

bool loadAirPumpSchedule(AirPumpSchedule &out)
{
   if (!SPIFFS.begin(true))
   {
      Serial.println("⚠️ SPIFFS mount failed, use default schedule");
      return false;
   }

   File f = SPIFFS.open(SCHEDULE_FILE, "r");
   if (!f)
   {
      Serial.println("⚠️ schedule.json not found, use default schedule");
      return false;
   }

   StaticJsonDocument<512> doc;
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

   out.enabled = air["enabled"] | false;
   out.windowCount = 0;

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
   }

   Serial.printf("✅ AirPump schedule loaded: enabled=%d, windows=%u\n",
                 out.enabled ? 1 : 0,
                 (unsigned)out.windowCount);

   return true;
}