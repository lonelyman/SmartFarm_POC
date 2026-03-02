#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "infrastructure/WifiConfigStore.h"

class WifiProvisioning
{
public:
   explicit WifiProvisioning(const char *configPath = "/wifi.json", uint16_t port = 80);

   void tick();
   bool isDone() const { return _done; }

private:
   WifiConfigStore _store;
   WebServer _server;

   bool _checkedConfig = false;
   bool _started = false;
   bool _done = false;

   bool _pendingRestart = false;
   uint32_t _restartAtMs = 0;

   String _apSsid = "SmartFarm-Setup";

   void startAp();
   void registerRoutes();

   // แยก html ออก (ตามที่คุณต้องการ) -> ไป Step ถัดไป
   const char *pageFormHtml() const;
   const char *pageSavedHtml() const;
};