// src/infrastructure/Esp32WiFiNetwork.cpp
#include "infrastructure/Esp32WiFiNetwork.h"

#include <WiFi.h>
#include <esp_wifi.h> // esp_wifi_set_ps() — ปิด power save ลด latency WebServer

// default fallback ถ้าไม่มี config เลย
static const char *DEFAULT_AP_SSID = "SmartFarm-Setup";

// ============================================================
//  Constructor
// ============================================================

Esp32WiFiNetwork::Esp32WiFiNetwork(const char *ssid, const char *pass, const char *configPath)
    : _store(configPath)
{
   if (ssid)
      _ssid = ssid;
   if (pass)
      _pass = pass;
}

// ============================================================
//  begin — โหลด config จาก store
// ============================================================

void Esp32WiFiNetwork::begin()
{
   WifiConfig cfg;
   if (_store.load(cfg) && cfg.isValid())
   {
      _cfg = cfg;
      _ssid = cfg.ssid;
      _pass = cfg.password;
      _hasCfg = true;
      return;
   }

   if (_ssid.length() > 0)
   {
      _hasCfg = true;
      Serial.printf("📡 [WiFi] using hardcoded ssid=%s\n", _ssid.c_str());
      return;
   }

   _hasCfg = false;
   Serial.println("⚠️ [WiFi] no valid config — STA disabled, AP only");
}

// ============================================================
//  Config
// ============================================================

bool Esp32WiFiNetwork::hasValidConfig() const
{
   return _hasCfg && _ssid.length() > 0;
}

// ============================================================
//  AP — ใช้ apSsid/apPass จาก config ถ้ามี
// ============================================================

void Esp32WiFiNetwork::startAp()
{
   WiFi.mode(WIFI_AP_STA);
   esp_wifi_set_ps(WIFI_PS_NONE);
   WiFi.softAPConfig(
       IPAddress(192, 168, 4, 1),
       IPAddress(192, 168, 4, 1),
       IPAddress(255, 255, 255, 0));

   const char *apSsid = (_cfg.apSsid.length() > 0)
                            ? _cfg.apSsid.c_str()
                            : DEFAULT_AP_SSID;

   const char *apPass = (_cfg.apPass.length() >= 8)
                            ? _cfg.apPass.c_str()
                            : nullptr; // open AP ถ้า password สั้นกว่า 8 ตัว

   const bool ok = WiFi.softAP(apSsid, apPass);

   if (ok)
      Serial.printf("📶 [WiFi] AP started: SSID='%s' %s IP=%s\n",
                    apSsid,
                    apPass ? "(password)" : "(open)",
                    WiFi.softAPIP().toString().c_str());
   else
      Serial.println("❌ [WiFi] AP start failed");
}

// ============================================================
//  STA — non-blocking
// ============================================================

void Esp32WiFiNetwork::startStaConnect()
{
   if (!hasValidConfig())
   {
      Serial.println("⚠️ [WiFi] startStaConnect skipped: no config");
      return;
   }

   if (WiFi.status() == WL_CONNECTED)
      return;

   WiFi.mode(WIFI_AP_STA);
   WiFi.enableAP(true);
   WiFi.enableSTA(true);
   WiFi.setAutoReconnect(true);

   const char *hn = _cfg.hostname.length() > 0 ? _cfg.hostname.c_str() : "smartfarm";
   WiFi.setHostname(hn);

   Serial.printf("📶 [WiFi] connecting to '%s'...\n", _ssid.c_str());
   WiFi.begin(_ssid.c_str(), _pass.c_str());
}

bool Esp32WiFiNetwork::pollStaConnected() const
{
   return WiFi.status() == WL_CONNECTED;
}

// ============================================================
//  STA — blocking (NTP sync)
// ============================================================

bool Esp32WiFiNetwork::ensureConnected(uint32_t timeoutMs)
{
   if (!hasValidConfig())
   {
      Serial.println("⚠️ [WiFi] ensureConnected skipped: no config");
      return false;
   }

   if (WiFi.status() == WL_CONNECTED)
      return true;

   startStaConnect();

   const uint32_t start = millis();
   while (WiFi.status() != WL_CONNECTED)
   {
      if (millis() - start >= timeoutMs)
      {
         Serial.println("❌ [WiFi] STA connect timeout (AP kept)");
         return false;
      }
      delay(200);
   }

   Serial.printf("✅ [WiFi] STA connected: IP=%s\n", WiFi.localIP().toString().c_str());
   return true;
}

bool Esp32WiFiNetwork::isConnected() const
{
   return WiFi.status() == WL_CONNECTED;
}

// ============================================================
//  Disconnect
// ============================================================

void Esp32WiFiNetwork::disconnectStaOnly()
{
   WiFi.setAutoReconnect(false);
   WiFi.disconnect(false);
   WiFi.enableSTA(false);
   WiFi.enableAP(true);
   WiFi.mode(WIFI_AP);
   Serial.printf("📴 [WiFi] STA disabled (AP kept) IP=%s\n", WiFi.softAPIP().toString().c_str());
}

void Esp32WiFiNetwork::disconnect()
{
   WiFi.disconnect(true);
   WiFi.softAPdisconnect(true);
   WiFi.mode(WIFI_OFF);
   Serial.println("📴 [WiFi] all interfaces OFF");
}