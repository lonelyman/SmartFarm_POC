#include "application/FarmManager.h"

FarmManager::FarmManager(const AirPumpSchedule *schedule)
    : _schedule(schedule) {}

FarmDecision FarmManager::update(const FarmInput &in)
{
   FarmDecision out{};

   switch (in.mode)
   {
   case SystemMode::IDLE:
      // ปิดหมด + reset latch ให้ deterministic
      out.pumpOn = false;
      out.mistOn = false;
      out.airOn = false;

      _pumpLatched = false;
      _mistLatched = false;
      _airLatched = false;
      break;

   case SystemMode::MANUAL:
      out = applyManual(in.manual);
      break;

   case SystemMode::AUTO:
      out = applyAuto(in);
      break;
   }

   // commit latch
   _pumpLatched = out.pumpOn;
   _mistLatched = out.mistOn;
   _airLatched = out.airOn;

   return out;
}

FarmDecision FarmManager::applyManual(const ManualOverrides &m)
{
   FarmDecision out{};
   out.pumpOn = m.wantPumpOn;
   out.mistOn = m.wantMistOn;
   out.airOn = m.wantAirOn;
   return out;
}

FarmDecision FarmManager::applyAuto(const FarmInput &in)
{
   FarmDecision out{};

   // ปั๊มน้ำยังไม่มี auto logic
   out.pumpOn = false;

   out.mistOn = decideMistByTemp(in.temperatureC, in.temperatureValid);
   out.airOn = decideAirBySchedule(in.minutesOfDay);

   return out;
}

bool FarmManager::decideMistByTemp(float tempC, bool valid)
{
   if (!valid)
      return false;

   // hysteresis thresholds (ถ้าคุณมี config ของจริง ค่อยย้ายไป Config.h ใน Milestone 3)
   constexpr float TEMP_ON = 32.0f;
   constexpr float TEMP_OFF = 29.0f;

   if (tempC >= TEMP_ON)
      return true;
   if (tempC <= TEMP_OFF)
      return false;

   return _mistLatched;
}

bool FarmManager::decideAirBySchedule(uint16_t minutesOfDay) const
{
   if (!_schedule || !_schedule->enabled || _schedule->windowCount == 0)
      return false;

   for (uint8_t i = 0; i < _schedule->windowCount; ++i)
   {
      const TimeWindow &w = _schedule->windows[i];
      if (minutesOfDay >= w.startMin && minutesOfDay < w.endMin)
         return true;
   }
   return false;
}