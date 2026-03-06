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

   
   void initRelayPins()
   {
      // boot glitch mitigation: set relay pins to inactive as early as possible
      const uint8_t inactive = RELAY_ACTIVE_LOW ? HIGH : LOW; // active-low: inactive=HIGH
      pinMode(PIN_RELAY_WATER_PUMP, OUTPUT);
      pinMode(PIN_RELAY_MIST, OUTPUT);
      pinMode(PIN_RELAY_AIR_PUMP, OUTPUT);
      digitalWrite(PIN_RELAY_WATER_PUMP, inactive);
      digitalWrite(PIN_RELAY_MIST, inactive);
      digitalWrite(PIN_RELAY_AIR_PUMP, inactive);
   }

   void initI2C()
   {
      Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
   }

   void initFileSystemAndSchedule(AirPumpSchedule &airSchedule)
   {
      if (!LittleFS.begin())
      {
         Serial.println("⚠️ LittleFS mount failed, air schedule disabled");
         airSchedule.enabled = false;
         airSchedule.windowCount = 0;
         return;
      }

      Serial.println("✅ LittleFS mount success, loading air schedule...");
      if (!loadAirScheduleFromFS("/schedule.json", airSchedule))
      {
         Serial.println("⚠️ load schedule failed, no schedule");
         airSchedule.enabled = false;
         airSchedule.windowCount = 0;
         return;
      }

      Serial.printf("✅ air schedule loaded: enabled=%d windows=%u\n",
                    airSchedule.enabled ? 1 : 0,
                    (unsigned)airSchedule.windowCount);
   }

   void initModeSource(SystemContext &ctx)
   {
      if (ctx.modeSource == nullptr)
      {
         Serial.println("⚠️ modeSource is null (mode switch disabled)");
         return;
      }
      ctx.modeSource->begin();
   }

   void initNetModeSource(SystemContext &ctx)
   {
      if (ctx.netModeSource == nullptr)
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

      // เพิ่มต่อจาก sensor begin() อื่นๆ
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

   static void initNetwork(SystemContext &ctx)
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
      if (ctx.clock == nullptr)
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
      xTaskCreatePinnedToCore(inputTask, "In", INPUT_TASK_STACK, &ctx, 1, nullptr, 1);
      xTaskCreatePinnedToCore(controlTask, "Ctrl", CONTROL_TASK_STACK, &ctx, 2, nullptr, 1);
      xTaskCreatePinnedToCore(commandTask, "Cmd", COMMAND_TASK_STACK, &ctx, 1, nullptr, 1);
      xTaskCreatePinnedToCore(networkTask, "Net", 4096, &ctx, 1, nullptr, 0);
      xTaskCreatePinnedToCore(webUiTask, "WebUiTask", 4096, &ctx, 1, nullptr, 1);
   }
}

void AppBoot::setup(SystemContext &ctx)
{
   printBanner();

   initI2C();
   initFileSystemAndSchedule(*ctx.airSchedule);

   initModeSource(ctx);
   initNetModeSource(ctx);
   initClock(ctx);
   initDrivers(ctx);
   initNetwork(ctx);
   setInitialSafeState(ctx);
   startTasks(ctx);

   Serial.println("🚀 SmartFarm System: Ready and Linked.");
}