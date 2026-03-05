// src/drivers/Esp32NetModeSwitch.cpp
#include "drivers/Esp32NetModeSwitch.h"

void Esp32NetModeSwitch::begin()
{
   pinMode(_pin, INPUT); // GPIO36 input-only ไม่มี pullup
   _stable = readRaw();
   _lastRaw = _stable;
   _lastChangeMs = millis();
}

NetDesiredMode Esp32NetModeSwitch::readRaw() const
{
   // (active LOW) กด = LOW = STA_PREFERRED, ไม่กด = HIGH = AP_PRIMARY
   return (digitalRead(_pin) == LOW)
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