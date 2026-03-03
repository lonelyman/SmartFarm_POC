// src/infrastructure/Esp32WebUi.cpp
#include <LittleFS.h>
#include <WiFi.h>
#include <ESP.h>
#include "infrastructure/Esp32WebUi.h"
#include "infrastructure/WifiConfigStore.h"

Esp32WebUi::Esp32WebUi(SystemContext &ctx, uint16_t port)
    : _ctx(ctx), _server(port), _started(false)
{
}

bool Esp32WebUi::begin()
{
   registerRoutes();
   Serial.println("ℹ️ WebUI: routes registered");
   return true;
}

void Esp32WebUi::tick()
{
   if (!_started)
   {
      if (!ensureStartedWhenHasIp())
         return;
   }
   _server.handleClient();
}

bool Esp32WebUi::ensureStartedWhenHasIp()
{
   if (_started)
      return true;

   IPAddress ip(0, 0, 0, 0);

   const wifi_mode_t mode = WiFi.getMode();
   if (mode == WIFI_AP || mode == WIFI_AP_STA)
      ip = WiFi.softAPIP();

   if (ip[0] == 0 && WiFi.status() == WL_CONNECTED)
      ip = WiFi.localIP();

   if (ip[0] == 0)
      return false;

   _server.begin();
   _started = true;

   Serial.printf("[WEB] UI ready: http://%s/\n", ip.toString().c_str());

   if (mode == WIFI_AP_STA && WiFi.status() == WL_CONNECTED)
      Serial.printf("[WEB] STA IP: http://%s/\n", WiFi.localIP().toString().c_str());

   return true;
}

static void serveFileOr500(WebServer &srv, const char *path, const char *mime)
{
   File f = LittleFS.open(path, "r");
   if (!f)
   {
      String msg = String("file not found in LittleFS: ") + path;
      srv.send(500, "text/plain; charset=utf-8", msg);
      return;
   }
   srv.streamFile(f, mime);
   f.close();
}

void Esp32WebUi::registerRoutes()
{
   // Dashboard
   _server.on("/", HTTP_GET, [&]()
              { serveFileOr500(_server, "/www/dashboard.html", "text/html; charset=utf-8"); });

   // ✅ WiFi setup page (หน้าเดิมของคุณ)
   _server.on("/wifi", HTTP_GET, [&]()
              { serveFileOr500(_server, "/www/wifi.html", "text/html; charset=utf-8"); });

   // ✅ Saved page (เผื่อเรียกตรง ๆ ได้ / หรือใช้จาก provisioning ก็ได้)
   _server.on("/wifi-saved", HTTP_GET, [&]()
              { serveFileOr500(_server, "/www/wifi-saved.html", "text/html; charset=utf-8"); });

   // STATUS JSON (+ network fields + netMsg)
   _server.on("/api/status", HTTP_GET, [&]()
              {
      auto snap = _ctx.state->getSnapshot();

      const wifi_mode_t mode = WiFi.getMode();
      const bool staConnected = (WiFi.status() == WL_CONNECTED);

      char netMsg[96];
      _ctx.state->getNetMessage(netMsg, sizeof(netMsg));

      IPAddress apIp(0, 0, 0, 0);
      if (mode == WIFI_AP || mode == WIFI_AP_STA) apIp = WiFi.softAPIP();

      IPAddress staIp(0, 0, 0, 0);
      if (staConnected) staIp = WiFi.localIP();

      String json = "{";
      json += "\"mode\":" + String((int)snap.mode) + ",";

      json += "\"pump\":" + String(snap.isPumpActive ? 1 : 0) + ",";
      json += "\"mist\":" + String(snap.isMistActive ? 1 : 0) + ",";
      json += "\"air\":"  + String(snap.isAirPumpActive ? 1 : 0) + ",";

      json += "\"lux\":"  + String(snap.light.value, 1) + ",";
      json += "\"luxValid\":" + String(snap.light.isValid ? 1 : 0) + ",";
      json += "\"temp\":" + String(snap.temperature.value, 2) + ",";
      json += "\"tempValid\":" + String(snap.temperature.isValid ? 1 : 0) + ",";

      json += "\"wifiMode\":" + String((int)mode) + ",";
      json += "\"staConnected\":" + String(staConnected ? 1 : 0) + ",";
      json += "\"apIp\":\"" + apIp.toString() + "\",";
      json += "\"staIp\":\"" + staIp.toString() + "\",";

      // ✅ UI feedback message
      json += "\"netMsg\":\"" + String(netMsg) + "\"";

      json += "}";
      _server.send(200, "application/json; charset=utf-8", json); });

   // (เผื่ออนาคต) mode control
   _server.on("/api/mode/auto", HTTP_POST, [&]()
              { _ctx.state->setMode(SystemMode::AUTO); _server.send(204); });
   _server.on("/api/mode/manual", HTTP_POST, [&]()
              { _ctx.state->setMode(SystemMode::MANUAL); _server.send(204); });
   _server.on("/api/mode/idle", HTTP_POST, [&]()
              { _ctx.state->setMode(SystemMode::IDLE); _server.send(204); });

   // network control (ส่งแค่ command ให้ NetworkTask ไปทำ)
   _server.on("/api/net/on", HTTP_POST, [&]()
              { _ctx.state->requestNetOn(); _server.send(204); });
   _server.on("/api/net/off", HTTP_POST, [&]()
              { _ctx.state->requestNetOff(); _server.send(204); });
   _server.on("/api/ntp", HTTP_POST, [&]()
              { _ctx.state->requestSyncNtp(); _server.send(204); });

   // ---------- WIFI PAGES (single server) ----------

   // GET /wifi -> serve wifi.html
   _server.on("/wifi", HTTP_GET, [&]()
              {
   File f = LittleFS.open("/www/wifi.html", "r");
   if (!f) {
      _server.send(500, "text/plain; charset=utf-8", "wifi.html not found in LittleFS: /www/wifi.html");
      return;
   }
   _server.streamFile(f, "text/html; charset=utf-8");
   f.close(); });

   // (optional) GET /saved -> serve wifi-saved.html (ไว้ทดสอบ)
   _server.on("/saved", HTTP_GET, [&]()
              {
   File f = LittleFS.open("/www/wifi-saved.html", "r");
   if (!f) {
      _server.send(404, "text/plain; charset=utf-8", "wifi-saved.html not found in LittleFS: /www/wifi-saved.html");
      return;
   }
   _server.streamFile(f, "text/html; charset=utf-8");
   f.close(); });

   // POST /save -> save /wifi.json -> show saved page -> reboot
   _server.on("/save", HTTP_POST, [&]()
              {
   String ssid = _server.arg("ssid");
   String pass = _server.arg("password");
   ssid.trim();

   if (ssid.length() == 0) {
      _server.send(400, "text/plain; charset=utf-8", "SSID is required");
      return;
   }

   WifiConfigStore store("/wifi.json");
   WifiConfig cfg;
   cfg.ssid = ssid;
   cfg.password = pass;
   cfg.hostname = "smartfarm";

   if (!store.save(cfg)) {
      _server.send(500, "text/plain; charset=utf-8", "Save failed");
      return;
   }

   // ส่งหน้า saved.html ให้ browser เห็นผล
   File f = LittleFS.open("/www/wifi-saved.html", "r");
   if (f) {
      _server.streamFile(f, "text/html; charset=utf-8");
      f.close();
   } else {
      _server.send(200, "text/plain; charset=utf-8", "Saved. Rebooting...");
   }

   delay(1200);
   ESP.restart(); });
}