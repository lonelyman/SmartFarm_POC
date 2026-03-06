#pragma once
#include <Arduino.h>

/**
 * Esp32ManualSwitch — อ่านสวิตช์ทางกายภาพแบบ active LOW + debounce
 * default: ใช้ INPUT_PULLUP (pull-up ภายใน)
 * optional: ถ้าสายยาว/มี noise สามารถเพิ่ม pull-up ภายนอก 10kΩ ไป 3.3V ได้
 * ไม่กด = HIGH, กด = LOW
 */
class Esp32ManualSwitch
{
public:
   Esp32ManualSwitch(int pin, uint32_t debounceMs = 50)
       : _pin(pin), _debounceMs(debounceMs) {}

   void begin()
   {
      // default: ใช้ pull-up ภายใน (และยังอนุญาตให้มี pull-up ภายนอกได้)
      pinMode(_pin, INPUT_PULLUP);
      _stable = readRaw();
      _lastRaw = _stable;
      _lastChangeMs = millis();
   }

   // คืน true เมื่อกดสวิตช์ (active LOW)
   bool isPressed()
   {
      bool raw = readRaw();
      uint32_t now = millis();

      if (raw != _lastRaw)
      {
         _lastRaw = raw;
         _lastChangeMs = now;
      }

      if ((now - _lastChangeMs) >= _debounceMs)
         _stable = _lastRaw;

      return _stable; // true = กดอยู่
   }

private:
   int _pin;
   uint32_t _debounceMs;
   bool _stable = false;
   bool _lastRaw = false;
   uint32_t _lastChangeMs = 0;

   bool readRaw() const
   {
      // active LOW: กด = LOW = true
      return (digitalRead(_pin) == LOW);
   }
};