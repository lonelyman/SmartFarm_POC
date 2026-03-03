#include "drivers/Esp32NetModeSwitch.h"

void Esp32NetModeSwitch::begin()
{
   // GPIO36/39: input-only และมักไม่มี pullup/pulldown -> ใช้ INPUT เฉยๆ
   pinMode(_pinAp, INPUT);
   pinMode(_pinSta, INPUT);

   _stable = readRaw();
   _lastRaw = _stable;
   _lastChangeMs = millis();
}

NetDesiredMode Esp32NetModeSwitch::readRaw() const
{
   const int ap = digitalRead(_pinAp);
   const int sta = digitalRead(_pinSta);

   // กติกาง่ายๆ:
   // - ถ้า STA pin เป็น HIGH และ AP pin เป็น LOW -> STA_PREFERRED
   // - ค่าอื่นๆ ให้ถือว่า AP_PRIMARY (ปลอดภัย)
   if (sta == HIGH && ap == LOW)
      return NetDesiredMode::STA_PREFERRED;
   return NetDesiredMode::AP_PRIMARY;
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
   {
      _stable = _lastRaw;
   }

   return _stable;
}