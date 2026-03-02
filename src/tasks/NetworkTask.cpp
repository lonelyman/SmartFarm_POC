#include <Arduino.h>

#include "tasks/TaskEntrypoints.h"
#include "infrastructure/SystemContext.h"
#include "infrastructure/WifiProvisioning.h"

static const uint32_t NET_CONNECT_TIMEOUT_MS = 15000;

static WifiProvisioning g_prov("/wifi.json", 80);

void networkTask(void *pvParameters)
{
   auto *ctx = static_cast<SystemContext *>(pvParameters);
   if (!ctx || !ctx->network)
   {
      vTaskDelete(nullptr);
      return;
   }

   Serial.println("🌐 NetworkTask: Boot");

   // 1️⃣ โหลด config
   ctx->network->begin();

   // 2️⃣ ถ้ามี config → ลอง connect
   if (ctx->network->hasValidConfig())
   {
      Serial.println("📡 Trying STA connect...");

      if (ctx->network->ensureConnected(NET_CONNECT_TIMEOUT_MS))
      {
         Serial.println("✅ STA connected");
         vTaskDelete(nullptr); // จบ task เลย
         return;
      }

      Serial.println("❌ STA failed → start provisioning");
   }
   else
   {
      Serial.println("⚠️ No valid config → start provisioning");
   }

   // 3️⃣ เปิด provisioning (AP mode)
   while (true)
   {
      g_prov.tick();
      vTaskDelay(pdMS_TO_TICKS(20));
   }
}