#pragma once

#include <Arduino.h>
#include "interfaces/IModeSource.h"

class Esp32ModeSwitchSource : public IModeSource
{
public:
   Esp32ModeSwitchSource(int pinA, int pinB);

   void begin() override;
   SystemMode readMode() override;

private:
   int _pinA;
   int _pinB;

   SystemMode decode(int a, int b) const;
};