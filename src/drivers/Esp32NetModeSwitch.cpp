// src/drivers/Esp32NetModeSwitch.cpp
#include "drivers/Esp32NetModeSwitch.h"

void Esp32NetModeSwitch::begin()
{
   pinMode(_pin, INPUT_PULLUP); // default: INPUT_PULLUP (GPIO39 input-only); logic: HIGH=STA, LOW=AP
   _stable = readRaw();
   _lastRaw = _stable;
   _lastChangeMs = millis();
}

NetDesiredMode Esp32NetModeSwitch::readRaw() const
{
   // logic ตาม Config.h / README: HIGH = STA, LOW = AP
   return (digitalRead(_pin) == HIGH)
              ? NetDesiredMode::STA_PREFERRED
              : NetDesiredMode::AP_PRIMARY;
}

NetDesiredMode Esp32NetModeSwitch::read()
{
   const NetDesiredMode raw = readRaw();
   const uint32_t now = millis();

   if (raw != _lastRaw)
   {
      _lastRaw = raw;
      _lastChangeMs = now;
   }

   if ((now - _lastChangeMs) >= _debounceMs)
      _stable = _lastRaw;

   return _stable;
}