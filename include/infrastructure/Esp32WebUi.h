#ifndef ESP32_WEB_UI_H
#define ESP32_WEB_UI_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "../interfaces/IUi.h"
#include "infrastructure/SystemContext.h"

class Esp32WebUi : public IUi
{
public:
   explicit Esp32WebUi(SystemContext &ctx, uint16_t port = 80);

   bool begin() override;
   void tick() override; // ✅ เปลี่ยนจาก poll() เป็น tick()

private:
   SystemContext &_ctx;
   WebServer _server;
   bool _started;

   void registerRoutes();
   bool ensureStartedWhenHasIp();
};

#endif