#include "infrastructure/Esp32ModeSwitchSource.h"

Esp32ModeSwitchSource::Esp32ModeSwitchSource(int pinA, int pinB)
    : _pinA(pinA), _pinB(pinB) {}

void Esp32ModeSwitchSource::begin()
{
   // default: Active LOW + INPUT_PULLUP (ตาม Config.h)
   pinMode(_pinA, INPUT_PULLUP);
   pinMode(_pinB, INPUT_PULLUP);
}

SystemMode Esp32ModeSwitchSource::decode(int a, int b) const
{
   // ยึด mapping ที่คุณเขียนไว้ใน README:
   // A=HIGH,B=HIGH -> IDLE
   // A=LOW,B=HIGH  -> AUTO
   // A=HIGH,B=LOW  -> MANUAL
   if (a == HIGH && b == HIGH)
      return SystemMode::IDLE;
   if (a == LOW && b == HIGH)
      return SystemMode::AUTO;
   if (a == HIGH && b == LOW)
      return SystemMode::MANUAL;

   // กรณีผิดปกติ (LOW,LOW) ให้ปลอดภัยสุดเป็น IDLE
   return SystemMode::IDLE;
}

SystemMode Esp32ModeSwitchSource::readMode()
{
   int a = digitalRead(_pinA);
   int b = digitalRead(_pinB);
   return decode(a, b);
}