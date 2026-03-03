// src/tasks/NetworkTask.cpp
#include <Arduino.h>
#include <WiFi.h>

#include "tasks/TaskEntrypoints.h"
#include "infrastructure/SystemContext.h"
#include "infrastructure/WifiProvisioning.h"
#include "infrastructure/SharedState.h" // เพื่อใช้ NetCommand / NetCmdType ให้ตรง

// AP-first policy
static const uint32_t NET_CONNECT_TIMEOUT_MS = 15000;

// ใช้ provisioning สำหรับ “ตั้งค่า wifi.json” (หน้า /wifi.html, /save)
// หมายเหตุ: provisioning ของคุณจะเปิด AP เฉพาะตอน wifi.json invalid เท่านั้น  [oai_citation:2‡GitHub](https://raw.githubusercontent.com/lonelyman/SmartFarm_POC/main/src/infrastructure/WifiProvisioning.cpp)
static WifiProvisioning g_prov("/wifi.json", 80);

// AP Primary (offline-first) — ตอนนี้ยังไม่มีสวิตช์ จึงบูตมาเป็น AP เสมอ
static const char *AP_PRIMARY_SSID = "SmartFarm-Setup"; // ให้ตรงกับ provisioning จะได้ไม่สลับชื่อ SSID
static const char *AP_PRIMARY_PASS = nullptr;           // แนะนำให้ตั้งรหัสผ่านภายหลัง

static void startApPrimary()
{
   // เปิด AP เพื่อให้เข้า dashboard ได้ทันที
   WiFi.mode(WIFI_AP); // วิธีที่ 1: AP/STA ไม่พร้อมกัน
   bool ok = WiFi.softAP(AP_PRIMARY_SSID, AP_PRIMARY_PASS);
   IPAddress ip = WiFi.softAPIP();

   if (ok)
      Serial.printf("📶 [AP_PRIMARY] started: SSID=%s IP=%s\n", AP_PRIMARY_SSID, ip.toString().c_str());
   else
      Serial.println("❌ [AP_PRIMARY] start failed");
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

   // 1) โหลด config (เพื่อรู้ว่า “มี wifi.json ไหม”)
   ctx->network->begin();

   // 2) บูตมา: เปิด AP เป็นค่าเริ่มต้นเสมอ (offline-first)
   //    ตอนนี้ “ยังไม่มีสวิตช์” -> ยึด AP เป็นหลักก่อน
   startApPrimary();

   // 3) วนลูปตลอด เพื่อ:
   //    - tick provisioning (ถ้า wifi.json invalid จะเปิด portal ให้ตั้งค่า)
   //    - consume net command (/api/net/on, /api/net/off, /api/ntp)
   while (true)
   {
      // Provisioning: จะทำงานเมื่อ wifi.json invalid เท่านั้น (ตาม implementation ปัจจุบัน)
      g_prov.tick();

      NetCommand cmd;
      if (ctx->state->consumeNetCommand(cmd))
      {
         // cmd.type เป็น NetCmdType::NetOn/NetOff/SyncNtp ตาม SharedState.h  [oai_citation:3‡GitHub](https://raw.githubusercontent.com/lonelyman/SmartFarm_POC/main/include/infrastructure/SharedState.h)
         switch (cmd.type)
         {
         case NetCmdType::NetOn:
         {
            // “เปิดเน็ต” = ลองต่อ STA (วิธีที่ 1: ถ้าต่อสำเร็จ จะหลุดจาก AP และต้องเข้า IP ใหม่ใน LAN)
            if (!ctx->network->hasValidConfig())
            {
               Serial.println("⚠️ NET_ON requested but no valid config → stay AP (use provisioning to set wifi.json)");
               ctx->state->setNetMessage("No WiFi config. Open /wifi to set SSID/password.");
               break;
            }

            Serial.println("📡 NET_ON → Trying STA connect... (AP will stop if STA mode is used)");
            if (ctx->network->ensureConnected(NET_CONNECT_TIMEOUT_MS))
            {
               Serial.println("✅ STA connected");
               // หมายเหตุ: ensureConnected() ของคุณตั้ง WiFi.mode(WIFI_STA) → AP จะถูกปิด (ตามวิธีที่ 1)
               // WebUI จะไปพร้อมที่ STA IP (คุณมี log แสดงอยู่แล้ว)
            }
            else
            {
               Serial.println("❌ STA failed → back to AP_PRIMARY");
               // กลับมา AP เพื่อให้เข้าเครื่องได้เสมอ
               startApPrimary();
            }
            break;
         }

         case NetCmdType::NetOff:
         {
            // “ปิดเน็ต” = ตัดการเชื่อมต่อ แล้วกลับ AP
            Serial.println("📴 NET_OFF → disconnect + back to AP_PRIMARY");
            ctx->network->disconnect(); // ปิด WiFi ทั้งหมด (ตาม implementation ของคุณ)
            startApPrimary();
            break;
         }

         case NetCmdType::SyncNtp:
         {
            // ตอนนี้แค่รับคำสั่งไว้ก่อน (อนาคตค่อยเชื่อม clock.syncFromNetwork ใน NetworkTask)
            Serial.println("⏱ SYNC_NTP requested");
            break;
         }

         case NetCmdType::None:
         default:
            break;
         }
      }

      vTaskDelay(pdMS_TO_TICKS(20));
   }
}