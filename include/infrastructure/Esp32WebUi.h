// include/infrastructure/Esp32WebUi.h
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "../interfaces/IUi.h"
#include "infrastructure/SystemContext.h"
#include "infrastructure/WifiConfigStore.h"

// ============================================================
//  Esp32WebUi — Web Server บน ESP32-S3
//
//  Design: lazy-start — server จะ begin() จริงเมื่อมี IP แล้วเท่านั้น
//  (AP IP พร้อมเร็วกว่า STA — tick() เช็คทุกรอบ)
//
//  ctx รับเป็น pointer เพื่อแก้ circular dependency:
//    SystemContext ต้องการ &webUi แต่ webUi ต้องการ ctx
//  → construct webUi ก่อน แล้วค่อย setContext() ใน AppBoot
// ============================================================

class Esp32WebUi : public IUi
{
public:
   explicit Esp32WebUi(uint16_t port = 80);

   void setContext(SystemContext *ctx);

   bool begin() override;
   void tick() override;

private:
   SystemContext *_ctx = nullptr;
   WebServer _server;
   bool _started = false;
   bool _pendingRestart = false;

   // cache wifi config — โหลดครั้งเดียวตอน begin()
   // ไม่ต้องเปิด LittleFS ซ้ำทุก GET /api/wifi/config
   WifiConfig _wifiCfg;
   bool _wifiCfgLoaded = false;

   void registerRoutes();
   bool ensureStartedWhenHasIp();

   // build JSON โดยใช้ ArduinoJson — กัน heap fragmentation
   String buildStatusJson() const;
   String buildWifiConfigJson() const;
};