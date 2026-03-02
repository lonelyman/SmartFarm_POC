#pragma once
#include <Arduino.h>

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
      pinMode(_ch1Pin, INPUT);
      pinMode(_ch2Pin, INPUT);
   }

   WaterLevelReadings read() const
   {
      WaterLevelReadings r{};
#if ENABLE_WATER_LEVEL_CH1
      r.ch1Low = (digitalRead(_ch1Pin) == LOW);
#else
      r.ch1Low = false; // disable -> never alarm
#endif

#if ENABLE_WATER_LEVEL_CH2
      r.ch2Low = (digitalRead(_ch2Pin) == LOW);
#else
      r.ch2Low = false; // disable -> never alarm
#endif

      return r;
   }

private:
   int _ch1Pin;
   int _ch2Pin;
};