// src/drivers/Esp32ManualSwitch.cpp
#include "drivers/Esp32ManualSwitch.h"

Esp32ManualSwitch::Esp32ManualSwitch(int pin, uint32_t debounceMs)
    : _pin(pin), _debounceMs(debounceMs) {}

void Esp32ManualSwitch::begin()
{
   // ใช้ INPUT_PULLUP ร่วมกับ external R10kΩ ที่ค่อมอยู่
   pinMode(_pin, INPUT_PULLUP);

   // อ่านค่าเริ่มต้นทันทีเพื่อไม่ให้ debounce หน่วงตอน boot
   _stable = readRaw();
   _lastRaw = _stable;
   _lastChangeMs = millis();
}

bool Esp32ManualSwitch::readRaw() const
{
   // บิดขวา = วงจรปิด = LOW = ON = true
   return (digitalRead(_pin) == LOW);
}

bool Esp32ManualSwitch::isOn()
{
   const bool raw = readRaw();
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