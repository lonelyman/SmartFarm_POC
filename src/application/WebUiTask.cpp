#include <Arduino.h>
#include "Config.h"
#include "infrastructure/SystemContext.h"

void webUiTask(void *pvParameters)
{
   auto *ctx = static_cast<SystemContext *>(pvParameters);
   if (!ctx || !ctx->ui)
   {
      vTaskDelete(nullptr);
      return;
   }

   Serial.println("ℹ️ WebUiTask: Active");

   while (true)
   {
      ctx->ui->tick();               // ✅ ถูกแล้ว: IUi::tick()
      vTaskDelay(pdMS_TO_TICKS(20)); // ~50Hz
   }
}