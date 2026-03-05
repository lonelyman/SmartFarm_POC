#include "application/FarmManager.h"
#include "Config.h"

FarmManager::FarmManager(const AirPumpSchedule *schedule)
    : _schedule(schedule) {}

FarmDecision FarmManager::update(const FarmInput &in)
{
   FarmDecision out{};

   switch (in.mode)
   {
   case SystemMode::IDLE:
      out.pumpOn = false;
      out.mistOn = false;
      out.airOn = false;
      _pumpLatched = false;
      _mistLatched = false;
      _airLatched = false;
      _mistForced = false;    // reset guard ตอน IDLE ด้วย
      _bootGuardDone = false; // reset boot guard ถ้า IDLE (เผื่อ restart)
      _bootTime = 0;
      break;

   case SystemMode::MANUAL:
      out = applyManual(in.manual);
      break;

   case SystemMode::AUTO:
      out = applyAuto(in);
      break;
   }

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

   // --- Boot guard: หน่วง 10 วินาทีหลัง boot ---
   if (!_bootGuardDone)
   {
      if (_bootTime == 0)
         _bootTime = millis();
      if (millis() - _bootTime < BOOT_GUARD_MS)
         return out; // ปิดหมด รอก่อน
      _bootGuardDone = true;
      Serial.println("[FarmManager] Boot guard done, AUTO enabled");
   }

   // --- Logic ปกติ ---
   out.pumpOn = false;
   out.airOn = decideAirBySchedule(in.minutesOfDay);

   bool sensorWantsOn = false;

   if (in.temperatureValid && in.humidityValid)
      sensorWantsOn = decideMistByTempAndHumidity(in.temperatureC, in.humidityRH);
   else if (in.humidityValid)
      sensorWantsOn = decideMistByHumidity(in.humidityRH, true);
   else
      sensorWantsOn = decideMistByTemp(in.temperatureC, in.temperatureValid);

   // --- Time guard: จำกัดเวลาพ่น/พัก ---
   out.mistOn = applyMistGuard(sensorWantsOn);

   return out;
}

// ใช้ทั้ง temp และ humidity ตัดสินใจพร้อมกัน
bool FarmManager::decideMistByTempAndHumidity(float tempC, float humRH)
{
   bool tooDry = humRH <= HYSTERESIS_HUMIDITY_ON;
   bool tooHot = tempC >= HYSTERESIS_TEMP_ON;

   bool coolEnough = tempC <= HYSTERESIS_TEMP_OFF;
   bool humidEnough = humRH >= HYSTERESIS_HUMIDITY_OFF;

   if (tooDry && tooHot)
      return true;
   if (coolEnough || humidEnough)
      return false;
   return _mistLatched; // dead-band → คงเดิม
}

// fallback: humidity อย่างเดียว
bool FarmManager::decideMistByHumidity(float humRH, bool valid)
{
   if (!valid)
      return _mistLatched;
   if (humRH <= HYSTERESIS_HUMIDITY_ON)
      return true;
   if (humRH >= HYSTERESIS_HUMIDITY_OFF)
      return false;
   return _mistLatched;
}

// fallback: temp อย่างเดียว
bool FarmManager::decideMistByTemp(float tempC, bool valid)
{
   if (!valid)
      return false;
   if (tempC >= HYSTERESIS_TEMP_ON)
      return true;
   if (tempC <= HYSTERESIS_TEMP_OFF)
      return false;
   return _mistLatched;
}

// จำกัดเวลาพ่นต่อเนื่อง และบังคับพักขั้นต่ำ
bool FarmManager::applyMistGuard(bool sensorWantsOn)
{
   uint32_t now = millis();

   // อยู่ใน forced-off → เช็คว่าพักครบยัง
   if (_mistForced)
   {
      if (now - _mistOffSince < MIST_MIN_OFF_MS)
      {
         Serial.println("[MistGuard] Forced OFF (cooling down)");
         return false;
      }
      _mistForced = false; // พักครบแล้ว
      Serial.println("[MistGuard] Cooldown done, allow sensor");
   }

   if (sensorWantsOn)
   {
      // บันทึกเวลาเริ่มพ่น (ครั้งแรกที่เปิด)
      if (!_mistLatched)
         _mistOnSince = now;

      // พ่นนานเกินกำหนด → บังคับหยุด
      if (now - _mistOnSince >= MIST_MAX_ON_MS)
      {
         _mistForced = true;
         _mistOffSince = now;
         Serial.println("[MistGuard] Max ON reached, forcing OFF");
         return false;
      }
      return true;
   }

   return false;
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