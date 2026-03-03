// src/infrastructure/WifiProvisioning.cpp
#include "infrastructure/WifiProvisioning.h"
#include <LittleFS.h>

WifiProvisioning::WifiProvisioning(const char *configPath, uint16_t port)
    : _store(configPath), _server(port)
{
}

void WifiProvisioning::tick()
{
   // 1) Deferred reboot ต้องมาก่อน และต้องทำได้แม้ _done=true
   if (_pendingRestart && (int32_t)(millis() - _restartAtMs) >= 0)
   {
      Serial.println("🔄 [PROV] reboot now");
      delay(200);
      ESP.restart();
      return; // safety
   }

   // 2) ถ้า provisioning จบแล้ว และไม่มี pending restart ก็ไม่ต้องทำอะไร
   if (_done)
   {
      return;
   }

   // 3) เช็ค config “ครั้งเดียวต่อบูต”
   if (!_checkedConfig)
   {
      _checkedConfig = true;

      WifiConfig cfg;
      if (_store.load(cfg) && cfg.isValid())
      {
         Serial.println("✅ [PROV] wifi.json valid -> skip provisioning");
         _done = true;
         return;
      }

      Serial.println("⚠️ [PROV] wifi.json invalid -> start AP provisioning");
      startAp();
      registerRoutes();
      _server.begin();
      _started = true;

      Serial.printf("✅ [PROV] AP ready: SSID=%s IP=%s\n",
                    _apSsid.c_str(), WiFi.softAPIP().toString().c_str());
   }

   // 4) handle client เฉพาะตอนเปิด AP แล้ว
   if (_started)
   {
      _server.handleClient();
   }
}

void WifiProvisioning::startAp()
{
   WiFi.mode(WIFI_AP);
   WiFi.softAP(_apSsid.c_str());
}

void WifiProvisioning::registerRoutes()
{
   // GET / -> serve wifi.html จาก LittleFS
   _server.on("/", HTTP_GET, [&]()
              {
                 if (!LittleFS.exists("/www/wifi.html"))
                 {
                    _server.send(500, "text/plain; charset=utf-8",
                                 "wifi.html not found in LittleFS: /www/wifi.html");
                    return;
                 }

                 File f = LittleFS.open("/www/wifi.html", "r");
                 if (!f)
                 {
                    _server.send(500, "text/plain; charset=utf-8",
                                 "open wifi.html failed");
                    return;
                 }

                 _server.streamFile(f, "text/html; charset=utf-8");
                 f.close(); });

   // POST /save -> save wifi.json, then serve saved.html, then deferred reboot
   _server.on("/save", HTTP_POST, [&]()
              {
                 String ssid = _server.arg("ssid");
                 String pass = _server.arg("password");

                 ssid.trim();
                 // pass ไม่ trim ก็ได้ เผื่อมี space จริง (แต่ส่วนใหญ่ไม่มี)

                 if (ssid.length() == 0)
                 {
                    _server.send(400, "text/plain; charset=utf-8", "SSID is required");
                    return;
                 }

                 WifiConfig cfg;
                 cfg.ssid = ssid;
                 cfg.password = pass;
                 cfg.hostname = "smartfarm";

                 if (!_store.save(cfg))
                 {
                    _server.send(500, "text/plain; charset=utf-8", "Save failed");
                    return;
                 }

                 Serial.printf("✅ [PROV] saved wifi.json (ssid=%s)\n", ssid.c_str());

                 // ส่งหน้า saved.html (ถ้ามี) เพื่อให้ browser ได้ response ที่ “ไม่ว่าง”
                 if (LittleFS.exists("/www/wifi-saved.html"))
                 {
                    File f = LittleFS.open("/www/wifi-saved.html", "r");
                    if (f)
                    {
                       _server.streamFile(f, "text/html; charset=utf-8");
                       f.close();
                    }
                    else
                    {
                       _server.send(200, "text/plain; charset=utf-8", "Saved. Rebooting...");
                    }
                 }
                 else
                 {
                    _server.send(200, "text/plain; charset=utf-8", "Saved. Rebooting...");
                 }

                 // deferred reboot (tick() จะทำก่อน _done)
                 _pendingRestart = true;
                 _restartAtMs = millis() + 1200;
                 _done = true; });
}