// src/infrastructure/Esp32WebUi.cpp
#include "infrastructure/Esp32WebUi.h"
#include <LittleFS.h>

// หมายเหตุ:
// - UI หลักถูกย้ายไปอยู่ที่ LittleFS: /index.html
// - เส้นทาง /api/status ยังคืน JSON (เพื่อให้หน้าเว็บไป render เป็น dashboard เอง)
// - ไม่เดา field อื่นนอกเหนือจากที่คุณมีอยู่แล้วใน SharedState/SystemStatus

Esp32WebUi::Esp32WebUi(SystemContext &ctx, uint16_t port)
    : _ctx(ctx), _server(port), _started(false)
{
}

bool Esp32WebUi::begin()
{
   // เตรียม route ไว้ก่อน แต่ยังไม่ start server จนกว่าจะมี IP
   registerRoutes();
   Serial.println("ℹ️ WebUI: routes registered");
   return true;
}

void Esp32WebUi::tick()
{
   // ยังไม่เริ่ม server ถ้ายังไม่มี IP
   if (!_started)
   {
      if (!ensureStartedWhenHasIp())
         return;
   }

   // ปั๊ม web server
   _server.handleClient();
}

bool Esp32WebUi::ensureStartedWhenHasIp()
{
   if (WiFi.status() != WL_CONNECTED)
      return false;

   IPAddress ip = WiFi.localIP();
   if (ip[0] == 0)
      return false;

   _server.begin();
   _started = true;

   Serial.printf("[WEB] UI ready: http://%s/\n", ip.toString().c_str());
   return true;
}

void Esp32WebUi::registerRoutes()
{
   // หน้า UI (โหลดจาก LittleFS)
   _server.on("/", HTTP_GET, [&]()
              {
      File f = LittleFS.open("/index.html", "r");
      if (!f)
      {
         _server.send(500, "text/plain; charset=utf-8", "index.html not found in LittleFS");
         return;
      }
      _server.streamFile(f, "text/html; charset=utf-8");
      f.close(); });

   // STATUS JSON
   _server.on("/api/status", HTTP_GET, [&]()
              {
      auto snap = _ctx.state->getSnapshot();

      String json = "{";
      json += "\"mode\":" + String((int)snap.mode) + ",";

      json += "\"pump\":" + String(snap.isPumpActive ? 1 : 0) + ",";
      json += "\"mist\":" + String(snap.isMistActive ? 1 : 0) + ",";
      json += "\"air\":"  + String(snap.isAirPumpActive ? 1 : 0) + ",";

      json += "\"lux\":"  + String(snap.light.value, 1) + ",";
      json += "\"luxValid\":" + String(snap.light.isValid ? 1 : 0) + ",";
      json += "\"temp\":" + String(snap.temperature.value, 2) + ",";
      json += "\"tempValid\":" + String(snap.temperature.isValid ? 1 : 0);

      json += "}";
      _server.send(200, "application/json; charset=utf-8", json); });

   // (เผื่ออนาคต) ถ้าจะสั่งโหมดผ่าน UI ก็เปิดใช้ได้
   _server.on("/api/mode/auto", HTTP_POST, [&]()
              { _ctx.state->setMode(SystemMode::AUTO); _server.send(204); });
   _server.on("/api/mode/manual", HTTP_POST, [&]()
              { _ctx.state->setMode(SystemMode::MANUAL); _server.send(204); });
   _server.on("/api/mode/idle", HTTP_POST, [&]()
              { _ctx.state->setMode(SystemMode::IDLE); _server.send(204); });

   // network control
   _server.on("/api/net/on", HTTP_POST, [&]()
              { _ctx.state->requestNetOn(); _server.send(204); });
   _server.on("/api/net/off", HTTP_POST, [&]()
              { _ctx.state->requestNetOff(); _server.send(204); });
   _server.on("/api/ntp", HTTP_POST, [&]()
              { _ctx.state->requestSyncNtp(); _server.send(204); });
}