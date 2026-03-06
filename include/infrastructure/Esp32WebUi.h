// include/infrastructure/Esp32WebUi.h
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
   // ctx รับเป็น pointer เพื่อให้ inject ได้หลัง construct
   // (แก้ปัญหา circular dependency: ctx ต้องการ &webUi, webUi ต้องการ ctx)
   explicit Esp32WebUi(uint16_t port = 80);
   void setContext(SystemContext *ctx);

   bool begin() override;
   void tick() override;

private:
   SystemContext *_ctx = nullptr;
   WebServer _server;
   bool _started;

   void registerRoutes();
   bool ensureStartedWhenHasIp();
};

#endif