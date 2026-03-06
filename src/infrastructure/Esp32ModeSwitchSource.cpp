#include "infrastructure/Esp32ModeSwitchSource.h"
#include "Config.h"

Esp32ModeSwitchSource::Esp32ModeSwitchSource(int pinA, int pinB, uint32_t debounceMs)
    : _pinA(pinA), _pinB(pinB), _debounceMs(debounceMs) {}

void Esp32ModeSwitchSource::begin()
{
   // ใช้ INPUT_PULLUP ร่วมกับ external R10kΩ ที่ค่อมอยู่
   pinMode(_pinA, INPUT_PULLUP);
   pinMode(_pinB, INPUT_PULLUP);

   // อ่านค่าเริ่มต้นทันทีเพื่อไม่ให้ debounce หน่วงตอน boot
   _stable = readRaw();
   _lastRaw = _stable;
   _lastChangeMs = millis();
}

SystemMode Esp32ModeSwitchSource::readRaw() const
{
   return decode(digitalRead(_pinA), digitalRead(_pinB));
}

SystemMode Esp32ModeSwitchSource::decode(int a, int b) const
{
   if (a == HIGH && b == HIGH)
      return SystemMode::IDLE;
   if (a == LOW && b == HIGH)
      return SystemMode::AUTO;
   if (a == HIGH && b == LOW)
      return SystemMode::MANUAL;

   // LOW,LOW — ผิดปกติ (สวิตช์เสีย / noise) → fallback ปลอดภัยสุด
   return SystemMode::IDLE;
}

SystemMode Esp32ModeSwitchSource::readMode()
{
   const SystemMode raw = readRaw();
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