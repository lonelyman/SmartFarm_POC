// src/infrastructure/Esp32WiFiNetwork.cpp
#include "infrastructure/Esp32WiFiNetwork.h"
#include <WiFi.h>

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

// ✅ AP-first: เปิด AP เป็นค่าเริ่มต้น
void Esp32WiFiNetwork::startAp()
{
   // ถ้าปิดหมดอยู่ ให้เปิดเป็น AP หรือ AP+STA (เผื่ออนาคต)
   // ตอนนี้เลือก WIFI_AP_STA เพื่อให้ “ยังต่อ STA ได้โดยไม่หลุด AP”
   WiFi.mode(WIFI_AP_STA);

   // ตั้ง hostname สำหรับ STA (ไว้ล่วงหน้า)
   if (_cfg.hostname.length() > 0)
      WiFi.setHostname(_cfg.hostname.c_str());
   else
      WiFi.setHostname("smartfarm");

   // ตั้งชื่อ AP ของอุปกรณ์ (คุณปรับชื่อได้)
   const char *apSsid = "SmartFarm_AP";
   const char *apPass = nullptr; // แนะนำให้ตั้งรหัสผ่านภายหลัง

   bool ok = WiFi.softAP(apSsid, apPass);
   IPAddress ip = WiFi.softAPIP();

   if (ok)
      Serial.printf("📶 [WiFi] AP started: ssid=%s ip=%s\n", apSsid, ip.toString().c_str());
   else
      Serial.println("❌ [WiFi] AP start failed");
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

   // สำคัญ: อย่าทับ mode เป็น WIFI_STA เพราะเราต้องคง AP อยู่
   // ให้แน่ใจว่าอย่างน้อยเป็น AP+STA
   wifi_mode_t mode = WiFi.getMode();
   if (mode != WIFI_AP_STA)
   {
      WiFi.mode(WIFI_AP_STA);
   }

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

// ✅ ตัดเฉพาะ STA แต่คง AP ไว้
void Esp32WiFiNetwork::disconnectStaOnly()
{
   WiFi.disconnect(true); // ตัด STA
   // คง mode เป็น AP หรือ AP+STA เพื่อไม่ให้หลุด local access
   wifi_mode_t mode = WiFi.getMode();
   if (mode != WIFI_AP && mode != WIFI_AP_STA)
   {
      WiFi.mode(WIFI_AP);
   }
   Serial.println("📴 [WiFi] STA disconnected (AP kept)");
}

// ปิดทุกอย่าง (ถ้าต้องการจริง ๆ)
void Esp32WiFiNetwork::disconnect()
{
   WiFi.disconnect(true);
   WiFi.softAPdisconnect(true);
   WiFi.mode(WIFI_OFF);
   Serial.println("📴 [WiFi] disconnected (STA+AP OFF)");
}