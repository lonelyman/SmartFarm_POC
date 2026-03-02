#include "infrastructure/Esp32WiFiNetwork.h"

#include <WiFi.h>

Esp32WiFiNetwork::Esp32WiFiNetwork(const char *configPath)
    : _store(configPath)
{
}

void Esp32WiFiNetwork::begin()
{
   // ตั้งค่า WiFi แบบไม่เขียนลง flash (ลดพัง/ลด write)
   WiFi.persistent(false);
   WiFi.setAutoReconnect(true);

   // โหลด config จาก LittleFS ครั้งเดียว (อย่าโหลดถี่ ๆ ใน loop)
   WifiConfig cfg;
   if (_store.load(cfg) && cfg.isValid())
   {
      _cfg = cfg;
      _hasCfg = true;

      Serial.printf("📡 [WiFi] using config from %s (ssid=%s)\n",
                    _store.path(),
                    _cfg.ssid.c_str());

      applyStaSettings_();
   }
   else
   {
      _hasCfg = false;
      Serial.println("⚠️ [WiFi] no valid wifi.json found (STA not started)");
      // ไม่เปิด STA ตรงนี้ เพื่อให้ provisioning ไปทำงาน
   }
}

void Esp32WiFiNetwork::applyStaSettings_()
{
   // เข้าโหมด STA (client)
   WiFi.mode(WIFI_STA);

   // hostname: ต้อง set ก่อน begin()
#if defined(ESP32)
   if (_cfg.hostname.length() > 0)
   {
      WiFi.setHostname(_cfg.hostname.c_str());
   }
#endif
}

bool Esp32WiFiNetwork::isConnected() const
{
   return WiFi.status() == WL_CONNECTED;
}

bool Esp32WiFiNetwork::ensureConnected(uint32_t timeoutMs)
{
   if (isConnected())
      return true;

   if (!_hasCfg || !_cfg.isValid())
   {
      Serial.println("[NET] cannot connect: wifi.json invalid / missing");
      return false;
   }

   // กันกรณีมีใครไปสลับโหมดเป็น AP
   if (WiFi.getMode() != WIFI_STA)
   {
      applyStaSettings_();
   }

   Serial.printf("[NET] Connecting to SSID: %s\n", _cfg.ssid.c_str());
   WiFi.begin(_cfg.ssid.c_str(), _cfg.password.c_str());

   const uint32_t start = millis();
   while (!isConnected() && (millis() - start) < timeoutMs)
   {
      delay(250);
   }

   if (isConnected())
   {
      Serial.printf("[NET] Connected, IP=%s\n", WiFi.localIP().toString().c_str());
      return true;
   }

   Serial.println("[NET] Connect timeout");
   return false;
}

void Esp32WiFiNetwork::disconnect()
{
   if (WiFi.getMode() != WIFI_STA && WiFi.getMode() != WIFI_AP_STA)
      return;

   if (isConnected())
   {
      Serial.println("[NET] Disconnecting...");
      WiFi.disconnect(true /*wifioff*/);
   }
   else
   {
      // ปิดวิทยุไว้เลยถ้าไม่ได้ใช้
      WiFi.disconnect(true);
   }
}