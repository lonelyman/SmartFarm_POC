// src/tasks/NetworkTask.cpp
#include <Arduino.h>

#include "tasks/TaskEntrypoints.h"
#include "infrastructure/SystemContext.h"
#include "infrastructure/SharedState.h"
#include "interfaces/INetModeSource.h"

static const uint32_t NET_CONNECT_TIMEOUT_MS = 15000;

namespace
{
   enum class StaConnState : uint8_t
   {
      Idle = 0,
      Connecting,
      Connected,
      Failed,
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
      Serial.println("⚠️ NetworkTask: netModeSource null (net switch ignored)");

   // network->begin() ถูกเรียกใน AppBoot::initNetwork() แล้ว — ไม่เรียกซ้ำ

   // 1) Boot policy: เปิด AP ก่อนเสมอ (offline-first)
   ctx->network->startAp();
   ctx->state->setNetState(NetState::AP_PRIMARY);
   ctx->state->setNetMessage("AP ready (192.168.4.1)");

   // --- STA state machine ---
   StaConnState staState = StaConnState::Idle;
   uint32_t staStartMs = 0;

   // Net switch edge-trigger — ต้องเป็น local ไม่ใช่ static
   // เพื่อให้ reset ได้ถ้า task restart
   bool swInited = false;
   NetDesiredMode lastSw = NetDesiredMode::AP_PRIMARY;

   while (true)
   {
      // --- Net switch → NetCommand (edge-trigger) ---
      if (ctx->netModeSource)
      {
         const NetDesiredMode sw = ctx->netModeSource->read();

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

      // --- Handle NetCommands ---
      NetCommand cmd;
      if (ctx->state->consumeNetCommand(cmd))
      {
         switch (cmd.type)
         {
         case NetCmdType::NetOn:
         {
            if (!ctx->network->hasValidConfig())
            {
               Serial.println("⚠️ [NET] NetOn: no config → stay AP");
               ctx->state->setNetState(NetState::AP_PRIMARY);
               ctx->state->setNetMessage("No WiFi config. Open /wifi to set SSID/password.");
               staState = StaConnState::Idle;
               break;
            }

            if (ctx->network->pollStaConnected())
            {
               Serial.println("✅ [NET] STA already connected");
               ctx->state->setNetState(NetState::STA_CONNECTED);
               ctx->state->setNetMessage("STA connected.");
               staState = StaConnState::Connected;
               break;
            }

            Serial.println("📡 [NET] NetOn → startStaConnect (AP kept)");
            ctx->state->setNetState(NetState::STA_CONNECTING);
            ctx->state->setNetMessage("Connecting to WiFi...");
            ctx->network->startStaConnect();
            staStartMs = millis();
            staState = StaConnState::Connecting;
            break;
         }

         case NetCmdType::NetOff:
         {
            Serial.println("📴 [NET] NetOff → disconnectStaOnly (AP kept)");
            ctx->network->disconnectStaOnly();
            ctx->state->setNetState(NetState::STA_STOPPED);
            ctx->state->setNetMessage("WiFi disconnected. AP mode active.");
            staState = StaConnState::Idle;
            break;
         }

         case NetCmdType::SyncNtp:
         {
            // ต้อง STA connected ก่อนเท่านั้น
            if (!ctx->network->isConnected())
            {
               Serial.println("⚠️ [NTP] SyncNtp skipped: STA not connected");
               ctx->state->setNetMessage("NTP sync failed: not connected.");
               break;
            }

            if (!ctx->clock)
            {
               Serial.println("⚠️ [NTP] SyncNtp skipped: clock is null");
               break;
            }

            ctx->state->setNetMessage("Syncing time from NTP...");
            if (ctx->clock->syncFromNetwork())
               ctx->state->setNetMessage("NTP sync OK.");
            else
               ctx->state->setNetMessage("NTP sync failed.");
            break;
         }

         case NetCmdType::None:
         default:
            break;
         }
      }

      // --- Connecting: poll timeout ---
      if (staState == StaConnState::Connecting)
      {
         if (ctx->network->pollStaConnected())
         {
            Serial.println("✅ [NET] STA connected (AP kept)");
            ctx->state->setNetState(NetState::STA_CONNECTED);
            ctx->state->setNetMessage("WiFi connected.");
            staState = StaConnState::Connected;
         }
         else if (millis() - staStartMs >= NET_CONNECT_TIMEOUT_MS)
         {
            Serial.println("❌ [NET] STA timeout → stay AP");
            ctx->network->disconnectStaOnly();
            ctx->state->setNetState(NetState::STA_FAILED);
            ctx->state->setNetMessage("WiFi timeout. Check credentials then retry.");
            staState = StaConnState::Failed;
         }
      }

      // --- Connected: detect drop ---
      if (staState == StaConnState::Connected && !ctx->network->pollStaConnected())
      {
         Serial.println("⚠️ [NET] STA dropped → AP only");
         ctx->state->setNetState(NetState::AP_PRIMARY);
         ctx->state->setNetMessage("WiFi dropped. AP mode active.");
         staState = StaConnState::Idle;
      }

      vTaskDelay(pdMS_TO_TICKS(20));
   }
}