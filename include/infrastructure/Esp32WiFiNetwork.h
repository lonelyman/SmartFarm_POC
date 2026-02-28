#pragma once

#include "interfaces/INetwork.h"

class Esp32WiFiNetwork : public INetwork
{
public:
   Esp32WiFiNetwork(const char *ssid, const char *pass);

   void begin() override;
   bool ensureConnected(uint32_t timeoutMs) override;
   bool isConnected() const override;
   void disconnect() override;

private:
   const char *_ssid;
   const char *_pass;
};