// src/drivers/Esp32NetModeSwitch.cpp
#include "drivers/Esp32NetModeSwitch.h"
#include "Config.h"

Esp32NetModeSwitch::Esp32NetModeSwitch(int pin, uint32_t debounceMs)
    : _pin(pin), _debounceMs(debounceMs) {}

void Esp32NetModeSwitch::begin()
{
   // ใช้ INPUT_PULLUP ร่วมกับ external R10kΩ ที่ค่อมอยู่
   pinMode(_pin, INPUT_PULLUP);

   // อ่านค่าเริ่มต้นทันทีเพื่อไม่ให้ debounce หน่วงตอน boot
   _stable = readRaw();
   _lastRaw = _stable;
   _lastChangeMs = millis();
}

NetDesiredMode Esp32NetModeSwitch::readRaw() const
{
   // HIGH = บิดซ้าย (วงจรเปิด) = AP
   // LOW  = บิดขวา  (วงจรปิด)  = STA
   return (digitalRead(_pin) == HIGH)
              ? NetDesiredMode::AP_PRIMARY
              : NetDesiredMode::STA_PREFERRED;
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