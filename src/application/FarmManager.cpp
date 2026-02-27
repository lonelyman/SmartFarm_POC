#include "application/FarmManager.h"

FarmManager::FarmManager(const AirPumpSchedule *schedule)
    : _schedule(schedule) {}

FarmDecision FarmManager::update(const SystemStatus &status,
                                 const ManualOverrides &manual,
                                 uint16_t minutesOfDay)
{
   FarmDecision out{};

   switch (status.mode)
   {
   case SystemMode::IDLE:
      out.pumpOn = false;
      out.mistOn = false;
      out.airOn = false;

      _pumpLatched = false;
      _mistLatched = false;
      _airLatched = false;
      break;

   case SystemMode::MANUAL:
      out = applyManual(manual);
      break;

   case SystemMode::AUTO:
      out = applyAuto(status, minutesOfDay);
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

FarmDecision FarmManager::applyAuto(const SystemStatus &status, uint16_t minutesOfDay)
{
   FarmDecision out{};
   out.pumpOn = false; // ยังไม่มี auto น้ำ
   out.mistOn = decideMistByTemp(status);
   out.airOn = decideAirBySchedule(minutesOfDay);
   return out;
}

bool FarmManager::decideMistByTemp(const SystemStatus &status)
{
   if (!status.temperature.isValid)
      return false;

   constexpr float TEMP_ON = 32.0f;
   constexpr float TEMP_OFF = 29.0f;

   float t = status.temperature.value;

   if (t >= TEMP_ON)
      return true;
   if (t <= TEMP_OFF)
      return false;

   return _mistLatched; // hysteresis
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