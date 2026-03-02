#pragma once

#include <Arduino.h>

#include "infrastructure/WifiConfigStore.h"
#include "interfaces/INetwork.h"

// ESP32 WiFi (STA) adapter:
// - Load SSID/PASS/HOSTNAME from WifiConfigStore (/wifi.json)
// - begin(): load config + prepare WiFi STA mode
// - ensureConnected(): connect (or reconnect) with timeout
class Esp32WiFiNetwork : public INetwork
{
public:
   explicit Esp32WiFiNetwork(const char *configPath = "/wifi.json");

   void begin() override;
   bool ensureConnected(uint32_t timeoutMs) override;
   bool isConnected() const override;
   void disconnect() override;

   // optional: for debug/telemetry
   const char *ssid() const { return _cfg.ssid.c_str(); }
   bool hasValidConfig() const { return _hasCfg; }

private:
   WifiConfigStore _store;
   WifiConfig _cfg;
   bool _hasCfg = false;

   // helper
   void applyStaSettings_();
};