#include <Arduino.h>

#include "tasks/TaskEntrypoints.h"
#include "infrastructure/SystemContext.h"
#include "infrastructure/SharedState.h"

static const uint32_t NET_CONNECT_TIMEOUT_MS = 15000;

void networkTask(void *pvParameters)
{
   auto *ctx = static_cast<SystemContext *>(pvParameters);
   if (ctx == nullptr || ctx->state == nullptr)
   {
      vTaskDelete(nullptr);
      return;
   }

   Serial.println("ℹ️ NetworkTask: Active");

   while (true)
   {
      NetCommand cmd{};
      if (ctx->state->consumeNetCommand(cmd))
      {
         switch (cmd.type)
         {
         case NetCmdType::NetOn:
            if (!ctx->network)
            {
               Serial.println("[NET] network adapter is null");
               break;
            }
            ctx->network->ensureConnected(NET_CONNECT_TIMEOUT_MS);
            break;

         case NetCmdType::NetOff:
            if (!ctx->network)
            {
               Serial.println("[NET] network adapter is null");
               break;
            }
            ctx->network->disconnect();
            break;

         case NetCmdType::SyncNtp:
            if (!ctx->network)
            {
               Serial.println("[NET] network adapter is null");
               break;
            }
            if (!ctx->clock)
            {
               Serial.println("[NET] clock is null");
               break;
            }

            if (!ctx->network->ensureConnected(NET_CONNECT_TIMEOUT_MS))
            {
               Serial.println("[NTP] cannot sync: network not connected");
               break;
            }

            if (ctx->clock->syncFromNetwork())
            {
               Serial.println("[NTP] sync success");
            }
            else
            {
               Serial.println("[NTP] sync failed (clock unsupported or NTP error)");
            }
            break;

         default:
            break;
         }
      }

      vTaskDelay(pdMS_TO_TICKS(200));
   }
}