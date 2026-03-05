// src/tasks/NetworkTask.cpp
#include <Arduino.h>

#include "tasks/TaskEntrypoints.h"
#include "infrastructure/SystemContext.h"
#include "infrastructure/SharedState.h" // NetCommand / NetCmdType
#include "interfaces/INetModeSource.h"  // NetDesiredMode (for net switch)

// AP-first policy
static const uint32_t NET_CONNECT_TIMEOUT_MS = 15000;

namespace
{
   enum class StaConnState : uint8_t
   {
      Idle = 0,
      Connecting,
      Connected,
      Failed
   };
}

void networkTask(void *pvParameters)
{
   auto *ctx = static_cast<SystemContext *>(pvParameters);
   if (!ctx || !ctx->network || !ctx->state)
   {
      vTaskDelete(nullptr);
      return;
   }

   Serial.println("🌐 NetworkTask: Boot");

   if (!ctx->netModeSource)
   {
      Serial.println("⚠️ NetworkTask: netModeSource is null (net switch ignored)");
   }

   // NOTE:
   // network->begin() is initialized in AppBoot::initNetwork(ctx).
   // Do NOT call begin() again here to avoid duplicate config loads/log spam.

   // 1) Boot policy: เปิด AP เป็นค่าเริ่มต้นเสมอ (offline-first)
   ctx->network->startAp();
   ctx->state->setNetState(NetState::AP_PRIMARY);
   ctx->state->setNetMessage("AP ready. You can use dashboard via AP (192.168.4.1).");

   // --- STA connect state machine ---
   StaConnState staState = StaConnState::Idle;
   uint32_t staStartMs = 0;

   while (true)
   {
      // --- NET switch -> NetCommand (edge-trigger) ---
      // Policy mapping:
      // - AP_PRIMARY => STA OFF
      // - any other mode => STA ON
      static bool swInited = false;
      static NetDesiredMode lastSw = NetDesiredMode::AP_PRIMARY;

      if (ctx->netModeSource)
      {
         NetDesiredMode sw = ctx->netModeSource->read();

         if (!swInited)
         {
            lastSw = sw;
            swInited = true;
         }
         else if (sw != lastSw)
         {
            lastSw = sw;

            if (sw == NetDesiredMode::AP_PRIMARY)
               ctx->state->requestNetOff();
            else
               ctx->state->requestNetOn();
         }
      }

      // --- Handle incoming NetCommands ---
      NetCommand cmd;
      if (ctx->state->consumeNetCommand(cmd))
      {
         switch (cmd.type)
         {
         case NetCmdType::NetOn:
         {
            if (!ctx->network->hasValidConfig())
            {
               Serial.println("⚠️ NET_ON requested but no valid config → stay AP");
               ctx->state->setNetState(NetState::AP_PRIMARY);
               ctx->state->setNetMessage("No WiFi config. Open /wifi to set SSID/password.");
               staState = StaConnState::Idle;
               break;
            }

            // If already connected, just reflect status
            if (ctx->network->pollStaConnected())
            {
               Serial.println("✅ STA already connected (AP kept)");
               ctx->state->setNetState(NetState::STA_CONNECTED);
               ctx->state->setNetMessage("STA already connected. You can open via LAN too.");
               staState = StaConnState::Connected;
               break;
            }

            Serial.println("📡 NET_ON → start STA connect (non-blocking, keep AP)");
            ctx->state->setNetState(NetState::STA_CONNECTING);
            ctx->state->setNetMessage("Connecting STA...");

            ctx->network->startStaConnect();
            staStartMs = millis();
            staState = StaConnState::Connecting;
            break;
         }

         case NetCmdType::NetOff:
         {
            Serial.println("📴 NET_OFF → disconnect STA only (keep AP)");
            ctx->network->disconnectStaOnly();
            ctx->state->setNetState(NetState::STA_STOPPED);
            ctx->state->setNetMessage("STA disconnected. AP mode active.");
            staState = StaConnState::Idle;
            break;
         }

         case NetCmdType::SyncNtp:
         {
            Serial.println("⏱ SYNC_NTP requested");
            ctx->state->setNetMessage("NTP sync requested.");
            break;
         }

         case NetCmdType::None:
         default:
            break;
         }
      }

      if (staState == StaConnState::Connecting)
      {
         if (ctx->network->pollStaConnected())
         {
            Serial.println("✅ STA connected (AP kept)");
            ctx->state->setNetState(NetState::STA_CONNECTED);
            ctx->state->setNetMessage("STA connected. You can open via LAN too.");
            staState = StaConnState::Connected;
         }
         else
         {
            const uint32_t now = millis();
            if (now - staStartMs >= NET_CONNECT_TIMEOUT_MS)
            {
               Serial.println("❌ STA connect timeout (stay AP)");
               ctx->network->disconnectStaOnly();
               ctx->state->setNetState(NetState::STA_FAILED);
               ctx->state->setNetMessage("STA timeout. Still in AP mode. Check WiFi then retry.");
               staState = StaConnState::Failed;
            }
         }
      }

      if (staState == StaConnState::Connected && !ctx->network->pollStaConnected())
      {
         Serial.println("⚠️ STA dropped (AP kept)");
         ctx->state->setNetState(NetState::AP_PRIMARY);
         ctx->state->setNetMessage("STA dropped. AP mode active.");
         staState = StaConnState::Idle;
      }

      vTaskDelay(pdMS_TO_TICKS(20));
   }
}