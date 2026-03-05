// include/drivers/Esp32NetModeSwitch.h
#pragma once
#include <Arduino.h>
#include "interfaces/INetModeSource.h"

class Esp32NetModeSwitch : public INetModeSource
{
public:
   Esp32NetModeSwitch(int pin, uint32_t debounceMs = 50)
       : _pin(pin), _debounceMs(debounceMs) {}

   void begin() override;
   NetDesiredMode read() override;

private:
   int _pin;
   uint32_t _debounceMs;

   NetDesiredMode _stable = NetDesiredMode::AP_PRIMARY;
   NetDesiredMode _lastRaw = NetDesiredMode::AP_PRIMARY;
   uint32_t _lastChangeMs = 0;

   NetDesiredMode readRaw() const;
};