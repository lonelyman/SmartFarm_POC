// include/application/FarmModels.h
#pragma once

#include <stdint.h>
#include "../interfaces/Types.h"

// ============================================================
//  FarmModels — Input/Output ของ FarmManager
//  ไม่มี Arduino API — ทดสอบได้โดยไม่ต้องมี hardware
// ============================================================

struct FarmInput
{
   SystemMode mode         = SystemMode::IDLE;
   uint16_t   minutesOfDay = 0;

   float temperatureC    = 0.0f;
   bool  temperatureValid = false;
   float humidityRH      = 0.0f;
   bool  humidityValid    = false;

   // manual overrides — FarmManager ไม่รู้จัก SharedState โดยตรง
   ManualOverrides manual{};
};

struct FarmDecision
{
   bool pumpOn = false; // water pump
   bool mistOn = false; // mist system
   // airOn ถูกตัดออก — ScheduledRelay เป็นเจ้าของเรื่อง air pump
};