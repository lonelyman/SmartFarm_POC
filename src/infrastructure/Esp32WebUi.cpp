// src/infrastructure/Esp32WebUi.cpp
#include "infrastructure/Esp32WebUi.h"

#include <LittleFS.h>
#include <WiFi.h>
#include <ESP.h>
#include <ArduinoJson.h>

// ============================================================
//  Constructor / setContext
// ============================================================

Esp32WebUi::Esp32WebUi(uint16_t port)
    : _server(port)
{
}

void Esp32WebUi::setContext(SystemContext *ctx)
{
   _ctx = ctx;
}

// ============================================================
//  begin / tick
// ============================================================

bool Esp32WebUi::begin()
{
   if (!_ctx)
   {
      Serial.println("❌ [WebUI] begin() called before setContext()");
      return false;
   }

   // โหลด wifi config ครั้งเดียว — cache ไว้ใน _wifiCfg
   WifiConfigStore store;
   _wifiCfgLoaded = store.load(_wifiCfg) && _wifiCfg.isValid();

   registerRoutes();
   Serial.println("ℹ️ [WebUI] routes registered");
   return true;
}

void Esp32WebUi::tick()
{
   // restart ที่นี่ (หลัง handleClient ส่ง response เสร็จแล้ว)
   if (_pendingRestart)
   {
      delay(200);
      ESP.restart();
   }

   if (!_started)
   {
      if (!ensureStartedWhenHasIp())
         return;
   }

   _server.handleClient();
}

// ============================================================
//  Lazy start
// ============================================================

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
      Serial.printf("[WEB] STA IP:  http://%s/\n", WiFi.localIP().toString().c_str());

   return true;
}

// ============================================================
//  JSON builders
// ============================================================

String Esp32WebUi::buildStatusJson() const
{
   const auto snap = _ctx->state->getSnapshot();
   const auto wifiMode = WiFi.getMode();
   const bool staConn = (WiFi.status() == WL_CONNECTED);

   char netMsg[96];
   _ctx->state->getNetMessage(netMsg, sizeof(netMsg));

   IPAddress apIp(0, 0, 0, 0);
   if (wifiMode == WIFI_AP || wifiMode == WIFI_AP_STA)
      apIp = WiFi.softAPIP();

   IPAddress staIp(0, 0, 0, 0);
   if (staConn)
      staIp = WiFi.localIP();

   JsonDocument doc;
   doc["mode"] = (int)snap.mode;
   doc["pump"] = snap.isPumpActive;
   doc["mist"] = snap.isMistActive;
   doc["air"] = snap.isAirPumpActive;
   doc["lux"] = serialized(String(snap.light.value, 1));
   doc["luxValid"] = snap.light.isValid;
   doc["temp"] = serialized(String(snap.temperature.value, 2));
   doc["tempValid"] = snap.temperature.isValid;
   doc["hum"] = serialized(String(snap.humidity.value, 1));
   doc["humValid"] = snap.humidity.isValid;
   doc["wifiMode"] = (int)wifiMode;
   doc["staConnected"] = staConn;
   doc["apIp"] = apIp.toString();
   doc["staIp"] = staIp.toString();
   doc["netState"] = (int)_ctx->state->getNetState();
   doc["netMsg"] = netMsg;

   String out;
   serializeJson(doc, out);
   return out;
}

String Esp32WebUi::buildWifiConfigJson() const
{
   JsonDocument doc;
   doc["hasConfig"] = _wifiCfgLoaded;
   doc["ssid"] = _wifiCfgLoaded ? _wifiCfg.ssid.c_str() : "";
   doc["hasPassword"] = _wifiCfgLoaded && _wifiCfg.password.length() > 0;
   doc["hostname"] = _wifiCfgLoaded ? _wifiCfg.hostname.c_str() : "";
   doc["apSsid"] = _wifiCfgLoaded ? _wifiCfg.apSsid.c_str() : "";
   doc["hasApPass"] = _wifiCfgLoaded && _wifiCfg.apPass.length() >= 8;

   String out;
   serializeJson(doc, out);
   return out;
}

// ============================================================
//  Helpers
// ============================================================

static void serveFileOr500(WebServer &srv, const char *path, const char *mime)
{
   File f = LittleFS.open(path, "r");
   if (!f)
   {
      srv.send(500, "text/plain; charset=utf-8",
               String("file not found: ") + path);
      return;
   }
   srv.streamFile(f, mime);
   f.close();
}

// ============================================================
//  Routes
// ============================================================

void Esp32WebUi::registerRoutes()
{
   // --- Static pages ---

   _server.on("/", HTTP_GET, [&]()
              { serveFileOr500(_server, "/www/dashboard.html", "text/html; charset=utf-8"); });

   _server.on("/wifi", HTTP_GET, [&]()
              { serveFileOr500(_server, "/www/wifi.html", "text/html; charset=utf-8"); });

   _server.on("/wifi-saved", HTTP_GET, [&]()
              { serveFileOr500(_server, "/www/wifi-saved.html", "text/html; charset=utf-8"); });

   // --- GET /api/wifi/config ---

   _server.on("/api/wifi/config", HTTP_GET, [&]()
              { _server.send(200, "application/json; charset=utf-8", buildWifiConfigJson()); });

   // --- GET /api/status ---

   _server.on("/api/status", HTTP_GET, [&]()
              { _server.send(200, "application/json; charset=utf-8", buildStatusJson()); });

   // --- Mode control ---

   _server.on("/api/mode/auto", HTTP_POST, [&]()
              { _ctx->state->setMode(SystemMode::AUTO);   _server.send(204); });
   _server.on("/api/mode/manual", HTTP_POST, [&]()
              { _ctx->state->setMode(SystemMode::MANUAL); _server.send(204); });
   _server.on("/api/mode/idle", HTTP_POST, [&]()
              { _ctx->state->setMode(SystemMode::IDLE);   _server.send(204); });

   // --- Network control ---

   _server.on("/api/net/on", HTTP_POST, [&]()
              { _ctx->state->requestNetOn();   _server.send(204); });
   _server.on("/api/net/off", HTTP_POST, [&]()
              { _ctx->state->requestNetOff();  _server.send(204); });
   _server.on("/api/ntp", HTTP_POST, [&]()
              { _ctx->state->requestSyncNtp(); _server.send(204); });

   // --- POST /save → บันทึก wifi.json แล้ว reboot ---

   _server.on("/save", HTTP_POST, [&]()
              {
      String ssid     = _server.arg("ssid");
      String pass     = _server.arg("password");
      String hostname = _server.arg("hostname");
      String apSsid   = _server.arg("apSsid");
      String apPass   = _server.arg("apPass");

      ssid.trim();
      hostname.trim();
      apSsid.trim();

      if (ssid.length() == 0)
      {
         _server.send(400, "text/plain; charset=utf-8", "SSID is required");
         return;
      }

      WifiConfigStore store;
      WifiConfig cfg;
      cfg.ssid     = ssid;
      cfg.password = pass;
      cfg.hostname = hostname.length() > 0 ? hostname : "smartfarm";
      cfg.apSsid   = apSsid.length()   > 0 ? apSsid   : "SmartFarm-Setup";
      cfg.apPass   = apPass;

      // ถ้า password field ว่าง → ใช้ค่าเดิมจาก store (ผู้ใช้ไม่ได้ตั้งใจเปลี่ยน)
      if (cfg.password.length() == 0 || cfg.apPass.length() == 0)
      {
         WifiConfig old;
         if (store.load(old))
         {
            if (cfg.password.length() == 0) cfg.password = old.password;
            if (cfg.apPass.length()   == 0) cfg.apPass   = old.apPass;
         }
      }

      if (!store.save(cfg))
      {
         _server.send(500, "text/plain; charset=utf-8", "Save failed");
         return;
      }

      // อัปเดต cache ทันที
      _wifiCfg       = cfg;
      _wifiCfgLoaded = true;

      serveFileOr500(_server, "/www/wifi-saved.html", "text/html; charset=utf-8");
      _pendingRestart = true; });
}