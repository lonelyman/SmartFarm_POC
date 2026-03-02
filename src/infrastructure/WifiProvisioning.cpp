// src/infrastructure/WifiProvisioning.cpp
#include "infrastructure/WifiProvisioning.h"

WifiProvisioning::WifiProvisioning(const char *configPath, uint16_t port)
    : _store(configPath), _server(port)
{
}

void WifiProvisioning::tick()
{
   // ✅ 0) ถ้า provisioning จบแล้ว ไม่ทำอะไร
   if (_done)
   {
      return;
   }

   // ✅ 1) เช็ค config “ครั้งเดียวต่อบูต”
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

   // ✅ 2) handle client เฉพาะตอนเปิด AP แล้ว
   if (_started)
   {
      _server.handleClient();
   }

   // ✅ 3) Deferred reboot (ห้าม restart ใน handler)
   if (_pendingRestart && (int32_t)(millis() - _restartAtMs) >= 0)
   {
      Serial.println("🔄 [PROV] reboot now");
      delay(200);
      ESP.restart();
   }
}

void WifiProvisioning::startAp()
{
   WiFi.mode(WIFI_AP);
   WiFi.softAP(_apSsid.c_str());
}

void WifiProvisioning::registerRoutes()
{
   _server.on("/", HTTP_GET, [&]()
              { _server.send(200, "text/html; charset=utf-8", pageFormHtml()); });

   _server.on("/save", HTTP_POST, [&]()
              {
                 String ssid = _server.arg("ssid");
                 String pass = _server.arg("password");

                 WifiConfig cfg;
                 cfg.ssid = ssid;
                 cfg.password = pass;
                 cfg.hostname = "smartfarm";

                 if (!_store.save(cfg))
                 {
                    _server.send(500, "text/plain; charset=utf-8", "Save failed");
                    return;
                 }

                 _server.send(200, "text/html; charset=utf-8", pageSavedHtml());
                 Serial.printf("✅ [PROV] saved wifi.json (ssid=%s)\n", ssid.c_str());

                 _done = true;
                 _pendingRestart = true;
                 _restartAtMs = millis() + 1200; // หน่วงให้ response ส่งออกก่อน
              });
}

// ชั่วคราว: html ไว้ใน .cpp ก่อน (Step ถัดไปค่อยแยกไฟล์)
const char *WifiProvisioning::pageFormHtml() const
{
   return R"HTML(
<!doctype html><html>
<head><meta name=viewport content="width=device-width, initial-scale=1">
<style>
body{font-family:Arial;margin:16px}
input{width:100%;padding:10px;margin:6px 0}
button{padding:10px 14px;margin-top:8px}
.card{max-width:420px;margin:auto;border:1px solid #ddd;border-radius:10px;padding:14px}
</style></head>
<body>
<div class="card">
<h3>SmartFarm WiFi Setup</h3>
<form method="POST" action="/save">
<label>SSID</label>
<input name="ssid" placeholder="WiFi name">
<label>Password</label>
<input name="password" placeholder="WiFi password" type="password">
<button type="submit">Save</button>
</form>
</div>
</body></html>
)HTML";
}

const char *WifiProvisioning::pageSavedHtml() const
{
   return R"HTML(
<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1">
</head><body>
<h3>Saved ✅</h3>
<p>Device will reboot now...</p>
</body></html>
)HTML";
}