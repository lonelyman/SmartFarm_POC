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
               ctx->state->setNetMessage("No WiFi config. Open /wifi to set SSID/password.");
               staState = StaConnState::Idle;
               break;
            }

            // If already connected, just reflect status
            if (ctx->network->pollStaConnected())
            {
               Serial.println("✅ STA already connected (AP kept)");
               ctx->state->setNetMessage("STA already connected. You can open via LAN too.");
               staState = StaConnState::Connected;
               break;
            }

            Serial.println("📡 NET_ON → start STA connect (non-blocking, keep AP)");
            ctx->state->setNetMessage("Connecting STA...");

            ctx->network->startStaConnect();
            staStartMs = millis();
            staState = StaConnState::Connecting;
            break;
         }

         case NetCmdType::NetOff:
         {
            // Cancel immediately even if currently connecting
            Serial.println("📴 NET_OFF → disconnect STA only (keep AP)");
            ctx->network->disconnectStaOnly();
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

      // --- STA connect progress (non-blocking) ---
      if (staState == StaConnState::Connecting)
      {
         if (ctx->network->pollStaConnected())
         {
            Serial.println("✅ STA connected (AP kept)");
            ctx->state->setNetMessage("STA connected. You can open via LAN too.");
            staState = StaConnState::Connected;
         }
         else
         {
            const uint32_t now = millis();
            if (now - staStartMs >= NET_CONNECT_TIMEOUT_MS)
            {
               Serial.println("❌ STA connect timeout (stay AP)");
               ctx->network->disconnectStaOnly(); // ensure STA not half-open
               ctx->state->setNetMessage("STA timeout. Still in AP mode. Check WiFi then retry.");
               staState = StaConnState::Failed;
            }
         }
      }

      // Optional: if STA was connected but later drops, you can reflect it
      // (leave it passive; do not auto-reconnect unless NetOn is requested)
      if (staState == StaConnState::Connected && !ctx->network->pollStaConnected())
      {
         // STA dropped unexpectedly; keep AP.
         Serial.println("⚠️ STA dropped (AP kept)");
         ctx->state->setNetMessage("STA dropped. AP mode active.");
         staState = StaConnState::Idle;
      }

      vTaskDelay(pdMS_TO_TICKS(20));
   }
}