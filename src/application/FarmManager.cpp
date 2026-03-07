// src/application/FarmManager.cpp
#include "application/FarmManager.h"
#include <Arduino.h>
#include "Config.h"

FarmDecision FarmManager::update(const FarmInput &in)
{
   FarmDecision out{};

   switch (in.mode)
   {
   case SystemMode::IDLE:
      // ปิดทุกอย่าง + reset state ทั้งหมด
      _pumpLatched = false;
      _mistLatched = false;
      _mistForced = false;
      _bootGuardDone = false;
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

   return out;
}

FarmDecision FarmManager::applyManual(const ManualOverrides &m)
{
   FarmDecision out{};
   out.pumpOn = m.wantPumpOn;
   out.mistOn = m.wantMistOn;
   // wantAirOn ไม่อยู่ใน FarmDecision — ControlTask ส่งต่อให้ ScheduledRelay เอง
   return out;
}

FarmDecision FarmManager::applyAuto(const FarmInput &in)
{
   FarmDecision out{};

   // --- Boot guard: หน่วง BOOT_GUARD_MS หลัง boot ---
   if (!_bootGuardDone)
   {
      if (_bootTime == 0)
         _bootTime = millis();
      if (millis() - _bootTime < BOOT_GUARD_MS)
         return out; // ปิดหมด รอก่อน
      _bootGuardDone = true;
      Serial.println("[FarmManager] Boot guard done, AUTO enabled");
   }

   out.pumpOn = false; // water pump AUTO logic ยังไม่ implement

   // --- Mist decision ---
   bool sensorWantsOn = false;

   if (in.temperatureValid && in.humidityValid)
      sensorWantsOn = decideMistByTempAndHumidity(in.temperatureC, in.humidityRH);
   else if (in.humidityValid)
      sensorWantsOn = decideMistByHumidity(in.humidityRH, true);
   else
      sensorWantsOn = decideMistByTemp(in.temperatureC, in.temperatureValid);

   out.mistOn = applyMistGuard(sensorWantsOn);

   return out;
}

bool FarmManager::decideMistByTempAndHumidity(float tempC, float humRH)
{
   const bool tooDry = humRH <= HYSTERESIS_HUMIDITY_ON;
   const bool tooHot = tempC >= HYSTERESIS_TEMP_ON;
   const bool coolEnough = tempC <= HYSTERESIS_TEMP_OFF;
   const bool humidEnough = humRH >= HYSTERESIS_HUMIDITY_OFF;

   if (tooDry && tooHot)
      return true;
   if (coolEnough || humidEnough)
      return false;
   return _mistLatched; // dead-band → คงเดิม
}

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

bool FarmManager::applyMistGuard(bool sensorWantsOn)
{
   const uint32_t now = millis();

   // อยู่ใน forced-off → เช็คว่าพักครบยัง
   if (_mistForced)
   {
      if (now - _mistOffSince < MIST_MIN_OFF_MS)
      {
         Serial.println("[MistGuard] Forced OFF (cooling down)");
         return false;
      }
      _mistForced = false;
      Serial.println("[MistGuard] Cooldown done, allow sensor");
   }

   if (sensorWantsOn)
   {
      if (!_mistLatched)
         _mistOnSince = now;

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