// include/infrastructure/Esp32WiFiNetwork.h
#pragma once
#include <Arduino.h>
#include <WiFi.h>

#include "infrastructure/WifiConfigStore.h"
#include "interfaces/INetwork.h"

class Esp32WiFiNetwork : public INetwork
{
public:
   // ✅ ทำให้ไม่ต้อง hardcode แล้ว
   Esp32WiFiNetwork(const char *ssid = nullptr, const char *pass = nullptr, const char *configPath = "/wifi.json");

   void begin() override;
   bool ensureConnected(uint32_t timeoutMs) override;
   bool isConnected() const override;
   void disconnect() override;

   bool hasValidConfig() const override; // ✅

private:
   String _ssid;
   String _pass;

   WifiConfigStore _store;
   WifiConfig _cfg;
   bool _hasCfg = false;
};