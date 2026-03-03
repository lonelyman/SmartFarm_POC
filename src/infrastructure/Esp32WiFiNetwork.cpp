// src/infrastructure/Esp32WiFiNetwork.cpp
#include "infrastructure/Esp32WiFiNetwork.h"

Esp32WiFiNetwork::Esp32WiFiNetwork(const char *ssid, const char *pass, const char *configPath)
    : _store(configPath)
{
   if (ssid)
      _ssid = ssid;
   if (pass)
      _pass = pass;
}

void Esp32WiFiNetwork::begin()
{
   // โหลดจากไฟล์ก่อน (production)
   WifiConfig cfg;
   if (_store.load(cfg) && cfg.isValid())
   {
      _cfg = cfg;
      _ssid = cfg.ssid;
      _pass = cfg.password;
      _hasCfg = true;

      Serial.printf("📡 [WiFi] using config from /wifi.json (ssid=%s)\n", _ssid.c_str());
      return;
   }

   // fallback: ถ้ามี hardcode ส่งเข้ามา (กรณีอยากใช้ชั่วคราว)
   if (_ssid.length() > 0)
   {
      _hasCfg = true;
      Serial.printf("📡 [WiFi] using hardcoded ssid=%s\n", _ssid.c_str());
      return;
   }

   _hasCfg = false;
   Serial.println("⚠️ [WiFi] no valid wifi config (STA not started)");
}

bool Esp32WiFiNetwork::hasValidConfig() const
{
   return _hasCfg && _ssid.length() > 0;
}

bool Esp32WiFiNetwork::ensureConnected(uint32_t timeoutMs)
{
   if (!hasValidConfig())
   {
      Serial.println("⚠️ [WiFi] ensureConnected skipped: no config");
      return false;
   }

   if (WiFi.status() == WL_CONNECTED)
      return true;

   WiFi.mode(WIFI_STA);

   // ตั้ง hostname ถ้ามี
   if (_cfg.hostname.length() > 0)
      WiFi.setHostname(_cfg.hostname.c_str());
   else
      WiFi.setHostname("smartfarm");

   Serial.printf("📶 [WiFi] connecting to %s ...\n", _ssid.c_str());
   WiFi.begin(_ssid.c_str(), _pass.c_str());

   const uint32_t start = millis();
   while (WiFi.status() != WL_CONNECTED)
   {
      if (millis() - start >= timeoutMs)
      {
         Serial.println("❌ [WiFi] connect timeout");
         return false;
      }
      delay(200);
   }

   Serial.printf("✅ [WiFi] connected, IP=%s\n", WiFi.localIP().toString().c_str());
   return true;
}

bool Esp32WiFiNetwork::isConnected() const
{
   return WiFi.status() == WL_CONNECTED;
}

void Esp32WiFiNetwork::disconnect()
{
   WiFi.disconnect(true);
   WiFi.mode(WIFI_OFF);
   Serial.println("📴 [WiFi] disconnected");
}