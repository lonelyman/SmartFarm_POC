// src/infrastructure/Esp32WiFiNetwork.cpp
#include "infrastructure/Esp32WiFiNetwork.h"
#include <WiFi.h>

static const char *AP_PRIMARY_SSID = "SmartFarm-Setup";
static const char *AP_PRIMARY_PASS = nullptr; // แนะนำให้ตั้งรหัสผ่านภายหลัง

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

      Serial.printf("✅ [WiFiCFG] loaded: ssid='%s' hostname='%s'\n",
                    _ssid.c_str(), _cfg.hostname.c_str());
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

void Esp32WiFiNetwork::startAp()
{
   // 1) ต้องคง AP ไว้ตลอด และเปิดทางให้ STA ตามมาทีหลัง
   if (WiFi.getMode() != WIFI_AP_STA)
      WiFi.mode(WIFI_AP_STA);

   // 2) ล็อก AP IP ให้เป็น 192.168.4.1 แบบ deterministic
   IPAddress apIP(192, 168, 4, 1);
   IPAddress apGW(192, 168, 4, 1);
   IPAddress apSN(255, 255, 255, 0);

   // softAPConfig ต้องเรียกก่อน softAP
   WiFi.softAPConfig(apIP, apGW, apSN);

   // 3) เปิด AP (เรียกซ้ำได้ ไม่ควรพัง)
   bool ok = WiFi.softAP(AP_PRIMARY_SSID, AP_PRIMARY_PASS);

   IPAddress ip = WiFi.softAPIP();
   if (ok)
      Serial.printf("📶 [WiFi] AP started: SSID=%s IP=%s mode=%d\n",
                    AP_PRIMARY_SSID, ip.toString().c_str(), (int)WiFi.getMode());
   else
      Serial.println("❌ [WiFi] AP start failed");
}

// ===================== Production-grade (non-blocking STA) =====================

void Esp32WiFiNetwork::startStaConnect()
{
   if (!hasValidConfig())
   {
      Serial.println("⚠️ [WiFi] startStaConnect skipped: no config");
      return;
   }

   if (WiFi.status() == WL_CONNECTED)
      return;

   // ✅ คง AP ไว้ และเปิดทางให้ STA ทำงาน
   if (WiFi.getMode() != WIFI_AP_STA)
      WiFi.mode(WIFI_AP_STA);

   // ✅ หลังจาก disconnectStaOnly() เราอาจปิด STA ไว้
   WiFi.enableAP(true);
   WiFi.enableSTA(true);

   // ✅ เปิด auto reconnect เพื่อให้ต่อกลับได้ปกติ (เปิดตอนเริ่ม connect)
   WiFi.setAutoReconnect(true);

   const char *hn = (_cfg.hostname.length() > 0) ? _cfg.hostname.c_str() : "smartfarm";
   WiFi.setHostname(hn);

   Serial.printf("📶 [WiFi] startStaConnect -> '%s'\n", _ssid.c_str());
   WiFi.begin(_ssid.c_str(), _pass.c_str());
}

bool Esp32WiFiNetwork::pollStaConnected() const
{
   return WiFi.status() == WL_CONNECTED;
}

// ===================== Legacy blocking (kept) =====================

bool Esp32WiFiNetwork::ensureConnected(uint32_t timeoutMs)
{
   if (!hasValidConfig())
   {
      Serial.println("⚠️ [WiFi] ensureConnected skipped: no config");
      return false;
   }

   if (WiFi.status() == WL_CONNECTED)
      return true;

   // kick off connect (non-blocking API) then wait here (legacy behavior)
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

   Serial.printf("✅ [WiFi] STA connected, IP=%s\n", WiFi.localIP().toString().c_str());
   return true;
}

bool Esp32WiFiNetwork::isConnected() const
{
   return WiFi.status() == WL_CONNECTED;
}

void Esp32WiFiNetwork::disconnectStaOnly()
{
   // 1) กัน STA เด้งกลับเอง (สำคัญมาก)
   WiFi.setAutoReconnect(false);

   // 2) disconnect เฉพาะ STA (ห้าม wifioff=true เพราะจะมีผลต่อ radio/โหมด)
   WiFi.disconnect(false);

   // 3) ปิด STA interface โดยคง AP ไว้
   WiFi.enableSTA(false);
   WiFi.enableAP(true);

   // 4) ให้ deterministic ว่า STA ปิดจริง
   WiFi.mode(WIFI_AP);

   Serial.printf("📴 [WiFi] STA disabled (AP kept), AP_IP=%s\n", WiFi.softAPIP().toString().c_str());
}

void Esp32WiFiNetwork::disconnect()
{
   // ปิดทุกอย่าง (ใช้เฉพาะกรณีพิเศษจริง ๆ)
   WiFi.disconnect(true);
   WiFi.softAPdisconnect(true);
   WiFi.mode(WIFI_OFF);
   Serial.println("📴 [WiFi] disconnected (STA+AP OFF)");
}