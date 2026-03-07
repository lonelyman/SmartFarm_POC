// src/infrastructure/AppBoot.cpp
#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>

#include "Config.h"

#include "infrastructure/AppBoot.h"
#include "infrastructure/ScheduleStore.h"

#include "tasks/TaskEntrypoints.h"
#include "tasks/WebUiTask.h"

namespace
{
   void printBanner()
   {
      Serial.println();
      Serial.println("===== SmartFarm Booting... =====");
   }

   // ⚠️ boot glitch mitigation (ESP32-S3)
   // ต้องทำก่อน initDrivers() และก่อนสร้าง FreeRTOS task เสมอ
   // เพราะ active-low relay: ขา floating ตอนบูต = LOW = relay ON ชั่วคราวได้
   void initRelayPins()
   {
      const uint8_t inactive = RELAY_ACTIVE_LOW ? HIGH : LOW;
      pinMode(PIN_RELAY_WATER_PUMP, OUTPUT);
      pinMode(PIN_RELAY_MIST, OUTPUT);
      pinMode(PIN_RELAY_AIR_PUMP, OUTPUT);
      digitalWrite(PIN_RELAY_WATER_PUMP, inactive);
      digitalWrite(PIN_RELAY_MIST, inactive);
      digitalWrite(PIN_RELAY_AIR_PUMP, inactive);
   }

   void initI2C()
   {
      // ESP32-S3: SDA=8, SCL=9
      Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
   }

   // โหลด schedule จาก LittleFS → ISchedule
   // ถ้าโหลดไม่สำเร็จ → schedule จะ disabled (clear() แล้ว setEnabled(false))
   void initFileSystemAndSchedule(SystemContext &ctx)
   {
      if (!LittleFS.begin(true)) // true = format on fail (ช่วย first boot)
      {
         Serial.println("⚠️ LittleFS mount failed, schedules disabled");
         return;
      }

      Serial.println("✅ LittleFS mount OK");

      if (ctx.scheduledAirPump)
      {
         // ดึง ISchedule จาก ScheduledRelay ผ่าน getSchedule()
         loadScheduleFromFS("/schedule.json", "air_pump",
                            ctx.scheduledAirPump->getSchedule());
      }
   }

   void initModeSource(SystemContext &ctx)
   {
      if (!ctx.modeSource)
      {
         Serial.println("⚠️ modeSource is null (mode switch disabled)");
         return;
      }
      ctx.modeSource->begin();
   }

   void initNetModeSource(SystemContext &ctx)
   {
      if (!ctx.netModeSource)
      {
         Serial.println("⚠️ netModeSource is null (net switch disabled)");
         return;
      }
      ctx.netModeSource->begin();
   }

   void initDrivers(SystemContext &ctx)
   {
      // Sensors
      ctx.lightSensor->begin();
      ctx.tempSensor->begin();

      if (ctx.waterLevelInput)
         ctx.waterLevelInput->begin();

      if (ctx.waterTempSensor)
         ctx.waterTempSensor->begin();

      if (ctx.ui)
         ctx.ui->begin();

      // Actuators
      ctx.waterPump->begin();
      ctx.mistSystem->begin();
      ctx.airPump->begin();

      if (ctx.swManualPump)
         ctx.swManualPump->begin();
      if (ctx.swManualMist)
         ctx.swManualMist->begin();
      if (ctx.swManualAir)
         ctx.swManualAir->begin();

      // Water level alarm LEDs
      const uint8_t LED_OFF = ALARM_LED_ACTIVE_HIGH ? LOW : HIGH;
      pinMode(PIN_WATER_LEVEL_CH1_ALARM_LED, OUTPUT);
      pinMode(PIN_WATER_LEVEL_CH2_ALARM_LED, OUTPUT);
      digitalWrite(PIN_WATER_LEVEL_CH1_ALARM_LED, LED_OFF);
      digitalWrite(PIN_WATER_LEVEL_CH2_ALARM_LED, LED_OFF);
   }

   void initNetwork(SystemContext &ctx)
   {
      if (!ctx.network)
      {
         Serial.println("⚠️ network is null (wifi disabled)");
         return;
      }
      ctx.network->begin();
   }

   void initClock(SystemContext &ctx)
   {
      if (!ctx.clock)
      {
         Serial.println("⚠️ clock is null (time disabled)");
         return;
      }
#if DEBUG_TIME_LOG
      Serial.println("[BOOT] clock.begin() ...");
#endif
      ctx.clock->begin();
   }

   void setInitialSafeState(SystemContext &ctx)
   {
      ctx.state->setMode(SystemMode::IDLE);
   }

   void startTasks(SystemContext &ctx)
   {
      // Core 1: task หนัก (sensor/control/webui)
      // Core 0: task เบา (network, command)
      xTaskCreatePinnedToCore(inputTask, "In", INPUT_TASK_STACK, &ctx, 1, nullptr, 1);
      xTaskCreatePinnedToCore(controlTask, "Ctrl", CONTROL_TASK_STACK, &ctx, 2, nullptr, 1);
      xTaskCreatePinnedToCore(commandTask, "Cmd", COMMAND_TASK_STACK, &ctx, 1, nullptr, 0);
      xTaskCreatePinnedToCore(networkTask, "Net", 4096, &ctx, 1, nullptr, 0);
      xTaskCreatePinnedToCore(webUiTask, "WebUiTask", 4096, &ctx, 1, nullptr, 1);
   }
}

void AppBoot::setup(SystemContext &ctx)
{
   printBanner();

   // ⚠️ ลำดับสำคัญ: initRelayPins ต้องมาก่อน initDrivers และ startTasks เสมอ
   initRelayPins();

   initI2C();
   initFileSystemAndSchedule(ctx);

   initModeSource(ctx);
   initNetModeSource(ctx);
   initClock(ctx);
   initDrivers(ctx);
   initNetwork(ctx);
   setInitialSafeState(ctx);
   startTasks(ctx);

   Serial.println("🚀 SmartFarm System: Ready and Linked.");
}