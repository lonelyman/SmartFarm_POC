#pragma once

#include <stdint.h>
#include "../interfaces/Types.h" // ใช้ SystemMode + ManualOverrides (portable model)

struct FarmInput
{
   // mode จากระบบ
   SystemMode mode = SystemMode::IDLE;

   // เวลา (นาทีของวัน)
   uint16_t minutesOfDay = 0;

   // อุณหภูมิ (เฉพาะค่าที่ logic ต้องใช้)
   float temperatureC = 0.0f;
   bool temperatureValid = false;
   float humidityRH = 0.0f;
   bool humidityValid = false;

   // manual overrides (มาจาก SharedState แต่ FarmManager ไม่รู้จัก SharedState)
   ManualOverrides manual{};
};

struct FarmDecision
{
   bool pumpOn = false;
   bool mistOn = false;
   bool airOn = false;
};