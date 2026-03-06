#pragma once
#include <Arduino.h>
#include "../Config.h"

struct WaterLevelReadings
{
   bool ch1Low = false;
   bool ch2Low = false;
};

class Esp32WaterLevelInput
{
public:
   Esp32WaterLevelInput(int ch1Pin, int ch2Pin) : _ch1Pin(ch1Pin), _ch2Pin(ch2Pin) {}

   void begin()
   {
      pinMode(_ch1Pin, INPUT_PULLUP);
      pinMode(_ch2Pin, INPUT_PULLUP);
   }

   WaterLevelReadings read() const
   {
      WaterLevelReadings r{};
#if ENABLE_WATER_LEVEL_CH1
      const bool rawLow1 = (digitalRead(_ch1Pin) == LOW);
      r.ch1Low = WATER_LEVEL_ALARM_LOW ? rawLow1 : !rawLow1;
#else
      r.ch1Low = false; // disable -> never alarm
#endif

#if ENABLE_WATER_LEVEL_CH2
      const bool rawLow2 = (digitalRead(_ch2Pin) == LOW);
      r.ch2Low = WATER_LEVEL_ALARM_LOW ? rawLow2 : !rawLow2;
#else
      r.ch2Low = false; // disable -> never alarm
#endif

      return r;
   }

private:
   int _ch1Pin;
   int _ch2Pin;
};