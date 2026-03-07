// include/infrastructure/Esp32WiFiNetwork.h
#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "infrastructure/WifiConfigStore.h"
#include "interfaces/INetwork.h"

// ============================================================
//  Esp32WiFiNetwork — WiFi AP+STA สำหรับ ESP32-S3
//
//  Design: AP-first — startAp() ก่อนเสมอ แล้วค่อย startStaConnect()
//  Config: โหลดจาก /wifi.json ผ่าน WifiConfigStore
//
//  ใช้งาน (AppBoot):
//    1. ctx.network->begin()         ← โหลด config
//    2. ctx.network->startAp()       ← เปิด AP
//    3. ctx.network->startStaConnect() ← เริ่ม STA (non-blocking)
//    4. NetworkTask เรียก pollStaConnected() ทุก loop
// ============================================================

class Esp32WiFiNetwork : public INetwork
{
public:
   // ssid/pass: hardcode fallback ถ้าไม่มีไฟล์ — ปกติปล่อย nullptr
   Esp32WiFiNetwork(const char *ssid = nullptr,
                    const char *pass = nullptr,
                    const char *configPath = "/wifi.json");

   void begin() override;
   bool ensureConnected(uint32_t timeoutMs) override;
   bool isConnected() const override;
   bool hasValidConfig() const override;
   void disconnect() override;
   void startAp() override;
   void disconnectStaOnly() override;
   void startStaConnect() override;
   bool pollStaConnected() const override;

private:
   String _ssid;
   String _pass;

   WifiConfigStore _store;
   WifiConfig _cfg;
   bool _hasCfg = false;
};